/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef TV_SETTING_RAM_H
#define TV_SETTING_RAM_H

#include "CTvSettingBaseDevice.h"

class CTvSettingDeviceRam: public CTvSettingBaseDevice {

public:
    CTvSettingDeviceRam();
    virtual ~CTvSettingDeviceRam();

    virtual int InitCheck();
    virtual int OpenDevice();
    virtual int CloseDevice(int *device_fd);
    virtual int GetDeviceTotalSize();

    virtual int CheckDeviceWrAvaliable(int offset, int len);
    virtual int WriteSpecialBytes(int offset, int len, unsigned char data_buf[]);
    virtual int ReadSpecialBytes(int offset, int len, unsigned char data_buf[]);

private:
    int ValidOperateCheck();

private:
    int RAM_DEV_TOTAL_SIZE;
    int RAM_DEV_RW_START_OFFSET;
    int RAM_DEV_RW_END_OFFSET;
    int RAM_DEV_W_PAGE_SIZE;
    int RAM_DEV_R_PAGE_SIZE;
    int RAM_DEV_SLAVE_ADDR;
    int RAM_DEV_RW_TEST_OFFSET;
    int device_use_buffer;
    unsigned char *device_buf;
    char gFilePathBuf[CC_MAX_FILE_PATH];
};

#endif // ANDROID_SSM_RAM_H
