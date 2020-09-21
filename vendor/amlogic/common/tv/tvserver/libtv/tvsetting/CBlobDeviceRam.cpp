/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvSettingDeviceRam"

#include <stdio.h>
#include <string.h>

#include <android/log.h>

#include "CTvSettingDeviceRam.h"
#include "CTvLog.h"

CTvSettingDeviceRam::CTvSettingDeviceRam()
{
    RAM_DEV_TOTAL_SIZE = 4 * 1024;
    RAM_DEV_RW_START_OFFSET = 0;
    RAM_DEV_RW_END_OFFSET = RAM_DEV_TOTAL_SIZE - 1;
    RAM_DEV_W_PAGE_SIZE = 32;
    RAM_DEV_R_PAGE_SIZE = 32;
    RAM_DEV_SLAVE_ADDR = (0xA0 >> 1);
    RAM_DEV_RW_TEST_OFFSET = -1;
    device_use_buffer = 0;
    device_buf = NULL;
    gFilePathBuf[0] = '\0';
}

CTvSettingDeviceRam::~CTvSettingDeviceRam()
{
}

int CTvSettingDeviceRam::GetDeviceTotalSize()
{
    return 0;
}
int CTvSettingDeviceRam::InitCheck()
{
    int tmp_dev_total_size = 0;

    if (device_buf == NULL) {
        tmp_dev_total_size = GetDeviceTotalSize();
        if (tmp_dev_total_size <= 0) {
            LOGE("%s, Device file size must be more than 0.\n", "TV");
            return -1;
        }

        device_buf = new unsigned char[tmp_dev_total_size];
        if (device_buf == NULL) {
            return -1;
        }

        memset((void *) device_buf, CC_INIT_BYTE_VAL, tmp_dev_total_size);
    }

    return 0;
}

int CTvSettingDeviceRam::OpenDevice()
{
    return 0;
}

int CTvSettingDeviceRam::CloseDevice(int *device_fd)
{


    return 0;
}

int CTvSettingDeviceRam::CheckDeviceWrAvaliable(int offset, int len)
{
    /*int tmp_dev_start_offset = 0;
    int tmp_dev_end_offset = 0;

    GetDeviceRWStartOffset(&tmp_dev_start_offset);
    if (tmp_dev_start_offset < 0) {
        LOGE("%s, Device file r/w start offset must be greater than or euqal to 0.\n", "TV");
        return -1;
    }

    GetDeviceRWEndOffset(&tmp_dev_end_offset);
    if (tmp_dev_end_offset < 0) {
        LOGE("%s, Device file r/w end offset must be more than 0.\n", "TV");
        return -1;
    }

    if (len <= 0) {
        LOGE("%s, The w/r length should be more than 0.\n", "TV");
        return -1;
    }

    if ((len + tmp_dev_start_offset + offset) > tmp_dev_end_offset + 1) {
        LOGE("%s, w/r would be overflow!!! len = %d, start offset = %d, offset = %d, end offset = %d.\n", "TV", len, tmp_dev_start_offset, offset, tmp_dev_end_offset);
        return -1;
    }

    if (ValidOperateCheck() < 0) {
        return -1;
    }*/

    return 0;
}

int CTvSettingDeviceRam::WriteSpecialBytes(int offset, int len, unsigned char data_buf[])
{
    int tmp_dev_w_page_size = 0;

    //GetDeviceWritePageSize(&tmp_dev_w_page_size);
    //if (len > tmp_dev_w_page_size) {
    //   LOGE("%s, The write length should be less than page size(%d).\n", "TV", tmp_dev_w_page_size);
    //   return -1;
    // }

    if (CheckDeviceWrAvaliable(offset, len) < 0) {
        return -1;
    }

    memcpy((void *) (device_buf + offset), (void *) data_buf, len);

    return 0;
}

int CTvSettingDeviceRam::ReadSpecialBytes(int offset, int len, unsigned char data_buf[])
{
    if (CheckDeviceWrAvaliable(offset, len) < 0) {
        return -1;
    }

    memcpy((void *) data_buf, (void *) (device_buf + offset), len);

    return 0;
}
