#include <string.h>
#include <math.h>

#include "jvs/io.h"
#include "console/debug.h"

int initIO(JVSIO *io)
{
	for (int player = 0; player < (io->capabilities.players + 1); player++)
		io->state.inputSwitch[player] = 0;

	for (int analogueChannels = 0; analogueChannels < io->capabilities.analogueInChannels; analogueChannels++)
		io->state.analogueChannel[analogueChannels] = 0;

	for (int rotaryChannels = 0; rotaryChannels < io->capabilities.rotaryChannels; rotaryChannels++)
		io->state.rotaryChannel[rotaryChannels] = 0;

	for (int player = 0; player < io->capabilities.coins; player++)
		io->state.coinCount[player] = 0;

	io->analogueMax = pow(2, io->capabilities.analogueInBits) - 1;
	io->gunXMax = pow(2, io->capabilities.gunXBits) - 1;
	io->gunYMax = pow(2, io->capabilities.gunYBits) - 1;

	return 1;
}

int setSwitch(JVSIO *io, JVSPlayer player, JVSInput switchNumber, int value)
{
	if (player > io->capabilities.players)
	{
		printf("Error: That player %d does not exist.\n", player);
		return 0;
	}

	if (value)
	{
		io->state.inputSwitch[player] |= switchNumber;
	}
	else
	{
		io->state.inputSwitch[player] &= ~switchNumber;
	}

	return 1;
}

int incrementCoin(JVSIO *io, JVSPlayer player, int amount)
{
	if (player == SYSTEM)
		return 0;

	io->state.coinCount[player - 1] = io->state.coinCount[player - 1] + amount;
	return 1;
}

int setAnalogue(JVSIO *io, JVSInput channel, double value)
{
	if (channel >= io->capabilities.analogueInChannels)
		return 0;
	io->state.analogueChannel[channel] = (int)((double)value * (double)io->analogueMax);
	return 1;
}

int setGun(JVSIO *io, JVSInput channel, double value)
{
	if (channel % 2 == 0)
	{
		io->state.gunChannel[channel] = (int)((double)value * (double)io->gunXMax);
	}
	else
	{
		io->state.gunChannel[channel] = (int)((double)((double)1.0 - value) * (double)io->gunYMax);
	}
	return 1;
}

int setRotary(JVSIO *io, JVSInput channel, double value)
{
	if (channel >= io->capabilities.rotaryChannels)
		return 0;

	value = value * 2 - 1;

	if(value > 0.2 || value < -0.2)
		io->state.rotaryChannel[channel] = io->state.rotaryChannel[channel] + value;
	printf("H %d\n", io->state.rotaryChannel[channel] );
	return 1;
}

JVSInput jvsInputFromString(char *jvsInputString)
{
	for (long unsigned int i = 0; i < sizeof(jvsInputConversion) / sizeof(jvsInputConversion[0]); i++)
	{
		if (strcmp(jvsInputConversion[i].string, jvsInputString) == 0)
			return jvsInputConversion[i].input;
	}
	debug(0, "Error: Could not find the JVS INPUT string specified for %s\n", jvsInputString);
	return -1;
}

JVSPlayer jvsPlayerFromString(char *jvsPlayerString)
{
	for (long unsigned int i = 0; i < sizeof(jvsPlayerConversion) / sizeof(jvsPlayerConversion[0]); i++)
	{
		if (strcmp(jvsPlayerConversion[i].string, jvsPlayerString) == 0)
			return jvsPlayerConversion[i].player;
	}
	debug(0, "Error: Could not find the JVS PLAYER string specified for %s\n", jvsPlayerString);
	return -1;
}
