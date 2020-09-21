/*
 * Copyright (C) 2011 The Android Open Source Project
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



#ifndef __MESSAGEQUEUE_H__
#define __MESSAGEQUEUE_H__

#include "DebugUtils.h"
#include <stdint.h>

///Uncomment this macro to debug the message queue implementation
//#define DEBUG_LOG

///Camera HAL Logging Functions
#ifndef MSGQ_DEBUG_LOG

#define MSGQ_LOGVA(str)
#define MSGQ_LOGVB(str, ...)
#define MSGQ_LOGDA(str)
#define MSGQ_LOGDB(str, ...)

#else

#define MSGQ_LOGVA CAMHAL_LOGVA
#define MSGQ_LOGVB CAMHAL_LOGVB
#define MSGQ_LOGDA CAMHAL_LOGDA
#define MSGQ_LOGDB CAMHAL_LOGDB

#endif

#define MSGQ_LOGEA CAMHAL_LOGEA
#define MSGQ_LOGEB CAMHAL_LOGEB


namespace MSGUTILS {

///Message type
struct Message
{
    unsigned int command;
    void*        arg1;
    void*        arg2;
    void*        arg3;
    void*        arg4;
    int64_t     id;
};

///Message queue implementation
class MessageQueue
{
public:

    MessageQueue();
    ~MessageQueue();

    ///Get a message from the queue
    android::status_t get(Message*);

    ///Get the input file descriptor of the message queue
    int getInFd();

    ///Set the input file descriptor for the message queue
    void setInFd(int fd);

    ///Queue a message
    android::status_t put(Message*);

    ///Returns if the message queue is empty or not
    bool isEmpty();

    void clear();

    ///Force whether the message queue has message or not
    void setMsg(bool hasMsg=false);

    ///Wait for message in maximum three different queues with a timeout
    static int waitForMsg(MessageQueue *queue1, MessageQueue *queue2=0, MessageQueue *queue3=0, int timeout = 0);

    bool hasMsg()
    {
      return mHasMsg;
    }

private:
    int fd_read;
    int fd_write;
    bool mHasMsg;

};

};

#endif
