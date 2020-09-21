/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CEpool"

#include "CEpoll.h"

CEpoll::CEpoll(int _max, int maxevents): max(_max),
    epoll_fd(-1),
    epoll_timeout(-1),
    epoll_maxevents(maxevents),
    backEvents(0)
{

}

CEpoll::~CEpoll()
{
    if (epoll_fd > 0) {
        close(epoll_fd);
    }
    delete[] backEvents;
}

void CEpoll::setTimeout(int timeout)
{
    epoll_timeout = timeout;
}

inline
void CEpoll::setMaxEvents(int maxevents)
{
    epoll_maxevents = maxevents;
}

inline
const epoll_event *CEpoll::events() const
{
    return backEvents;
}

int CEpoll::create()
{
    epoll_fd = ::epoll_create(max);
    if (epoll_fd > 0) {
        backEvents = new epoll_event[epoll_maxevents];
        return epoll_fd;
    } else {
        SYS_LOGE("epoll_create error!\n");
        return -1;
    }
}

int CEpoll::add(int fd, epoll_event *event)
{
    if (epoll_fd > 0) {
        return ::epoll_ctl(epoll_fd, ADD, fd, event);
    } else {
        SYS_LOGE("%s : epoll_fd invalid!\n", __FUNCTION__);
        return -1;
    }
}

int CEpoll::mod(int fd, epoll_event *event)
{
    if (epoll_fd > 0) {
        return ::epoll_ctl(epoll_fd, MOD, fd, event);
    } else {
        SYS_LOGE("%s : epoll_fd invalid!\n", __FUNCTION__);
        return -1;
    }
}

int CEpoll::del(int fd, epoll_event *event)
{
    if (epoll_fd > 0) {
        return ::epoll_ctl(epoll_fd, DEL, fd, event);
    } else {
        SYS_LOGE("%s : epoll_fd invalid!\n", __FUNCTION__);
        return -1;
    }
}

int CEpoll::wait()
{
    if (epoll_fd > 0) {
        return ::epoll_wait(epoll_fd, backEvents, epoll_maxevents, epoll_timeout);
    } else {
        SYS_LOGE("%s : epoll_fd invalid!\n", __FUNCTION__);
        return -1;
    }
}
