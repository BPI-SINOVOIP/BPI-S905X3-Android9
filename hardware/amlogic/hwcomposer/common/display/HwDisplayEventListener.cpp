/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <poll.h>
#include <string.h>
#include <cutils/uevent.h>
#include <MesonLog.h>

#include "HwDisplayEventListener.h"


ANDROID_SINGLETON_STATIC_INSTANCE(HwDisplayEventListener)

#define HDMITX_HOTPLUG_EVENT \
    "change@/devices/virtual/amhdmitx/amhdmitx0/hdmi"
#define HDMITX_HDCP_EVENT \
    "change@/devices/virtual/amhdmitx/amhdmitx0/hdcp"
#define VOUT_MODE_EVENT \
    "change@/devices/platform/vout/extcon/setmode"
#define VOUT2_MODE_EVENT \
    "change@/devices/platform/vout2/extcon/setmode2"

#define UEVENT_MAX_LEN (4096)

#define OLD_EVENT_STATE_ENABLE "SWITCH_STATE=1"
#define OLD_EVENT_STATE_DISABLE "SWITCH_STATE=0"
#define NEW_EVENT_STATE_ENABLE "STATE=HDMI=1"
#define NEW_EVENT_STATE_DISABLE "STATE=HDMI=0"
#define VOUT_EVENT_MODESWITCH_BEGIN "STATE=ACA=1"
#define VOUT_EVENT_MODESWITCH_COMPLETE "STATE=ACA=0"

typedef struct drm_uevent_info {
    const char * head;
    drm_display_event eventType;
    const char * stateEnable;
    const char * stateDisable;
}drm_uevent_info_t;

/*load uevent parser*/
#if PLATFORM_SDK_VERSION >= 28
static drm_uevent_info_t mUeventParser[] = {
    {HDMITX_HOTPLUG_EVENT, DRM_EVENT_HDMITX_HOTPLUG,
        NEW_EVENT_STATE_ENABLE, NEW_EVENT_STATE_DISABLE},
    {HDMITX_HDCP_EVENT, DRM_EVENT_HDMITX_HDCP,
        NEW_EVENT_STATE_ENABLE, NEW_EVENT_STATE_DISABLE},
    {VOUT_MODE_EVENT, DRM_EVENT_VOUT1_MODE_CHANGED,
        VOUT_EVENT_MODESWITCH_COMPLETE, VOUT_EVENT_MODESWITCH_BEGIN},
    {VOUT2_MODE_EVENT, DRM_EVENT_VOUT2_MODE_CHANGED,
        VOUT_EVENT_MODESWITCH_COMPLETE, VOUT_EVENT_MODESWITCH_BEGIN}
};
#else
static drm_uevent_info_t mUeventParser[] = {
    {HDMITX_HOTPLUG_EVENT, DRM_EVENT_HDMITX_HOTPLUG,
        OLD_EVENT_STATE_ENABLE, OLD_EVENT_STATE_DISABLE},
    {HDMITX_HDCP_EVENT, DRM_EVENT_HDMITX_HDCP,
        OLD_EVENT_STATE_ENABLE, OLD_EVENT_STATE_DISABLE},
    {VOUT_MODE_EVENT, DRM_EVENT_VOUT1_MODE_CHANGED,
        OLD_EVENT_STATE_ENABLE, OLD_EVENT_STATE_DISABLE}
    {VOUT2_MODE_EVENT, DRM_EVENT_VOUT2_MODE_CHANGED,
        OLD_EVENT_STATE_ENABLE, OLD_EVENT_STATE_DISABLE}
};
#endif

