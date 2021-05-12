#include "console/debug.h"

#include <stdarg.h>

int globalLevel = 0;

int initDebug(int level)
{
    globalLevel = level;
    if (globalLevel > 0)
    {
        debug(0, "\nWarning: OpenJVS is running in debug mode. This will slow down the overall emulation\n\n");
    }
    return 1;
}

void debug(int level, const char *format, ...)
{
    if (globalLevel < level)
        return;

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void debugBuffer(int level, unsigned char *buffer, int length)
{
    if (globalLevel < level)
        return;

    for (int i = 0; i < length; i++)
        printf("0x%02hhX ", buffer[i]);
    printf("\n");
}

void debugPacket(int level, JVSPacket *packet)
{
    if (globalLevel < level)
        return;

    printf("DESTINATION: %d\n", packet->destination);
    printf("LENGTH: %d\n", packet->length);
    printf("DATA: ");
    for (int i = 0; i < packet->length; i++)
    {
        printf("0x%02hhX ", packet->data[i]);
    }
    printf("\n");
}
