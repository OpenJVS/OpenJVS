#ifndef CLI_H_
#define CLI_H_

typedef enum
{
    JVS_CLI_STATUS_ERROR = 0,
    JVS_CLI_STATUS_SUCCESS_CLOSE,
    JVS_CLI_STATUS_SUCCESS_CONTINUE,
} JVSCLIStatus;

JVSCLIStatus parseArguments(int argc, char **argv, char *map);

#endif // CLI_H_