HwDisplayEventListener::HwDisplayEventListener()
    :   mUeventMsg(NULL),
        mCtlInFd(-1),
        mCtlOutFd(-1) {
    /*init uevent socket.*/
    mEventSocket = uevent_open_socket(64*1024, true);
    if (mEventSocket < 0) {
        MESON_LOGE("uevent_init: uevent_open_socket failed\n");
        return;
    }

    /*make pipe fd for exit uevent thread.*/
    int ctlPipe[2];
    if (pipe(ctlPipe) < 0) {
        MESON_LOGE("make control pipe fail.\n");
        return;
    }
    mCtlInFd = ctlPipe[0];
    mCtlOutFd = ctlPipe[1];

    mUeventMsg = new char[UEVENT_MAX_LEN];
    memset(mUeventMsg, 0, UEVENT_MAX_LEN);

    pthread_mutex_init(&hw_event_mutex, NULL);
}

HwDisplayEventListener::~HwDisplayEventListener() {
    if (mCtlInFd >= 0) {
        close(mCtlInFd);
        mCtlInFd = -1;
    }

//    requestExitAndWait();

    if (mEventSocket >= 0) {
        close(mEventSocket);
        mEventSocket = -1;
    }

    if (mCtlOutFd >= 0) {
        close(mCtlOutFd);
        mCtlOutFd = -1;
    }

    delete mUeventMsg;

    mEventHandler.clear();
    pthread_mutex_destroy(&hw_event_mutex);
}

void HwDisplayEventListener::createThread() {
    int ret = pthread_create(&hw_event_thread, NULL, ueventThread, this);
    if (ret) {
        MESON_LOGE("failed to start uevent thread: %s", strerror(ret));
        return;
    }
}

void HwDisplayEventListener::handleUevent() {
    for (drm_uevent_info_t uevent : mUeventParser) {
        if (strcmp(mUeventMsg, uevent.head) == 0) {
            char * msg = mUeventMsg;
            while (*msg) {
                MESON_LOGD("received Uevent: %s", msg);
                if (strstr(msg, uevent.stateEnable)) {
                    handle(uevent.eventType, 1);
                    return;
                } else if (strstr(msg, uevent.stateDisable)) {
                    handle(uevent.eventType, 0);
                    return;
                }
                msg += strlen(msg) + 1;
            }
        }
    }
}

void * HwDisplayEventListener::ueventThread(void * data) {
    HwDisplayEventListener* pThis = (HwDisplayEventListener*)data;
    while (true) {
        pthread_mutex_lock(&pThis->hw_event_mutex);

        int rtn;
        struct pollfd fds[2];

        fds[0].fd = pThis->mEventSocket;
        fds[0].events = POLLIN;
        fds[0].revents = 0;
        fds[1].fd = pThis->mCtlOutFd;
        fds[1].events = POLLIN;
        fds[1].revents = 0;

        rtn = poll(fds, 2, -1);

        if (rtn > 0 && fds[0].revents == POLLIN) {
            ssize_t len = uevent_kernel_multicast_recv(pThis->mEventSocket,
                pThis->mUeventMsg, UEVENT_MAX_LEN - 2);
            if (len > 0)
                pThis->handleUevent();
        } else if (fds[1].revents) {
            MESON_LOGE("exit display event thread.");
            return NULL;
        }

        pthread_mutex_unlock(&pThis->hw_event_mutex);
    }
    return NULL;
}

int32_t HwDisplayEventListener::handle(drm_display_event event, int val) {
    std::multimap<drm_display_event, HwDisplayEventHandler *>::iterator it;
    for (it = mEventHandler.begin(); it != mEventHandler.end(); it++) {
        if (it->first == event || it->first == DRM_EVENT_ALL)
            it->second->handleEvent(event, val);
    }

    return 0;
}

int32_t HwDisplayEventListener::registerHandler(
    drm_display_event event, HwDisplayEventHandler * handler) {
    std::multimap<drm_display_event, HwDisplayEventHandler* >::iterator it;
    switch (event) {
        case DRM_EVENT_HDMITX_HOTPLUG:
        case DRM_EVENT_HDMITX_HDCP:
        case DRM_EVENT_VOUT1_MODE_CHANGED:
        case DRM_EVENT_VOUT2_MODE_CHANGED:
        case DRM_EVENT_ALL:
            mEventHandler.insert(std::make_pair(event, handler));
            createThread();
            return 0;
        default:
            return -ENOENT ;
    };
}

