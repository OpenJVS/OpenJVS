#include "ffb/ffb.h"
#include "console/debug.h"
#include "hardware/device.h"
#include "ffb/wheel.h"
#include "jvs/jvs.h"

void *ffbThread(void *_args);

#define READY 0x00
#define NOT_INIT 0x11
#define BUSY 0x44

FFBStatus initFFB(FFBState *state, JVSIO *io, FFBEmulationType type, char *serialPath)
{
    debug(1, "Init FFB %s\n", serialPath);
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
    printf("bindController() Called\n");

    if (state->controller > 0)
        return FFB_STATUS_ERROR_CONTROLLED_ALREADY_BOUND;

    if (initWheel(controller) == 0)
    {
        printf("Failed to init controller\n");
    }

    setCentering(0);
    setGain(100);
    setForce(0);

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

    int wheelMax = state->io->analogueMax;
    double wheelPos = (double)state->io->state.analogueChannel[0] / (double)wheelMax;

    unsigned char buffer[4] = {0};
    int index = 0;

    double amount = 0.1;

    setRumble(0);
    setForce(0);
    setCentering(0);
    setGain(100);

    while (state->running)
    {
        int keepAlive = 0;
        wheelPos = (double)state->io->state.analogueChannel[0] / (double)wheelMax;
        unsigned char byte = 0;
        if (readSerialBytes(state->serial, &byte, 1) > 0)
        {
            buffer[index++] = byte;
        }
        if (index == 4)
        {

            index = 0;

            switch (buffer[0])
            {
            case 0x0B:
                setCentering(100);
                break;
            case 0xFF: // Reset
                printf("RESET\n");
                reply = READY;
                break;
            case 0x81:
                printf("RESET 2\n");
                reply = NOT_INIT;
                break;
            case 0xFC:
                setCentering(0);
                printf("START_INIT\n");
                turnWheel = 1;
                break;
            case 0x80:
            {
                if (buffer[1] == 0 && buffer[2] == 0)
                {
                    printf("STOP!\n");
                    setForce(0);
                }
                if (buffer[1] == 1 && buffer[2] == 1)
                {

                    if (wheelPos >= 0.9 && amount > 0)
                        break;

                    if (wheelPos <= 0.1 && amount < 0)
                        break;

                    setForce(amount);
                }
            }
            break;
            case 0x9e:
            case 0x84: // Set the moving amount
            {
                //keepAlive = 1;
                reply = READY;
                if (buffer[1] == 1)
                    amount = (-1 * ((double)buffer[2] / 128.0)) * 2;

                if (buffer[1] == 0)
                    amount = (1 - ((double)buffer[2] / 128.0)) * 2;

                printf("SET_AMOUNT %f\n", amount);

                setForce(amount);
            }
            break;
            case 0xFA:
            case 0xFD:
            {
                keepAlive = 1;

                // Sort the auto turn right init
                if (turnWheel)
                {
                    reply = BUSY;
                    setForce(0.2);
                    if (wheelPos > 0.9)
                    {
                        reply = READY;
                        turnWheel = 0;
                        setForce(0);
                    }
                }
            }
            break;
            case 0xFB: // Rumble Command
            {
                printf("0x%02hhX 0x%02hhX 0x%02hhX 0x%02hhX %d\n", buffer[0], buffer[1], buffer[2], buffer[3], reply);
                printf("RUMBLE\n");
                switch (buffer[2])
                {
                case 0x00: // Grass
                    setRumble(10);
                    break;
                case 0x02:
                    setRumble(20);
                    break;
                case 0x10: // Car
                    setRumble(50);
                    break;
                case 0x0B: // Hit Left
                case 0x1B: // Hit Right
                    setRumble(80);
                    break;
                case 0x04:
                { // Road Again?
                    printf("ROAD AGAIN\n");
                }
                break;
                }
                setCentering(100);
            }
            break;
            default:
                printf("0x%02hhX 0x%02hhX 0x%02hhX 0x%02hhX %d\n", buffer[0], buffer[1], buffer[2], buffer[3], reply);
                break;
            }

            if (!keepAlive)
                setForce(0);

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
