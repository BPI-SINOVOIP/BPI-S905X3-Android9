/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __TVCONFIG_API_H__
#define __TVCONFIG_API_H__

#define CC_CFG_KEY_STR_MAX_LEN                  (128)
#define CC_CFG_VALUE_STR_MAX_LEN                (512)

//for tv config
#define CFG_SECTION_TV                          "TV"
#define CFG_SECTION_ATV                         "ATV"
#define CFG_SECTION_SRC_INPUT                   "SourceInputMap"
#define CFG_SECTION_SETTING                     "SETTING"
#define CFG_SECTION_FBCUART                     "FBCUART"


#define CGF_DEFAULT_INPUT_IDS                   "tv.source.input.ids.default"

#define CFG_TVCHANNEL_ATV                       "ro.tv.tvinchannel.atv"
#define CFG_TVCHANNEL_AV1                       "ro.tv.tvinchannel.av1"
#define CFG_TVCHANNEL_AV2                       "ro.tv.tvinchannel.av2"
#define CFG_TVCHANNEL_YPBPR1                    "ro.tv.tvinchannel.ypbpr1"
#define CFG_TVCHANNEL_YPBPR2                    "ro.tv.tvinchannel.ypbpr2"
#define CFG_TVCHANNEL_HDMI1                     "ro.tv.tvinchannel.hdmi1"
#define CFG_TVCHANNEL_HDMI2                     "ro.tv.tvinchannel.hdmi2"
#define CFG_TVCHANNEL_HDMI3                     "ro.tv.tvinchannel.hdmi3"
#define CFG_TVCHANNEL_HDMI4                     "ro.tv.tvinchannel.hdmi4"
#define CFG_TVCHANNEL_VGA                       "ro.tv.tvinchannel.vga"

#define CGF_TV_SUPPORT_COUNTRY                  "tv.support.country.name"
#define CGF_TV_AUTOSWITCH_MONITOR_EN            "tv.autoswitch.monitor.en"
#define CFG_SSM_HDMI_AV_DETECT                  "ssm.hdmi_av.hotplug.detect.en"
#define CFG_SSM_HDMI_EDID_EN                    "ssm.handle.hdmi.edid.en"
#define CFG_SSM_HDMI_EDID_VERSION               "ssm.handle.hdmi.edid.version"
#define CFG_SSM_PRECOPY_ENABLE                  "ssm.precopying.en"
#define CFG_SSM_PRECOPY_FILE_PATH               "ssm.precopying.devpath"

#define CFG_ATV_FREQ_LIST                       "atv.get.min.max.freq"
#define CFG_ATV_AUTO_SCAN_MODE                  "atv.auto.scan.mode"

#define CFG_DTV_MODE                            "dtv.mode"
#define CFG_DTV_SCAN_SORT_MODE                  "dtv.scan.sort.mode"
#define CFG_DTV_SCAN_STOREMODE_FTA              "dtv.scan.skip.scramble"
#define CFG_DTV_SCAN_STOREMODE_NOPAL            "dtv.scan.skip.pal"
#define CFG_DTV_SCAN_STOREMODE_NOVCT            "dtv.scan.skip.onlyvct"
#define CFG_DTV_SCAN_STOREMODE_NOVCTHIDE        "dtv.scan.skip.vcthide"
#define CFG_DTV_SCAN_STD_FORCE                  "dtv.scan.std.force"

#define CFG_DTV_CHECK_SCRAMBLE_MODE             "dtv.check.scramble.mode"
#define CFG_DTV_CHECK_SCRAMBLE_AV               "dtv.check.scramble.av"
#define CFG_DTV_CHECK_DATA_AUDIO                "dtv.check.data.audio"
#define CFG_DTV_SCAN_STOREMODE_VALIDPID         "dtv.scan.skip.invalidpid"

