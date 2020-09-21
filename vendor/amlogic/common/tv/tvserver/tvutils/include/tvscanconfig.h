/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __TVSCANCONFIG_API_H__
#define __TVSCANCONFIG_API_H__

#define TV_SCAN_CONFIG_FILE_PATH        "/vendor/etc/tvconfig/tvscan.conf"

#define CFG_TV_SCAN_SECTION             "TV_SCAN_CONFIG"
#define CFG_TV_COUNYRT_SECTION          "COUNTRY_"
#define CFG_TV_SUPPORT_COUNTRY_LIST     "tv.support.country.list"
#define CFG_TV_CURRENT_COUNTRY          "tv.current.country"
#define CFG_TV_COUNTRY_CODE             "tv.country.code"
#define CFG_TV_COUNTRY_NAME             "tv.country.name"
#define CFG_TV_SEARCH_MODE              "tv.search.mode"
#define CFG_TV_DTV_SUPPORT              "tv.dtv.support"
#define CFG_TV_DTV_SYSTEM               "tv.dtv.sys"
#define CFG_TV_ATV_SUPPORT              "tv.atv.support"
#define CFG_TV_ATV_COLOR_SYSTEM         "tv.atv.color.sys"
#define CFG_TV_ATV_SOUND_SYSTEM         "tv.atv.sound.sys"
#define CFG_TV_ATV_MIN_MAX_FREQ         "tv.atv.min.max.freq"
#define CFG_TV_ATV_STEP_SCAN            "tv.atv.step.scan"

#define CFG_TV_COUNYRT_SECTION_LEN       (sizeof(CFG_TV_COUNYRT_SECTION) + 3)

int tv_scan_config_load(const char *file_name);
int tv_scan_config_unload(void);

const char* get_tv_support_country_list(void);
const char* get_tv_current_country(void);
const char* get_tv_country_name(const char *country_code);
const char* get_tv_search_mode(const char *country_code);
bool get_tv_dtv_support(const char *country_code);
const char* get_tv_dtv_system(const char *country_code);
bool get_tv_atv_support(const char *country_code);
const char* get_tv_atv_color_system(const char *country_code);
const char* get_tv_atv_sound_system(const char *country_code);
const char* get_tv_atv_min_max_freq(const char *country_code);
bool get_tv_atv_step_scan(const char *country_code);

#endif /* __TVSCANCONFIG_API_H__ */
