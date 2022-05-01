#include "ffb/ffb.h"
#include "console/debug.h"
#include "controller/threading.h"

void *ffbThread(void *_args);

FFBStatus initFFB(FFBState *state, FFBEmulationType type, JVSIO *io, char *serialPath)
{
    state->type = type;
    state->io = io;
    state->serial = -1;
    state->controller = -1;
    state->running = 1;

    /* JVS FFB doesn't require an extra thread */
    if (state->type == FFB_EMULATION_TYPE_SEGA_JVS)
        return FFB_STATUS_SUCCESS;

    if (createThread(ffbThread, state) != THREAD_STATUS_SUCCESS)
        return FFB_STATUS_ERROR;

    return FFB_STATUS_SUCCESS;
}

FFBStatus closeFFB(FFBState *state)
{
    state->running = 0;
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

FFBStatus processFFB(FFBState *state)
{
    printf("Processing FFB\n");

    unsigned char *GPIO = state->io->state.generalPurposeOutput;

    unsigned char command = GPIO[0] << 6 | GPIO[1] >> 2;

    printf("Hello FFB %X\n", command);

    return FFB_STATUS_SUCCESS;
}

void *ffbThread(void *_args)
{
    FFBState *state = (FFBState *)_args;

    debug(0, "Hello from the thread %d\n", state->serial);

    unsigned char *GPIO = state->io->state.generalPurposeOutput;

    while (state->running)
    {
        unsigned char command = GPIO[0] << 6 | GPIO[1] >> 2;

        printf("Hello FFB %X\n", command);

        sleep(5);
    }

    return 0;
}
