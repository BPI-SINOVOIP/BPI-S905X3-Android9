/*
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef EVENT_THREAD_H
#define EVENT_THREAD_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <pthread.h>
#include <BasicTypes.h>

class EventHandler{
public:
    virtual ~EventHandler() { }
    virtual void handleEvent(int what) = 0;
};

class EventThread {
public:
    EventThread(const char * name);
    ~EventThread();

public:
    void setHandler(EventHandler * handler);
    void start();

    void sendEvent(int what);
    void removeEvent(int what);
    void sendEventDelayed(int what, uint32_t delayMs);

protected:
    static void * threadMain(void * data);

    void processEvents();

protected:
        struct EventPack {
            int what;
            nsecs_t dueTime;
        };

        pthread_t mEventThread;

        std::vector<EventPack> mEvents;
        std::mutex mEventMutex;
        std::condition_variable mEventCond;

        EventHandler * mHandler;
        bool mExit;
        char * mName;
};
#endif
