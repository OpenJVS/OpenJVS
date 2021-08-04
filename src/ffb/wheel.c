#include "ffb/wheel.h"

#include <linux/input.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include "console/debug.h"

/* Number of bits for 1 unsigned char */
#define nBitsPerUchar (sizeof(unsigned char) * 8)

/* Number of unsigned chars to contain a given number of bits */
#define nUcharsForNBits(nBits) ((((nBits)-1) / nBitsPerUchar) + 1)

/* Index=Offset of given bit in 1 unsigned char */
#define bitOffsetInUchar(bit) ((bit) % nBitsPerUchar)

/* Index=Offset of the unsigned char associated to the bit
   at the given index=offset */
#define ucharIndexForBit(bit) ((bit) / nBitsPerUchar)

/* Value of an unsigned char with bit set at given index=offset */
#define ucharValueForBit(bit) (((unsigned char)(1)) << bitOffsetInUchar(bit))

/* Test the bit with given index=offset in an unsigned char array */
#define testBit(bit, array) ((array[ucharIndexForBit(bit)] >> bitOffsetInUchar(bit)) & 1)
unsigned char ff_bits[1 + FF_MAX / 8 / sizeof(unsigned char)];

int device_handle = -1;
struct input_event event;
struct ff_effect forceEffect;
struct ff_effect rumbleEffect;

unsigned char lastFFB;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

unsigned char reverse(unsigned char b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

int setCentering(int value)
{
    printf("Device handle %d\n", device_handle);
    if (device_handle == -1)
        return 0;

    memset(&event, 0, sizeof(event));
    event.type = EV_FF;
    event.code = FF_AUTOCENTER;
    event.value = 0xFFFFUL * value / 100;
    if (write(device_handle, &event, sizeof(event)) != sizeof(event))
    {
        fprintf(stderr, "ERROR: failed to disable auto centering (%s) [%s:%d]\n",
                strerror(errno), __FILE__, __LINE__);
        return 0;
    }

    return 1;
}

int setUncentering(int value)
{
    return value;
}

int setGain(int value)
{
    if (device_handle == -1)
        return 0;

    memset(&event, 0, sizeof(event));
    event.type = EV_FF;
    event.code = FF_GAIN;
    event.value = 0xFFFFUL * value / 100;
    if (write(device_handle, &event, sizeof(event)) != sizeof(event))
    {
        fprintf(stderr, "ERROR: failed to disable gain (%s) [%s:%d]\n",
                strerror(errno), __FILE__, __LINE__);
        return 0;
    }

    return 1;
}

int setForce(double force)
{

    /* Set force */
    if (force > 1.0)
        force = 1.0;
    else if (force < -1.0)
        force = -1.0;
    forceEffect.u.constant.level = (short)(force * 32767.0);
    forceEffect.direction = 0xC000;
    forceEffect.u.constant.envelope.attack_level = (short)(force * 32767.0); /* this one counts! */
    forceEffect.u.constant.envelope.fade_level = (short)(force * 32767.0);   /* only to be safe */

    /* Upload effect */
    if (ioctl(device_handle, EVIOCSFF, &forceEffect) < 0)
    {
        perror("upload effect");
        /* We do not exit here. Indeed, too frequent updates may be
		 * refused, but that is not a fatal error */
    }
    return 1;
}

int setRumble(double force)
{
    rumbleEffect.u.periodic.magnitude = 0xffff * (force / 100.0);
    rumbleEffect.u.periodic.envelope.attack_level = 0xffff * (force / 100.0);
    rumbleEffect.u.periodic.envelope.fade_level = 0xffff * (force / 100.0);

    if (ioctl(device_handle, EVIOCSFF, &rumbleEffect) < 0)
        perror("upload effect rumble");

    /* Start effect */
    memset(&event, 0, sizeof(event));
    event.type = EV_FF;
    event.code = rumbleEffect.id;
    event.value = 1;
    if (write(device_handle, &event, sizeof(event)) != sizeof(event))
    {
        fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",
                strerror(errno), __FILE__, __LINE__);
        return 0;
    }

    return 1;
}

