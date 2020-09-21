/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c file
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include "log_print.h"
#include "sub_subtitle.h"

#include <android/log.h>

#define  LOG_TAG    "sub_jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

//static char *pixData1, *pixData2;
#define OSD_HALF_SIZE (1920*1280/8)

static unsigned short DecodeRL(unsigned short RLData, unsigned short *pixelnum,
                               unsigned short *pixeldata)
{
    unsigned short nData = RLData;
    unsigned short nShiftNum;
    unsigned short nDecodedBits;
    if (nData & 0xc000)
        nDecodedBits = 4;
    else if (nData & 0x3000)
        nDecodedBits = 8;
    else if (nData & 0x0c00)
        nDecodedBits = 12;
    else
        nDecodedBits = 16;
    nShiftNum = 16 - nDecodedBits;
    *pixeldata = (nData >> nShiftNum) & 0x0003;
    *pixelnum = nData >> (nShiftNum + 2);
    return nDecodedBits;
}

static unsigned short GetWordFromPixBuffer(unsigned short bitpos,
        unsigned short *pixelIn)
{
    unsigned char hi = 0, lo = 0, hi_ = 0, lo_ = 0;
    char *tmp = (char *)pixelIn;
    hi = *(tmp + 0);
    lo = *(tmp + 1);
    hi_ = *(tmp + 2);
    lo_ = *(tmp + 3);
    if (bitpos == 0)
    {
        return (hi << 0x8 | lo);
    }
    else
    {
        return (((hi << 0x8 | lo) << bitpos) |
                ((hi_ << 0x8 | lo_) >> (16 - bitpos)));
    }
}

unsigned char FillPixel(char *ptrPXDRead, char *pixelOut, int n,
                        AML_SPUVAR *sub_frame, int field_offset)
{
    unsigned short nPixelNum = 0, nPixelData = 0;
    unsigned short nRLData, nBits;
    unsigned short nDecodedPixNum = 0;
    unsigned short i, j;
    unsigned short rownum = sub_frame->spu_width;   //
    unsigned short height = sub_frame->spu_height;  //
    unsigned short nAddPixelAtStart = 0, nAddPixelAtEnd = 1;
    unsigned short bg_data = 0;
    unsigned char spu_data = 0xff;
    unsigned short pixTotalNum = 0;
    unsigned short PXDBufferBitPos = 0, WrOffset = 16, PXDRdOffsetEven =
                                         0, PXDRdOffsetOdd = 0;
    unsigned short totalBits = 0;
    pixTotalNum = height * rownum;
    unsigned short *ptrPXDWrite = pixelOut;
    unsigned short *ptrPXDWriteEnd = 0;
    int current_pixdata = 1;
    // 4 buffer for pix data
    //    if (n==1) { // 1 for odd
    //        if (current_pixdata == 1){
    //            memset(pixData1, 0, OSD_HALF_SIZE);
    //            ptrPXDWrite = (unsigned short *)pixData1;
    //        }
    //        else if (current_pixdata == 2){
    //            memset(pixData2, 0, OSD_HALF_SIZE);
    //            ptrPXDWrite = (unsigned short *)pixData2;
    //        }
    //        else {
    //            return -1;
    //        }
    //    }
    //    else if (n==2) {        // 2 for even
    //        if (current_pixdata == 1){
    //            memset(pixData1+OSD_HALF_SIZE, 0, OSD_HALF_SIZE);
    //            ptrPXDWrite = (unsigned short *)(pixData1+OSD_HALF_SIZE);
    //        }
    //        else if (current_pixdata == 2){
    //            memset(pixData2+OSD_HALF_SIZE, 0, OSD_HALF_SIZE);
    //            ptrPXDWrite = (unsigned short *)(pixData2+OSD_HALF_SIZE);
    //        }
    //        else {
    //            return -1;
    //        }
    //    }
    //    else {
    //        return -1;
    //    }
    ptrPXDWriteEnd =
        (unsigned short *)(((char *)ptrPXDWrite) + OSD_HALF_SIZE);
    for (j = 0; j < height / 2; j++)
    {
        while (nDecodedPixNum < rownum)
        {
            nRLData =
                GetWordFromPixBuffer(PXDBufferBitPos, ptrPXDRead);
            nBits = DecodeRL(nRLData, &nPixelNum, &nPixelData);
            PXDBufferBitPos += nBits;
            if (PXDBufferBitPos >= 16)
            {
                PXDBufferBitPos -= 16;
                ptrPXDRead += 2;
            }
            if (nPixelNum == 0)
            {
                nPixelNum = rownum - nDecodedPixNum % rownum;
            }
            for (i = 0; i < nPixelNum; i++)
            {
                WrOffset -= 2;
                *ptrPXDWrite |= nPixelData << WrOffset;
                //              ASSERT((ptrPXDWrite>=ptrPXDWriteEnd), "AVI: subtitle write pointer out of range\n");
                if (WrOffset == 0)
                {
                    WrOffset = 16;
                    ptrPXDWrite++;
                    // avoid out of range
                    if (ptrPXDWrite >= ptrPXDWriteEnd)
                        ptrPXDWrite =
                            ptrPXDWriteEnd - 1;
                    //              *ptrPXDWrite = bg_data;
                }
            }
            totalBits += nBits;
            nDecodedPixNum += nPixelNum;
        }
        if (PXDBufferBitPos == 4)   //Rule 6
        {
            PXDBufferBitPos = 8;
        }
        else if (PXDBufferBitPos == 12)
        {
            PXDBufferBitPos = 0;
            ptrPXDRead += 2;
        }
        if (WrOffset != 16)
        {
            WrOffset = 16;
            ptrPXDWrite++;
            // avoid out of range
            if (ptrPXDWrite >= ptrPXDWriteEnd)
            {
                ptrPXDWrite = ptrPXDWriteEnd - 1;
            }
        }
        nDecodedPixNum -= rownum;
    }
    if (totalBits == field_offset)
    {
        return 1;
    }
    return 0;
}
