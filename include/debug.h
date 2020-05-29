#ifndef DEBUG_H_
#define DEBUG_H_

#include "jvs.h"

int initDebug(int level);
void debug(int level, const char *format, ...);
void debugPacket(int level, JVSPacket *packet);
void debugBuffer(int level, char *buffer, int length);

#endif // DEBUG_H_
