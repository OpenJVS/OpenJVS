#include "jvs.h"
#include "device.h"
#include "config.h"
#include "debug.h"
#include "ffb.h"

#include <time.h>

/* The deviceID is set to 1 to allow the system to work after a reboot of OpenJVS */
int deviceID = 1;

/* Pointer for the capabilities and state of the IO */
JVSCapabilities *localCapabilities;
JVSState *localState;
JVSConfig *localConfig;

/* The in and out packets used to read and write to and from*/
JVSPacket inputPacket, outputPacket;

/* The in and out buffer used to read and write to and from */
unsigned char outputBuffer[JVS_MAX_PACKET_SIZE], inputBuffer[JVS_MAX_PACKET_SIZE];

/* Variables to store bit alignment */
int analogueRestBits, gunXRestBits, gunYRestBits;

/**
 * Initialise the JVS emulation
 *
 * Setup the JVS emulation on a specific device path with an
 * IO mapping provided.
 *
 * @param devicePath The linux filepath for the RS485 adapter
 * @param capabilitiesSetup The representation of the IO to emulate
 * @returns 1 if the device was initialised successfully, 0 otherwise.
 */
int initJVS(char *devicePath, const JVSCapabilities *capabilitiesSetup)
{
	/* Save pointers to capabilities, state of the IO and config */
	localCapabilities = getCapabilities();
	localState = getState();
	localConfig = getConfig();

	/* Init the connection to the Naomi */
	if (!initDevice(devicePath, localConfig->senseLineType, localConfig->senseLinePin))
		return 0;

	/* Init the Virtual IO */
	if (!initIO((JVSCapabilities *)capabilitiesSetup))
		return 0;

	/* Calculate the alignments for analogue and gun channels, default is left */
	if (!localCapabilities->rightAlignBits)
	{
		analogueRestBits = 16 - localCapabilities->analogueInBits;
		gunXRestBits = 16 - localCapabilities->gunXBits;
		gunYRestBits = 16 - localCapabilities->gunYBits;
	}

	/* Float the sense line ready for connection */
	setSenseLine(0);

	return 1;
}

/**
 * Disconnect from the JVS device
 *
 * Disconnects from the device communicating with the
 * arcade machine so JVS can be shutdown safely.
 *
 * @returns 1 if the device disconnected succesfully, 0 otherwise.
 */
int disconnectJVS()
{
	return closeDevice();
}

/**
 * Writes a single feature to an output packet
 *
 * Writes a single JVS feature, which are specified
 * in the JVS spec, to the output packet.
 *
 * @param outputPacket The packet to write to.
 * @param capability The specific capability to write
 * @param arg0 The first argument of the capability
 * @param arg1 The second argument of the capability
 * @param arg2 The final argument of the capability
 */
void writeFeature(JVSPacket *outputPacket, char capability, char arg0, char arg1, char arg2)
{
	outputPacket->data[outputPacket->length] = capability;
	outputPacket->data[outputPacket->length + 1] = arg0;
	outputPacket->data[outputPacket->length + 2] = arg1;
	outputPacket->data[outputPacket->length + 3] = arg2;
	outputPacket->length += 4;
}

/**
 * Write the entire set of features to an output packet
 *
 * Writes the set of features specified in the JVSCapabilities
 * struct to the specified output packet.
 *
 * @param outputPacket The packet to write to.
 * @param capabilities The capabilities object to read from
 */
