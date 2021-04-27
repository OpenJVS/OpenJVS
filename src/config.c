#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"

char *getNextToken(char *buffer, char *seperator, char **saveptr)
{
    char *token = strtok_r(buffer, seperator, saveptr);
    if (token == NULL)
        return NULL;

    for (int i = 0; i < (int)strlen(token); i++)
    {
        if ((token[i] == '\n') || (token[i] == '\r'))
        {
            token[i] = 0;
        }
    }
    return token;
}

JVSConfig config = {
    .senseLineType = 0,
    .senseLinePin = 12,
    .defaultGamePath = "generic",
    .debugLevel = 0,
    .devicePath = "/dev/ttyUSB0",
    .capabilities = SEGA_TYPE_3_IO,
};

JVSConfig *getConfig()
{
    return &config;
}

JVSConfigStatus parseConfig(char *path)
{
    FILE *file;
    char buffer[MAX_LINE_LENGTH];
    char *saveptr = NULL;

    if ((file = fopen(path, "r")) == NULL)
        return JVS_CONFIG_STATUS_FILE_NOT_FOUND;

    while (fgets(buffer, MAX_LINE_LENGTH, file))
    {

        /* Check for comments */
        if (buffer[0] == '#' || buffer[0] == 0 || buffer[0] == ' ' || buffer[0] == '\r' || buffer[0] == '\n')
            continue;

        char *command = getNextToken(buffer, " ", &saveptr);

        if (strcmp(command, "INCLUDE") == 0)
            parseConfig(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "SENSE_LINE_TYPE") == 0)
            config.senseLineType = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "EMULATE") == 0)
            jvsCapabilitiesFromString(&config.capabilities, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "SENSE_LINE_PIN") == 0)
            config.senseLinePin = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEFAULT_GAME") == 0)
            strcpy(config.defaultGamePath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEBUG_MODE") == 0)
            config.debugLevel = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEVICE_PATH") == 0)
            strcpy(config.devicePath, getNextToken(NULL, " ", &saveptr));

        else
            printf("Error: Unknown configuration command %s\n", command);
    }

    fclose(file);

    return JVS_CONFIG_STATUS_SUCCESS;
}

JVSConfigStatus parseInputMapping(char *path, InputMappings *inputMappings)
{
    FILE *file;
    char buffer[MAX_LINE_LENGTH];
    char *saveptr = NULL;

    char gamePath[MAX_PATH_LENGTH];
    strcpy(gamePath, DEFAULT_DEVICE_MAPPING_PATH);
    strcat(gamePath, path);

    if ((file = fopen(gamePath, "r")) == NULL)
        return JVS_CONFIG_STATUS_FILE_NOT_FOUND;

    while (fgets(buffer, MAX_LINE_LENGTH, file))
    {

        /* Check for comments */
        if (buffer[0] == '#' || buffer[0] == 0 || buffer[0] == ' ' || buffer[0] == '\r' || buffer[0] == '\n')
            continue;

        char *command = getNextToken(buffer, " ", &saveptr);

        if (strcmp(command, "INCLUDE") == 0)
        {
            InputMappings tempInputMappings;
            JVSConfigStatus status = parseInputMapping(getNextToken(NULL, " ", &saveptr), &tempInputMappings);
            if (status == JVS_CONFIG_STATUS_SUCCESS)
                memcpy(inputMappings, &tempInputMappings, sizeof(InputMappings));
        }
        else if (command[0] == 'K' || command[0] == 'B' || command[0] == 'C')
        {
            int code = evDevFromString(command);
            ControllerInput input = controllerInputFromString(getNextToken(NULL, " ", &saveptr));

            InputMapping mapping = {
                .type = SWITCH,
                .code = code,
                .input = input};

            inputMappings->mappings[inputMappings->length] = mapping;
            inputMappings->length++;
        }
        else if (command[0] == 'A')
        {
            char *firstArgument = getNextToken(NULL, " ", &saveptr);
            InputMapping mapping;

            if (strlen(firstArgument) > 11 && firstArgument[11] == 'B')
            {
                // This suggests we are doing it as a hat!
                InputMapping hatMapping = {
                    .type = HAT,
                    .code = evDevFromString(command),
                    .input = controllerInputFromString(firstArgument),
                    .inputSecondary = controllerInputFromString(getNextToken(NULL, " ", &saveptr)),
                };
                mapping = hatMapping;
            }
            else
            {
                // Normal Analogue Mapping
                InputMapping analogueMapping = {
                    .type = ANALOGUE,
                    .code = evDevFromString(command),
                    .input = controllerInputFromString(firstArgument),
                    .reverse = 0,
                    .multiplier = 1,
                };

                /* Check to see if we should reverse */
                char *extra = getNextToken(NULL, " ", &saveptr);
                while (extra != NULL)
                {
                    if (strcmp(extra, "REVERSE") == 0)
                    {
                        analogueMapping.reverse = 1;
                    }
                    else if (strcmp(extra, "SENSITIVITY") == 0)
                    {
                        analogueMapping.multiplier = atof(getNextToken(NULL, " ", &saveptr));
                    }
                    extra = getNextToken(NULL, " ", &saveptr);
                }

                mapping = analogueMapping;
            }

            inputMappings->mappings[inputMappings->length] = mapping;
            inputMappings->length++;
        }
        else
        {
            printf("Error: Unknown mapping command %s\n", command);
        }
    }

    fclose(file);

    return JVS_CONFIG_STATUS_SUCCESS;
}

