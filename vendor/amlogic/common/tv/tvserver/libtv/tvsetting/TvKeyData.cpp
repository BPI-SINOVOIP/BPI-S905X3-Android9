/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "TvKeyData"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cutils/properties.h>

#include <netinet/ether.h>
#include <netinet/if_ether.h>

#include <netutils/ifc.h>
//#include <netutils/dhcp.h>

#include <tvconfig.h>
#include "../tvin/CTvin.h"

#include "TvKeyData.h"


static unsigned char mHDCPKeyDefHeaderBuf[CC_HDCP_KEY_HEAD_SIZE] = {
    //40 bytes
    0x53, 0x4B, 0x59, 0x01, 0x00, 0x10, 0x0D, 0x15, 0x3A, 0x8E, // 000~009
    0x99, 0xEE, 0x2A, 0x55, 0x58, 0xEE, 0xED, 0x4B, 0xBE, 0x00, // 010~019
    0x74, 0xA9, 0x00, 0x10, 0x0A, 0x21, 0xE3, 0x30, 0x66, 0x34, // 020~029
    0xCE, 0x9C, 0xC7, 0x8B, 0x51, 0x27, 0xF9, 0x0B, 0xAD, 0x09, // 030~039
};

static unsigned char mDefHDCPKeyContentBuf[CC_HDCP_KEY_CONTENT_SIZE] = {
    //328 bytes
    0x5F, 0x4D, 0xC2, 0xCA, 0xA2, 0x13, 0x06, 0x18, 0x8D, 0x34, // 000~009
    0x82, 0x46, 0x2D, 0xC9, 0x4B, 0xB0, 0x1C, 0xDE, 0x3D, 0x49, // 010~019
    0x39, 0x58, 0xEF, 0x2B, 0x68, 0x39, 0x71, 0xC9, 0x4D, 0x25, // 020~029
    0xE9, 0x75, 0x4D, 0xAC, 0x62, 0xF5, 0xF5, 0x87, 0xA0, 0xB2, // 030~039
    0x4A, 0x60, 0xD3, 0xF1, 0x09, 0x3A, 0xB2, 0x3E, 0x19, 0x4F, // 040~049
    0x3B, 0x1B, 0x2F, 0x85, 0x14, 0x28, 0x44, 0xFC, 0x69, 0x6F, // 050~059
    0x50, 0x42, 0x81, 0xBF, 0x7C, 0x2B, 0x3A, 0x17, 0x2C, 0x15, // 060~069
    0xE4, 0x93, 0x77, 0x74, 0xE8, 0x1F, 0x1C, 0x38, 0x54, 0x49, // 070~079
    0x10, 0x64, 0x5B, 0x7D, 0x90, 0x3D, 0xA0, 0xE1, 0x8B, 0x67, // 080~089
    0x5C, 0x19, 0xE6, 0xCA, 0x9D, 0xE9, 0x68, 0x5A, 0xB5, 0x62, // 090~099
    0xDF, 0xA1, 0x28, 0xBC, 0x68, 0x82, 0x9A, 0x22, 0xC4, 0xDC, // 100~109
    0x48, 0x85, 0x0F, 0xF1, 0x3E, 0x05, 0xDD, 0x1B, 0x2D, 0xF5, // 120~119
    0x49, 0x3A, 0x15, 0x29, 0xE7, 0xB6, 0x0B, 0x2A, 0x40, 0xE3, // 120~129
    0xB0, 0x89, 0xD5, 0x75, 0x84, 0x2E, 0x76, 0xE7, 0xBC, 0x63, // 130~139
    0x67, 0xE3, 0x57, 0x67, 0x86, 0x81, 0xF4, 0xD7, 0xEA, 0x4D, // 140~149
    0x89, 0x8E, 0x37, 0x95, 0x59, 0x1C, 0x8A, 0xCD, 0x79, 0xF8, // 150~159
    0x4F, 0x82, 0xF2, 0x6C, 0x7E, 0x7F, 0x79, 0x8A, 0x6B, 0x90, // 160~169
    0xC0, 0xAF, 0x4C, 0x8D, 0x43, 0x47, 0x1F, 0x9A, 0xF1, 0xBB, // 170~179
    0x88, 0x64, 0x49, 0x14, 0x50, 0xD1, 0xC3, 0xDF, 0xA6, 0x87, // 180~189
    0xA0, 0x15, 0x98, 0x51, 0x81, 0xF5, 0x97, 0x55, 0x10, 0x4A, // 190~199
    0x99, 0x30, 0x54, 0xA4, 0xFC, 0xDA, 0x0E, 0xAC, 0x6A, 0xFA, // 200~209
    0x90, 0xEE, 0x12, 0x70, 0x69, 0x74, 0x63, 0x46, 0x63, 0xFB, // 210~219
    0xE6, 0x1F, 0x72, 0xEC, 0x43, 0x5D, 0x50, 0xFF, 0x03, 0x4F, // 220~229
    0x05, 0x33, 0x88, 0x36, 0x93, 0xE4, 0x72, 0xD5, 0xCC, 0x34, // 230~239
    0x52, 0x96, 0x15, 0xCE, 0xD0, 0x32, 0x52, 0x41, 0x4F, 0xBC, // 240~249
    0x2D, 0xDF, 0xC5, 0xD6, 0x7F, 0xD5, 0x74, 0xCE, 0x51, 0xDC, // 250~259
    0x10, 0x5E, 0xF7, 0xAA, 0x4A, 0x2D, 0x20, 0x9A, 0x17, 0xDD, // 260~269
    0x30, 0x89, 0x71, 0x82, 0x36, 0x50, 0x09, 0x1F, 0x7C, 0xF3, // 270~279
    0x12, 0xE9, 0x43, 0x10, 0x5F, 0x51, 0xBF, 0xB8, 0x45, 0xA8, // 280~289
    0x5A, 0x8D, 0x3F, 0x77, 0xE5, 0x96, 0x73, 0x68, 0xAB, 0x73, // 290~299
    0xE5, 0x4C, 0xFB, 0xE5, 0x98, 0xB9, 0xAE, 0x74, 0xEB, 0x51, // 300~309
    0xDB, 0x91, 0x07, 0x7B, 0x66, 0x02, 0x9B, 0x79, 0x03, 0xC5, // 310~319
    0x34, 0x1C, 0x58, 0x13, 0x31, 0xD2, 0x4A, 0xEC, // 320~327
};