void writeFeatures(JVSPacket *outputPacket, JVSCapabilities *capabilities)
{
	outputPacket->data[outputPacket->length] = REPORT_SUCCESS;
	outputPacket->length += 1;

	/* Input Functions */

	if (capabilities->players)
		writeFeature(outputPacket, CAP_PLAYERS, capabilities->players, capabilities->switches, 0x00);

	if (capabilities->coins)
		writeFeature(outputPacket, CAP_COINS, capabilities->coins, 0x00, 0x00);

	if (capabilities->analogueInChannels)
		writeFeature(outputPacket, CAP_ANALOG_IN, capabilities->analogueInChannels, capabilities->analogueInBits, 0x00);

	if (capabilities->rotaryChannels)
		writeFeature(outputPacket, CAP_ROTARY, capabilities->rotaryChannels, 0x00, 0x00);

	if (capabilities->keypad)
		writeFeature(outputPacket, CAP_KEYPAD, 0x00, 0x00, 0x00);

	if (capabilities->gunChannels)
		writeFeature(outputPacket, CAP_LIGHTGUN, capabilities->gunXBits, capabilities->gunYBits, capabilities->gunChannels);

	if (capabilities->generalPurposeInputs)
		writeFeature(outputPacket, CAP_GPI, 0x00, capabilities->generalPurposeInputs, 0x00);

	/* Output Functions */

	if (capabilities->card)
		writeFeature(outputPacket, CAP_CARD, capabilities->card, 0x00, 0x00);

	if (capabilities->hopper)
		writeFeature(outputPacket, CAP_HOPPER, capabilities->hopper, 0x00, 0x00);

	if (capabilities->generalPurposeOutputs)
		writeFeature(outputPacket, CAP_GPO, capabilities->generalPurposeOutputs, 0x00, 0x00);

	if (capabilities->analogueOutChannels)
		writeFeature(outputPacket, CAP_ANALOG_OUT, capabilities->analogueOutChannels, 0x00, 0x00);

	if (capabilities->displayOutColumns)
		writeFeature(outputPacket, CAP_DISPLAY, capabilities->displayOutColumns, capabilities->displayOutRows, capabilities->displayOutEncodings);

	/* Other */

	if (capabilities->backup)
		writeFeature(outputPacket, CAP_BACKUP, 0x00, 0x00, 0x00);

	outputPacket->data[outputPacket->length] = CAP_END;
	outputPacket->length += 1;
}

/**
 * Processes and responds to an entire JVS packet
 *
 * Follows the JVS spec and proceses and responds
 * to a single entire JVS packet.
 *
 * @returns The status of the entire operation
 */
