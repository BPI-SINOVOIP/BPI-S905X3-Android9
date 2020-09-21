/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ubootenv/Ubootenv.h"
#include "SysWrite.h"
#include "DisplayMode.h"

int set_display_mode(const char *path)
{
    Ubootenv *pUbootenv = new Ubootenv();

    DisplayMode displayMode(path, pUbootenv);
    displayMode.setRecoveryMode(true);
    displayMode.init();

    //don't end this progress, wait for hdmi plug detect thread.
    while (1) {
        usleep(10000000);
    }

    return 0;
}
