#include "console/watchdog.h"
#include "console/debug.h"
#include "controller/threading.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "controller/input.h"

// Poll rotary every one second
#define TIME_POLL_ROTARY 1

typedef struct
{
    volatile int *running;
    JVSRotaryStatus rotaryStatus;

} WatchdogThreadArguments;

void *watchdogThread(void *_args)
{
    int error = 0;
    WatchdogThreadArguments *args = (WatchdogThreadArguments *)_args;

    DeviceList *deviceList = NULL;
    deviceList = malloc(sizeof(DeviceList));

    if (deviceList == NULL)
    {
        debug(0, "Error: Failed to malloc\n");
        error = -1;
    }

    if (error == 0)
    {
        int originalDevicesCount = 0;
        originalDevicesCount = getNumberOfDevices();

        int rotaryValue = -1;

        if (args->rotaryStatus == JVS_ROTARY_STATUS_SUCCESS)
        {
            rotaryValue = getRotaryValue();
        }

        while (getThreadsRunning())
        {
            if ((args->rotaryStatus == JVS_ROTARY_STATUS_SUCCESS) && (rotaryValue != getRotaryValue()))
            {
                *args->running = 0;
                break;
            }

            if ((getInputs(deviceList) == 0) || (deviceList->length != originalDevicesCount))
            {
                *args->running = 0;
                break;
            }
            sleep(TIME_POLL_ROTARY);
        }
    }

    if (deviceList != NULL)
    {
        free(deviceList);
        deviceList = NULL;
    }

    if (_args != NULL)
    {
        free(_args);
        _args = NULL;
    }

    return 0;
}

WatchdogStatus initWatchdog(volatile int *running, JVSRotaryStatus rotaryStatus)
{
    WatchdogThreadArguments *args = malloc(sizeof(WatchdogThreadArguments));
    args->running = running;
    args->rotaryStatus = rotaryStatus;

    if (THREAD_STATUS_SUCCESS != createThread(watchdogThread, args))
    {
        return WATCHDOG_STATUS_ERROR;
    }

    return WATCHDOG_STATUS_SUCCESS;
}