static int TransStringToHex(int data_cnt, char data_buf[],
                            unsigned char hex_buf[])
{
    int i = 0, j = 0, tmp_val = 0;
    char tmp_buf[3] = { 0, 0, 0 };

    while (i < data_cnt) {
        tmp_val = 0;
        tmp_buf[0] = data_buf[i];
        tmp_buf[1] = data_buf[i + 1];
        tmp_val = strtoul(tmp_buf, NULL, 16);
        hex_buf[j] = tmp_val;
        //LOGD("%s, hex_buf[%d] = 0x%x\n", __FUNCTION__, j, hex_buf[j]);
        i += 2;
        j += 1;
    }

    return j;
}

static int TransToHexString(int hex_cnt, char data_buf[],
                            unsigned char hex_buf[])
{
    int i = 0;
    char tmp_buf[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    data_buf[0] = 0;
    for (i = 0; i < hex_cnt; i++) {
        sprintf(tmp_buf, "%02X", (unsigned char) hex_buf[i]);
        strcat(data_buf, tmp_buf);
    }

    return 2 * hex_cnt;
}

static int write_partiton_raw(const char *partition, const void *data, const int size)
{
    int fd = 0;

    fd = open(partition, O_WRONLY);
    if (fd < 0) {
        LOGE("%s, open %s failed!\n", __FUNCTION__, partition);
        return -1;
    }

    if (write(fd, data, size) != size) {
        LOGE("%s, write %s size:%d failed!\n", __FUNCTION__, partition, size);
        close(fd);
        return -1;
    }

    //debugP("write partition:%s, data:%s\n", partition, (char *)data);

    close(fd);
    return 0;
}

static int read_partiton_raw(const char *partition, char *data, const int size)
{
    int fd = 0;
    int len = 0;

    fd = open(partition, O_RDONLY);
    if (fd < 0) {
        LOGE("%s, open %s failed!\n", __FUNCTION__, partition);
        return -1;
    }

    len = read(fd, data, size);

    //LOGD("%s, read partition:%s, data:%s\n", __FUNCTION__, partition, (char *)data);

    close(fd);
    return len;
}

int ReadKeyData(const char *key_name, unsigned char data_buf[], int rd_size)
{
    int ret = 0;

    if (write_partiton_raw(CS_KEY_DATA_ATTACH_DEV_PATH, "1", strlen("1"))) {
        LOGE("%s, attach failed!\n", __FUNCTION__);
        return -__LINE__;
    }

    if (write_partiton_raw(CS_KEY_DATA_NAME_DEV_PATH, key_name, strlen(key_name))) {
        LOGE("%s, name failed!\n", __FUNCTION__);
        return -__LINE__;
    }

    ret = read_partiton_raw(CS_KEY_DATA_READ_DEV_PATH, (char*)data_buf, rd_size);
    if (ret != rd_size) {
        LOGE("%s, Fail in read (%s) in len %d\n", __FUNCTION__, key_name, CC_HDCP_KEY_CONTENT_SIZE);
        return -__LINE__;
    } else {
        return ret;
    }
}

int WriteKeyData(const char *key_name, int wr_size, char data_buf[])
{
    FILE *dev_fp = NULL;
    int wr_cnt = 0;

    dev_fp = fopen(CS_KEY_DATA_NAME_DEV_PATH, "w");
    if (dev_fp == NULL) {
        LOGE("%s, open %s ERROR(%s)!!\n", __FUNCTION__,
             CS_KEY_DATA_NAME_DEV_PATH, strerror(errno));
        return -1;
    }

    fprintf(dev_fp, "%s", key_name);

    fclose(dev_fp);
    dev_fp = NULL;

    dev_fp = fopen(CS_KEY_DATA_WRITE_DEV_PATH, "w");
    if (dev_fp == NULL) {
        LOGE("%s, open %s ERROR(%s)!!\n", __FUNCTION__,
             CS_KEY_DATA_WRITE_DEV_PATH, strerror(errno));
        return -1;
    }

    wr_cnt = fwrite(data_buf, 1, wr_size, dev_fp);

    fclose(dev_fp);
    dev_fp = NULL;

    return wr_cnt;
}

int KeyData_GetMacAddressDataLen()
{
    return CC_MAC_LEN;
}

int KeyData_ReadMacAddress(unsigned char data_buf[])
{
    int i = 0, rd_size = 0;
    int data_i_buf[CC_MAC_LEN] = { 0, 0, 0, 0, 0, 0 };
    unsigned char rd_buf[128] = { 0 };

    memset((void *)rd_buf, 0 , 128);
    rd_size = ReadKeyData(CS_MAC_KEY_NAME, rd_buf, 17);
    LOGD("%s, rd_size = %d\n", __FUNCTION__, rd_size);
    if (rd_size == 17) {
        char *tmp_buf = NULL;
        tmp_buf = (char *)malloc(sizeof(char) * (sizeof(rd_buf)+1));
#if ANDROID_PLATFORM_SDK_VERSION == 19
    memcpy((void *)tmp_buf, (const void *)rd_buf, 128);
    rd_size = TransStringToHex(rd_size, (char *)rd_buf, tmp_buf);
#endif

#if ANDROID_PLATFORM_SDK_VERSION >= 21
    memcpy((void *)tmp_buf, (const void *)rd_buf, 128);
#endif
        sscanf(tmp_buf, "%02x:%02x:%02x:%02x:%02x:%02x",
               &data_i_buf[0], &data_i_buf[1], &data_i_buf[2], &data_i_buf[3],
               &data_i_buf[4], &data_i_buf[5]);
        for (i = 0; i < CC_MAC_LEN; i++) {
            data_buf[i] = data_i_buf[i] & 0xFF;
            //LOGD("data_buf[%d] = 0x%x\n", i, data_buf[i]);
        }
        free(tmp_buf);
        return KeyData_GetMacAddressDataLen();
    }else {
        return 0;
    }
}

int KeyData_SaveMacAddress(unsigned char data_buf[])
{
    int tmp_ret = 0, wr_size = 0;
    unsigned char hex_buf[128] = { 0 };
    char tmp_buf[128] = { 0 };

    sprintf((char *) hex_buf, "%02x:%02x:%02x:%02x:%02x:%02x", data_buf[0],
            data_buf[1], data_buf[2], data_buf[3], data_buf[4], data_buf[5]);

#if ANDROID_PLATFORM_SDK_VERSION == 19
    memset((void *)tmp_buf, 0, 128);
    TransToHexString(strlen((char *) hex_buf), tmp_buf, hex_buf);
#endif

#if ANDROID_PLATFORM_SDK_VERSION >= 21
    memset((void *)tmp_buf, 0, 128);
    memcpy(tmp_buf, (const char *)hex_buf, strlen((char *) hex_buf));
#endif

    wr_size = strlen(tmp_buf);
    tmp_ret = WriteKeyData(CS_MAC_KEY_NAME, wr_size, tmp_buf);
    if (tmp_ret != wr_size) {
        return -1;
    }

    CreateMacAddressStartWorkThread();

    return 0;
}

static int gSSMBarCodeLen = -1;
int KeyData_GetBarCodeDataLen()
{
    const char *config_value;

    if (gSSMBarCodeLen <= 0) {
        config_value = config_get_str(CFG_SECTION_TV, CS_BARCODE_LEN_CFG, "null");
        if (strcmp(config_value, "null") == 0) {
            gSSMBarCodeLen = 32;
        } else {
            gSSMBarCodeLen = strtol(config_value, NULL, 10);
        }
    }

    return gSSMBarCodeLen;
}

int KeyData_ReadBarCode(unsigned char data_buf[])
{
    int rd_size = 0, tmp_len = 0;
    unsigned char rd_buf[CC_MAX_KEY_DATA_SIZE] = { 0 };

    tmp_len = KeyData_GetBarCodeDataLen();
    if (tmp_len > 0) {
        rd_size = ReadKeyData(CS_BARCODE_KEY_NAME, rd_buf, tmp_len);
        LOGD("%s, rd_size = %d\n", __FUNCTION__, rd_size);

#if ANDROID_PLATFORM_SDK_VERSION == 19
        unsigned char tmp_buf[CC_MAX_KEY_DATA_SIZE] = { 0 };

        memcpy((void *)tmp_buf, (void *)rd_buf, CC_MAX_KEY_DATA_SIZE);
        rd_size = TransStringToHex(rd_size, (char *)rd_buf, tmp_buf);

        if (rd_size == tmp_len) {
            memcpy(data_buf, tmp_buf, rd_size);
            return rd_size;
        }
#endif

#if ANDROID_PLATFORM_SDK_VERSION >= 21
        if (rd_size == tmp_len) {
            memcpy(data_buf, rd_buf, rd_size);
            return rd_size;
        }
#endif
    } else {
        LOGD("%s: KeyData_GetBarCodeDataLen failed!\n", __FUNCTION__);
    }
    return 0;
}

int KeyData_SaveBarCode(unsigned char data_buf[])
{
    int tmp_len = 0, wr_size = 0;
    char tmp_buf[CC_MAX_KEY_DATA_SIZE] = { 0 };

    tmp_len = KeyData_GetBarCodeDataLen();

#if ANDROID_PLATFORM_SDK_VERSION == 19
    memset((void *)tmp_buf, 0, CC_MAX_KEY_DATA_SIZE);
    TransToHexString(tmp_len, tmp_buf, data_buf);
#endif

#if ANDROID_PLATFORM_SDK_VERSION >= 21
    memset((void *)tmp_buf, 0, CC_MAX_KEY_DATA_SIZE);
    memcpy(tmp_buf, (const char *)data_buf, strlen((char *) data_buf));
#endif

    wr_size = strlen(tmp_buf);
    tmp_len = WriteKeyData(CS_BARCODE_KEY_NAME, wr_size, tmp_buf);
    if (tmp_len != wr_size) {
        return -1;
    }

    return 0;
}

int SSMReadHDCPKey(unsigned char hdcp_key_buf[])
{
    int tmp_ret = 0, rd_size = 0;
    unsigned char rd_buf[CC_MAX_KEY_DATA_SIZE] = { 0 };
    //unsigned char tmp_buf[CC_MAX_KEY_DATA_SIZE] = { 0 };

    tmp_ret = GetHDCPKeyFromFile(0, CC_HDCP_KEY_TOTAL_SIZE, hdcp_key_buf);
    if (tmp_ret < 0) {
        rd_size = ReadKeyData(CS_RX_HDCP14_KEY_NAME, rd_buf, CC_HDCP_KEY_CONTENT_SIZE);
        LOGD("%s, rd_size = %d\n", __FUNCTION__, rd_size);

        //memcpy((void *)tmp_buf, (void *)rd_buf, CC_MAX_KEY_DATA_SIZE);
        //rd_size = TransStringToHex(rd_size, (char *)rd_buf, tmp_buf);

		//LOGD("%s, after TransStringToHex rd_size = %d\n", __FUNCTION__, rd_size);
		LOGD("%s, rd_buf = %x, %x, %x\n", __FUNCTION__, rd_buf[0], rd_buf[1], rd_buf[3]);
		if (rd_size == CC_HDCP_KEY_CONTENT_SIZE) {
			memcpy(hdcp_key_buf, mHDCPKeyDefHeaderBuf, CC_HDCP_KEY_HEAD_SIZE);
			memcpy(hdcp_key_buf + CC_HDCP_KEY_HEAD_SIZE, rd_buf, CC_HDCP_KEY_CONTENT_SIZE);
			return CC_HDCP_KEY_TOTAL_SIZE;
		}
        return 0;
    }
    return CC_HDCP_KEY_TOTAL_SIZE;
}



int SSMSaveHDCPKey(unsigned char hdcp_key_buf[])
{
    int tmp_ret = 0, wr_size = 0;
    char tmp_buf[CC_MAX_KEY_DATA_SIZE] = { 0 };

    tmp_ret = SaveHDCPKeyToFile(0, CC_HDCP_KEY_TOTAL_SIZE, hdcp_key_buf);
    if (tmp_ret < 0) {
        memset((void *)tmp_buf, 0, CC_MAX_KEY_DATA_SIZE);
        TransToHexString(CC_HDCP_KEY_TOTAL_SIZE, tmp_buf, hdcp_key_buf);

        wr_size = strlen(tmp_buf);
        tmp_ret = WriteKeyData(CS_RX_HDCP_KEY_NAME, wr_size, tmp_buf);
        if (tmp_ret != wr_size) {
            tmp_ret = -1;
        } else {
            tmp_ret = 0;
        }
    }

    return tmp_ret;
}

int SSMSetHDCPKey()
{
    unsigned char hdcp_key_buf[CC_HDCP_KEY_TOTAL_SIZE];

    if (GetSSMHandleHDCPKeyEnableCFG() == 1) {
        if (GetSSMHandleHDCPKeyDemoEnableCFG() == 1) {
            return SSMSetDefaultHDCPKey(hdcp_key_buf);
        } else {
            if (SSMReadHDCPKey(hdcp_key_buf) == CC_HDCP_KEY_TOTAL_SIZE) {
                LOGD("%s, using ssm's hdcp key.\n", __FUNCTION__);
                return RealHandleHDCPKey(hdcp_key_buf);
            }
        }
    }

    return -1;
}

int SSMRefreshHDCPKey()
{
    int ret = -1;
    ret = SSMSetHDCPKey();
    system ( "/vendor/bin/dec" );
    return ret;
}

int SSMGetHDCPKeyDataLen()
{
    return CC_HDCP_KEY_TOTAL_SIZE;
}

//hdmi edid
int SSMSetHDMIEdid(int port)
{
    unsigned char customer_hdmi_edid_buf[CC_CUSTOMER_HDMI_EDID_TOTAL_SIZE];
    unsigned char hdmi_edid_buf[SSM_HDMI_EDID_SIZE];

    if (port < HDMI_PORT_1 || port > HDMI_PORT_MAX) {
        LOGD("%s, hdmi port error.%d\n", __FUNCTION__, port);
        return -1;
    }

    if (GetSSMHandleHDMIEdidByCustomerEnableCFG() == 1) {
        if (SSMReadHDMIEdid(port, hdmi_edid_buf) == 0) {
            LOGD("%s, using ssm's hdmi edid.\n", __FUNCTION__);
            LOGD("%s, begin to write hdmi edid:0x%x, 0x%x, 0x%x, 0x%x.\n",
                 __FUNCTION__, hdmi_edid_buf[8], hdmi_edid_buf[9],
                 hdmi_edid_buf[10], hdmi_edid_buf[255]);
            if ( AppendEdidPrefixCode(customer_hdmi_edid_buf, hdmi_edid_buf) == 0 )
                return RealHandleHDMIEdid(customer_hdmi_edid_buf);
        }
    }

    return -1;
}

int SSMReadHDMIEdid(int port, unsigned char hdmi_edid_buf[])
{
    int tmp_ret = 0;
    LOGD("%s, read hdmi edid from bin file.\n", __FUNCTION__);
    tmp_ret = GetHDMIEdidFromFile(0, SSM_HDMI_EDID_SIZE, port, hdmi_edid_buf);
    if (tmp_ret < 0) {
        LOGD("%s, read hdmi edid error.\n", __FUNCTION__);
    } else {
        LOGD("%s, 0x%x, 0x%x, 0x%x, 0x%x.\n", __FUNCTION__, hdmi_edid_buf[8],
             hdmi_edid_buf[9], hdmi_edid_buf[10], hdmi_edid_buf[255]);
    }
    return tmp_ret;
}

int KeyData_SaveProjectID(int rw_val)
{
    int tmp_ret = 0, wr_size = 0;
    char tmp_buf[64] = { 0 };

    sprintf(tmp_buf, "%08X", rw_val);

    wr_size = strlen(tmp_buf);
    tmp_ret = WriteKeyData(CS_PROJECT_ID_KEY_NAME, wr_size, tmp_buf);
    if (tmp_ret != wr_size) {
        return -1;
    }

    return 0;
}

int KeyData_ReadProjectID()
{
    int rd_size = 0, tmp_val = 0;
    unsigned char tmp_buf[64] = { 0 };

    rd_size = ReadKeyData(CS_PROJECT_ID_KEY_NAME, tmp_buf, 4);
    LOGD("%s, rd_size = %d\n", __FUNCTION__, rd_size);
    if (rd_size == 4) {
        tmp_val = 0;
        tmp_val |= tmp_buf[0] << 24;
        tmp_val |= tmp_buf[1] << 16;
        tmp_val |= tmp_buf[2] << 8;
        tmp_val |= tmp_buf[3] << 0;
    }

    return tmp_val;
}

int SSMSaveAudioNoLinePoints(int offset, int size, unsigned char tmp_buf[])
{
    return SaveAudioNoLinePointsDataToFile(offset, size, tmp_buf);
}

int SSMReadAudioNoLinePoints(int offset, int size, unsigned char tmp_buf[])
{
    return GetAudioNoLinePointsDataFromFile(offset, size, tmp_buf);
}

/**************************** start mac address static functions ****************************/
#define CC_ERR_THREAD_ID    (0)

static pthread_t mMacAddressStartWorkThreadID = CC_ERR_THREAD_ID;

static volatile unsigned int mMacAddressLow = -1;
static volatile unsigned int mMacAddressHigh = -1;
static volatile int mMacAddressStartWorkThreadExecFlag = -1;
static volatile int mMacAddressStartWorkThreadTurnOnFlag = -1;

static pthread_mutex_t mac_address_low_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mac_address_high_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mac_address_exec_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mac_address_turnon_mutex = PTHREAD_MUTEX_INITIALIZER;

static int GetSSMMacAddressStartWorkEnableCFG()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, CS_MAC_ADDRESS_STARTWRK_EN_CFG, "null");
    if (strcmp(config_value, "null") == 0) {
        LOGD(
            "%s, get config is \"%s\", return 0 to not enable mac address start work.\n",
            __FUNCTION__, config_value);
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

static unsigned int SetMacAddressLow(unsigned int low_val)
{
    unsigned int tmp_val;

    pthread_mutex_lock(&mac_address_low_mutex);

    tmp_val = mMacAddressLow;

    mMacAddressLow = low_val;

    pthread_mutex_unlock(&mac_address_low_mutex);

    return tmp_val;
}

static unsigned int GetMacAddressLow()
{
    unsigned int tmp_val = 0;

    pthread_mutex_lock(&mac_address_low_mutex);

    tmp_val = mMacAddressLow;

    pthread_mutex_unlock(&mac_address_low_mutex);

    return tmp_val;
}

static unsigned int SetMacAddressHigh(unsigned int high_val)
{
    unsigned int tmp_val;

    pthread_mutex_lock(&mac_address_high_mutex);

    tmp_val = mMacAddressHigh;

    mMacAddressHigh = high_val;

    pthread_mutex_unlock(&mac_address_high_mutex);

    return tmp_val;
}

static unsigned int GetMacAddressHigh()
{
    int tmp_val = 0;

    pthread_mutex_lock(&mac_address_high_mutex);

    tmp_val = mMacAddressHigh;

    pthread_mutex_unlock(&mac_address_high_mutex);

    return tmp_val;
}

static int SetMacAddressStartWorkThreadExecFlag(int tmp_flag)
{
    int tmp_val;

    pthread_mutex_lock(&mac_address_exec_mutex);

    tmp_val = mMacAddressStartWorkThreadExecFlag;

    mMacAddressStartWorkThreadExecFlag = tmp_flag;

    pthread_mutex_unlock(&mac_address_exec_mutex);

    return tmp_val;
}

static int GetMacAddressStartWorkThreadExecFlag()
{
    int tmp_val = 0;

    pthread_mutex_lock(&mac_address_exec_mutex);

    tmp_val = mMacAddressStartWorkThreadExecFlag;

    pthread_mutex_unlock(&mac_address_exec_mutex);

    return tmp_val;
}

static int SetMacAddressStartWorkThreadTurnOnFlag(int tmp_flag)
{
    int tmp_val;

    pthread_mutex_lock(&mac_address_turnon_mutex);

    tmp_val = mMacAddressStartWorkThreadTurnOnFlag;

    mMacAddressStartWorkThreadTurnOnFlag = tmp_flag;

    pthread_mutex_unlock(&mac_address_turnon_mutex);

    return tmp_val;
}

static int GetMacAddressStartWorkThreadTurnOnFlag()
{
    int tmp_val = 0;

    pthread_mutex_lock(&mac_address_turnon_mutex);

    tmp_val = mMacAddressStartWorkThreadTurnOnFlag;

    pthread_mutex_unlock(&mac_address_turnon_mutex);

    return tmp_val;
}

static void *SSMMacAddressStartWorkMainApp(void *data __unused)
{
    unsigned int curMacAddrLow = 0, curMacAddrHigh = 0;
    int p_status;
    char ssm_addr_str[128];
    const char *iname = "eth0";
    pid_t pid;

    LOGD("%s, entering...\n", __FUNCTION__);

    if (GetSSMMacAddressStartWorkEnableCFG() == 0) {
        LOGE("%s, ssm mac address start work is not enable.\n", CFG_SECTION_TV);
        return NULL;
    }

    curMacAddrLow = GetMacAddressLow();
    curMacAddrHigh = GetMacAddressHigh();

    while (GetMacAddressStartWorkThreadTurnOnFlag() == 1) {
        pid = fork();
        if (pid == 0) {
            if (execl("/vendor/bin/stop", "stop_eth_dhcpcd", "eth_dhcpcd", NULL)
                    < 0) {
                _exit(-1);
            }
            _exit(0);
        }
        waitpid(pid, &p_status, 0);

        ifc_init();

        ifc_down(iname);

        sprintf(ssm_addr_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                ((curMacAddrLow >> 0) & 0xFF), ((curMacAddrLow >> 8) & 0xFF),
                ((curMacAddrLow >> 16) & 0xFF), ((curMacAddrLow >> 24) & 0xFF),
                ((curMacAddrHigh >> 0) & 0xFF), ((curMacAddrHigh >> 8) & 0xFF));
        struct ether_addr *addr = ether_aton(ssm_addr_str);
        if (addr) {
            ifc_set_hwaddr(iname, addr->ether_addr_octet);
        }

        ifc_up(iname);

        ifc_close();

        if (curMacAddrLow == GetMacAddressLow()
                && curMacAddrHigh == GetMacAddressHigh()) {
            break;
        }

        curMacAddrLow = GetMacAddressLow();
        curMacAddrHigh = GetMacAddressHigh();
    }

    return NULL;
}

static void *SSMMacAddressStartWorkThreadMain(void *data __unused)
{
    void *tmp_ret = NULL;
    SetMacAddressStartWorkThreadExecFlag(1);
    tmp_ret = SSMMacAddressStartWorkMainApp(NULL);
    SetMacAddressStartWorkThreadExecFlag(0);
    return tmp_ret;
}

static int KillMacAddressStartWorkThread()
{
    int i = 0, tmp_timeout_count = 600;

    SetMacAddressStartWorkThreadTurnOnFlag(0);
    while (1) {
        if (GetMacAddressStartWorkThreadExecFlag() == 0) {
            break;
        }

        if (i >= tmp_timeout_count) {
            break;
        }

        i++;

        usleep(100 * 1000);
    }

    if (i == tmp_timeout_count) {
        LOGE(
            "%s, we have try %d times, but the mac address start work thread's exec flag is still(%d)!!!\n",
            CFG_SECTION_TV, tmp_timeout_count,
            GetMacAddressStartWorkThreadExecFlag());
        return -1;
    }

    pthread_join(mMacAddressStartWorkThreadID, NULL);
    mMacAddressStartWorkThreadID = CC_ERR_THREAD_ID;

    LOGD("%s, kill the mac address start work thread sucess.\n", __FUNCTION__);

    return 0;
}

int CreateMacAddressStartWorkThread()
{
    unsigned int macAddrLow = 0, macAddrHigh = 0;
    pthread_attr_t attr;
    struct sched_param param;
    unsigned char ssm_addr_buf[6] = { 0, 0, 0, 0, 0, 0 };

    if (KeyData_ReadMacAddress(ssm_addr_buf) < 0) {
        return -1;
    }

    macAddrLow = 0;
    macAddrLow |= ((ssm_addr_buf[0] & 0xFF) << 0);
    macAddrLow |= ((ssm_addr_buf[1] & 0xFF) << 8);
    macAddrLow |= ((ssm_addr_buf[2] & 0xFF) << 16);
    macAddrLow |= ((ssm_addr_buf[3] & 0xFF) << 24);

    macAddrHigh = 0;
    macAddrHigh |= ((ssm_addr_buf[4] & 0xFF) << 0);
    macAddrHigh |= ((ssm_addr_buf[5] & 0xFF) << 8);

    if (mMacAddressStartWorkThreadID != CC_ERR_THREAD_ID) {
        if (GetMacAddressStartWorkThreadExecFlag() == 1) {
            SetMacAddressLow(macAddrLow);
            SetMacAddressHigh(macAddrHigh);
            return 0;
        } else {
            KillMacAddressStartWorkThread();
        }
    }

    SetMacAddressLow(macAddrLow);
    SetMacAddressHigh(macAddrHigh);
    SetMacAddressStartWorkThreadTurnOnFlag(1);

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = 20;
    pthread_attr_setschedparam(&attr, &param);

    if (pthread_create(&mMacAddressStartWorkThreadID, &attr,
                       SSMMacAddressStartWorkThreadMain, NULL) < 0) {
        pthread_attr_destroy(&attr);
        mMacAddressStartWorkThreadID = CC_ERR_THREAD_ID;
        return -1;
    }

    pthread_attr_destroy(&attr);

    LOGD("%s, create channel select thread sucess.\n", __FUNCTION__);

    return 0;
}
/**************************** end mac address static functions ****************************/

/**************************** start hdcp key static functions ****************************/
int GetSSMHandleHDCPKeyEnableCFG()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, CS_HDCP_KEY_EN_CFG, "null");
#if 0
    LOGD("%s, get \"%s\" is \"%s\".\n", __FUNCTION__, CS_HDCP_KEY_EN_CFG,
         config_value);