#define CFG_TVIN_KERNELPET_DISABLE              "tvin.kernelpet_disable"
#define CFG_TVIN_KERNELPET_TIMEROUT             "tvin.kernelpet.timeout"
#define CFG_TVIN_USERPET                        "tvin.userpet"
#define CFG_TVIN_USERPET_TIMEROUT               "tvin.userpet.timeout"
#define CFG_TVIN_USERPET_RESET                  "tvin.userpet.reset"
#define CFG_TVIN_DISPLAY_FREQ_AUTO              "tvin.autoset.displayfreq"
#define CFG_TVIN_DISPLAY_RESOLUTION_FIRST       "tvin.autosetdisplay.resolution.first.en"
#define CFG_TVIN_DB_REG                         "tvin.db.reg.en"
#define CFG_TVIN_THERMAL_THRESHOLD_ENABLE       "tvin.thermal.threshold.enable"
#define CFG_TVIN_THERMAL_THRESHOLD_VALUE        "tvin.thermal.threshold.value"
#define CFG_TVIN_THERMAL_FBC_NORMAL_VALUE       "tvin.thermal.fbc.normal.value"
#define CFG_TVIN_THERMAL_FBC_COLD_VALUE         "tvin.thermal.fbc.cold.value"
#define CFG_TVIN_ATV_DISPLAY_SNOW               "tvin.atv.display.snow"
#define CFG_BLUE_SCREEN_COLOR                   "tvin.bluescreen.color"
#define CFG_TVIN_AUDIO_INSOURCE_ATV             "tvin.aud.insource.atv"
#define CFG_TVIN_TVPATH_SET_MANUAL              "tvin.manual.set.tvpath"
#define CFG_TVIN_TVPATH_SET_DEFAULT             "tvin.manual.set.defaultpath"


#define CFG_FBC_PANEL_INFO                      "fbc.get.panelinfo"
#define CFG_FBC_USED                            "platform.havefbc"
#define CFG_PROJECT_INFO                        "platform.projectinfo.src"

#define CFG_CAHNNEL_DB                          "tv.channel.db"

#if ANDROID_PLATFORM_SDK_VERSION >= 28
#define DEF_VALUE_CAHNNEL_DB                    "/mnt/vendor/param/dtv.db"
#else
#define DEF_VALUE_CAHNNEL_DB                    "/param/dtv.db"
#endif

//for tv property
#define PROP_DEF_CAPTURE_NAME                   "snd.card.default.card.capture"
#define PROP_AUDIO_CARD_NUM                     "snd.card.totle.num"

#define UBOOTENV_OUTPUTMODE                     "ubootenv.var.outputmode"
#define UBOOTENV_HDMIMODE                       "ubootenv.var.hdmimode"
#define UBOOTENV_LCD_REVERSE                    "ubootenv.var.lcd_reverse"
#define UBOOTENV_CONSOLE                        "ubootenv.var.console"
#define UBOOTENV_PROJECT_INFO                   "ubootenv.var.project_info"
#define UBOOTENV_AMPINDEX                       "ubootenv.var.ampindex"

//hdmi edid
#define UBOOTENV_EDID14_PATH                    "ubootenv.var.edid_14_dir"
#define UBOOTENV_EDID20_PATH                    "ubootenv.var.edid_20_dir"
#define UBOOTENV_EDID_VERSION                   "ubootenv.var.edid_select"
#define UBOOTENV_PORT_MAP                       "ubootenv.var.port_map"

//for tv sysfs
#define SYS_SPDIF_MODE_DEV_PATH                 "/sys/class/audiodsp/digital_raw"
#define SYS_VECM_DNLP_ADJ_LEVEL                 "/sys/module/am_vecm/parameters/dnlp_adj_level"

#define SYS_AUIDO_DSP_AC3_DRC                   "/sys/class/audiodsp/ac3_drc_control"
#define SYS_AUIDO_DIRECT_RIGHT_GAIN             "/sys/class/amaudio2/aml_direct_right_gain"
#define SYS_AUIDO_DIRECT_LEFT_GAIN              "/sys/class/amaudio2/aml_direct_left_gain"

#define SYS_VIDEO_FRAME_HEIGHT                  "/sys/class/video/frame_height"
#define SYS_VIDEO_SCALER_PATH_SEL               "/sys/class/video/video_scaler_path_sel"

#define SYS_DROILOGIC_DEBUG                     "/sys/class/amlogic/debug"
#define SYS_ATV_DEMOD_DEBUG                     "/sys/class/amlatvdemod/atvdemod_debug"

#define SYS_SCAN_TO_PRIVATE_DB                  "tuner.device.scan.to.private.db.en"

#define FRONTEND_TS_SOURCE                      "frontend.ts.source"

extern int tv_config_load(const char *file_name);
extern int tv_config_unload();
extern int tv_config_save();

//[TV] section
extern int config_set_str(const char *section, const char *key, const char *value);
extern const char *config_get_str(const char *section, const char *key, const char *def_value);
extern int config_get_int(const char *section, const char *key, const int def_value);
extern int config_set_int(const char *section, const char *key, const int value);
extern float config_get_float(const char *section, const char *key, const float def_value);
extern int config_set_float(const char *section, const char *key, const int value);

#endif //__TVCONFIG_API_H__
