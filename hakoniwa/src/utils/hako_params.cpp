#include "hako_params.hpp"
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    const char* value;
} HakoParamStringType;

typedef struct {
    const char *name;
    int value;
} HakoParamIntegerType;

#define HAKO_PARAM_STRING_NUM 1
static HakoParamStringType hako_param_string[HAKO_PARAM_STRING_NUM] = {
    {
        "HAKO_CAPTURE_SAVE_FILEPATH",
        "./capture.bin"
    },
};
#define HAKO_PARAM_INTEGER_NUM 1
static HakoParamIntegerType hako_param_integer[HAKO_PARAM_INTEGER_NUM] = {
    {
        "HAKO_BYPASS_PORTNO",
        54001
    },
};

bool hako_param_env_init()
{
    for (int i = 0; i < HAKO_PARAM_STRING_NUM; i++) {
        char *value = getenv(hako_param_string[i].name);
        if (value != NULL) {
            hako_param_string[i].value = value;
        }
    }
    for (int i = 0; i < HAKO_PARAM_INTEGER_NUM; i++) {
        char *value = getenv(hako_param_integer[i].name);
        if (value != NULL) {
            hako_param_integer[i].value = atoi(value);
        }
    }
    return true;
}

const char* hako_param_env_get_string(const char* param_name)
{
    size_t param_name_len = strlen(param_name);
    for (int i = 0; i < HAKO_PARAM_STRING_NUM; i++) {
        if (param_name_len != strlen(hako_param_string[i].name)) {
            continue;
        }
        if (strcmp(param_name, hako_param_string[i].name) != 0) {
            continue;
        }
        return hako_param_string[i].value;
    }
    return NULL;
}

bool hako_param_env_get_int(const char* param_name, int* value)
{
    size_t param_name_len = strlen(param_name);
    for (int i = 0; i < HAKO_PARAM_INTEGER_NUM; i++) {
        if (param_name_len != strlen(hako_param_integer[i].name)) {
            continue;
        }
        if (strcmp(param_name, hako_param_integer[i].name) != 0) {
            continue;
        }
        *value = hako_param_integer[i].value;
        return true;
    }
    return false;
}