int initWheel(int fd)
{

    debug(0, "OpenJVS FFB Wheel LIB Loaded\n");

    /* Only setup FFB for the first wheel */
    if (device_handle != -1)
        return 0;

    /* Update global device handle */
    device_handle = fd;
    printf("dev hand %d\n", device_handle);

    /* Now get some information about force feedback */
    memset(ff_bits, 0, sizeof(ff_bits));
    if (ioctl(device_handle, EVIOCGBIT(EV_FF, sizeof(ff_bits)), ff_bits) < 0)
    {
        fprintf(stderr, "ERROR: can not get ff bits (%s) [%s:%d]\n",
                strerror(errno), __FILE__, __LINE__);
        return 0;
    }

    /* force feedback supported? */
    if (!testBit(FF_CONSTANT, ff_bits))
    {
        fprintf(stderr, "ERROR: device (or driver) has no constant force feedback support [%s:%d]\n",
                __FILE__, __LINE__);
        return 0;
    }

    /* force feedback supported? */
    if (!testBit(FF_PERIODIC, ff_bits))
    {
        fprintf(stderr, "ERROR: device (or driver) has no rumble support [%s:%d]\n",
                __FILE__, __LINE__);
        return 0;
    }

    memset(&rumbleEffect, 0, sizeof(rumbleEffect));
    rumbleEffect.type = FF_PERIODIC;
    rumbleEffect.id = -1;
    rumbleEffect.u.periodic.waveform = FF_SINE;
    rumbleEffect.u.periodic.period = 100;  /* 0.1 second */
    rumbleEffect.u.periodic.magnitude = 0; /* 0.5 * Maximum magnitude */
    rumbleEffect.u.periodic.offset = 0;
    rumbleEffect.u.periodic.phase = 0;
    rumbleEffect.direction = 0x4000; /* Along X axis */
    rumbleEffect.u.periodic.envelope.attack_length = 1000;
    rumbleEffect.u.periodic.envelope.attack_level = 0;
    rumbleEffect.u.periodic.envelope.fade_length = 1000;
    rumbleEffect.u.periodic.envelope.fade_level = 0;
    rumbleEffect.trigger.button = 0;
    rumbleEffect.trigger.interval = 0;
    rumbleEffect.replay.length = 100;

    /* Upload effect */
    if (ioctl(device_handle, EVIOCSFF, &rumbleEffect) < 0)
    {
        fprintf(stderr, "ERROR: uploading effect failed (%s) [%s:%d]\n",
                strerror(errno), __FILE__, __LINE__);
        return 0;
    }

    /* Start effect */
    memset(&event, 0, sizeof(event));
    event.type = EV_FF;
    event.code = rumbleEffect.id;
    event.value = 1;
    if (write(device_handle, &event, sizeof(event)) != sizeof(event))
    {
        fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",
                strerror(errno), __FILE__, __LINE__);
        return 0;
    }

    /* Setup constant force */
    /* Initialize constant force effect */
    memset(&forceEffect, 0, sizeof(forceEffect));
    forceEffect.type = FF_CONSTANT;
    forceEffect.id = -1;
    forceEffect.trigger.button = 0;
    forceEffect.trigger.interval = 0;
    forceEffect.replay.length = 0xffff;
    forceEffect.replay.delay = 0;
    forceEffect.u.constant.level = 0;
    forceEffect.direction = 0xC000;
    forceEffect.u.constant.envelope.attack_length = 0;
    forceEffect.u.constant.envelope.attack_level = 0;
    forceEffect.u.constant.envelope.fade_length = 0;
    forceEffect.u.constant.envelope.fade_level = 0;

    /* Upload effect */
    if (ioctl(device_handle, EVIOCSFF, &forceEffect) < 0)
    {
        fprintf(stderr, "ERROR: uploading effect failed (%s) [%s:%d]\n",
                strerror(errno), __FILE__, __LINE__);
        return 0;
    }

    /* Start effect */
    memset(&event, 0, sizeof(event));
    event.type = EV_FF;
    event.code = forceEffect.id;
    event.value = 1;
    if (write(device_handle, &event, sizeof(event)) != sizeof(event))
    {
        fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",
                strerror(errno), __FILE__, __LINE__);
        return 0;
    }

    return 1;
}
