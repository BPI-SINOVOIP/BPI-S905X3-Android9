/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "FBC"
#define LOG_FBC_TAG "CVirtualInput"

#include "CVirtualInput.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <CFbcLog.h>
#include <unistd.h>

CVirtualInput::CVirtualInput()
{
    setup_uinput_device();
}

CVirtualInput::~CVirtualInput()
{
    close(uinp_fd);
}

int CVirtualInput::setup_uinput_device()
{
    struct uinput_user_dev uinp; // uInput device structure
    // Open the input device
    uinp_fd = open(DEV_UINPUT, O_WRONLY | O_NDELAY);
    if (uinp_fd < 0) {
        LOGE("Unable to open /dev/uinput!!!\n");
        return -1;
    }

    memset(&uinp, 0, sizeof(uinp)); // Intialize the uInput device to NULL
    strncpy(uinp.name, "FBC key event", UINPUT_MAX_NAME_SIZE);
    uinp.id.version = 1;
    uinp.id.bustype = BUS_VIRTUAL;

    // Setup the uinput device
    ioctl(uinp_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinp_fd, UI_SET_EVBIT, EV_REL);
    for (int i = 0; i < 256; i++) {
        ioctl(uinp_fd, UI_SET_KEYBIT, i);
    }

    /* Create input device into input sub-system */
    write(uinp_fd, &uinp, sizeof(uinp));
    if (ioctl(uinp_fd, UI_DEV_CREATE)) {
        LOGE("Unable to create UINPUT device.\n");
        return -1;
    }
    return 1;
}

void CVirtualInput::sendVirtualkeyEvent(__u16 key_code)
{
    struct input_event event;
    if (uinp_fd < 0) {
        LOGE("uinput not open, sendVirtualkeyEvent failed!!!\n");
        return;
    }
    // Report BUTTON CLICK - PRESS event
    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, NULL);
    event.type = EV_KEY;
    event.code = key_code;
    event.value = 1;
    write(uinp_fd, &event, sizeof(event));

    event.type = EV_SYN;
    event.code = SYN_REPORT;
    event.value = 0;
    write(uinp_fd, &event, sizeof(event));

    // Report BUTTON CLICK - RELEASE event
    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, NULL);
    event.type = EV_KEY;
    event.code = key_code;
    event.value = 0;

    write(uinp_fd, &event, sizeof(event));
    event.type = EV_SYN;
    event.code = SYN_REPORT;
    event.value = 0;
    write(uinp_fd, &event, sizeof(event));
}

