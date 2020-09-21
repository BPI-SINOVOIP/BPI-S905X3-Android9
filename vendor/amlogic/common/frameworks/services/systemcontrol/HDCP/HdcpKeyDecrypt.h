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
 *  @date     2016/5/17
 *  @par function description:
 *  - 1 decrypt key from storage, generate 2 files
 *  - 2 one is random number, another is key file
 */

#ifndef __HDCP_HEY_DESCRYPT_H__
#define __HDCP_HEY_DESCRYPT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int        __u32;
typedef signed int          __s32;
typedef unsigned char       __u8;
typedef signed char         __s8;

typedef __u32   u32;
typedef __s32   s32;
typedef __u8    u8;
typedef __s8    s8;

#define IH_MAGIC	    0x27051956	/* Image Magic Number		*/
#define IH_NMLEN		32	/* Image Name Length		*/


#define AML_RES_IMG_ITEM_ALIGN_SZ   16
#define AML_RES_IMG_V1_MAGIC_LEN    8
#define AML_RES_IMG_V1_MAGIC        "AML_HDK!"//8 chars
#define AML_RES_IMG_HEAD_SZ         (24)//64
#define AML_RES_ITEM_HEAD_SZ        (48)//64

#define AML_RES_IMG_VERSION_V1      (0x01)

#pragma pack(push, 1)
typedef struct pack_header{
	unsigned int	totalSz;/* Item Data total Size*/
	unsigned int	dataSz;	/* Item Data used  Size*/
	unsigned int	dataOffset;	/* Item data offset*/
	unsigned char   type;	/* Image Type, not used yet*/
	unsigned char 	comp;	/* Compression Type	*/
    unsigned short  reserv;
	char 	name[IH_NMLEN];	/* Image Name		*/
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

 enum {
    PC_TOOL = 0,
    ARM_TOOL
};

int do_aes(bool isEncrypt, unsigned char* pIn, int nInLen, unsigned char* pOut, int* pOutLen);

int hdcpKeyUnpack(const char* inBuf, int inBufLen,
    const char *srcAicPath, const char *desAicPath, const char *keyPath);

#ifdef __cplusplus
}
#endif
#endif//#ifndef __HDCP_HEY_DESCRYPT_H__


