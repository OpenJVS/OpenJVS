#ifndef CONFIG_H_
#define CONFIG_H_

#include "input.h"

#define DEFAULT_CONFIG_PATH "/etc/openjvs/config"
#define DEFAULT_DEVICE_MAPPING_PATH "/etc/openjvs/devices/"
#define DEFAULT_GAME_MAPPING_PATH "/etc/openjvs/games/"
#define DEFAULT_ROTARY_PATH "/etc/openjvs/rotary"

#define MAX_PATH_LENGTH 1024
#define MAX_LINE_LENGTH 1024

typedef struct
{
    int senseLineType;
    int senseLinePin;
    char defaultGamePath[MAX_PATH_LENGTH];
    char devicePath[MAX_PATH_LENGTH];
    int debugLevel;
    JVSCapabilities capabilities;
} JVSConfig;

typedef enum
{
    JVS_CONFIG_STATUS_ERROR = 0,
    JVS_CONFIG_STATUS_SUCCESS = 1,
    JVS_CONFIG_STATUS_FILE_NOT_FOUND,
    JVS_CONFIG_STATUS_PARSE_ERROR,
} JVSConfigStatus;

JVSConfig *getConfig();
JVSConfigStatus parseConfig(char *path);
JVSConfigStatus parseInputMapping(char *path, InputMappings *inputMappings);
JVSConfigStatus parseOutputMapping(char *path, OutputMappings *inputMappings);
JVSConfigStatus parseRotary(char *path, int rotary, char *output);

#endif // CONFIG_H_
