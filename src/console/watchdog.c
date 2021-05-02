#include "console/watchdog.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "controller/input.h"

// Poll rotary every one second
#define TIME_POLL_ROTARY 1

pthread_t watchdogThreadID;

typedef struct
{
    volatile int *running;
    JVSRotaryStatus rotaryStatus;

} WatchdogThreadArguments;

void *watchdogThread(void *_args)
{
    WatchdogThreadArguments *args = (WatchdogThreadArguments *)_args;

    DeviceList deviceList;
    int originalDevicesCount = 0;
    if (getInputs(&deviceList))
    {
        originalDevicesCount = deviceList.length;
    }

    int rotaryValue = -1;
    time_t rotaryTime = time(NULL);

    if (args->rotaryStatus == JVS_ROTARY_STATUS_SUCCESS)
        rotaryValue = getRotaryValue();

    while (args->running)
    {
        time_t now = time(NULL);
        if ((now - rotaryTime) > TIME_POLL_ROTARY)
        {
            if (args->rotaryStatus == JVS_ROTARY_STATUS_SUCCESS && rotaryValue != getRotaryValue())
            {
                *args->running = 0;
                break;
            }

            if (getInputs(&deviceList) == 0 || deviceList.length != originalDevicesCount)
            {
                *args->running = 0;
                break;
            }
            rotaryTime = now;
        }
    }

    free(_args);

    return 0;
}

WatchdogStatus initWatchdog(volatile int *running, JVSRotaryStatus rotaryStatus)
{
    WatchdogThreadArguments *args = malloc(sizeof(WatchdogThreadArguments));
    args->running = running;
    args->rotaryStatus = rotaryStatus;

    if (pthread_create(&watchdogThreadID, NULL, watchdogThread, args) != 0)
    {
        return WATCHDOG_STATUS_ERROR;
    }

    return WATCHDOG_STATUS_SUCCESS;
}