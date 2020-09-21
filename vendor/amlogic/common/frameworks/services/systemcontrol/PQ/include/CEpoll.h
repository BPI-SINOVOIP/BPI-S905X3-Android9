/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef CEPOLL_H
#define CEPOLL_H

#include <sys/epoll.h>
#include <unistd.h>
#include "CPQLog.h"

class CEpoll {
public:
    enum EPOLL_OP {ADD = EPOLL_CTL_ADD, MOD = EPOLL_CTL_MOD, DEL = EPOLL_CTL_DEL};
    CEpoll(int _max = 30, int maxevents = 20);
    ~CEpoll();
    int create();
    int add(int fd, epoll_event *event);
    int mod(int fd, epoll_event *event);
    int del(int fd, epoll_event *event);
    void setTimeout(int timeout);
    void setMaxEvents(int maxevents);
    int wait();
    const epoll_event *events() const;
    const epoll_event &operator[](int index)
    {
        return backEvents[index];
    }
private:
    int max;
    int epoll_fd;
    int epoll_timeout;
    int epoll_maxevents;
    epoll_event *backEvents;
};
#endif //CEPOLL_H

