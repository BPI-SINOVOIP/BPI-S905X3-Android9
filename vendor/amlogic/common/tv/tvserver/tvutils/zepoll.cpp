/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#include "include/zepoll.h"

Epoll::Epoll(int _max, int maxevents): max(_max),
    epoll_fd(-1),
    epoll_timeout(-1),
    epoll_maxevents(maxevents),
    backEvents(0)
{

}

Epoll::~Epoll()
{
    if (isValid()) {
        close(epoll_fd);
    }
    delete[] backEvents;
}


inline
bool Epoll::isValid() const
{
    return epoll_fd > 0;
}

void Epoll::setTimeout(int timeout)
{
    epoll_timeout = timeout;
}

inline
void Epoll::setMaxEvents(int maxevents)
{
    epoll_maxevents = maxevents;
}

inline
const epoll_event *Epoll::events() const
{
    return backEvents;
}



int Epoll::create()
{
    epoll_fd = ::epoll_create(max);
    if (isValid()) {
        backEvents = new epoll_event[epoll_maxevents];
    }
    return epoll_fd;
}

int Epoll::add(int fd, epoll_event *event)
{
    if (isValid()) {
        return ::epoll_ctl(epoll_fd, ADD, fd, event);
    }
    return -1;

}

int Epoll::mod(int fd, epoll_event *event)
{
    if (isValid()) {
        return ::epoll_ctl(epoll_fd, MOD, fd, event);
    }
    return -1;

}

int Epoll::del(int fd, epoll_event *event)
{
    if (isValid()) {
        return ::epoll_ctl(epoll_fd, DEL, fd, event);
    }
    return -1;
}

int Epoll::wait()
{
    if (isValid()) {
        return ::epoll_wait(epoll_fd, backEvents, epoll_maxevents, epoll_timeout);
    }
    return -1;
}
