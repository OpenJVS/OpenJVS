#include "ffb.h"

#include <linux/input.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include "debug.h"

/* Number of bits for 1 unsigned char */
#define nBitsPerUchar (sizeof(unsigned char) * 8)

/* Number of unsigned chars to contain a given number of bits */
#define nUcharsForNBits(nBits) ((((nBits)-1) / nBitsPerUchar) + 1)

/* Index=Offset of given bit in 1 unsigned char */
#define bitOffsetInUchar(bit) ((bit) % nBitsPerUchar)

/* Index=Offset of the unsigned char associated to the bit
   at the given index=offset */
#define ucharIndexForBit(bit) ((bit) / nBitsPerUchar)

/* Value of an unsigned char with bit set at given index=offset */
#define ucharValueForBit(bit) (((unsigned char)(1)) << bitOffsetInUchar(bit))

/* Test the bit with given index=offset in an unsigned char array */
#define testBit(bit, array) ((array[ucharIndexForBit(bit)] >> bitOffsetInUchar(bit)) & 1)
unsigned char ff_bits[1 + FF_MAX / 8 / sizeof(unsigned char)];

int device_handle = -1;
struct input_event event;
struct ff_effect effect;

unsigned char lastFFB;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
	(byte & 0x80 ? '1' : '0'),     \
		(byte & 0x40 ? '1' : '0'), \
		(byte & 0x20 ? '1' : '0'), \
		(byte & 0x10 ? '1' : '0'), \
		(byte & 0x08 ? '1' : '0'), \
		(byte & 0x04 ? '1' : '0'), \
		(byte & 0x02 ? '1' : '0'), \
		(byte & 0x01 ? '1' : '0')

unsigned char reverse(unsigned char b)
{
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

int setCentering(int value)
{
	if (device_handle == -1)
		return 0;

	memset(&event, 0, sizeof(event));
	event.type = EV_FF;
	event.code = FF_AUTOCENTER;
	event.value = 0xFFFFUL * value / 100;
	if (write(device_handle, &event, sizeof(event)) != sizeof(event))
	{
		fprintf(stderr, "ERROR: failed to disable auto centering (%s) [%s:%d]\n",
				strerror(errno), __FILE__, __LINE__);
		return 0;
	}

	return 1;
}

int setUncentering(int value)
{
	return value;
}

int setGain(int value)
{
	if (device_handle == -1)
		return 0;

	memset(&event, 0, sizeof(event));
	event.type = EV_FF;
	event.code = FF_GAIN;
	event.value = 0xFFFFUL * value / 100;
	if (write(device_handle, &event, sizeof(event)) != sizeof(event))
	{
		fprintf(stderr, "ERROR: failed to disable gain (%s) [%s:%d]\n",
				strerror(errno), __FILE__, __LINE__);
		return 0;
	}

	return 1;
}

int setForce(double force)
{

	/* Set force */
	if (force > 1.0)
		force = 1.0;
	else if (force < -1.0)
		force = -1.0;
	effect.u.constant.level = (short)(force * 32767.0);
	effect.direction = 0xC000;
	effect.u.constant.envelope.attack_level = (short)(force * 32767.0); /* this one counts! */
	effect.u.constant.envelope.fade_level = (short)(force * 32767.0);	/* only to be safe */

	/* Upload effect */
	if (ioctl(device_handle, EVIOCSFF, &effect) < 0)
	{
		perror("upload effect");
		/* We do not exit here. Indeed, too frequent updates may be
		 * refused, but that is not a fatal error */
	}
	return 1;
}

int initFFB(int fd)
{

	debug(0, "OpenJVS FFB LIB Loaded\n");

	/* Only setup FFB for the first wheel */
	if (device_handle != -1)
		return 0;

	/* Update global device handle */
	device_handle = fd;

	/* Now get some information about force feedback */
	memset(ff_bits, 0, sizeof(ff_bits));
	if (ioctl(device_handle, EVIOCGBIT(EV_FF, sizeof(ff_bits)), ff_bits) < 0)
	{
		fprintf(stderr, "ERROR: can not get ff bits (%s) [%s:%d]\n",
				strerror(errno), __FILE__, __LINE__);
		return 0;
	}

	/* force feedback supported? */
	if (!testBit(FF_CONSTANT, ff_bits))
	{
		fprintf(stderr, "ERROR: device (or driver) has no constant force feedback support [%s:%d]\n",
				__FILE__, __LINE__);
		return 0;
	}

	/* Setup constant force */
	/* Initialize constant force effect */
	memset(&effect, 0, sizeof(effect));
	effect.type = FF_CONSTANT;
	effect.id = -1;
	effect.trigger.button = 0;
	effect.trigger.interval = 0;
	effect.replay.length = 0xffff;
	effect.replay.delay = 0;
	effect.u.constant.level = 0;
	effect.direction = 0xC000;
	effect.u.constant.envelope.attack_length = 0;
	effect.u.constant.envelope.attack_level = 0;
	effect.u.constant.envelope.fade_length = 0;
	effect.u.constant.envelope.fade_level = 0;

	/* Upload effect */
	if (ioctl(device_handle, EVIOCSFF, &effect) < 0)
	{
		fprintf(stderr, "ERROR: uploading effect failed (%s) [%s:%d]\n",
				strerror(errno), __FILE__, __LINE__);
		return 0;
	}

	/* Start effect */
	memset(&event, 0, sizeof(event));
	event.type = EV_FF;
	event.code = effect.id;
	event.value = 1;
	if (write(device_handle, &event, sizeof(event)) != sizeof(event))
	{
		fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",
				strerror(errno), __FILE__, __LINE__);
		return 0;
	}

	return 1;
}