#endif
    if (strcmp(config_value, "null") == 0) {
        LOGD(
            "%s, get config \"%s\" is \"%s\", return 0 to not enable handle hdcp key.\n",
            __FUNCTION__, CS_HDCP_KEY_EN_CFG, config_value);
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetSSMHandleHDCPKeyDemoEnableCFG()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, CS_HDCP_KEY_DEMO_EN_CFG, "null");
#if 0
    LOGD("%s, get \"%s\" is \"%s\".\n", __FUNCTION__, CS_HDCP_KEY_DEMO_EN_CFG,
         config_value);
#endif
    if (strcmp(config_value, "null") == 0) {
        LOGD(
            "%s, get config \"%s\" is \"%s\", return 0 to not enable handle hdcp key demo.\n",
            __FUNCTION__, CS_HDCP_KEY_DEMO_EN_CFG, config_value);
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}


int SSMSetDefaultHDCPKey(unsigned char hdcp_key_buf[])
{
    int i = 0;

    for (i = 0; i < CC_HDCP_KEY_HEAD_SIZE; i++) {
        hdcp_key_buf[i] = mHDCPKeyDefHeaderBuf[i];
    }

    for (i = 0; i < CC_HDCP_KEY_CONTENT_SIZE; i++) {
        hdcp_key_buf[i + CC_HDCP_KEY_HEAD_SIZE] = mDefHDCPKeyContentBuf[i];
    }

    LOGD("%s, using default hdcp key.\n", __FUNCTION__);

    return RealHandleHDCPKey(hdcp_key_buf);
}

