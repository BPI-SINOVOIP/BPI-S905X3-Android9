/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#include <stdio.h>
#include <string.h>

#include <am_ver.h>

static char gitversionstr[256] = "N/A";

static int version_info_init(void) {
    static int info_is_inited = 0;
    int dirty_num = 0;

    if (info_is_inited > 0) {
        return 0;
    }
    info_is_inited++;

#ifdef HAVE_VERSION_INFO

#ifdef LIBDVB_GIT_UNCOMMIT_FILE_NUM
#if LIBDVB_GIT_UNCOMMIT_FILE_NUM>0
    dirty_num = LIBDVB_GIT_UNCOMMIT_FILE_NUM;
#endif
#endif

#ifdef LIBDVB_GIT_VERSION
    if (dirty_num > 0) {
        snprintf(gitversionstr, 250, "%s-with-%d-dirty-files", LIBDVB_GIT_VERSION, dirty_num);
    } else {
        snprintf(gitversionstr, 250, "%s", LIBDVB_GIT_VERSION);
    }
#endif

#endif

    return 0;
}

const char *dvb_get_git_version_info(void) {
    version_info_init();
    return gitversionstr;
}

const char *dvb_get_last_chaned_time_info(void) {
#ifdef HAVE_VERSION_INFO
#ifdef LIBDVB_LAST_CHANGED
    return LIBDVB_LAST_CHANGED;
#endif
#endif
    return " Unknow ";
}

const char *dvb_get_git_branch_info(void) {
#ifdef HAVE_VERSION_INFO
#ifdef LIBDVB_GIT_BRANCH
    return LIBDVB_GIT_BRANCH;
#endif
#endif
    return " Unknow ";
}

const char *dvb_get_build_time_info(void) {
#ifdef HAVE_VERSION_INFO
#ifdef LIBDVB_BUILD_TIME
    return LIBDVB_BUILD_TIME;
#endif
#endif
    return " Unknow ";
}

const char *dvb_get_build_name_info(void) {
#ifdef HAVE_VERSION_INFO
#ifdef LIBDVB_BUILD_NAME
    return LIBDVB_BUILD_NAME;
#endif
#endif
    return " Unknow ";
}
