/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _VIRTUAL_INPUT_H
#define _VIRTUAL_INPUT_H
#include <linux/input.h>
#include <linux/uinput.h>

#define DEV_UINPUT         "/dev/uinput"

class CVirtualInput {
public:
    CVirtualInput();
    ~CVirtualInput();
    void sendVirtualkeyEvent(__u16 key_code);

private:
    bool  threadLoop();
    int setup_uinput_device();
    int uinp_fd;
};
#endif