JVSStatus processPacket()
{
	/* Initially read in a packet */
	JVSStatus readPacketStatus = readPacket(&inputPacket);
	if (readPacketStatus != JVS_STATUS_SUCCESS)
		return readPacketStatus;

	/* Check if the packet is for us */
	if (inputPacket.destination != BROADCAST && inputPacket.destination != deviceID)
		return JVS_STATUS_NOT_FOR_US;

	/* Handle re-transmission requests */
	if (inputPacket.data[0] == CMD_RETRANSMIT)
		return writePacket(&outputPacket);

	/* Setup the output packet */
	outputPacket.length = 0;
	outputPacket.destination = BUS_MASTER;

	int index = 0;

	/* Set the entire packet success line */
	outputPacket.data[outputPacket.length++] = STATUS_SUCCESS;

	/* Run any custom logic */
	processJVSFFB(localState);

	while (index < inputPacket.length - 1)
	{
		int size = 1;
		switch (inputPacket.data[index])
		{

		/* The arcade hardware sends a reset command and we clear our memory */
		case CMD_RESET:
		{
			debug(1, "CMD_RESET\n");
			size = 2;
			deviceID = -1;
			setSenseLine(0);
		}
		break;

		/* The arcade hardware assigns an address to our IO */
		case CMD_ASSIGN_ADDR:
		{
			debug(1, "CMD_ASSIGN_ADDR\n");
			size = 2;
			deviceID = inputPacket.data[index + 1];
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
			setSenseLine(1);
		}
		break;

		/* Ask for the name of the IO board */
		case CMD_REQUEST_ID:
		{
			debug(1, "CMD_REQUEST_ID\n");
			outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
			memcpy(&outputPacket.data[outputPacket.length + 1], localCapabilities->name, strlen(localCapabilities->name) + 1);
			outputPacket.length += strlen(localCapabilities->name) + 2;
		}
		break;

		/* Asks for version information */
		case CMD_COMMAND_VERSION:
		{
			debug(1, "CMD_COMMAND_VERSION\n");
			outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
			outputPacket.data[outputPacket.length + 1] = localCapabilities->commandVersion;
			outputPacket.length += 2;
		}
		break;

		/* Asks for version information */
		case CMD_JVS_VERSION:
		{
			debug(1, "CMD_JVS_VERSION\n");
			outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
			outputPacket.data[outputPacket.length + 1] = localCapabilities->jvsVersion;
			outputPacket.length += 2;
		}
		break;

		/* Asks for version information */
		case CMD_COMMS_VERSION:
		{
			debug(1, "CMD_COMMS_VERSION\n");
			outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
			outputPacket.data[outputPacket.length + 1] = localCapabilities->commsVersion;
			outputPacket.length += 2;
		}
		break;

		/* Asks what our IO board supports */
		case CMD_CAPABILITIES:
		{
			debug(1, "CMD_CAPABILITIES\n");
			writeFeatures(&outputPacket, localCapabilities);
		}
		break;

		/* Asks for the status of our IO boards switches */
		case CMD_READ_SWITCHES:
		{
			debug(1, "CMD_READ_SWITCHES\n");
			size = 3;
			outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
			outputPacket.data[outputPacket.length + 1] = localState->inputSwitch[0];
			outputPacket.length += 2;
			for (int i = 0; i < inputPacket.data[index + 1]; i++)
			{
				for (int j = 0; j < inputPacket.data[index + 2]; j++)
				{
					outputPacket.data[outputPacket.length++] = localState->inputSwitch[i + 1] >> (8 - (j * 8));
				}
			}
		}
		break;

		case CMD_READ_COINS:
		{
			debug(1, "CMD_READ_COINS\n");
			size = 2;
			int numberCoinSlots = inputPacket.data[index + 1];
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

			for (int i = 0; i < numberCoinSlots; i++)
			{
				outputPacket.data[outputPacket.length] = (localState->coinCount[i] << 8) & 0x1F;
				outputPacket.data[outputPacket.length + 1] = localState->coinCount[i] & 0xFF;
				outputPacket.length += 2;
			}
		}
		break;

		case CMD_READ_ANALOGS:
		{
			debug(1, "CMD_READ_ANALOGS\n");
			size = 2;

			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

			for (int i = 0; i < inputPacket.data[index + 1]; i++)
			{
				/* By default left align the data */
				int analogueData = localState->analogueChannel[i] << analogueRestBits;
				outputPacket.data[outputPacket.length] = analogueData >> 8;
				outputPacket.data[outputPacket.length + 1] = analogueData;
				outputPacket.length += 2;
			}
		}
		break;

		case CMD_READ_ROTARY:
		{
			debug(1, "CMD_READ_ROTARY\n");
			size = 2;

			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

			for (int i = 0; i < inputPacket.data[index + 1]; i++)
			{
				outputPacket.data[outputPacket.length] = localState->rotaryChannel[i] >> 8;
				outputPacket.data[outputPacket.length + 1] = localState->rotaryChannel[i];
				outputPacket.length += 2;
			}
		}
		break;

		case CMD_READ_KEYPAD:
		{
			debug(1, "CMD_READ_KEYPAD\n");
			outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
			outputPacket.data[outputPacket.length + 1] = 0x00;
			outputPacket.length += 2;
		}
		break;

		case CMD_READ_GPI:
		{
			debug(1, "CMD_READ_GPI\n");
			size = 2;
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
			for (int i = 0; i < inputPacket.data[index + 1]; i++)
			{
				outputPacket.data[outputPacket.length++] = 0x55;
			}
		}
		break;

		case CMD_REMAINING_PAYOUT:
		{
			debug(1, "CMD_REMAINING_PAYOUT\n");
			size = 2;
			outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
			outputPacket.data[outputPacket.length + 1] = 0;
			outputPacket.data[outputPacket.length + 2] = 0;
			outputPacket.data[outputPacket.length + 3] = 0;
			outputPacket.data[outputPacket.length + 4] = 0;
			outputPacket.length += 5;
		}
		break;

		case CMD_SET_PAYOUT:
		{
			debug(1, "CMD_SET_PAYOUT\n");
			size = 4;
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
		}
		break;

		case CMD_WRITE_GPO:
		{
			debug(1, "CMD_WRITE_GPO\n");
			size = 2 + inputPacket.data[index + 1];
			for (int i = 0; i < inputPacket.data[index + 1]; i++)
			{
				setGeneralPurposeOutputByte(i, inputPacket.data[index + 2 + i]);
			}
			outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
			outputPacket.length += 1;
		}
		break;

		case CMD_WRITE_GPO_BYTE:
		{
			debug(1, "CMD_WRITE_GPO_BYTE\n");
			size = 3;
			setGeneralPurposeOutputByte(inputPacket.data[index + 1], inputPacket.data[index + 2]);
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
		}
		break;

		case CMD_WRITE_GPO_BIT:
		{
			debug(1, "CMD_WRITE_GPO_BIT\n");
			size = 3;
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
		}
		break;

		case CMD_WRITE_ANALOG:
		{
			debug(1, "CMD_WRITE_ANALOG\n");
			size = inputPacket.data[index + 1] * 2 + 2;
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
		}
		break;

		case CMD_SUBTRACT_PAYOUT:
		{
			debug(1, "CMD_SUBTRACT_PAYOUT\n");
			size = 3;
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
		}
		break;

		case CMD_WRITE_COINS:
		{
			debug(1, "CMD_WRITE_COINS\n");
			size = 3;
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
		}
		break;

		case CMD_WRITE_DISPLAY:
		{
			debug(1, "CMD_WRITE_DISPLAY\n");
			size = (inputPacket.data[index + 1] * 2) + 2;
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
		}
		break;

		case CMD_DECREASE_COINS:
		{
			debug(1, "CMD_DECREASE_COINS\n");
			size = 4;
			int coin_decrement = ((int)(inputPacket.data[index + 2]) | ((int)(inputPacket.data[index + 3]) << 8));

			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

			/* Prevent underflow of coins */
			if (coin_decrement > localState->coinCount[0])
				coin_decrement = localState->coinCount[0];

			for (int i = 0; i < localCapabilities->coins; i++)
				localState->coinCount[i] -= coin_decrement;
		}
		break;

		case CMD_CONVEY_ID:
		{
			debug(1, "CMD_CONVEY_ID\n");
			size = 1;
			outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
			char idData[100];
			for (int i = 1; i < 100; i++)
			{
				idData[i] = (char)inputPacket.data[index + i];
				size++;
				if (!inputPacket.data[index + i])
					break;
			}
			printf("CMD_CONVEY_ID = %s\n", idData);
		}
		break;

		/* The touch screen and light gun input, simply using analogue channels */
		case CMD_READ_LIGHTGUN:
		{
			debug(1, "CMD_READ_LIGHTGUN\n");
			size = 2;

			int analogueXData = localState->gunChannel[0] << gunXRestBits;
			int analogueYData = localState->gunChannel[1] << gunYRestBits;
			outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
			outputPacket.data[outputPacket.length + 1] = analogueXData >> 8;
			outputPacket.data[outputPacket.length + 2] = analogueXData;
			outputPacket.data[outputPacket.length + 3] = analogueYData >> 8;
			outputPacket.data[outputPacket.length + 4] = analogueYData;
			outputPacket.length += 5;
		}
		break;

		default:
		{
			if (inputPacket.data[index] == CMD_NAMCO_SPECIFIC)
				debug(0, "CMD_NAMCO_SPECIFIC (Command shown below)\n");

			debug(0, "CMD_UNSUPPORTED (Unsupported command [0x%02hhX])\n", inputPacket.data[index]);
		}
		}
		index += size;
	}
	debug(1, "");

	return writePacket(&outputPacket);
}

