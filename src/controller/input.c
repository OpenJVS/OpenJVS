/**
 * OpenJVS Input Controller
 * Authors: Bobby Dilley, Redone, Fred 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <linux/input.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/select.h>
#include <math.h>

#include "controller/input.h"
#include "console/debug.h"
#include "console/config.h"
#include "controller/threading.h"

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1) / BITS_PER_LONG) + 1)
#define OFF(x) ((x) % BITS_PER_LONG)
#define LONG(x) ((x) / BITS_PER_LONG)
#define test_bit_diff(bit, array) ((array[LONG(bit)] >> OFF(bit)) & 1)

#define DEV_INPUT_EVENT "/dev/input"
#define test_bit(bit, array) (array[bit / 8] & (1 << (bit % 8)))

typedef struct
{
    ThreadSharedData *sharedData_p;
    JVSIO *jvsIO;
    char devicePath[MAX_PATH_LENGTH];
    EVInputs inputs;
    int player;
} MappingThreadArguments;

void *wiiDeviceThread(void *_args)
{
    MappingThreadArguments *args = (MappingThreadArguments *)_args;

    int fd = open(args->devicePath, O_RDONLY);
    if (fd < 0)
    {
        printf("Failed to open device descriptor");
        return 0;
    }

    struct input_event event;
    fd_set file_descriptor;
    struct timeval tv;

    /* Wii Remote Variables */
    int x0 = 0, x1 = 0, y0 = 0, y1 = 0;

    while (getThreadsRunning())
    {
        FD_ZERO(&file_descriptor);
        FD_SET(fd, &file_descriptor);

        tv.tv_sec = 0;
        tv.tv_usec = 2 * 1000;

        if (select(fd + 1, &file_descriptor, NULL, NULL, &tv) < 1)
            continue;

        if (read(fd, &event, sizeof(event)) == sizeof(event))
        {
            switch (event.type)
            {
            case EV_ABS:
            {
                bool outOfBounds = true;
                if (event.type == EV_ABS)
                {
                    switch (event.code)
                    {
                    case 16:
                        x0 = event.value;
                        break;
                    case 17:
                        y0 = event.value;
                        break;
                    case 18:
                        x1 = event.value;
                        break;
                    case 19:
                        y1 = event.value;
                        break;
                    }
                }

                if ((x0 != 1023) && (x1 != 1023) && (y0 != 1023) && (y1 != 1023))
                {
                    /* Set screen in player 1 */
                    setSwitch(args->jvsIO, args->player, args->inputs.key[KEY_O].output, 0);
                    int oneX, oneY, twoX, twoY;
                    if (x0 > x1)
                    {
                        oneY = y0;
                        oneX = x0;
                        twoY = y1;
                        twoX = x1;
                    }
                    else
                    {
                        oneY = y1;
                        oneX = x1;
                        twoY = y0;
                        twoX = x0;
                    }

                    /* Use some fancy maths that I don't understand fully */
                    double valuex = 512 + cos(atan2(twoY - oneY, twoX - oneX) * -1) * (((oneX - twoX) / 2 + twoX) - 512) - sin(atan2(twoY - oneY, twoX - oneX) * -1) * (((oneY - twoY) / 2 + twoY) - 384);
                    double valuey = 384 + sin(atan2(twoY - oneY, twoX - oneX) * -1) * (((oneX - twoX) / 2 + twoX) - 512) + cos(atan2(twoY - oneY, twoX - oneX) * -1) * (((oneY - twoY) / 2 + twoY) - 384);

                    double finalX = (((double)valuex / (double)1023) * 1.0);
                    double finalY = 1.0f - ((double)valuey / (double)1023);

                    // check for out-of-bound after rotation ..
                    if ((!(finalX > 1.0f) || (finalY > 1.0f) || (finalX < 0) || (finalY < 0)))
                    {
                        setAnalogue(args->jvsIO, args->inputs.abs[ABS_X].output, args->inputs.abs[ABS_X].reverse ? 1 - finalX : finalX);
                        setAnalogue(args->jvsIO, args->inputs.abs[ABS_Y].output, args->inputs.abs[ABS_Y].reverse ? 1 - finalY : finalY);
                        setGun(args->jvsIO, args->inputs.abs[ABS_X].output, args->inputs.abs[ABS_X].reverse ? 1 - finalX : finalX);
                        setGun(args->jvsIO, args->inputs.abs[ABS_Y].output, args->inputs.abs[ABS_Y].reverse ? 1 - finalY : finalY);

                        outOfBounds = false;
                    }
                }

                if (outOfBounds)
                {
                    /* Set screen out player 1 */
                    setSwitch(args->jvsIO, args->player, args->inputs.key[KEY_O].output, 1);

                    setAnalogue(args->jvsIO, args->inputs.abs[ABS_X].output, 0);
                    setAnalogue(args->jvsIO, args->inputs.abs[ABS_Y].output, 0);

                    setGun(args->jvsIO, args->inputs.abs[ABS_X].output, 0);
                    setGun(args->jvsIO, args->inputs.abs[ABS_Y].output, 0);
                }
                continue;
            }
            break;
            }
        }
    }

    close(fd);
    free(args);

    return 0;
}

