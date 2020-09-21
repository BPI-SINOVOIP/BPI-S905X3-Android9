/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "version"

#include <stdio.h>
#include <string.h>
#include <android/log.h>

#include "version.h"

static char gitversionstr[256] = "N/A";

static int tvservice_version_info_init(void)
{
    static int info_is_inited = 0;
    int dirty_num = 0;

    if (info_is_inited > 0) {
        return 0;
    }
    info_is_inited++;

#ifdef HAVE_VERSION_INFO

#ifdef LIBTVSERVICE_GIT_UNCOMMIT_FILE_NUM
#if LIBTVSERVICE_GIT_UNCOMMIT_FILE_NUM>0
    dirty_num = LIBTVSERVICE_GIT_UNCOMMIT_FILE_NUM;
#endif
#endif

#ifdef LIBTVSERVICE_GIT_VERSION
    if (dirty_num > 0) {
        snprintf(gitversionstr, 250, "%s-with-%d-dirty-files", LIBTVSERVICE_GIT_VERSION, dirty_num);
    } else {
        snprintf(gitversionstr, 250, "%s", LIBTVSERVICE_GIT_VERSION);
    }
#endif

#endif

    return 0;
}

const char *tvservice_get_git_version_info(void)
{
    tvservice_version_info_init();
    return gitversionstr;
}

const char *tvservice_get_last_chaned_time_info(void)
{
#ifdef HAVE_VERSION_INFO
#ifdef LIBTVSERVICE_LAST_CHANGED
    return LIBTVSERVICE_LAST_CHANGED;
#endif
#endif
    //return " Unknow ";
}

const char *tvservice_get_git_branch_info(void)
{
#ifdef HAVE_VERSION_INFO
#ifdef LIBTVSERVICE_GIT_BRANCH
    return LIBTVSERVICE_GIT_BRANCH;
#endif
#endif
    //return " Unknow ";
}

const char *tvservice_get_build_time_info(void)
{
#ifdef HAVE_VERSION_INFO
#ifdef LIBTVSERVICE_BUILD_TIME
    return LIBTVSERVICE_BUILD_TIME;
#endif
#endif
    //return " Unknow ";
}

const char *tvservice_get_build_name_info(void)
{
#ifdef HAVE_VERSION_INFO
#ifdef LIBTVSERVICE_BUILD_NAME
    return LIBTVSERVICE_BUILD_NAME;
#endif
#endif
    //return " Unknow ";
}

const char *tvservice_get_board_version_info(void)
{
#ifdef HAVE_VERSION_INFO
#ifdef TVAPI_BOARD_VERSION
    return TVAPI_BOARD_VERSION;
#endif
#endif
    //return " Unknow ";
}
