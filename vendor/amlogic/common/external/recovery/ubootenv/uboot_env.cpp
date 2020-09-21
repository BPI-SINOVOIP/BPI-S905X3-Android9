/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ubootenv/Ubootenv.h"


// ------------------------------------
// for uboot environment variable operation
// ------------------------------------


int set_bootloader_env(const char* name, const char* value)
{
    Ubootenv *ubootenv = new Ubootenv();

    char ubootenv_name[128] = {0};
    const char *ubootenv_var = "ubootenv.var.";
    sprintf(ubootenv_name, "%s%s", ubootenv_var, name);

    if (ubootenv->updateValue(ubootenv_name, value)) {
        fprintf(stderr,"could not set boot env\n");
        return -1;
    }

    return 0;
}

char *get_bootloader_env(const char * name)
{
    Ubootenv *ubootenv = new Ubootenv();
     char ubootenv_name[128] = {0};
    const char *ubootenv_var = "ubootenv.var.";
    sprintf(ubootenv_name, "%s%s", ubootenv_var, name);
    return (char *)ubootenv->getValue(ubootenv_name);
}


int set_env_optarg(char * optarg)
{
    int ret = -1;
    char *buffer = NULL, *name = NULL, *value = NULL;
    if ((optarg == NULL) || (strlen(optarg) == 0)) {
        printf("param error!\n");
        return -1;
    }

    buffer = strdup(optarg);
    if (!buffer) {
        printf("strdup for buffer failed\n");
        return -1;
    }

    name = strtok(buffer, "=");
    if (strlen(name) == strlen(optarg)) {
        printf("strtok for '=' failed\n");
        goto END;
    }

    value = optarg + strlen(name) + 1;
    if (strlen(name) == 0) {
        printf("name is NULL\n");
        goto END;
    }

    if (strlen(value) == 0) {
        printf("value is NULL\n");
        goto END;
    }

    ret = set_bootloader_env(name, value);
    if (ret < 0) {
        printf("set env :%s=%s failed", name, value);
    }

END:

    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
    return ret;
}

int get_env_optarg(const char * optarg)
{
    int ret = -1;

    if (optarg == NULL) {
        printf("param error!\n");
        return -1;
    }

    char *env = get_bootloader_env(optarg);
    if (env != NULL)
    {
        printf("get %s value:%s\n", optarg, env);
        ret = 0;
    }

    return ret;
}