int RealHandleHDCPKey(unsigned char hdcp_key_buf[])
{
    int dev_fd = -1;

    if (hdcp_key_buf == NULL) {
        return -1;
    }

    dev_fd = open("/sys/class/hdmirx/hdmirx0/edid", O_RDWR);
    if (dev_fd < 0) {
        LOGE("%s, open edid file ERROR(%s)!!\n", CFG_SECTION_TV, strerror(errno));
        return -1;
    }

    if (write(dev_fd, hdcp_key_buf, CC_HDCP_KEY_TOTAL_SIZE) < 0) {
        close(dev_fd);
        dev_fd = -1;
        LOGE("%s, write edid file ERROR(%s)!!\n", CFG_SECTION_TV, strerror(errno));

        return -1;
    }

    close(dev_fd);
    dev_fd = -1;
    return 0;
}

/**************************** end hdcp key static functions ****************************/

/**************************** start hdmi edid static functions ****************************/
int GetSSMHandleHDMIEdidByCustomerEnableCFG()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, CFG_SSM_HDMI_EDID_EN, "null");
    if (strcmp(config_value, "null") == 0) {
        LOGD(
            "%s, get config \"%s\" is \"%s\", return 0 to not enable handle hdmi edid by customer.\n",
            __FUNCTION__, CFG_SSM_HDMI_EDID_EN, config_value);
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int RealHandleHDMIEdid(unsigned char customer_hdmi_edid_buf[])
{
    int dev_fd = -1;

    if (customer_hdmi_edid_buf == NULL) {
        return -1;
    }

    dev_fd = open("/sys/class/hdmirx/hdmirx0/edid", O_RDWR);
    if (dev_fd < 0) {
        LOGE("%s, open edid file ERROR(%s)!!\n", CFG_SECTION_TV, strerror(errno));
        return -1;
    }

    if (write(dev_fd, customer_hdmi_edid_buf, CC_CUSTOMER_HDMI_EDID_TOTAL_SIZE)
            < 0) {
        close(dev_fd);
        dev_fd = -1;
        LOGE("%s, write edid file ERROR(%s)!!\n", CFG_SECTION_TV, strerror(errno));

        return -1;
    }

    close(dev_fd);
    dev_fd = -1;
    return 0;
}

int AppendEdidPrefixCode(unsigned char customer_hdmi_edid_buf[],
                         unsigned char hdmi_edid_buf[])
{
    if (customer_hdmi_edid_buf == NULL || hdmi_edid_buf == NULL) {
        LOGE("%s, Append hdmi edid's prefixCode ERROR(%s)!!\n", CFG_SECTION_TV,
             strerror(errno));
        return -1;
    }
    memset(customer_hdmi_edid_buf, 0,
           sizeof(char) * CC_CUSTOMER_HDMI_EDID_TOTAL_SIZE);
    customer_hdmi_edid_buf[0] = 'E';
    customer_hdmi_edid_buf[1] = 'D';
    customer_hdmi_edid_buf[2] = 'I';
    customer_hdmi_edid_buf[3] = 'D';
    memcpy(customer_hdmi_edid_buf + 4, hdmi_edid_buf,
           CC_CUSTOMER_HDMI_EDID_TOTAL_SIZE - 4);

    return 0;
}

/**************************** end hdmi edid static functions ****************************/

/**************************** start critical data op functions ****************************/
#define CC_OP_TYPE_READ                             (0)
#define CC_OP_TYPE_SAVE                             (1)
#define CC_DATA_TYPE_CHAR                           (0)
#define CC_DATA_TYPE_INT                            (1)

typedef int (*op_fun_ptr)(char *, int, int, unsigned char *);

typedef struct tagRWDataInfo {
    int op_type;
    int data_type;
    int max_size;
    int rw_off;
    int rw_size;
    void *data_buf;
    char *path_cfg_name;
    char *off_cfg_name;
    op_fun_ptr op_cb;
} RWDataInfo;

static int GetFilePathCFG(char *key_str, char path_buf[])
{
    int tmp_ret = 0;
    const char *cfg_value;

    path_buf[0] = '\0';
    cfg_value = config_get_str(CFG_SECTION_TV, key_str, "");
    strcpy(path_buf, cfg_value);
#if 0
    LOGD("%s, get \"%s\" is \"%s\".\n", CFG_SECTION_TV, key_str, path_buf);
#endif
    return tmp_ret;
}

static int GetFileOffsetCFG(char *key_str)
{
    const char *cfg_value;

    cfg_value = config_get_str(CFG_SECTION_TV, key_str, "null");
#if 0
    LOGD("%s, get \"%s\" is \"%s\".\n", CFG_SECTION_TV, key_str, cfg_value);
#endif
    if (strcmp(cfg_value, "null") == 0) {
        LOGD("%s, get config \"%s\" is \"%s\", return 0 for default.\n", CFG_SECTION_TV,
             key_str, cfg_value);
        return 0;
    }

    return strtol(cfg_value, NULL, 10);
}

static int handleDataFilePath(char *file_name, int offset __unused, int nsize __unused, char file_path[] __unused)
{
    if (file_name == NULL) {
        LOGE("%s, file_name is NULL!!!\n", CFG_SECTION_TV);
        return -1;
    }

    return 0;
}

static int ReadDataFromFile(char *file_name, int offset, int nsize,
                            unsigned char data_buf[])
{
    int device_fd = -1;
    int tmp_ret = 0;
    char *tmp_ptr = NULL;
    char file_path[512] = { '\0' };

    if (data_buf == NULL) {
        LOGE("%s, data_buf is NULL!!!\n", CFG_SECTION_TV);
        return -1;
    }

    tmp_ret = handleDataFilePath(file_name, offset, nsize, file_path);
    if (tmp_ret < 0) {
        tmp_ptr = NULL;
    } else if (tmp_ret == 0) {
        tmp_ptr = file_name;
    } else if (tmp_ret == 1) {
        tmp_ptr = file_path;
    }

    if (tmp_ptr == NULL) {
        return -1;
    }

    device_fd = open(tmp_ptr, O_RDONLY);
    if (device_fd < 0) {
        LOGE("%s: open file \"%s\" error(%s).\n", CFG_SECTION_TV, file_name,
             strerror(errno));
        return -1;
    }

    tmp_ret = lseek(device_fd, offset, SEEK_SET);
    if (tmp_ret == -1)
    {
        return -1;
    }
    tmp_ret = read(device_fd, data_buf, nsize);
    if (tmp_ret < 0)
    {
        return -1;
    }

    close(device_fd);
    device_fd = -1;

    return 0;
}

static int SaveDataToFile(char *file_name, int offset, int nsize,
                          unsigned char data_buf[])
{
    int device_fd = -1;
    int tmp_ret = 0;
    char *tmp_ptr = NULL;
    char file_path[512] = { '\0' };

    if (data_buf == NULL) {
        LOGE("%s, data_buf is NULL!!!\n", CFG_SECTION_TV);
        return -1;
    }

    tmp_ret = handleDataFilePath(file_name, offset, nsize, file_path);
    if (tmp_ret < 0) {
        tmp_ptr = NULL;
    } else if (tmp_ret == 0) {
        tmp_ptr = file_name;
    } else if (tmp_ret == 1) {
        tmp_ptr = file_path;
    }

    if (tmp_ptr == NULL) {
        return -1;
    }

    device_fd = open(tmp_ptr, O_RDWR | O_SYNC);
    if (device_fd < 0) {
        LOGE("%s: open file \"%s\" error(%s).\n", CFG_SECTION_TV, file_name,
             strerror(errno));
        return -1;
    }

    tmp_ret = lseek(device_fd, offset, SEEK_SET);
    if (tmp_ret == -1)
    {
        return -1;
    }
    write(device_fd, data_buf, nsize);
    fsync(device_fd);

    close(device_fd);
    device_fd = -1;

    return 0;
}

static int RealRWData(RWDataInfo *data_info)
{
    int file_off = 0;
    char file_name[256] = { '\0' };

    memset(file_name, '\0', 256);
    GetFilePathCFG(data_info->path_cfg_name, file_name);
#if 0
    LOGD("%s, file_name is %s.\n", __FUNCTION__, file_name);
#endif
    if (strlen(file_name) == 0) {
        LOGE("%s, length of file_name is 0!!!\n", CFG_SECTION_TV);
        return -2;
    }

    if (data_info->rw_off < 0) {
        LOGE("%s, data_info->rw_off (%d) is less than 0!!!\n", CFG_SECTION_TV,
             data_info->rw_off);
        return -1;
    }

    if (data_info->rw_off + data_info->rw_size > data_info->max_size) {
        LOGE(
            "%s, data_info->rw_off + data_info->rw_size (%d) is more than data_info->max_size(%d) !!!\n",
            CFG_SECTION_TV, data_info->rw_off + data_info->rw_size,
            data_info->max_size);
        return -1;
    }

    file_off = GetFileOffsetCFG(data_info->off_cfg_name);
    if (file_off < 0) {
        LOGE("%s, file_off (%d) is less than 0!!!\n", CFG_SECTION_TV, file_off);
        return -1;
    }

    file_off += data_info->rw_off;

    if (data_info->op_cb(file_name, file_off, data_info->rw_size,
                         (unsigned char *) data_info->data_buf) < 0) {
        return -1;
    }

    return 0;
}

static int HandleRWData(RWDataInfo *data_info)
{
    int i = 0, tmp_ret = 0;
    int *tmp_iptr = NULL;
    unsigned char *tmp_cptr = NULL;
    RWDataInfo tmpInfo;

    if (data_info == NULL) {
        return -1;
    }

    tmpInfo = *data_info;

    if (data_info->data_type == CC_DATA_TYPE_INT) {
        tmp_cptr = new unsigned char[data_info->rw_size];
        if (tmp_cptr != NULL) {
            tmpInfo.data_buf = tmp_cptr;

            if (tmpInfo.op_type == CC_OP_TYPE_SAVE) {
                tmp_iptr = (int *) data_info->data_buf;
                for (i = 0; i < data_info->rw_size; i++) {
                    tmp_cptr[i] = tmp_iptr[i];
                }

                tmp_ret |= RealRWData(&tmpInfo);
            } else {
                tmp_ret |= RealRWData(&tmpInfo);

                tmp_iptr = (int *) data_info->data_buf;
                for (i = 0; i < data_info->rw_size; i++) {
                    tmp_iptr[i] = tmp_cptr[i];
                }
            }

            delete[] tmp_cptr;
            tmp_cptr = NULL;

            return tmp_ret;
        }
    }

    return RealRWData(&tmpInfo);
}

int GetAudioNoLinePointsDataFromFile(int rd_off, int rd_size,
                                     unsigned char data_buf[])
{
    RWDataInfo tmpInfo;

    tmpInfo.op_type = CC_OP_TYPE_READ;
    tmpInfo.data_type = CC_DATA_TYPE_CHAR;
    tmpInfo.max_size = 256;
    tmpInfo.rw_off = rd_off;
    tmpInfo.rw_size = rd_size;
    tmpInfo.data_buf = data_buf;
    tmpInfo.path_cfg_name = (char *) CS_AUDIO_NOLINEPOINTS_FILE_PATH_CFG;
    tmpInfo.off_cfg_name = (char *) CS_AUDIO_NOLINEPOINTS_FILE_OFFSET_CFG;
    tmpInfo.op_cb = ReadDataFromFile;

    return HandleRWData(&tmpInfo);
}

int SaveAudioNoLinePointsDataToFile(int wr_off, int wr_size,
                                    unsigned char data_buf[])
{
    RWDataInfo tmpInfo;

    tmpInfo.op_type = CC_OP_TYPE_SAVE;
    tmpInfo.data_type = CC_DATA_TYPE_CHAR;
    tmpInfo.max_size = 256;
    tmpInfo.rw_off = wr_off;
    tmpInfo.rw_size = wr_size;
    tmpInfo.data_buf = data_buf;
    tmpInfo.path_cfg_name = (char *) CS_AUDIO_NOLINEPOINTS_FILE_PATH_CFG;
    tmpInfo.off_cfg_name = (char *) CS_AUDIO_NOLINEPOINTS_FILE_OFFSET_CFG;
    tmpInfo.op_cb = SaveDataToFile;

    return HandleRWData(&tmpInfo);
}

int GetHDCPKeyFromFile(int rd_off, int rd_size,
                       unsigned char data_buf[])
{
    RWDataInfo tmpInfo;

    tmpInfo.op_type = CC_OP_TYPE_READ;
    tmpInfo.data_type = CC_DATA_TYPE_CHAR;
    tmpInfo.max_size = CC_HDCP_KEY_TOTAL_SIZE;
    tmpInfo.rw_off = rd_off;
    tmpInfo.rw_size = rd_size;
    tmpInfo.data_buf = data_buf;
    tmpInfo.path_cfg_name = (char *) CS_HDCP_KEY_FILE_PATH_CFG;
    tmpInfo.off_cfg_name = (char *) CS_HDCP_KEY_FILE_OFFSET_CFG;
    tmpInfo.op_cb = ReadDataFromFile;

    return HandleRWData(&tmpInfo);
}

int SaveHDCPKeyToFile(int wr_off, int wr_size,
                      unsigned char data_buf[])
{
    RWDataInfo tmpInfo;

    tmpInfo.op_type = CC_OP_TYPE_SAVE;
    tmpInfo.data_type = CC_DATA_TYPE_CHAR;
    tmpInfo.max_size = CC_HDCP_KEY_TOTAL_SIZE;
    tmpInfo.rw_off = wr_off;
    tmpInfo.rw_size = wr_size;
    tmpInfo.data_buf = data_buf;
    tmpInfo.path_cfg_name = (char *) CS_HDCP_KEY_FILE_PATH_CFG;
    tmpInfo.off_cfg_name = (char *) CS_HDCP_KEY_FILE_OFFSET_CFG;
    tmpInfo.op_cb = SaveDataToFile;

    return HandleRWData(&tmpInfo);
}

int GetHDMIEdidFromFile(int rd_off, int rd_size, int port,
                        unsigned char data_buf[])
{
    RWDataInfo tmpInfo;

    tmpInfo.op_type = CC_OP_TYPE_READ;
    tmpInfo.data_type = CC_DATA_TYPE_CHAR;
    tmpInfo.max_size = SSM_HDMI_EDID_SIZE;
    tmpInfo.rw_off = rd_off;
    tmpInfo.rw_size = rd_size;
    tmpInfo.data_buf = data_buf;
    switch (port) {
    case 1:
        tmpInfo.path_cfg_name = (char *) CS_HDMI_PORT1_EDID_FILE_PATH_CFG;
        break;
    case 2:
        tmpInfo.path_cfg_name = (char *) CS_HDMI_PORT2_EDID_FILE_PATH_CFG;
        break;
    case 3:
        tmpInfo.path_cfg_name = (char *) CS_HDMI_PORT3_EDID_FILE_PATH_CFG;
        break;
    case 4:
        tmpInfo.path_cfg_name = (char *) CS_HDMI_PORT4_EDID_FILE_PATH_CFG;
        break;
    default:
        LOGE("%s, port is error, =%d\n", CFG_SECTION_TV, port);
        tmpInfo.path_cfg_name = (char *) CS_HDMI_EDID_FILE_PATH_CFG;
        break;
    }
    tmpInfo.off_cfg_name = (char *) CS_HDMI_EDID_FILE_OFFSET_CFG;
    tmpInfo.op_cb = ReadDataFromFile;

    return HandleRWData(&tmpInfo);
}

/**************************** end critical data op functions ****************************/
