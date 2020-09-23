
/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "AmloadAmlogicPlayers"
#include <utils/Log.h>
#include <cutils/properties.h>
#include <utils/threads.h>
#include <utils/KeyedVector.h>

#include "AmSupportModules_priv.h"

namespace android
{

static sp<AmSharedLibrary> gLibAmlMedia;
static sp<AmSharedLibrary> gLibAmNuPlayer;
static sp<AmSharedLibrary> gLibDrmPlayer;
static sp<AmSharedLibrary> gLibAmMediaPlayer;
bool  LoadAndInitAmlogicMediaFactory(void)
{
    int err;
    String8 name("libmedia_amlogic.so");
    gLibAmlMedia = new AmSharedLibrary(name);
    if (gLibAmlMedia != NULL) {
        typedef int (*init_fun)(void);

        init_fun init =
            (init_fun)gLibAmlMedia->lookup("_ZN7android23AmlogicMediaFactoryInitEv");

        if (init != NULL) {
            err = init();
            if (err) {
                 ALOGE("AmlogicMediaFactoryInit failed:%s", gLibAmlMedia->lastError());
                 gLibAmlMedia.clear();
             }
        } else {
            ALOGE("AmlogicMediaFactoryInit failed:%s", gLibAmlMedia->lastError());
            gLibAmlMedia.clear();
        }


    } else {
        ALOGE("load libmedia_amlogic.so for amlogicmedia failed:%s", gLibAmlMedia->lastError());
        gLibAmlMedia.clear();
    }


////////////////////////////gLibAmNuPlayer
    String8 name_nuplayer("libamnuplayer.so");
    gLibAmNuPlayer = new AmSharedLibrary(name_nuplayer);
    if (gLibAmNuPlayer != NULL) {
        typedef int (*init_fun)(void);

        init_fun init =
            (init_fun)gLibAmNuPlayer->lookup("_ZN7android26AmlogicNuPlayerFactoryInitEv");

        if (init != NULL) {
            err = init();
            if (err) {
                 ALOGE("AmlogicMediaFactoryInit failed:%s", gLibAmNuPlayer->lastError());
                 gLibAmNuPlayer.clear();
             }
        } else {
            ALOGE("AmlogicMediaFactoryInit failed:%s", gLibAmNuPlayer->lastError());
            gLibAmNuPlayer.clear();
        }


    } else {
        ALOGE("load AmlogicNuPlayerFactory.so for amlogicmedia failed:%s", gLibAmNuPlayer->lastError());
        gLibAmNuPlayer.clear();
    }
////////////////////////////gLibDrmPlayer
    String8 name_drmplayer("libDrmPlayer.so");
    gLibDrmPlayer = new AmSharedLibrary(name_drmplayer);
    if (gLibDrmPlayer != NULL) {
        typedef int (*init_fun)(void);

        init_fun init =
            (init_fun)gLibDrmPlayer->lookup("_ZN7android20DrmPlayerFactoryInitEv");

        if (init != NULL) {
            err = init();
            if (err) {
                 ALOGE("DrmPlayerFactoryInit failed:%s", gLibDrmPlayer->lastError());
                 gLibDrmPlayer.clear();
             }
        } else {
            ALOGE("DrmPlayerFactoryInit failed:%s", gLibDrmPlayer->lastError());
            gLibDrmPlayer.clear();
        }


    } else {
        ALOGE("load libDrmPlayer.so for amlogicmedia failed:%s", gLibDrmPlayer->lastError());
        gLibDrmPlayer.clear();
    }

////////////////////////////gLibAmMediaPlayer
    String8 name_ammediaplayer("libAmIptvMedia.so");
    gLibAmMediaPlayer = new AmSharedLibrary(name_ammediaplayer);
    if (gLibAmMediaPlayer != NULL) {
        typedef int (*init_fun)(void);

        init_fun init =
            (init_fun)gLibAmMediaPlayer->lookup("_ZN7android24AmMediaPlayerFactoryInitEv");

        if (init != NULL) {
            err = init();
            if (err) {
                 ALOGE("AmMediaPlayerFactory failed:%s", gLibAmMediaPlayer->lastError());
                 gLibAmMediaPlayer.clear();
             }
        } else {
            ALOGE("AmMediaPlayerFactory failed:%s", gLibAmMediaPlayer->lastError());
            gLibAmMediaPlayer.clear();
        }


    } else {
        ALOGE("load libAmIptvMedia.so for amlogicmedia failed:%s", gLibAmMediaPlayer->lastError());
        gLibAmMediaPlayer.clear();
    }
    return true;
}

}


