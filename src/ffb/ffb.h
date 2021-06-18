#ifndef FFB_H_
#define FFB_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

#include "jvs/io.h"

typedef enum
{
    FFB_STATUS_SUCCESS,
    FFB_STATUS_ERROR,
    FFB_STATUS_ERROR_CONTROLLED_ALREADY_BOUND
} FFBStatus;

typedef enum
{
    FFB_EMULATION_TYPE_SEGA,
    FFB_EMULATION_TYPE_NAMCO
} FFBEmulationType;

typedef struct
{
    FFBEmulationType type;
    int controller;
    int serial;
    pthread_t threadID;
    volatile int running;
    JVSIO *io;
} FFBState;

FFBStatus initFFB(FFBState *state, JVSIO *io, FFBEmulationType type, char *serialPath);
FFBStatus bindController(FFBState *state, int controller);
FFBStatus closeFFB(FFBState *state);

#endif // FFB_H_
