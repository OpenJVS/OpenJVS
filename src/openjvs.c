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
volatile int signalKillProcess = 0;

int main(int argc, char **argv)
{
  JVSRotaryStatus rotaryStatus = JVS_ROTARY_UNUSED;
  int rotaryValue = -1;
  ThreadManagerInit();
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

  // If rotary is selected as the default game, look for the rotary text file
  if (strcmp(localConfig->defaultGamePath, "rotary") == 0 || strcmp(localConfig->defaultGamePath, "ROTARY") == 0)
  {
    rotaryStatus = initRotary();
    rotaryValue = getRotaryValue();
  }

  debug(0, "OpenJVS Version 3.4\n\n");

  do
  {
    /* Set threads to*/
    ThreadManagerSetRunnable();

    if (rotaryStatus == JVS_ROTARY_STATUS_SUCCESS)
    {
      parseRotary(DEFAULT_ROTARY_PATH, rotaryValue, localConfig->defaultGamePath);
      printf("localConfig->defaultGamePath:%s \n", localConfig->defaultGamePath);
      debug(0, "  Rotary Position:\t%d\n", rotaryValue);
    }

    if (initInputs(localConfig->defaultGamePath))
    {
      debug(0, "Error: Could not initialise the inputs - make sure you are root\n");
      debug(0, "Try running `sudo openjvs --list` to see the devices\n");
    }
    debug(0, "  Output:\t\t%s\n", localConfig->defaultGamePath);

    /* Setup the JVS Emulator with the RS485 path and capabilities */
    if (!initJVS(localConfig->devicePath, &localConfig->capabilities))
    {
      debug(0, "Error: Could not initialise JVS\n");
      return EXIT_FAILURE;
    }

    debug(0, "\nYou are currently emulating a \033[0;31m%s\033[0m on %s.\n\n", localConfig->capabilities.displayName, localConfig->devicePath);

    /* Process packets forever (as long as there is no SIGINT or change in rotary switch) */
    JVSStatus processingStatus;
    int reinitConfig = 0;
    while (1)
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

      /* If rotary switch is used check value cyclically */
      if (rotaryStatus == JVS_ROTARY_STATUS_SUCCESS)
      {
        static time_t rotaryTime;
        time_t now;

        now = time(NULL);

        if ((now - rotaryTime) > TIME_POLL_ROTARY)
        {
          int rotaryValueNew = getRotaryValue();

          if (rotaryValue != rotaryValueNew)
          {
            /* Reinit OpenJVS config when rotary switch changed */
            rotaryValue = rotaryValueNew;
            reinitConfig = 1;
          }
          rotaryTime = now;
        }
      }

      if (signalKillProcess || reinitConfig)
      {
        if (signalKillProcess)
        {
          debug(0, "\nClosing down OpenJVS...\n");
        }

        if (reinitConfig)
        {
          debug(0, "\nreinit OpenJVS...\n");
        }

        ThreadManagerStopAll();

        /* Close the file pointer */
        if (!disconnectJVS())
        {
          debug(0, "Error: Could not disconnect from serial\n");
          return EXIT_FAILURE;
        }

        /* Break processPacket() loop */
        break;
      }
    } /* while - processPacket() */
  }
  /* Reinit as long as we did not see the SIGINT signal */
  while (!signalKillProcess);

  return EXIT_SUCCESS;
}

void handleSignal(int signal)
{
  if (signal == 2)
  {
    signalKillProcess = 1;
  }
}