void *deviceThread(void *_args)
{
    MappingThreadArguments *args = (MappingThreadArguments *)_args;

    int fd = open(args->devicePath, O_RDONLY);
    if (fd < 0)
    {
        printf("Critical: Failed to open device file descriptor %d \n", fd);
        free(args);
        return 0;
    }

    struct input_event event;

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    uint8_t absoluteBitmask[ABS_MAX / 8 + 1];
    struct input_absinfo absoluteFeatures;

    memset(absoluteBitmask, 0, sizeof(absoluteBitmask));
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absoluteBitmask)), absoluteBitmask) < 0)
        perror("Error: Failed to get bit mask for analogue values");

    for (int axisIndex = 0; axisIndex < ABS_MAX; ++axisIndex)
    {
        if (test_bit(axisIndex, absoluteBitmask))
        {
            if (ioctl(fd, EVIOCGABS(axisIndex), &absoluteFeatures))
                perror("Error: Failed to get device analogue limits");

            args->inputs.absMax[axisIndex] = (double)absoluteFeatures.maximum;
            args->inputs.absMin[axisIndex] = (double)absoluteFeatures.minimum;
        }
    }

    fd_set file_descriptor;
    struct timeval tv;

    while (getThreadsRunning())
    {

        FD_ZERO(&file_descriptor);
        FD_SET(fd, &file_descriptor);

        tv.tv_sec = 0;
        tv.tv_usec = 2 * 1000;

        if (select(fd + 1, &file_descriptor, NULL, NULL, &tv) < 1)
            continue;

        if (read(fd, &event, sizeof(event)) == sizeof(event))
        {
            switch (event.type)
            {

            case EV_KEY:
            {
                /* Check if the coin button has been pressed */
                if (args->inputs.key[event.code].output == COIN)
                {
                    if (event.value == 1)
                        incrementCoin(args->jvsIO, args->inputs.key[event.code].jvsPlayer, 1);

                    continue;
                }

                setSwitch(args->jvsIO, args->inputs.key[event.code].jvsPlayer, args->inputs.key[event.code].output, event.value == 0 ? 0 : 1);

                if (args->inputs.key[event.code].outputSecondary != NONE)
                    setSwitch(args->jvsIO, args->inputs.key[event.code].jvsPlayer, args->inputs.key[event.code].outputSecondary, event.value == 0 ? 0 : 1);
            }
            break;

            case EV_ABS:
            {
                /* Support HAT Controlls */
                if (args->inputs.abs[event.code].type == HAT)
                {

                    if (event.value == args->inputs.absMin[event.code])
                    {
                        setSwitch(args->jvsIO, args->inputs.abs[event.code].jvsPlayer, args->inputs.abs[event.code].output, 1);
                    }
                    else if (event.value == args->inputs.absMax[event.code])
                    {
                        setSwitch(args->jvsIO, args->inputs.abs[event.code].jvsPlayer, args->inputs.abs[event.code].outputSecondary, 1);
                    }
                    else
                    {
                        setSwitch(args->jvsIO, args->inputs.abs[event.code].jvsPlayer, args->inputs.abs[event.code].output, 0);
                        setSwitch(args->jvsIO, args->inputs.abs[event.code].jvsPlayer, args->inputs.abs[event.code].outputSecondary, 0);
                    }
                    continue;
                }

                // Useful for mapping analogue buttons to digital buttons,
                // for example the triggers on a gamepad.
                if (args->inputs.abs[event.code].type == SWITCH)
                {
                    // Allows mapping an axis button to a coin
                    if (args->inputs.key[event.code].output == COIN)
                    {
                        if (event.value == args->inputs.absMax[event.code])
                        {
                            incrementCoin(args->jvsIO, args->inputs.key[event.code].jvsPlayer, 1);
                        }
                        continue;
                    }
                    else if (event.value == args->inputs.absMin[event.code])
                    {
                        setSwitch(args->jvsIO, args->inputs.key[event.code].jvsPlayer, args->inputs.key[event.code].output, 0);
                    }
                    else
                    {
                        setSwitch(args->jvsIO, args->inputs.key[event.code].jvsPlayer, args->inputs.key[event.code].output, 1);
                    }
                    continue;
                }

                /* Handle normally mapped analogue controls */
                if (args->inputs.absEnabled[event.code])
                {
                    double scaled = ((double)((double)event.value * (double)args->inputs.absMultiplier[event.code]) - args->inputs.absMin[event.code]) / (args->inputs.absMax[event.code] - args->inputs.absMin[event.code]);

                    /* Make sure it doesn't go over 1 or below 0 if its multiplied */
                    scaled = scaled > 1 ? 1 : scaled;
                    scaled = scaled < 0 ? 0 : scaled;

                    setAnalogue(args->jvsIO, args->inputs.abs[event.code].output, args->inputs.abs[event.code].reverse ? 1 - scaled : scaled);
                    setGun(args->jvsIO, args->inputs.abs[event.code].output, args->inputs.abs[event.code].reverse ? 1 - scaled : scaled);
                }
            }
            break;

            case EV_MSC:
            {
                if (args->inputs.key[event.code].output == COIN)
                {
                    // The event's value is passed through, allowing the
                    // source of the event to define how many coins to
                    // insert at once.
                    if (event.value > 0)
                        incrementCoin(args->jvsIO, args->inputs.key[event.code].jvsPlayer, event.value);

                    continue;
                }
            }
            break;
            }
        }
    }

    close(fd);
    free(args);

    return 0;
}
void startThread(EVInputs *inputs, char *devicePath, int wiiMode, int player, JVSIO *jvsIO)
{
    MappingThreadArguments *args = malloc(sizeof(MappingThreadArguments));
    strcpy(args->devicePath, devicePath);
    memcpy(&args->inputs, inputs, sizeof(EVInputs));
    args->player = player;
    args->jvsIO = jvsIO;

    if (wiiMode)
    {
        createThread(wiiDeviceThread, args);
    }
    else
    {
        createThread(deviceThread, args);
    }
}