/**
 * Read a JVS Packet
 *
 * A single JVS packet is read into the packet pointer
 * after it has been received, unescaped and checked
 * for any checksum errors.
 *
 * @param packet The packet to read into
 */
JVSStatus readPacket(JVSPacket *packet)
{
	int bytesAvailable = 0, escape = 0, phase = 0, index = 0, dataIndex = 0, finished = 0;
	unsigned char checksum = 0x00;

	while (!finished)
	{
		int bytesRead = readBytes(inputBuffer + bytesAvailable, JVS_MAX_PACKET_SIZE - bytesAvailable);

		if (bytesRead < 0)
			return JVS_STATUS_ERROR_TIMEOUT;

		bytesAvailable += bytesRead;

		while ((index < bytesAvailable) && !finished)
		{
			/* If we encounter a SYNC start again */
			if (!escape && (inputBuffer[index] == SYNC))
			{
				phase = 0;
				dataIndex = 0;
				index++;
				continue;
			}

			/* If we encounter an ESCAPE byte escape the next byte */
			if (!escape && inputBuffer[index] == ESCAPE)
			{
				escape = 1;
				index++;
				continue;
			}

			/* Escape next byte by adding 1 to it */
			if (escape)
			{
				inputBuffer[index]++;
				escape = 0;
			}

			/* Deal with the main bulk of the data */
			switch (phase)
			{
			case 0: // If we have not yet got the address
				packet->destination = inputBuffer[index];
				checksum = packet->destination & 0xFF;
				phase++;
				break;
			case 1: // If we have not yet got the length
				packet->length = inputBuffer[index];
				checksum = (checksum + packet->length) & 0xFF;
				phase++;
				break;
			case 2: // If there is still data to read
				if (dataIndex == (packet->length - 1))
				{
					if (checksum != inputBuffer[index])
						return JVS_STATUS_ERROR_CHECKSUM;
					finished = 1;
					break;
				}
				packet->data[dataIndex++] = inputBuffer[index];
				checksum = (checksum + inputBuffer[index]) & 0xFF;
				break;
			default:
				return JVS_STATUS_ERROR;
			}
			index++;
		}
	}

	debug(2, "INPUT:\n");
	debugBuffer(2, inputBuffer, index);

	return JVS_STATUS_SUCCESS;
}

