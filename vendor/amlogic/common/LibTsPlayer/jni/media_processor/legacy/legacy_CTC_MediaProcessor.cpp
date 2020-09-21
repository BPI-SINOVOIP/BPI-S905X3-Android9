/*
 * author: bo.cao@amlogic.com
 * date: 2012-07-20
 * wrap original source code for CTC usage
 */

#include "CTC_MediaProcessor.h"
#include "../CTsPlayerImpl.h"
#if ANDROID_PLATFORM_SDK_VERSION <= 27
#include "CTsOmxPlayer.h"
#endif
#include <android/log.h>
#include <cutils/properties.h>
#include "Amsysfsutils.h"
#include <dlfcn.h>
#include <utils/Log.h>
#include <string.h>
#include <stdlib.h>

#define LOG_TAG "CTC_MediaProcessor"
typedef ITsPlayer *(*CreateTsPlayerFunc)(void);
static CreateTsPlayerFunc createLivePlayer = NULL;
static CreateTsPlayerFunc get_createLivePlayer() {

    void *libHandle = dlopen("/system/lib/libliveplayer.so", RTLD_NOW);

    if (libHandle == NULL) {
        ALOGD("open libliveplayer.so failed for reason: %s", dlerror());
        return NULL;
    }
    createLivePlayer = (CreateTsPlayerFunc)dlsym(
                            libHandle,
                            "_Z14createTsPlayerv");
    if (createLivePlayer == NULL) {
        ALOGD("dlsym on liveplayer failed for reason %s", dlerror());
        dlclose(libHandle);
        libHandle = NULL;
        return NULL;
    }
    ALOGD("dlopen libliveplayer success, libHandle=%p\n", libHandle);
    return createLivePlayer;
}

// need single instance?
ITsPlayer* GetMediaProcessor()
{
    char value[PROPERTY_VALUE_MAX] = {0};
    int middle_soft_demux = 0;
    int multi_enable = 0;

    amsysfs_set_sysfs_str("iptv.middle.softdemux", "0");
    amsysfs_set_sysfs_str("media.ctcplayer.enable", "0");

    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("iptv.middle.softdemux", value, "0");
    middle_soft_demux = atoi(value);

    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("media.ctcplayer.enable", value, "0");
    multi_enable = atoi(value);
    /* +[SE] [BUG][BUG-167372][yanan.wang] added:increase the keep_mode_threshold from 85 to 110 when multi-instances*/
    amsysfs_set_sysfs_int("/sys/class/thermal/thermal_zone0/keep_mode_threshold", 85);

    ALOGI("GetMediaProcessor, middle_soft_demux=%d, multi_enable=%d\n", middle_soft_demux, multi_enable);
    return new CTsPlayer();
}

ITsPlayer* GetMediaProcessor(player_type_t type)
{
    int mOmxDebug = 0;
    int check_apk_use = 0;
    int use_param = 0;
    int real_type = 0;
    char value[PROPERTY_VALUE_MAX] = {0};
    ITsPlayer *ret = NULL;
    if (createLivePlayer == NULL)
        createLivePlayer = get_createLivePlayer();

    use_param =  type & PLAYER_TYPE_USE_PARAM;
    real_type = type & (~PLAYER_TYPE_USE_PARAM);
    ALOGI("GetMediaProcessor,real_type:%d type=%d,use_param:%d\n",real_type, type,use_param);
    type = (player_type_t)real_type;

    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("media.ctcplayer.omxdebug", value, "0");
    mOmxDebug = atoi(value);

    /*display_mode 1: vd2; 2: 9-video layer */
    int display_mode = 0;
    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("media.ctc.display.mode", value, "0");
    display_mode = atoi(value);
    ALOGI("display mode:%d\n", display_mode);
    /* +[SE] [BUG][IPTV-2452][yinli.xia]
    added: distinguish PIP player from apk to url*/
    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("media.ctcplayer.enable", value, NULL);
    check_apk_use = atoi(value);

    if (check_apk_use ||use_param) {
        if (type == PLAYER_TYPE_OMX) {
#if ANDROID_PLATFORM_SDK_VERSION <= 27
            return new CTsOmxPlayer();
#else
            return (*createLivePlayer)();
#endif

        } else if (type == PLAYER_TYPE_HWOMX || mOmxDebug == 1 || display_mode == 2) {
            return (*createLivePlayer)();
        } else if (type == PLAYER_TYPE_NORMAL_MULTI) {
            struct CTsParameter p;
            p.mMultiSupport = 1;
            return new CTsPlayer(p);
        } else {
            return new CTsPlayer();
        }
    } else {
        if (type == PLAYER_TYPE_OMX) {
#if ANDROID_PLATFORM_SDK_VERSION <= 27
            return new CTsOmxPlayer();
#else
            return (*createLivePlayer)();
#endif
        } else if (type == PLAYER_TYPE_HWOMX || mOmxDebug == 1 || display_mode == 1 || display_mode == 2) {
            return (*createLivePlayer)();
        } else if (type == PLAYER_TYPE_NORMAL_MULTI) {
            struct CTsParameter p;
            p.mMultiSupport = 1;
            return new CTsPlayer(p);
        } else {
            return new CTsPlayer();
        }
    }
}
#ifdef USE_OPTEEOS
ITsPlayer* GetMediaProcessor(bool DRMMode)
{
    return new CTsPlayer(DRMMode);
}

ITsPlayer* GetMediaProcessor(player_type_t type, bool DRMMode)
{
    if (type == PLAYER_TYPE_OMX)
#if ANDROID_PLATFORM_SDK_VERSION <= 27
        return new CTsOmxPlayer();
#else
        return (*createLivePlayer)();
#endif
    else
        return new CTsPlayer(DRMMode);
}
#endif

int GetMediaProcessorVersion()
{
#ifdef TELECOM_QOS_SUPPORT
    return 3;
#else
    return 2;
#endif
}
