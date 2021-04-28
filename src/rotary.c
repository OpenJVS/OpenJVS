#include "rotary.h"
#include "device.h"
#include "debug.h"

/**
 * Init Rotary on Raspberry Pi HAT
 * 
 * Inits the rotary controller on the Raspberry Pi HAT
 * to select which map we will use
 * 
 * @returns JVS_ROTARY_STATUS_SUCCESS if it inited correctly.
 */
JVSRotaryStatus initRotary()
{
    setupGPIO(18);
    if (!setGPIODirection(18, IN))
    {
        debug(1, "Warning: Failed to set Raspberry Pi GPIO Pin 18");
        return JVS_ROTARY_STATUS_ERROR;
    }

    setupGPIO(19);
    if (!setGPIODirection(19, IN))
    {
        debug(1, "Warning: Failed to set Raspberry Pi GPIO Pin 19");
        return JVS_ROTARY_STATUS_ERROR;
    }

    setupGPIO(20);
    if (!setGPIODirection(20, IN))
    {
        debug(1, "Warning: Failed to set Raspberry Pi GPIO Pin 20");
        return JVS_ROTARY_STATUS_ERROR;
    }

    setupGPIO(21);
    if (!setGPIODirection(21, IN))
    {
        debug(1, "Warning: Failed to set Raspberry Pi GPIO Pin 21");
        return JVS_ROTARY_STATUS_ERROR;
    }

    return JVS_ROTARY_STATUS_SUCCESS;
}

/**
 * Get rotary value
 * 
 * Returns the value from 0 to 15 for
 * which map to use.
 * 
 * @returns The value from 0 to 15 on the rotary encoder
 */
int getRotaryValue()
{
    int value = 0;
    value = value | readGPIO(18) << 0;
    value = value | readGPIO(19) << 1;
    value = value | readGPIO(20) << 2;
    value = value | readGPIO(21) << 3;

    value = ~value & 0x0F;

    return value;
}