/**
 * Write a JVS Packet
 *
 * A single JVS Packet is written to the arcade
 * system after it has been escaped and had
 * a checksum calculated.
 *
 * @param packet The packet to send
 */
JVSStatus writePacket(JVSPacket *packet)
{
	/* Don't return anything if there isn't anything to write! */
	if (packet->length < 2)
		return JVS_STATUS_SUCCESS;

	/* Get pointer to raw data in packet */
	unsigned char *packetPointer = (unsigned char *)packet;

	/* Add SYNC and reset buffer */
	int checksum = 0;
	int outputIndex = 1;
	outputBuffer[0] = SYNC;

	packet->length++;

	/* Write out entire packet */
	for (int i = 0; i < packet->length + 1; i++)
	{
		if (packetPointer[i] == SYNC || packetPointer[i] == ESCAPE)
		{
			outputBuffer[outputIndex++] = ESCAPE;
			outputBuffer[outputIndex++] = (packetPointer[i] - 1);
		}
		else
		{
			outputBuffer[outputIndex++] = (packetPointer[i]);
		}
		checksum = (checksum + packetPointer[i]) & 0xFF;
	}

	/* Write out escaped checksum */
	if (checksum == SYNC || checksum == ESCAPE)
	{
		outputBuffer[outputIndex++] = ESCAPE;
		outputBuffer[outputIndex++] = (checksum - 1);
	}
	else
	{
		outputBuffer[outputIndex++] = checksum;
	}

	debug(2, "OUTPUT:\n");
	debugBuffer(2, outputBuffer, outputIndex);

	int written = 0, timeout = 0;
	while (written < outputIndex)
	{
		if (written != 0)
			timeout = 0;

		if (timeout > JVS_RETRY_COUNT)
			return JVS_STATUS_ERROR_WRITE_FAIL;

		written += writeBytes(outputBuffer + written, outputIndex - written);
		timeout++;
	}

	return JVS_STATUS_SUCCESS;
}
