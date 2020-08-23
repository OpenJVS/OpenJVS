#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "openjvs.h"

#include "jvs.h"
#include "config.h"
#include "debug.h"
#include "io.h"
#include "input.h"

void handleSignal(int signal);
int handleArguments(int argc, char **argv);

int running = 1;

int main(int argc, char **argv)
{
  signal(SIGINT, handleSignal);

  printf("OpenJVS Version 3.3\n\n");

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
  if (argc > 1)
  {
    if (argv[1][0] == '-')
    {
      return handleArguments(argc, argv);
    }
    strcpy(localConfig->defaultGamePath, argv[1]);
  }

  debug(0, "You are currently emulating a \033[0;31m%s\033[0m on %s.\n\n", localConfig->capabilities.displayName, localConfig->devicePath);
  debug(0, "  Output:\t\t%s\n", localConfig->defaultGamePath);

  if (!initInputs(localConfig->defaultGamePath))
  {
    debug(0, "Error: Could not initialise the inputs - make sure you are root\n");
    debug(0, "Try running `sudo openjvs --list` to see the devices\n");
    return EXIT_FAILURE;
  }

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

int handleArguments(int argc, char **argv)
{
  if (argc < 1)
  {
    debug(0, "Error: No arguments were passed to the handleArguments() function\n");
    return EXIT_FAILURE;
  }
  if (strcmp(argv[1], "--version") == 0)
  {
    debug(0, "Written by: Bobby Dilley, RedOne and debugged by the team!\n");
    return EXIT_SUCCESS;
  }
  else if (strcmp(argv[1], "--enable") == 0)
  {
    char gamePath[MAX_PATH_LENGTH];
    strcpy(gamePath, DEFAULT_DEVICE_MAPPING_PATH);
    strcat(gamePath, argv[2]);

    char gamePathDisabled[MAX_PATH_LENGTH];
    strcpy(gamePathDisabled, gamePath);
    strcat(gamePathDisabled, ".disabled");

    if (rename(gamePathDisabled, gamePath) < 0)
    {
      debug(0, "Failed to enable device, does it exist and is it already enabled?\n");
      return EXIT_FAILURE;
    }
    debug(0, "Enabled %s\n", argv[2]);
    return EXIT_SUCCESS;
  }
  else if (strcmp(argv[1], "--disable") == 0)
  {
    char gamePath[MAX_PATH_LENGTH];
    strcpy(gamePath, DEFAULT_DEVICE_MAPPING_PATH);
    strcat(gamePath, argv[2]);

    char gamePathDisabled[MAX_PATH_LENGTH];
    strcpy(gamePathDisabled, gamePath);
    strcat(gamePathDisabled, ".disabled");

    if (rename(gamePath, gamePathDisabled) < 0)
    {
      debug(0, "Failed to disable device, does it exist and is it already disabled?\n");
      return EXIT_FAILURE;
    }
    debug(0, "Disabled %s\n", argv[2]);
    return EXIT_SUCCESS;
  }
  else if (strcmp(argv[1], "--list") == 0)
  {
    DeviceList deviceList;
    if (!getInputs(&deviceList))
    {
      debug(0, "Failed to read devices - are you root?\n");
      return EXIT_FAILURE;
    }
    InputMappings inputMappings;
    inputMappings.length = 0;
    debug(0, "Enabled:\n");
    for (int i = 0; i < deviceList.length; i++)
    {
      char disabledString[MAX_PATH_LENGTH];
      strcpy(disabledString, deviceList.devices[i].name);
      strcat(disabledString, ".disabled");
      int enabled = parseInputMapping(deviceList.devices[i].name, &inputMappings) == JVS_CONFIG_STATUS_SUCCESS;
      if (enabled)
      {
        printf("  %s\n", deviceList.devices[i].name);
      }
    }
    debug(0, "\nDisabled:\n");
    for (int i = 0; i < deviceList.length; i++)
    {
      char disabledString[MAX_PATH_LENGTH];
      strcpy(disabledString, deviceList.devices[i].name);
      strcat(disabledString, ".disabled");
      int disabled = parseInputMapping(disabledString, &inputMappings) == JVS_CONFIG_STATUS_SUCCESS;
      if (disabled)
      {
        printf("  %s\n", deviceList.devices[i].name);
      }
    }
    debug(0, "\nNo Mapping Present:\n");
    for (int i = 0; i < deviceList.length; i++)
    {
      char disabledString[MAX_PATH_LENGTH];
      strcpy(disabledString, deviceList.devices[i].name);
      strcat(disabledString, ".disabled");
      int enabled = parseInputMapping(deviceList.devices[i].name, &inputMappings) == JVS_CONFIG_STATUS_SUCCESS;
      int disabled = parseInputMapping(disabledString, &inputMappings) == JVS_CONFIG_STATUS_SUCCESS;
      if (!enabled && !disabled)
      {
        printf("  %s\n", deviceList.devices[i].name);
      }
    }
    return EXIT_SUCCESS;
  }
  debug(0, "Unknown argument %s\n", argv[1]);
  return EXIT_FAILURE;
}
