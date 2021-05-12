#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jvs/io.h"

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

JVSConfigStatus getDefaultConfig(JVSConfig *config)
{
    config->senseLineType = DEFAULT_SENSE_LINE_TYPE;
    config->senseLinePin = DEFAULT_SENSE_LINE_PIN;
    config->debugLevel = DEFAULT_DEBUG_LEVEL;
    config->autoControllerDetection = DEFAULT_AUTO_CONTROLLER_DETECTION;
    strcpy(config->defaultGamePath, DEFAULT_GAME);
    strcpy(config->devicePath, DEFAULT_DEVICE_PATH);
    strcpy(config->capabilitiesPath, DEFAULT_IO);
    return JVS_CONFIG_STATUS_SUCCESS;
}

JVSConfigStatus parseConfig(char *path, JVSConfig *config)
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

        /* This will get overwritten! Need to do defaults somewhere else */
        if (strcmp(command, "INCLUDE") == 0)
            parseConfig(getNextToken(NULL, " ", &saveptr), config);

        else if (strcmp(command, "SENSE_LINE_TYPE") == 0)
            config->senseLineType = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "EMULATE") == 0)
            strcpy(config->capabilitiesPath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "SENSE_LINE_PIN") == 0)
            config->senseLinePin = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEFAULT_GAME") == 0)
            strcpy(config->defaultGamePath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEBUG_MODE") == 0)
            config->debugLevel = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEVICE_PATH") == 0)
            strcpy(config->devicePath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "AUTO_CONTROLLER_DETECTION") == 0)
            config->autoControllerDetection = atoi(getNextToken(NULL, " ", &saveptr));

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

JVSConfigStatus parseOutputMapping(char *path, OutputMappings *outputMappings, char *configPath)
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
        int analogueToDigital;
        if (strcmp(command, "DIGITAL") == 0)
        {
            analogueToDigital = 1;
            // DIGITAL is the first token for these, coming before the
            // axis name; if we found DIGITAL, we need to read the next
            // token for the actual axis.
            command = getNextToken(NULL, " ", &saveptr);
        }
        else
        {
            analogueToDigital = 0;
        }

        if (strcmp(command, "INCLUDE") == 0)
        {
            OutputMappings tempOutputMappings;
            JVSConfigStatus status = parseOutputMapping(getNextToken(NULL, " ", &saveptr), &tempOutputMappings, configPath);
            if (status == JVS_CONFIG_STATUS_SUCCESS)
                memcpy(outputMappings, &tempOutputMappings, sizeof(OutputMappings));
        }
        else if (strcmp(command, "EMULATE") == 0)
        {
            strcpy(configPath, getNextToken(NULL, " ", &saveptr));
        }
        else if (command[11] == 'B' || analogueToDigital)
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

        if (line)
        {
            free(line);
            line = NULL;
            len = 0;
        }
    }

    fclose(file);

    strcpy(output, rotaryGames[rotary]);

    return JVS_CONFIG_STATUS_SUCCESS;
}

JVSConfigStatus parseIO(char *path, JVSCapabilities *capabilities)
{
    FILE *file;
    char buffer[MAX_LINE_LENGTH];
    char *saveptr = NULL;

    char ioPath[MAX_PATH_LENGTH];
    strcpy(ioPath, DEFAULT_IO_PATH);
    strcat(ioPath, path);

    if ((file = fopen(ioPath, "r")) == NULL)
        return JVS_CONFIG_STATUS_FILE_NOT_FOUND;

    while (fgets(buffer, MAX_LINE_LENGTH, file))
    {

        /* Check for comments */
        if (buffer[0] == '#' || buffer[0] == 0 || buffer[0] == ' ' || buffer[0] == '\r' || buffer[0] == '\n')
            continue;

        char *command = getNextToken(buffer, " ", &saveptr);

        if (strcmp(command, "DISPLAY_NAME") == 0)
            strcpy(capabilities->displayName, getNextToken(NULL, "\n", &saveptr));

        else if (strcmp(command, "NAME") == 0)
            strcpy(capabilities->name, getNextToken(NULL, "\n", &saveptr));

        else if (strcmp(command, "COMMAND_VERSION") == 0)
            capabilities->commandVersion = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "JVS_VERSION") == 0)
            capabilities->jvsVersion = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "COMMS_VERSION") == 0)
            capabilities->commsVersion = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYERS") == 0)
            capabilities->players = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "SWITCHES") == 0)
            capabilities->switches = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "COINS") == 0)
            capabilities->coins = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "ANALOGUE_IN_CHANNELS") == 0)
            capabilities->analogueInChannels = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "ANALOGUE_IN_BITS") == 0)
            capabilities->analogueInBits = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "ROTARY_CHANNELS") == 0)
            capabilities->rotaryChannels = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "KEYPAD") == 0)
            capabilities->keypad = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "GUN_CHANNELS") == 0)
            capabilities->gunChannels = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "GUN_X_BITS") == 0)
            capabilities->gunXBits = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "GUN_Y_BITS") == 0)
            capabilities->gunYBits = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "GENERAL_PURPOSE_INPUTS") == 0)
            capabilities->generalPurposeInputs = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "CARD") == 0)
            capabilities->card = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "HOPPER") == 0)
            capabilities->hopper = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "GENERAL_PURPOSE_OUTPUTS") == 0)
            capabilities->generalPurposeOutputs = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "ANALOGUE_OUT_CHANNELS") == 0)
            capabilities->analogueOutChannels = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DISPLAY_OUT_ROWS") == 0)
            capabilities->displayOutRows = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DISPLAY_OUT_COLUMNS") == 0)
            capabilities->displayOutColumns = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DISPLAY_OUT_ENCODINGS") == 0)
            capabilities->displayOutEncodings = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "BACKUP") == 0)
            capabilities->backup = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "RIGHT_ALIGN_BITS") == 0)
            capabilities->rightAlignBits = atoi(getNextToken(NULL, " ", &saveptr));

        else
            printf("Error: Unknown IO configuration command %s\n", command);
    }

    fclose(file);

    return JVS_CONFIG_STATUS_SUCCESS;
}