int evDevFromString(char *evDevString)
{
    for (long unsigned int i = 0; i < sizeof(evDevConversion) / sizeof(evDevConversion[0]); i++)
    {
        if (strcmp(evDevConversion[i].string, evDevString) == 0)
        {
            return evDevConversion[i].number;
        }
    }
    debug(0, "Error: Could not find the EV DEV string specified for %s\n", evDevString);
    return -1;
}

ControllerInput controllerInputFromString(char *controllerInputString)
{
    for (long unsigned int i = 0; i < sizeof(controllerInputConversion) / sizeof(controllerInputConversion[0]); i++)
    {
        if (strcmp(controllerInputConversion[i].string, controllerInputString) == 0)
            return controllerInputConversion[i].input;
    }
    debug(0, "Error: Could not find the CONTROLLER INPUT string specified for %s\n", controllerInputString);
    return -1;
}

ControllerPlayer controllerPlayerFromString(char *controllerPlayerString)
{
    for (long unsigned int i = 0; i < sizeof(controllerPlayerConversion) / sizeof(controllerPlayerConversion[0]); i++)
    {
        if (strcmp(controllerPlayerConversion[i].string, controllerPlayerString) == 0)
            return controllerPlayerConversion[i].player;
    }
    debug(0, "Error: Could not find the CONTROLLER PLAYER string specified for %s\n", controllerPlayerString);
    return -1;
}

static const char *stringFromControllerInput(ControllerInput controllerInput)
{
    for (long unsigned int i = 0; i < sizeof(controllerInputConversion) / sizeof(controllerInputConversion[0]); i++)
    {
        if (controllerInputConversion[i].input == controllerInput)
            return controllerInputConversion[i].string;
    }
    debug(0, "Error: Could not find the CONTROLLER INPUT string specified for controller input\n");
    return NULL;
}

