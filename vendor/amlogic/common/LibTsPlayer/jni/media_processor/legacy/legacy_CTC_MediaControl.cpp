/*
 * author: wei.liu@amlogic.com
 * date: 2012-07-12
 * wrap original source code for CTC usage
 */

#include "CTC_MediaControl.h"
#include "../CTsPlayerImpl.h"
#if ANDROID_PLATFORM_SDK_VERSION <= 27
#include "CTsOmxPlayer.h"
#endif
#include <cutils/properties.h>
#include <dlfcn.h>

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
ITsPlayer* GetMediaControl(int use_omx_decoder)
{
	ITsPlayer *ret = NULL;
    if (createLivePlayer == NULL)
        createLivePlayer = get_createLivePlayer();

    switch (use_omx_decoder) {
		case 0:
		ret = new CTsPlayer();
		break;
		case 1:
#if ANDROID_PLATFORM_SDK_VERSION <= 27
		ret = new CTsOmxPlayer();
#else
		ret = (*createLivePlayer)();
#endif
		break;
		case 2:
		ret = (*createLivePlayer)();
		break;
		default:
		return NULL;
	}
	return ret;
}

ITsPlayer* GetMediaControl()
{
    char value[PROPERTY_VALUE_MAX] = {0};
    property_get("iptv.decoder.omx", value, "0");
    int prop_use_omxdecoder = atoi(value);

    if (prop_use_omxdecoder)
#if ANDROID_PLATFORM_SDK_VERSION <= 27
        return new CTsOmxPlayer();
#else
        return (*createLivePlayer)();
#endif
    else
        return new CTsPlayer();
}

int Get_MediaControlVersion()
{
	return 1;
}

class CTC_MediaControl:public CTsPlayer
{

};

