/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __TV_KEY_DATA_H__
#define __TV_KEY_DATA_H__

#include <pthread.h>
#include <stdint.h>

#include <CTvLog.h>

#define SSM_CR_RGBOGO_LEN                           (256)
#define SSM_CR_RGBOGO_CHKSUM_LEN                    (2)
#define SSM_HDMI_EDID_SIZE                          (256)

#if ANDROID_PLATFORM_SDK_VERSION == 19
#define CS_KEY_DATA_NAME_DEV_PATH                   "/sys/class/aml_keys/aml_keys/key_name"
#define CS_KEY_DATA_READ_DEV_PATH                   "/sys/class/aml_keys/aml_keys/key_read"
#define CS_KEY_DATA_WRITE_DEV_PATH                  "/sys/class/aml_keys/aml_keys/key_write"

#define CS_MAC_KEY_NAME                             "mac"
#define CS_BARCODE_KEY_NAME                         "usid"
#define CS_RX_HDCP_KEY_NAME                         "rxhdcp20"
#define CS_PROJECT_ID_KEY_NAME                      "projid"
#endif

#if ANDROID_PLATFORM_SDK_VERSION >= 21
#define CS_KEY_DATA_ATTACH_DEV_PATH                 "/sys/class/unifykeys/attach"
#define CS_KEY_DATA_NAME_DEV_PATH                   "/sys/class/unifykeys/name"
#define CS_KEY_DATA_READ_DEV_PATH                   "/sys/class/unifykeys/read"
#define CS_KEY_DATA_WRITE_DEV_PATH                  "/sys/class/unifykeys/write"

#define CS_MAC_KEY_NAME                             "mac"
#define CS_BARCODE_KEY_NAME                         "usid"
#define CS_RX_HDCP_KEY_NAME                         "hdcp2_rx"
#define CS_RX_HDCP14_KEY_NAME                       "hdcp14_rx"
#define CS_PROJECT_ID_KEY_NAME                      "projid"
#endif

#define CC_MAX_KEY_DATA_SIZE                    (2048)
#define CC_MAX_FILE_PATH                        (256)

#define CC_MAC_LEN                              (6)
#define CC_HDCP_KEY_TOTAL_SIZE                  (368)
#define CC_HDCP_KEY_HEAD_SIZE                   (40)
#define CC_HDCP_KEY_CONTENT_SIZE                (CC_HDCP_KEY_TOTAL_SIZE - CC_HDCP_KEY_HEAD_SIZE)

#define CC_CUSTOMER_HDMI_EDID_TOTAL_SIZE        (SSM_HDMI_EDID_SIZE + 4)

#define CS_MAC_ADDRESS_STARTWRK_EN_CFG          "ssm.macaddr.startwork.en"
#define CS_BARCODE_LEN_CFG                      "ssm.barcode.len"

#define CS_HDCP_KEY_EN_CFG                      "ssm.handle.hdcpkey.en"
#define CS_HDCP_KEY_DEMO_EN_CFG                 "ssm.handle.hdcpkey.demo.en"
#define CS_HDCP_KEY_FILE_PATH_CFG               "ssm.handle.hdcpkey.file.path"
#define CS_HDCP_KEY_FILE_OFFSET_CFG             "ssm.handle.hdcpkey.file.offset"


//#define CS_HDMI_EDID_EN_CFG                     "ssm.handle.hdmi.edid.en"
#define CS_HDMI_EDID_USE_CFG                    "ssm.handle.hdmi.edid.use"
#define CS_HDMI_EDID_FILE_PATH_CFG              "ssm.handle.hdmi.edid.file.path"
#define CS_HDMI_PORT1_EDID_FILE_PATH_CFG        "ssm.handle.hdmi.port1.edid.file.path"
#define CS_HDMI_PORT2_EDID_FILE_PATH_CFG        "ssm.handle.hdmi.port2.edid.file.path"
#define CS_HDMI_PORT3_EDID_FILE_PATH_CFG        "ssm.handle.hdmi.port3.edid.file.path"
#define CS_HDMI_PORT4_EDID_FILE_PATH_CFG        "ssm.handle.hdmi.port4.edid.file.path"

#define CS_HDMI_EDID_FILE_OFFSET_CFG            "ssm.handle.hdmi.edid.file.offset"

#define CS_AUDIO_NOLINEPOINTS_FILE_PATH_CFG     "ssm.audio.nolinepoints.file.path"
#define CS_AUDIO_NOLINEPOINTS_FILE_OFFSET_CFG   "ssm.audio.nolinepoints.file.offset"
int ReadKeyData(const char *key_name, unsigned char data_buf[], int rd_size);
int WriteKeyData(const char *key_name, int wr_size, char data_buf[]);

int KeyData_SaveProjectID(int rw_val);
int KeyData_ReadProjectID();

int KeyData_GetMacAddressDataLen();
int KeyData_ReadMacAddress(unsigned char data_buf[]);
int KeyData_SaveMacAddress(unsigned char data_buf[]);

int KeyData_GetBarCodeDataLen();
int KeyData_ReadBarCode(unsigned char data_buf[]);
int KeyData_SaveBarCode(unsigned char data_buf[]);

int SSMReadHDCPKey(unsigned char hdcp_key_buf[]);
int SSMSaveHDCPKey(unsigned char hdcp_key_buf[]);
int SSMSetHDCPKey();
int SSMGetHDCPKeyDataLen();
int SSMRefreshHDCPKey();


int SSMReadHDMIEdid(int port, unsigned char hdmi_edid_buf[]);
int SSMSetHDMIEdid(int port);

int SSMReadAudioNoLinePoints(int offset, int size, unsigned char tmp_buf[]);
int SSMSaveAudioNoLinePoints(int offset, int size, unsigned char tmp_buf[]);
int CreateMacAddressStartWorkThread();

int GetSSMHandleHDCPKeyEnableCFG();
int GetSSMHandleHDCPKeyDemoEnableCFG();
int SSMSetDefaultHDCPKey(unsigned char hdcp_key_buf[]);
int RealHandleHDCPKey(unsigned char hdcp_key_buf[]);
int GetHDCPKeyFromFile(int rd_off, int rd_size, unsigned char data_buf[]);
int SaveHDCPKeyToFile(int wr_off, int wr_size, unsigned char data_buf[]);

int GetHDMIEdidFromFile(int rd_off, int rd_size, int port, unsigned char data_buf[]);
int RealHandleHDMIEdid(unsigned char hdmi_edid_buf[]);
int GetSSMHandleHDMIEdidByCustomerEnableCFG();
int AppendEdidPrefixCode(unsigned char customer_hdmi_edid_buf[], unsigned char hdmi_edid_buf[]);

int GetAudioNoLinePointsDataFromFile(int offset, int size, unsigned char data_buf[]);
int SaveAudioNoLinePointsDataToFile(int offset, int size, unsigned char data_buf[]);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif  //__TV_KEY_DATA_H__
