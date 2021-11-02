#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "console/cli.h"
#include "console/config.h"
#include "console/debug.h"
#include "controller/input.h"
#include "controller/threading.h"
#include "console/watchdog.h"
#include "hardware/device.h"
#include "hardware/rotary.h"
#include "jvs/io.h"
#include "jvs/jvs.h"
#include "ffb/ffb.h"
#include "version.h"

/* Time between reinit in ms */
#define TIME_REINIT 200

void cleanup();
void handleSignal(int signal);

volatile int running = 1;

int main(int argc, char **argv)
{
    signal(SIGINT, handleSignal);

    /* Read the initial config */
    JVSConfig config;
    getDefaultConfig(&config);
    if (parseConfig(DEFAULT_CONFIG_PATH, &config) != JVS_CONFIG_STATUS_SUCCESS)
    {
        printf("Warning: No valid config file found, defaults are being used\n");
    }

    /* Initialise the debug output */
    initDebug(config.debugLevel);

    /* Get the correct game output mapping */
    JVSCLIStatus argumentsStatus = parseArguments(argc, argv, config.defaultGamePath);
    switch (argumentsStatus)
    {
    case JVS_CLI_STATUS_ERROR:
        return EXIT_FAILURE;
        break;
    case JVS_CLI_STATUS_SUCCESS_CLOSE:
        return EXIT_SUCCESS;
        break;
    case JVS_CLI_STATUS_SUCCESS_CONTINUE:
        break;
    default:
        break;
    }

    debug(0, "OpenJVS Version %s\n\n", PROJECT_VER);

    /* Init the thread managre */
    ThreadStatus threadStatus = initThreadManager();
    if (threadStatus != THREAD_STATUS_SUCCESS)
    {
        debug(0, "Critical: Could not initialise the thread manager.\n");
        return EXIT_FAILURE;
    }

    /* Init the connection to the Naomi */
    if (!initDevice(config.devicePath, config.senseLineType, config.senseLinePin))
    {
        debug(0, "Critical: Failed to init the RS485 device at %s, you must be root.\n", config.devicePath);
        return EXIT_FAILURE;
    }

    /* Init the rotrary status*/
    JVSRotaryStatus rotaryStatus = JVS_ROTARY_STATUS_UNUSED;
    int rotaryValue = -1;
    if (strcmp(config.defaultGamePath, "rotary") == 0 || strcmp(config.defaultGamePath, "ROTARY") == 0)
    {
        rotaryStatus = initRotary();
    }

    JVSInputStatus lastInputState = JVS_INPUT_STATUS_SUCCESS;
    int lastRotaryValue = -1;
    while (running != -1)
    {
        /* Init the watchdog to check the rotary and inputs */
        debug(1, "Init watchdog\n");
        running = 1;
        setThreadsRunning(1);
        initWatchdog(&running, rotaryStatus);

        if (rotaryStatus == JVS_ROTARY_STATUS_SUCCESS)
        {
            rotaryValue = getRotaryValue();
            parseRotary(DEFAULT_ROTARY_PATH, rotaryValue, config.defaultGamePath);
        }

        // Create the JVSIO
        JVSIO io = {0};
        io.deviceID = 1;

        debug(1, "Init inputs\n");
        JVSInputStatus inputStatus = initInputs(config.defaultGamePath, config.capabilitiesPath, &io, config.autoControllerDetection);

        // Only report these errors if the status has changed
        // from the last run. Since we restart this thread every 200ms
        // on errors, we could end up spamming the logs extremely quickly
        // when a controller just isn't plugged in.
        if (inputStatus != lastInputState)
        {
            switch (inputStatus)
            {
            case JVS_INPUT_STATUS_MALLOC_ERROR:
                debug(0, "Error: Failed to malloc\n");
                break;
            case JVS_INPUT_STATUS_DEVICE_OPEN_ERROR:
                debug(0, "Error: Failed to open devices\n");
                break;
            case JVS_INPUT_STATUS_OUTPUT_MAPPING_ERROR:
                debug(0, "Error: Cannot find an output mapping\n");
                break;
            default:
                break;
            }

            if (inputStatus != JVS_INPUT_STATUS_SUCCESS)
            {
                debug(0, "Critical: Could not initialise any inputs, check they're plugged in and you are root!\n");
            }
        }

        if (inputStatus != JVS_INPUT_STATUS_SUCCESS)
        {
            // Cleanup then wait before reconnecting
            lastInputState = inputStatus;
            lastRotaryValue = rotaryValue;
            cleanup();
            continue;
        }

        if (rotaryStatus == JVS_ROTARY_STATUS_SUCCESS && rotaryValue != lastRotaryValue)
        {
            debug(0, "  Rotary Position:\t%d\n", rotaryValue);
        }

        debug(0, "  Output:\t\t%s\n", config.defaultGamePath);

        debug(1, "Parse IO\n");
        JVSConfigStatus ioStatus = parseIO(config.capabilitiesPath, &io.capabilities);
        if (ioStatus != JVS_CONFIG_STATUS_SUCCESS)
        {
            switch (ioStatus)
            {
            case JVS_CONFIG_STATUS_FILE_NOT_FOUND:
                debug(0, "Critical: Could not find IO definition named %s\n", config.capabilitiesPath);
                break;
            default:
                debug(0, "Critical: Failed to parse an IO file.\n");
            }
            return EXIT_FAILURE;
        }

        /* Init the Virtual IO */
        debug(1, "Init IO\n");
        if (!initIO(&io))
        {
            debug(0, "Critical: Failed to init IO\n");
            return EXIT_FAILURE;
        }

        /* Setup the JVS Emulator with the RS485 path and capabilities */
        debug(1, "Init JVS\n");
        if (!initJVS(&io))
        {
            debug(0, "Critical: Could not initialise JVS\n");
            return EXIT_FAILURE;
        }

        debug(0, "\nYou are currently emulating a \033[0;31m%s\033[0m on %s.\n\n", io.capabilities.displayName, config.devicePath);

        /* Process packets forever */
        JVSStatus processingStatus;
        while (running == 1)
        {
            processingStatus = processPacket(&io);
            switch (processingStatus)
            {
            case JVS_STATUS_ERROR_CHECKSUM:
                debug(0, "Error: A checksum error occoured\n");
                break;
            case JVS_STATUS_ERROR_WRITE_FAIL:
                debug(0, "Error: A write failure occoured\n");
                break;
            case JVS_STATUS_ERROR:
                debug(0, "Error: A generic error occoured\n");
                break;
            default:
                break;
            }
        }

        lastInputState = inputStatus;
        lastRotaryValue = rotaryValue;
        cleanup();
    }

    /* Close the file pointer */
    if (!disconnectJVS())
    {
        debug(0, "Critical: Could not disconnect from serial\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void cleanup()
{
    /* Stop threads managed by ThreadManager */
    stopAllThreads();

    /* Take a short break on reinit to reduce load */
    usleep(TIME_REINIT);
}

void handleSignal(int signal)
{
    if (signal == 2)
    {
        debug(0, "\nWarning: OpenJVS is shutting down\n");
        running = -1;
    }
}
