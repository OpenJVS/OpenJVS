#ifndef ROTARY_H_
#define ROTARY_H_

typedef enum
{
    JVS_ROTARY_STATUS_ERROR,
    JVS_ROTARY_STATUS_SUCCESS
} JVSRotaryStatus;

JVSRotaryStatus initRotary();
int getRotaryValue();

#endif // ROTARY_H_
