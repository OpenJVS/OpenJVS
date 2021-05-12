#ifndef FFB_H_
#define FFB_H_

#include <pthread.h>

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
} FFBState;

FFBStatus initFFB(FFBState *state, FFBEmulationType type, char *serialPath);
FFBStatus bindController(FFBState *state, int controller);
FFBStatus closeFFB(FFBState *state);

#endif // FFB_H_