JVSConfigStatus parseOutputMapping(char *path, OutputMappings *outputMappings)
{
    FILE *file;
    char buffer[MAX_LINE_LENGTH];
    char *saveptr = NULL;

    char gamePath[MAX_PATH_LENGTH];
    strcpy(gamePath, DEFAULT_GAME_MAPPING_PATH);
    strcat(gamePath, path);

    if ((file = fopen(gamePath, "r")) == NULL)
        return JVS_CONFIG_STATUS_FILE_NOT_FOUND;

    while (fgets(buffer, MAX_LINE_LENGTH, file))
    {

        /* Check for comments */
        if (buffer[0] == '#' || buffer[0] == 0 || buffer[0] == ' ' || buffer[0] == '\r' || buffer[0] == '\n')
            continue;

        char *command = getNextToken(buffer, " ", &saveptr);

        if (strcmp(command, "INCLUDE") == 0)
        {
            OutputMappings tempOutputMappings;
            JVSConfigStatus status = parseOutputMapping(getNextToken(NULL, " ", &saveptr), &tempOutputMappings);
            if (status == JVS_CONFIG_STATUS_SUCCESS)
                memcpy(outputMappings, &tempOutputMappings, sizeof(OutputMappings));
        }
        else if (strcmp(command, "EMULATE") == 0)
        {
            jvsCapabilitiesFromString(&config.capabilities, getNextToken(NULL, " ", &saveptr));
        }
        else if (command[11] == 'B')
        {
            ControllerPlayer controllerPlayer = controllerPlayerFromString(getNextToken(NULL, " ", &saveptr));
            OutputMapping mapping = {
                .type = SWITCH,
                .input = controllerInputFromString(command),
                .controllerPlayer = controllerPlayer,
                .output = jvsInputFromString(getNextToken(NULL, " ", &saveptr)),
                .outputSecondary = NONE,
                .jvsPlayer = jvsPlayerFromString(getNextToken(NULL, " ", &saveptr))};

            /* Check to see if we have a secondary output */
            char *secondaryOutput = getNextToken(NULL, " ", &saveptr);
            if (secondaryOutput != NULL)
            {
                printf("Adding secondary output\n");
                mapping.outputSecondary = jvsInputFromString(secondaryOutput);
            }

            outputMappings->mappings[outputMappings->length] = mapping;
            outputMappings->length++;
        }
        else if (command[11] == 'A')
        {
            OutputMapping mapping = {
                .type = ANALOGUE,
                .input = controllerInputFromString(command),
                .controllerPlayer = controllerPlayerFromString(getNextToken(NULL, " ", &saveptr)),
                .output = jvsInputFromString(getNextToken(NULL, " ", &saveptr))};

            /* Check to see if we should reverse */
            char *reverse = getNextToken(NULL, " ", &saveptr);
            if (reverse != NULL && strcmp(reverse, "REVERSE") == 0)
            {
                mapping.reverse = 1;
            }

            outputMappings->mappings[outputMappings->length] = mapping;
            outputMappings->length++;
        }
        else
        {
            printf("Error: Unknown mapping command %s\n", command);
        }
    }

    fclose(file);

    return JVS_CONFIG_STATUS_SUCCESS;
}

JVSConfigStatus parseRotary(char *path, int rotary, char *output)
{
    FILE *file;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if ((file = fopen(path, "r")) == NULL)
        return JVS_CONFIG_STATUS_FILE_NOT_FOUND;

    int counter = 0;
    char rotaryGames[16][MAX_LINE_LENGTH];

    for (int i = 0; i < 16; i++)
    {
        strcpy(rotaryGames[i], "generic");
    }

    while ((read = getline(&line, &len, file)) != -1 && counter < 16)
    {
        strcpy(rotaryGames[counter], line);
        for (size_t i = 0; i < strlen(line); i++)
        {
            if (rotaryGames[counter][i] == '\n' || rotaryGames[counter][i] == '\r')
            {
                rotaryGames[counter][i] = 0;
            }
        }
        counter++;
    }

    fclose(file);

    if (line)
        free(line);

    strcpy(output, rotaryGames[rotary]);

    return JVS_CONFIG_STATUS_SUCCESS;
}