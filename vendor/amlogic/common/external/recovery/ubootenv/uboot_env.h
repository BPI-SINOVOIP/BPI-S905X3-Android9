/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#ifndef BOOTLOADER_ENV_H_
#define BOOTLOADER_ENV_H_
/*
* set bootloader environment variable
* 0: success, <0: fail
*/
extern int set_bootloader_env(const char* name, const char* value);

/*
* get bootloader environment variable
* NULL: init failed or get env value is NULL
* NONE NULL: env value
*/
extern char *get_bootloader_env(const char * name);



extern int set_env_optarg(const char * optarg);



extern int get_env_optarg(const char * optarg);

#endif