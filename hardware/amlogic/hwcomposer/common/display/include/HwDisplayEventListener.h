/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef HWDISPLAY_EVENT_LISTENER_H
#define HWDISPLAY_EVENT_LISTENER_H

#include <DrmTypes.h>
#include <BasicTypes.h>
#include <utils/threads.h>
#include <pthread.h>


class HwDisplayEventHandler {
public:
  HwDisplayEventHandler() {
  }
  virtual ~HwDisplayEventHandler() {
  }

  virtual void handleEvent(drm_display_event event, int val) = 0;
};

class HwDisplayEventListener
    :   public android::Singleton<HwDisplayEventListener> {

public:
    HwDisplayEventListener();
    ~HwDisplayEventListener();

    int32_t registerHandler(
        drm_display_event event, HwDisplayEventHandler * handler);


protected:
    std::multimap<drm_display_event, HwDisplayEventHandler* >
        mEventHandler;

    char * mUeventMsg;
    int mEventSocket;
    int mCtlInFd;
    int mCtlOutFd;

protected:
    static void * ueventThread(void * data);
    void handleUevent();

private:
    void createThread();
    int32_t handle(drm_display_event event, int val);

    pthread_t hw_event_thread;

    pthread_mutex_t hw_event_mutex;
};

#endif/*HWDISPLAY_EVENT_LISTENER_H*/
