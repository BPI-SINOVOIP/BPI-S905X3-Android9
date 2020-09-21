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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2016/09/06
 *  @par function description:
 *  - 1 process HDCP RX key combie and refresh
 */

#ifndef _HDCP_RX_KEY_H
#define _HDCP_RX_KEY_H

#include <assert.h>
#include "common.h"
#include "../SysWrite.h"

#define HDMI_IOC_MAGIC 'H'
#define HDMI_IOC_HDCP14_KEY_MODE    _IOR(HDMI_IOC_MAGIC, 0x0d, enum hdcp14_key_mode_e)
#define HDMI_IOC_HDCP22_NOT_SUPPORT _IO(HDMI_IOC_MAGIC, 0x0e)
#define HDMI_TX_REPEATER_PATH               "/sys/class/amhdmitx/amhdmitx0/hdmi_repeater_tx"
#define HDCP_RPTX22_SRC_FW_PATH             "/vendor/etc/firmware/hdcp_rp22/firmware_rptx.le"
#define HDCP_RPTX22_DES_FW_PATH             "/mnt/vendor/param/firmware_rptx.le"

enum {
    HDCP_RX_14_KEY                  = 0,
    HDCP_RX_22_KEY                  = 1
};

enum hdcp14_key_mode_e {
    HDCP14_KEY_MODE_NORMAL          = 0,
    HDCP14_KEY_MODE_SECURE
};

class HDCPRxKey {
public:
    HDCPRxKey(int keyType);

    ~HDCPRxKey();

    void init();
    /**
     * refresh the 1.4 key or 2.2 key firmware
     */
    bool refresh();
    int getHdcpRX14key(char *value, int size);
    int setHdcpRX14key(const char *value, const int size);
    int setHdcpRX14keyMode(hdcp14_key_mode_e mode);
    int getHdcpRX22key(char *value, int size);
    int setHdcpRX22key(const char *value, const int size);
    int copyHdcpFwToParam(const char* firmwarele, const char* newFw);

private:
    int mKeyType;

    bool setHDCP14Key();
    bool setHDCP22Key();

    bool aicTool(const char* pRpvalue);
    bool esmSwap(const char* pRpvalue);
    bool genKeyImg(const char* pRpvalue);
    bool combineFirmwarewithArmTool(const char* pSourcePath, const char* pDestPath, const char* pTempPath);
    bool combineFirmwarewithPCTool(const char* pKeyName, const char* pCrcName, const char* pSourcePath, const char* pDestPath);
    void getFirmwareSourcePath(const char* pRpvalue, char* pPath);
    void getFirmwareDestPath(const char* pRpvalue, char* pPath);
    void getRepeaterValue(char* pRvalue);
    int setHdcpRX22SupportStatus();
    SysWrite sysWrite;
};

#endif // _HDCP_RX_KEY_H
