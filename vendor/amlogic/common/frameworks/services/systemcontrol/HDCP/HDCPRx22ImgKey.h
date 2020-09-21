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
 *  @version  2.0
 *  @date     2017/11/22
 *  @par function description:
 *  - 1 rx key combine method
 */

#ifndef __HDCPRX22_IMG_KEY_H__
#define __HDCPRX22_IMG_KEY_H__

//old path use for tcl
//#define IMPDATA_HDCP_RX_KEY
#define HDCP_FW_LE_OLD_PATH         "/system/etc/firmware.le"
#define HDCP_PACKED_IMG_PATH        "/impdata/hdcp_key2.0.bin"
#define HDCP_NEW_KEY_CREATED        "/impdata/newkeyCreated"

//#define HDCP_RX_KEY_PC_TOOL
#define HDCP_RX_SRC_FW_PATH         "/system/etc/firmware/hdcp_rx22/firmware.le"
#define HDCP_RX_DES_FW_PATH         "/mnt/vendor/param/firmware.le"
#define HDMI_RX_KEY_COMBINE         "/sys/module/tvin_hdmirx/parameters/hdcp22_firmware_ok_flag"

#define UNIFYKEY_NMAE        "hdcp22_fw_private"

#define HDCP_RX_PRIVATE         "hdcp22_rx_private"
#define HDCP_RX                        "hdcp2_rx"
#define HDCP_RX_FW              "extractedKey"

#define UNIFYKEY_ATTACH     "/sys/class/unifykeys/attach"
#define UNIFYKEY_NAME        "/sys/class/unifykeys/name"
#define UNIFYKEY_WRITE      "/sys/class/unifykeys/write"
#define UNIFYKEY_READ        "/sys/class/unifykeys/read"
#define UNIFYKEY_EXIST       "/sys/class/unifykeys/exist"
#define HDCP_RX_DEBUG_PATH              "/sys/class/hdmirx/hdmirx0/debug"

#define WRITE_SIZE     (200*1024)
#define MAX_STR_LEN         4096

typedef __s32 fpi_error;

typedef unsigned int        __u32;
typedef signed int          __s32;
typedef unsigned char       __u8;
typedef signed char         __s8;

#define IH_MAGIC    0x27051956  /* Image Magic Number*/
#define IH_NMLEN    32   /* Image Name Length*/


#define AML_RES_IMG_ITEM_ALIGN_SZ   16
#define AML_RES_IMG_V1_MAGIC_LEN    8
#define AML_RES_IMG_V1_MAGIC        "AML_HDK!"//8 chars
#define AML_RES_IMG_HEAD_SZ         (24)//64
#define AML_RES_ITEM_HEAD_SZ        (48)//64

#define AML_RES_IMG_VERSION_V1      (0x01)

#pragma pack(push, 1)
typedef struct pack_header{
    unsigned int    totalSz;/* Item Data total Size*/
    unsigned int    dataSz;/* Item Data used  Size*/
    unsigned int    dataOffset;/* Item data offset*/
    unsigned char   type;/* Image Type, not used yet*/
    unsigned char   comp;/* Compression Type*/
    unsigned short  reserv;
    char    name[IH_NMLEN];/* Image Name*/
}AmlResItemHead_t;
#pragma pack(pop)

//typedef for amlogic resource image
#pragma pack(push, 4)
typedef struct {
    __u32   crc;    //crc32 value for the resouces image
    __s32   version;//0x01 means 'AmlResItemHead_t' attach to each item , 0x02 means all 'AmlResItemHead_t' at the head

    __u8    magic[AML_RES_IMG_V1_MAGIC_LEN];  //resources images magic

    __u32   imgSz;  //total image size in byte
    __u32   imgItemNum;//total item packed in the image

}AmlResImgHead_t;
#pragma pack(pop)

/*The Amlogic resouce image is consisted of a AmlResImgHead_t and many
 *
 * |<---AmlResImgHead_t-->|<--AmlResItemHead_t-->---...--|<--AmlResItemHead_t-->---...--|....
 *
 */

int aml_hdcp22_key_pack(const char** const path_src, const int totalFileNum, const char* const packedImg);

int aml_hdcp22_extract_firmwarele(const char* firmwarele, const char* extractedKey);

int generateHdcpFw(const char* firmwarele, const char* packedImg, const char* newFw);
int generateHdcpFwFromStorage(const char* firmwarele, const char* newFw);
int setImgPath(const char *path);

#endif// #ifndef __HDCPRX22_IMG_KEY_H__

