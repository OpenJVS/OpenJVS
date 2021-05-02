#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#include "hardware/rotary.h"

typedef enum
{
    WATCHDOG_STATUS_SUCCESS,
    WATCHDOG_STATUS_ERROR
} WatchdogStatus;

WatchdogStatus initWatchdog(volatile int *running, JVSRotaryStatus rotaryStatus);

#endif // WATCHDOG_H_
