/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#include "CFbcEpoll.h"

FbcEpoll::FbcEpoll(int _max, int maxevents): max(_max),
    epoll_fd(-1),
    epoll_timeout(-1),
    epoll_maxevents(maxevents),
    backEvents(0)
{

}

FbcEpoll::~FbcEpoll()
{
    if (isValid()) {
        close(epoll_fd);
    }
    delete[] backEvents;
}


inline bool FbcEpoll::isValid() const
{
    return epoll_fd > 0;
}

void FbcEpoll::setTimeout(int timeout)
{
    epoll_timeout = timeout;
}

inline void FbcEpoll::setMaxEvents(int maxevents)
{
    epoll_maxevents = maxevents;
}

inline const epoll_event *FbcEpoll::events() const
{
    return backEvents;
}

int FbcEpoll::create()
{
    epoll_fd = ::epoll_create(max);
    if (isValid()) {
        backEvents = new epoll_event[epoll_maxevents];
    }
    return epoll_fd;
}

int FbcEpoll::add(int fd, epoll_event *event)
{
    if (isValid()) {
        return ::epoll_ctl(epoll_fd, ADD, fd, event);
    }
    return -1;

}

int FbcEpoll::mod(int fd, epoll_event *event)
{
    if (isValid()) {
        return ::epoll_ctl(epoll_fd, MOD, fd, event);
    }
    return -1;

}

int FbcEpoll::del(int fd, epoll_event *event)
{
    if (isValid()) {
        return ::epoll_ctl(epoll_fd, DEL, fd, event);
    }
    return -1;
}

int FbcEpoll::wait()
{
    if (isValid()) {
        return ::epoll_wait(epoll_fd, backEvents, epoll_maxevents, epoll_timeout);
    }
    return -1;
}