int processMappings(InputMappings *inputMappings, OutputMappings *outputMappings, EVInputs *evInputs, ControllerPlayer player)
{
    for (int i = 0; i < inputMappings->length; i++)
    {
        int found = 0;
        double multiplier = 1;
        OutputMapping tempMapping;
        for (int j = outputMappings->length - 1; j >= 0; j--)
        {

            if ((outputMappings->mappings[j].input == inputMappings->mappings[i].input) && (outputMappings->mappings[j].controllerPlayer == player))
            {
                tempMapping = outputMappings->mappings[j];

                /* Find the second mapping if needed*/
                if (inputMappings->mappings[i].type == HAT)
                {
                    int foundSecondaryMapping = 0;
                    for (int p = outputMappings->length - 1; p >= 0; p--)
                    {
                        if (outputMappings->mappings[p].input == inputMappings->mappings[i].inputSecondary && outputMappings->mappings[p].controllerPlayer == player)
                        {
                            tempMapping.outputSecondary = outputMappings->mappings[p].output;
                            tempMapping.type = HAT;
                            found = 1;
                            break;
                        }
                    }
                    if (!foundSecondaryMapping)
                    {
                        debug(1, "Warning: No outside secondary mapping found for HAT\n");
                        continue;
                    }
                }

                tempMapping.reverse ^= inputMappings->mappings[i].reverse;
                multiplier = inputMappings->mappings[i].multiplier;
                found = 1;
                break;
            }
        }

        if (!found)
        {
            debug(1, "Warning: No outside mapping found for %s\n", stringFromControllerInput(inputMappings->mappings[i].input));
            continue;
        }

        if (inputMappings->mappings[i].type == HAT)
        {
            evInputs->abs[inputMappings->mappings[i].code] = tempMapping;
            evInputs->abs[inputMappings->mappings[i].code].type = HAT;
            evInputs->absEnabled[inputMappings->mappings[i].code] = 1;
        }

        if (inputMappings->mappings[i].type == ANALOGUE && tempMapping.type == ANALOGUE)
        {
            evInputs->abs[inputMappings->mappings[i].code] = tempMapping;
            evInputs->abs[inputMappings->mappings[i].code].type = ANALOGUE;
            evInputs->absEnabled[inputMappings->mappings[i].code] = 1;
            evInputs->absMultiplier[inputMappings->mappings[i].code] = multiplier;
        }
        else if (inputMappings->mappings[i].type == SWITCH || tempMapping.type == SWITCH)
        {
            evInputs->key[inputMappings->mappings[i].code] = tempMapping;
            evInputs->abs[inputMappings->mappings[i].code].type = SWITCH;
            evInputs->absEnabled[inputMappings->mappings[i].code] = 1;
        }

        if (inputMappings->mappings[i].type == CARD)
        {
            evInputs->key[inputMappings->mappings[i].code] = tempMapping;
            evInputs->abs[inputMappings->mappings[i].code].type = (InputType)COIN;
            evInputs->absEnabled[inputMappings->mappings[i].code] = 1;
        }
    }
    return 1;
}

int isEventDevice(const struct dirent *dir)
{
    return strncmp("event", dir->d_name, 5) == 0;
}

int getNumberOfDevices()
{
    struct dirent **namelist;
    return scandir(DEV_INPUT_EVENT, &namelist, isEventDevice, NULL);
}

