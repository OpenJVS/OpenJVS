#include "ffb/ffb.h"
#include "console/debug.h"

void *ffbThread(void *_args);

FFBStatus initFFB(FFBState *state, FFBEmulationType type, char *serialPath)
{
    debug(0, "Init ffb %s\n", serialPath);
    state->type = type;
    state->serial = -1;
    state->controller = -1;

    if (pthread_create(&state->threadID, NULL, ffbThread, state) != 0)
    {
        return FFB_STATUS_ERROR;
    }

    return FFB_STATUS_SUCCESS;
}

FFBStatus closeFFB(FFBState *state)
{
    state->serial = -1;
    return FFB_STATUS_SUCCESS;
}

FFBStatus bindController(FFBState *state, int controller)
{
    if (state->controller > -1)
        return FFB_STATUS_ERROR_CONTROLLED_ALREADY_BOUND;

    state->controller = controller;

    return FFB_STATUS_SUCCESS;
}

void *ffbThread(void *_args)
{
    FFBState *args = (FFBState *)_args;

    debug(0, "Hello from the thread %d\n", args->serial);

    return 0;
}
