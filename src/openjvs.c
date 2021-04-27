#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "cli.h"
#include "config.h"
#include "debug.h"
#include "input.h"
#include "io.h"
#include "jvs.h"
#include "rotary.h"

void handleSignal(int signal);

int running = 1;

int main(int argc, char **argv)
{
  signal(SIGINT, handleSignal);

  /* Read the initial config */
  if (parseConfig(DEFAULT_CONFIG_PATH) != JVS_CONFIG_STATUS_SUCCESS)
  {
    printf("Warning: No valid config file found, a default is being used\n");
  }
  JVSConfig *localConfig = getConfig();

  /* Initialise the debug output */
  if (!initDebug(localConfig->debugLevel))
  {
    printf("Failed to initialise debug output\n");
  }

  /* Get the correct game output mapping */
  JVSCLIStatus argumentsStatus = parseArguments(argc, argv, localConfig->defaultGamePath);
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

  if (strcmp(localConfig->defaultGamePath, "rotary") == 0 || strcmp(localConfig->defaultGamePath, "ROTARY") == 0)
  {
    JVSRotaryStatus rotaryStatus = initRotary();
    if (rotaryStatus == JVS_ROTARY_STATUS_SUCCESS)
    {
      printf("Pin rotary %d\n", getRotaryValue());
    }
  }

  debug(0, "OpenJVS Version 3.3.3\n\n");
  debug(0, "You are currently emulating a \033[0;31m%s\033[0m on %s.\n\n", localConfig->capabilities.displayName, localConfig->devicePath);

  if (initInputs(localConfig->defaultGamePath))
  {
    debug(0, "Error: Could not initialise the inputs - make sure you are root\n");
    debug(0, "Try running `sudo openjvs --list` to see the devices\n");
  }

  debug(0, "  Output:\t\t%s\n", localConfig->defaultGamePath);
  debug(0, "\nDebug messages will appear below, you are in debug mode %d.\n\n", localConfig->debugLevel);

  /* Setup the JVS Emulator with the RS485 path and capabilities */
  if (!initJVS(localConfig->devicePath, &localConfig->capabilities))
  {
    debug(0, "Error: Could not initialise JVS\n");
    return EXIT_FAILURE;
  }

  /* Process packets forever */
  JVSStatus processingStatus;
  while (running)
  {
    processingStatus = processPacket();
    switch (processingStatus)
    {
    case JVS_STATUS_ERROR_CHECKSUM:
      debug(0, "Error: A checksum error occoured\n");
      break;
    case JVS_STATUS_ERROR_TIMEOUT:
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

  /* Close the file pointer */
  if (!disconnectJVS())
  {
    debug(0, "Error: Could not disconnect from serial\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void handleSignal(int signal)
{
  if (signal == 2)
  {
    debug(0, "\nClosing down OpenJVS...\n");
    running = 0;
  }
}