JVSInputStatus getInputs(DeviceList *deviceList)
{
    memset(deviceList, 0, sizeof(DeviceList));
    struct dirent **namelist;

    deviceList->length = scandir(DEV_INPUT_EVENT, &namelist, isEventDevice, alphasort);

    if (deviceList->length == 0)
        return JVS_INPUT_STATUS_NO_DEVICE_ERROR;

    char aimtrakRemap[3][32] = {
        AIMTRAK_DEVICE_NAME_REMAP_JOYSTICK,
        AIMTRAK_DEVICE_NAME_REMAP_OUT_SCREEN,
        AIMTRAK_DEVICE_NAME_REMAP_IN_SCREEN};
    int aimtrakCount = 0;

    for (int i = 0; i < deviceList->length; i++)
    {
        snprintf(deviceList->devices[i].path, sizeof(deviceList->devices[i].path), "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);
        free(namelist[i]);

        strcpy(deviceList->devices[i].name, "unknown");
        strcpy(deviceList->devices[i].fullName, "Unknown");
        deviceList->devices[i].type = DEVICE_TYPE_UNKNOWN;

        int device = open(deviceList->devices[i].path, O_RDONLY);
        if (device < 0)
            continue;

        // Get product vendor and ID information
        struct input_id device_info;
        ioctl(device, EVIOCGID, &device_info);
        deviceList->devices[i].vendorID = device_info.vendor;
        deviceList->devices[i].productID = device_info.product;
        deviceList->devices[i].version = device_info.version;
        deviceList->devices[i].bus = device_info.bustype;

        // Get the name string
        ioctl(device, EVIOCGNAME(sizeof(deviceList->devices[i].fullName)), deviceList->devices[i].fullName);

        // Get the physical location string
        ioctl(device, EVIOCGPHYS(sizeof(deviceList->devices[i].physicalLocation)), deviceList->devices[i].physicalLocation);
        for (size_t j = 0; j < strlen(deviceList->devices[i].physicalLocation); j++)
        {
            if (deviceList->devices[i].physicalLocation[j] == '/')
            {
                deviceList->devices[i].physicalLocation[j] = 0;
                break;
            }
        }

        // Make it lower case and replace letters
        for (size_t j = 0; j < strlen(deviceList->devices[i].fullName); j++)
        {
            deviceList->devices[i].name[j] = tolower(deviceList->devices[i].fullName[j]);
            if (deviceList->devices[i].name[j] == ' ' ||
                deviceList->devices[i].name[j] == '/' ||
                deviceList->devices[i].name[j] == '(' ||
                deviceList->devices[i].name[j] == ')')
            {
                deviceList->devices[i].name[j] = '-';
            }
        }

        // Assign the correct names for the aimtracks
        if (strcmp(deviceList->devices[i].name, AIMTRAK_DEVICE_NAME) == 0)
        {
            strcpy(deviceList->devices[i].name, aimtrakRemap[aimtrakCount++]);
            if (aimtrakCount == 3)
                aimtrakCount = 0;
        }

        // Attempt to work out the device type
        unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
        memset(bit, 0, sizeof(bit));
        ioctl(device, EVIOCGBIT(0, EV_MAX), bit[0]);

        // If it does repeating events and key events, it's probably a keyboard.
        if (!test_bit_diff(EV_ABS, bit[0]) && test_bit_diff(EV_REP, bit[0]) && test_bit_diff(EV_KEY, bit[0]))
            deviceList->devices[i].type = DEVICE_TYPE_KEYBOARD;

        // Relative events means its a mouse!
        if (test_bit_diff(EV_REL, bit[0]))
            deviceList->devices[i].type = DEVICE_TYPE_MOUSE;

        // If it has a start button then it's probably a joystick!
        if (test_bit_diff(EV_KEY, bit[0]))
        {
            ioctl(device, EVIOCGBIT(EV_KEY, KEY_MAX), bit[EV_KEY]);
            if (test_bit_diff(BTN_START, bit[EV_KEY]))
                deviceList->devices[i].type = DEVICE_TYPE_JOYSTICK;
        }

        close(device);
    }

    free(namelist);

    // Now we can bubble sort the device list by name and then physical location
    for (int i = 0; i < deviceList->length - 1; i++)
    {
        for (int j = 0; j < deviceList->length - 1 - i; j++)
        {
            Device tmp;
            if (strcmp(deviceList->devices[j].name, deviceList->devices[j + 1].name) > 0)
            {
                tmp = deviceList->devices[j];
                deviceList->devices[j] = deviceList->devices[j + 1];
                deviceList->devices[j + 1] = tmp;
            }
        }
    }

    for (int i = 0; i < deviceList->length - 1; i++)
    {
        for (int j = 0; j < deviceList->length - 1 - i; j++)
        {
            Device tmp;
            if (strcmp(deviceList->devices[j].physicalLocation, deviceList->devices[j + 1].physicalLocation) > 0)
            {
                tmp = deviceList->devices[j];
                deviceList->devices[j] = deviceList->devices[j + 1];
                deviceList->devices[j + 1] = tmp;
            }
        }
    }

    return JVS_INPUT_STATUS_SUCCESS;
}

/**
 * Initialise all of the input devices and start the threads
 * 
 * This function initialises all the input devices that have mappings and
 * starts all of the appropriate threads.
 * 
 * @param outputMappingPath The path of the game mapping file
 * @param configPath The path to the configuration file
 * @param jvsIO The JVS IO object that we will send inputs to
 * @param autoDetect If we should automatically map controllers without mappings
 * @returns The status of the operation
 **/
JVSInputStatus initInputs(char *outputMappingPath, char *configPath, JVSIO *jvsIO, int autoDetect)
{
    OutputMappings outputMappings = {0};
    DeviceList *deviceList = (DeviceList *)malloc(sizeof(DeviceList));

    if (deviceList == NULL)
        return JVS_INPUT_STATUS_MALLOC_ERROR;

    if (getInputs(deviceList) != JVS_INPUT_STATUS_SUCCESS)
        return JVS_INPUT_STATUS_DEVICE_OPEN_ERROR;

    if (parseOutputMapping(outputMappingPath, &outputMappings, configPath) != JVS_CONFIG_STATUS_SUCCESS)
        return JVS_INPUT_STATUS_OUTPUT_MAPPING_ERROR;

    int playerNumber = 1;

    for (int i = 0; i < deviceList->length; i++)
    {
        Device *device = &deviceList->devices[i];

        char disabledPath[MAX_PATH_LENGTH];
        strcpy(disabledPath, DEFAULT_DEVICE_MAPPING_PATH);
        strcat(disabledPath, device->name);
        strcat(disabledPath, ".disabled");
        FILE *file = fopen(disabledPath, "r");
        if (file != NULL)
        {
            fclose(file);
            continue;
        }

        /* Attempt to do a generic map */
        char specialMap[256] = "";
        InputMappings inputMappings = {0};

        // Put the device name into a temp variable so it can be changed
        char deviceName[MAX_PATH_LENGTH];
        strcpy(deviceName, device->name);

        // Use the standard nintendo-wii-remote mapping file for the IR Version too
        if (strcmp(deviceName, WIIMOTE_DEVICE_NAME_IR) == 0)
            strcpy(deviceName, WIIMOTE_DEVICE_NAME);

        // Use the standard ultimarc-aimtrak mapping file for both screen events
        if (strcmp(deviceName, AIMTRAK_DEVICE_NAME_REMAP_JOYSTICK) == 0 || strcmp(deviceName, AIMTRAK_DEVICE_NAME_REMAP_OUT_SCREEN) == 0 || strcmp(deviceName, AIMTRAK_DEVICE_NAME_REMAP_IN_SCREEN) == 0)
            strcpy(deviceName, AIMTRAK_DEVICE_MAPPING_NAME);

        if (parseInputMapping(deviceName, &inputMappings) != JVS_CONFIG_STATUS_SUCCESS || inputMappings.length == 0)
        {
            if (autoDetect)
            {
                switch (deviceList->devices[i].type)
                {
                case DEVICE_TYPE_JOYSTICK:
                    if (parseInputMapping("generic-joystick", &inputMappings) != JVS_CONFIG_STATUS_SUCCESS || inputMappings.length == 0)
                        continue;
                    strcpy(specialMap, " (Generic Joystick Map)");
                    break;
                case DEVICE_TYPE_KEYBOARD:
                    if (parseInputMapping("generic-keyboard", &inputMappings) != JVS_CONFIG_STATUS_SUCCESS || inputMappings.length == 0)
                        continue;
                    strcpy(specialMap, " (Generic Keyboard Map)");
                    break;
                default:
                    continue;
                }
            }
        }

        EVInputs evInputs = {0};
        if (!processMappings(&inputMappings, &outputMappings, &evInputs, (ControllerPlayer)playerNumber))
        {
            debug(0, "Error: Failed to process the mapping for %s\n", deviceList->devices[i].name);
            continue;
        }

        if (inputMappings.player != -1)
        {
            startThread(&evInputs, device->path, strcmp(device->name, WIIMOTE_DEVICE_NAME_IR), inputMappings.player, jvsIO);
            debug(0, "  Player %d (Fixed via config):\t\t%s%s\n", inputMappings.player, deviceList->devices[i].name, specialMap);
        }
        else
        {
            startThread(&evInputs, device->path, strcmp(device->name, WIIMOTE_DEVICE_NAME_IR) == 0, playerNumber, jvsIO);
            if (strcmp(deviceList->devices[i].name, AIMTRAK_DEVICE_NAME_REMAP_IN_SCREEN) != 0 && strcmp(deviceList->devices[i].name, AIMTRAK_DEVICE_NAME_REMAP_JOYSTICK) != 0 && strcmp(deviceList->devices[i].name, WIIMOTE_DEVICE_NAME) != 0)
            {
                debug(0, "  Player %d:\t\t%s%s\n", playerNumber, deviceName, specialMap);
                playerNumber++;
            }
        }
    }

    free(deviceList);
    deviceList = NULL;

    return JVS_INPUT_STATUS_SUCCESS;
}