int processJVSFFB(JVSState *localState)
{
	unsigned char analogueData = (int)((double)(localState->analogueChannel[0] / 1023.0) * 255);

	/* Run any extra simulators */
	setSwitch(PLAYER_1, BUTTON_1, !((analogueData >> 0) & 0x01));
	setSwitch(PLAYER_2, BUTTON_1, !((analogueData >> 1) & 0x01));

	setSwitch(PLAYER_1, BUTTON_2, !((analogueData >> 2) & 0x01));
	setSwitch(PLAYER_2, BUTTON_2, !((analogueData >> 3) & 0x01));

	setSwitch(PLAYER_1, BUTTON_3, !((analogueData >> 4) & 0x01));
	setSwitch(PLAYER_2, BUTTON_3, !((analogueData >> 5) & 0x01));

	setSwitch(PLAYER_1, BUTTON_4, !((analogueData >> 6) & 0x01));
	setSwitch(PLAYER_2, BUTTON_4, !((analogueData >> 7) & 0x01));

	unsigned char ffb;
	ffb = localState->generalPurposeOutput[0] << 6 | localState->generalPurposeOutput[1] >> 2;

	if (ffb != lastFFB)
	{

		int found = 0;
		if (ffb == 0x3E)
		{
			printf("REPEAT?\n");
			found = 1;
		}

		if (ffb == 0)
		{
			printf("ATTRACT MODE?\n");
			setForce(0);
			setGain(100);
			setCentering(100);
			found = 1;
		}

		if (ffb == 0x3F)
		{
			printf("IN GAME MODE?\n");
			setCentering(100);
			setForce(0);
			setGain(100);
			found = 1;
		}

		if (ffb == 0xAF)
		{
			printf("INIT 01!\n");
			setCentering(100);
			setForce(0);
			setGain(100);
			found = 1;
		}

		if (ffb == 0x7F)
		{
			printf("CHILL OUT MODE?\n");
			setForce(0);
			setGain(0);
			setCentering(0);
			found = 1;
		}

		if (ffb == 0xF7)
		{
			printf("INIT 02?\n");
			found = 1;
		}

		if (ffb == 0x1A)
		{
			printf("NORMAL OP?");
			found = 1;
		}

		if (ffb == 0xBD)
		{
			printf("RESET\n");
			setForce(0);
			setGain(100);
			//setCentering(100);
			found = 1;
		}

		if (ffb == 0xFF)
		{
			printf("INIT");
			setForce(0);
			setGain(100);
			setCentering(100);
			found = 1;
		}

		int value = 100 - (int)(((double)(reverse(ffb) & 0x07) / 7.0) * 100);
		if ((ffb & 0x1E) == 24)
		{
			//setGain(100);
			//setCentering(0);
			printf("ROLL RIGHT %d\n", value);
			setForce((double)(value / 100.0));

			found = 1;
		}

		if ((ffb & 0x1E) == 0x14)
		{
			printf("ROLL LEFT %d\n", value);
			//setGain(100);
			//setCentering(0);
			setForce((double)(value / 100.0) * -1.0);

			found = 1;
		}

		if ((ffb & 0x1E) == 0x1A)
		{
			printf("CLUTCH %d\n", value);
			setGain(value);
			found = 1;
		}

		if ((ffb & 0x1E) == 0x1c)
		{
			//printf("UNCENTER %d\n", value);
			found = 1;
		}

		if ((ffb & 0x1E) == 0x02)
		{
			printf("CENTER %d\n", value);
			setCentering(100);
			setForce(0);
			setGain(100);
			found = 1;
		}

		if (!found)
		{
			printf("UNKNOWN " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(ffb));

			printf("\n");
		}

		lastFFB = ffb;
	}

	return 1;
}