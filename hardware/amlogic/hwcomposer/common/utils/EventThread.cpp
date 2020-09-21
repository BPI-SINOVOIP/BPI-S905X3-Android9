/*
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <EventThread.h>
#include <MesonLog.h>
#include <chrono>

EventThread::EventThread(const char * name) {
    MESON_ASSERT(name, "EventThread need a non-NULL name.");

    mExit = false;
    mHandler = NULL;

    int nameLen = strlen(name) + 1;
    mName = new char [nameLen];
    memcpy(mName, name, nameLen - 1);
}

EventThread::~EventThread() {
    std::unique_lock<std::mutex> eventLock(mEventMutex);
    mExit = true;
    eventLock.unlock();
    mEventCond.notify_all();
    pthread_join(mEventThread, NULL);
    delete mName;
}

void EventThread::setHandler(EventHandler * handler) {
    MESON_ASSERT(handler, "setHandler should not NULL.");
    mHandler = handler;
}

void EventThread::start() {
    int ret = pthread_create(&mEventThread, NULL, EventThread::threadMain, (void *)this);
    if (ret) {
        MESON_LOGE("failed to start EventThread: %s", strerror(ret));
        return;
    }
}

void EventThread::sendEvent(int what) {
    std::unique_lock<std::mutex> eventLock(mEventMutex);
    mEvents.push_back({what, 0});
    eventLock.unlock();
    mEventCond.notify_one();
}

void EventThread::sendEventDelayed(int what,  uint32_t delayMs) {
    std::unique_lock<std::mutex> eventLock(mEventMutex);
    nsecs_t now = systemTime(CLOCK_MONOTONIC);
    nsecs_t delayTime = delayMs;
    delayTime = delayTime * 1000000;

    MESON_LOGE("send event delayed %lld + %lld = %lld", now, delayTime,
        now + delayTime);
    mEvents.push_back({what,  now + delayTime});
    eventLock.unlock();
    mEventCond.notify_one();
}

void EventThread::removeEvent(int what) {
    std::unique_lock<std::mutex> eventLock(mEventMutex);
    for (auto it = mEvents.begin(); it != mEvents.end();) {
        if (it->what == what)
            it = mEvents.erase(it);
        else
            ++ it;
    }
    eventLock.unlock();
    mEventCond.notify_one();
}

void EventThread::processEvents() {
    std::unique_lock<std::mutex> eventLock(mEventMutex);
    nsecs_t closestDueTime = 0;

    for (auto it = mEvents.begin(); it != mEvents.end();) {
        bool bHandle = false;
        if (it->dueTime == 0) {
            bHandle = true;
        } else {
            nsecs_t now = systemTime(CLOCK_MONOTONIC);
            if (it->dueTime < now) {
                bHandle = true;
            }
        }

        if (bHandle) {
            mHandler->handleEvent(it->what);
            it = mEvents.erase(it);
        } else {
            if (closestDueTime == 0 || closestDueTime > it->dueTime)
                closestDueTime = it->dueTime;
            ++ it;
        }
    }

    nsecs_t now = systemTime(CLOCK_MONOTONIC);
    if (closestDueTime > now) {
        nsecs_t delayed = closestDueTime - now;
        MESON_LOGE("get time %lld sleep %lld to wait", closestDueTime, delayed);
        mEventCond.wait_for(eventLock,
            std::chrono::nanoseconds(delayed));
    } else  {
        mEventCond.wait(eventLock);
    }
}

void * EventThread::threadMain(void * data) {
    MESON_ASSERT(data, "EventThread data should not be NULL.");
    EventThread * pThis = (EventThread *) data;

    while (!pThis->mExit) {
        pThis->processEvents();
    }

    pthread_exit(0);
    return NULL;
}

