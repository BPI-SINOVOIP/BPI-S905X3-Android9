/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "tvconfig"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "include/tvscanconfig.h"

#include "include/CTvLog.h"
#include "include/CIniFile.h"

//INI_CONFIG* mpConfig = NULL;
static char mpFilePath[256] = {0};

static CIniFile *pIniFile = NULL;

int tv_scan_config_load(const char *file_name)
{
    if (pIniFile != NULL)
        delete pIniFile;

    pIniFile = new CIniFile();
    pIniFile->LoadFromFile(file_name);
    strncpy(mpFilePath, file_name, sizeof(mpFilePath)-1);

    return 0;
}

int tv_scan_config_unload(void)
{
    if (pIniFile != NULL)
        delete pIniFile;

    return 0;
}

const char* get_tv_support_country_list(void)
{
    return pIniFile->GetString(CFG_TV_SCAN_SECTION, CFG_TV_SUPPORT_COUNTRY_LIST, NULL);
}

const char* get_tv_current_country(void)
{
    return pIniFile->GetString(CFG_TV_SCAN_SECTION, CFG_TV_CURRENT_COUNTRY, NULL);
}

const char* get_tv_country_name(const char *country_code)
{
    char section[CFG_TV_COUNYRT_SECTION_LEN] = { 0 };
    strcat(strcat(section, CFG_TV_COUNYRT_SECTION), country_code);
    return pIniFile->GetString(section, CFG_TV_COUNTRY_NAME, NULL);
}

const char* get_tv_search_mode(const char *country_code)
{
    char section[CFG_TV_COUNYRT_SECTION_LEN] = { 0 };
    strcat(strcat(section, CFG_TV_COUNYRT_SECTION), country_code);
    return pIniFile->GetString(section, CFG_TV_SEARCH_MODE, NULL);
}

bool get_tv_dtv_support(const char *country_code)
{
    char section[CFG_TV_COUNYRT_SECTION_LEN] = { 0 };
    strcat(strcat(section, CFG_TV_COUNYRT_SECTION), country_code);
    return (pIniFile->GetInt(section, CFG_TV_DTV_SUPPORT, 0) != 0);
}

const char* get_tv_dtv_system(const char *country_code)
{
    char section[CFG_TV_COUNYRT_SECTION_LEN] = { 0 };
    strcat(strcat(section, CFG_TV_COUNYRT_SECTION), country_code);
    return pIniFile->GetString(section, CFG_TV_DTV_SYSTEM, NULL);
}

bool get_tv_atv_support(const char *country_code)
{
    char section[CFG_TV_COUNYRT_SECTION_LEN] = { 0 };
    strcat(strcat(section, CFG_TV_COUNYRT_SECTION), country_code);
    return (pIniFile->GetInt(section, CFG_TV_ATV_SUPPORT, 0) != 0);
}

const char* get_tv_atv_color_system(const char *country_code)
{
    char section[CFG_TV_COUNYRT_SECTION_LEN] = { 0 };
    strcat(strcat(section, CFG_TV_COUNYRT_SECTION), country_code);
    return pIniFile->GetString(section, CFG_TV_ATV_COLOR_SYSTEM, NULL);
}

const char* get_tv_atv_sound_system(const char *country_code)
{
    char section[CFG_TV_COUNYRT_SECTION_LEN] = { 0 };
    strcat(strcat(section, CFG_TV_COUNYRT_SECTION), country_code);
    return pIniFile->GetString(section, CFG_TV_ATV_SOUND_SYSTEM, NULL);
}

const char* get_tv_atv_min_max_freq(const char *country_code)
{
    char section[CFG_TV_COUNYRT_SECTION_LEN] = { 0 };
    strcat(strcat(section, CFG_TV_COUNYRT_SECTION), country_code);
    return pIniFile->GetString(section, CFG_TV_ATV_MIN_MAX_FREQ, NULL);
}

bool get_tv_atv_step_scan(const char *country_code)
{
    char section[CFG_TV_COUNYRT_SECTION_LEN] = { 0 };
    strcat(strcat(section, CFG_TV_COUNYRT_SECTION), country_code);
    return (pIniFile->GetInt(section, CFG_TV_ATV_STEP_SCAN, 0) != 0);
}
