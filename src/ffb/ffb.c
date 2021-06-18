#include "ffb/ffb.h"
#include "console/debug.h"
#include "hardware/device.h"
#include "ffb/wheel.h"

void *ffbThread(void *_args);

#define READY 0x00
#define NOT_INIT 0x11
#define BUSY 0x44

FFBStatus initFFB(FFBState *state, JVSIO *io, FFBEmulationType type, char *serialPath)
{
    debug(0, "Init ffb %s\n", serialPath);
    state->type = type;
    state->serial = -1;
    state->controller = -1;
    state->io = io;

    if ((state->serial = open(serialPath, O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY)) < 0)
        return FFB_STATUS_ERROR;

    state->running = 1;
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
    printf("Controller bounding\n");

    if (state->controller > 0)
        return FFB_STATUS_ERROR_CONTROLLED_ALREADY_BOUND;

    if (initWheel(controller) == 0)
    {
        printf("Failed to init controller\n");
    }
    else
    {
        printf("Controller bound all good\n");
        state->controller = controller;
    }

    return FFB_STATUS_SUCCESS;
}

static int readSerialBytes(int serialIO, unsigned char *buffer, int amount)
{
    fd_set fd_serial;
    struct timeval tv;

    FD_ZERO(&fd_serial);
    FD_SET(serialIO, &fd_serial);

    tv.tv_sec = 0;
    tv.tv_usec = 1000 * 1000;

    int filesReadyToRead = select(serialIO + 1, &fd_serial, NULL, NULL, &tv);

    if (filesReadyToRead < 1)
        return -1;

    if (!FD_ISSET(serialIO, &fd_serial))
        return -1;

    return read(serialIO, buffer, amount);
}

static int writeSerialBytes(int serialIO, unsigned char *buffer, int amount)
{
    return write(serialIO, buffer, amount);
}

void processSEGAFFB(FFBState *state)
{
    /* Setup the serial connection */
    setSerialAttributes(state->serial, B38400);

    unsigned char reply = READY;

    int turnWheel = 0;

    double wheelPos = 0.5;

    unsigned char buffer[4] = {0};
    int index = 0;

    int move = 0;
    double amount = 0.01;
    while (state->running)
    {
        unsigned char byte = 0;
        if (readSerialBytes(state->serial, &byte, 1) > 0)
        {
            buffer[index++] = byte;
        }

        if (index == 4)
        {
            index = 0;
            printf("0x%02hhX 0x%02hhX 0x%02hhX 0x%02hhX %d\n", buffer[0], buffer[1], buffer[2], buffer[3], reply);

            switch (buffer[0])
            {
            case 0xFF: // Reset
                printf("RESET\n");
                wheelPos = 0.5;
                reply = READY;
                break;
            case 0x81:
                printf("Unsure, not init\n");
                reply = NOT_INIT;
                break;
            case 0xFC:
                printf("WHEEL INIT\n");
                turnWheel = 1;
                break;
            case 0x9D:
            {
                if (move)
                {
                    printf("0x80 Moving");
                    if (wheelPos >= 1 && amount > 0)
                    {
                        break;
                    }

                    if (wheelPos <= 0 && amount < 0)
                    {
                        break;
                    }

                    wheelPos = wheelPos + amount;
                }
            }
            break;

            case 0x80:
            {
                if (buffer[1] == 0 && buffer[2] == 0)
                {
                    printf("STOP!\n");
                    move = 0;
                }
                if (buffer[1] == 1 && buffer[2] == 1)
                {

                    if (move)
                    {
                        printf("0x80 Moving");
                        if (wheelPos >= 1 && amount > 0)
                        {
                            break;
                        }

                        if (wheelPos <= 0 && amount < 0)
                        {
                            break;
                        }

                        wheelPos = wheelPos + amount;
                    }
                }
            }
            break;
            case 0x9e:
            case 0x84: // Move
            {
                printf("Moving 0x%02hhX 0x%02hhX 0x%02hhX 0x%02hhX %d\n", buffer[0], buffer[1], buffer[2], buffer[3], reply);

                move = 1;
                if (buffer[1] == 1)
                {
                    amount = -0.0001 * buffer[2];
                }

                if (buffer[1] == 0)
                {
                    amount = 0.0001 * buffer[2];
                }
            }
            break;
            case 0xFD:
            {
                printf("Keep alive\n");
                if (turnWheel)
                {
                    printf("Auto spin to right\n");
                    reply = BUSY;
                    wheelPos += 0.01;
                    if (wheelPos > 0.6)
                    {
                        reply = READY;
                        turnWheel = 0;
                    }
                }

                if (move)
                {
                    printf("BOBBY DILLEY - Moving %f %f\n", wheelPos, amount);
                    if (wheelPos >= 1 && amount > 0)
                    {
                        break;
                    }

                    if (wheelPos <= 0 && amount < 0)
                    {
                        break;
                    }

                    wheelPos = wheelPos + amount;
                }
            }
            break;
            default:
                printf("0x%02hhX 0x%02hhX 0x%02hhX 0x%02hhX %d\n", buffer[0], buffer[1], buffer[2], buffer[3], reply);

                break;
            }
            printf(" WHEEL POS %f\n", wheelPos);

            setAnalogue(state->io, ANALOGUE_1, wheelPos);

            writeSerialBytes(state->serial, &reply, 1);
        }
    }
}

void *ffbThread(void *_state)
{
    FFBState *state = (FFBState *)_state;

    switch (state->type)
    {
    case FFB_EMULATION_TYPE_SEGA:
        processSEGAFFB(state);
        break;
    default:
        break;
    }

    tcflush(state->serial, TCIOFLUSH);
    close(state->serial);

    return 0;
}
