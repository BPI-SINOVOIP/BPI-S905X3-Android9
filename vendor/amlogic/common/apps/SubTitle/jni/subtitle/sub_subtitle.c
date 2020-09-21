/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c file
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <android/log.h>
#include "amstream.h"

//#include "sub_control.h"
#include "sub_subtitle.h"
#include "sub_vob_sub.h"
#include "sub_dvb_sub.h"
#include "sub_pgs_sub.h"
#include "sub_set_sys.h"
#include "sub_teletextdec.h"

#include "sub_io.h"

typedef struct _DivXSubPictColor
{
    char red;
    char green;
    char blue;
} DivXSubPictColor;

#pragma pack(1)
typedef struct _DivXSubPictHdr
{
    char duration[27];
    unsigned short width;
    unsigned short height;
    unsigned short left;
    unsigned short top;
    unsigned short right;
    unsigned short bottom;
    unsigned short field_offset;
    DivXSubPictColor background;
    DivXSubPictColor pattern1;
    DivXSubPictColor pattern2;
    DivXSubPictColor pattern3;
    unsigned char *rleData;
} DivXSubPictHdr;

typedef struct _DivXSubPictHdr_HD
{
    char duration[27];
    unsigned short width;
    unsigned short height;
    unsigned short left;
    unsigned short top;
    unsigned short right;
    unsigned short bottom;
    unsigned short field_offset;
    DivXSubPictColor background;
    DivXSubPictColor pattern1;
    DivXSubPictColor pattern2;
    DivXSubPictColor pattern3;
    unsigned char background_transparency;  //HD profile only
    unsigned char pattern1_transparency;    //HD profile only
    unsigned char pattern2_transparency;    //HD profile only
    unsigned char pattern3_transparency;    //HD profile only
    unsigned char *rleData;
} DivXSubPictHdr_HD;
#pragma pack()

#define  LOG_TAG    "sub_subtitle"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define SUBTITLE_READ_DEVICE    "/dev/amstream_sub_read"
#define SUBTITLE_FILE "/tmp/subtitle.db"
#define VOB_SUBTITLE_FRAMW_SIZE   (4+1+4+4+2+2+2+2+2+4+VOB_SUB_SIZE)
#define MAX_SUBTITLE_PACKET_WRITE   1000
static int sublen = MAX_SUBTITLE_PACKET_WRITE;  // if image format sublen=50, if string format sublen=1000
#define ADD_SUBTITLE_POSITION(x)  (((x+1)<sublen)?(x+1):0)
#define DEC_SUBTITLE_POSITION(x)  (((x-1)>=0)?(x-1):(sublen-1))
static off_t file_position = 0;
static off_t read_position = 0;
static int aml_sub_handle = -1;
char *restbuf = NULL;
int restlen = 0;
//extern int sub_thread;
lock_t sublock;
lock_t closelock;
lock_t handlelock;
int subtitle_status = 0;

typedef struct
{
    int subtitle_type;  //add yjf
    int subtitle_size;
    unsigned subtitle_pts;
    unsigned subtitle_delay_pts;
    int data_size;
    int subtitle_width;
    int subtitle_height;
    int resize_height;
    int resize_width;
    int resize_xstart;
    int resize_ystart;
    int resize_size;
    unsigned short sub_alpha;
    unsigned rgba_enable;
    unsigned rgba_background;
    unsigned rgba_pattern1;
    unsigned rgba_pattern2;
    unsigned rgba_pattern3;
    char *data;
    unsigned int spu_origin_display_w;  //for bitmap subtitle
    unsigned int spu_origin_display_h;
    unsigned short spu_start_x;
    unsigned short spu_start_y;
} subtitle_data_t;
static subtitle_data_t inter_subtitle_data[MAX_SUBTITLE_PACKET_WRITE];

static int inter_subtitle_type = 0;

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

int get_ass_spu(char *spu_buf, unsigned length, AML_SPUVAR *spu)
{
    int ret = 0;
    int i = 0, j = 0;
    //LOGE("spubuf  %c %c %c %c %c %c %c %c   %c %c %c %c %c %c %c %c  \n %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c\n",
    //    spu_buf[0], spu_buf[1],spu_buf[2],spu_buf[3],spu_buf[4],spu_buf[5],spu_buf[6],spu_buf[7],
    //    spu_buf[8],spu_buf[9],spu_buf[10],spu_buf[11],spu_buf[12],spu_buf[13],spu_buf[14],spu_buf[15],
    //   spu_buf[16],spu_buf[17],spu_buf[18],spu_buf[19],spu_buf[20],spu_buf[21],spu_buf[22],spu_buf[23],
    //   spu_buf[24],spu_buf[25],spu_buf[26],spu_buf[27],spu_buf[28],spu_buf[29],spu_buf[30],spu_buf[31] ,spu_buf[32] );
    unsigned hour, min, sec, mills, startmills, endmills;
    if (length > 33 && strncmp(spu_buf, "Dialogue:", 9) == 0)   //ass Events match
    {
        i = 9;
        while (((spu_buf[i] != ':') || (spu_buf[i + 3] != ':'))
                && (i < length))
            i++;
        i--;
        hour = spu_buf[i] - 0x30;
        min = (spu_buf[i + 2] - 0x30) * 10 + (spu_buf[i + 3] - 0x30);
        sec = (spu_buf[i + 5] - 0x30) * 10 + (spu_buf[i + 6] - 0x30);
        mills = (spu_buf[i + 8] - 0x30) * 10 + (spu_buf[i + 9] - 0x30);
        startmills =
            (hour * 60 * 60 + min * 60 + sec) * 1000 + mills * 10;
        spu->pts = startmills * 90;
        LOGE("%d:%d:%d:%d, start mills=0x%x\n", hour, min, sec, mills,
             startmills);
        hour = spu_buf[i + 11] - 0x30;
        min = (spu_buf[i + 13] - 0x30) * 10 + (spu_buf[i + 14] - 0x30);
        sec = (spu_buf[i + 16] - 0x30) * 10 + (spu_buf[i + 17] - 0x30);
        mills =
            (spu_buf[i + 19] - 0x30) * 10 + (spu_buf[i + 20] - 0x30);
        endmills =
            (hour * 60 * 60 + min * 60 + sec) * 1000 + mills * 10;
        spu->m_delay = endmills * 90;
        LOGE("%d:%d:%d:%d, end mills=0x%x m-delay=0x%x\n", hour, min,
             sec, mills, endmills, spu->m_delay);
    }
    // remove the chars before  '\}'
    j = 0;
    for (i = 0; i < 35; i++)
    {
        //if (strncmp(spu_buf + i, "Default", 7) == 0)
        //{
            j = i;
            unsigned char *p0 = strstr(spu_buf + i, "0000,0000,0000,,");
            if (p0)
            {
                spu->buffer_size -= (p0 + 16 - spu->spu_data);
                memmove(spu->spu_data, p0 + 16, spu->buffer_size);
                break;
            }

            unsigned char *p2 = strstr(spu_buf + i, "0000,0000,0000,karaoke,");
            if (p2)
            {
                spu->buffer_size -= (p2 + 23 - spu->spu_data);
                memmove(spu->spu_data, p2 + 23, spu->buffer_size);
                break;
            }

            unsigned char *p1 = strstr(spu_buf + i, "0000,0000,0000,!Effect,");
            if (p1)
            {
                spu->buffer_size -= (p1 + 23 - spu->spu_data);
                memmove(spu->spu_data, p1 + 23, spu->buffer_size);
            }
            else
            {
                j = j + i;
                spu->buffer_size -= j;
                memmove(spu->spu_data, spu->spu_data + j,
                spu->buffer_size);
            }
            break;
        //}
    }
    return ret;
}

unsigned char spu_fill_pixel(unsigned short *pixelIn, char *pixelOut,
                             AML_SPUVAR *sub_frame, int n)
{
    unsigned short nPixelNum = 0, nPixelData = 0;
    unsigned short nRLData, nBits;
    unsigned short nDecodedPixNum = 0;
    unsigned short i, j;
    unsigned short PXDBufferBitPos = 0, WrOffset = 16;
    unsigned short change_data = 0;
    unsigned short PixelDatas[4] = { 0, 1, 2, 3 };
    unsigned short rownum = sub_frame->spu_width;
    unsigned short height = sub_frame->spu_height;
    unsigned short _alpha = sub_frame->spu_alpha;
    static unsigned short *ptrPXDWrite;
    memset(pixelOut, 0, VOB_SUB_SIZE / 2);
    ptrPXDWrite = (unsigned short *)pixelOut;
    if (_alpha & 0xF)
    {
        _alpha = _alpha >> 4;
        change_data++;
        while (_alpha & 0xF)
        {
            change_data++;
            _alpha = _alpha >> 4;
        }
        PixelDatas[0] = change_data;
        PixelDatas[change_data] = 0;
        if (n == 2)
            sub_frame->spu_alpha =
                (sub_frame->
                 spu_alpha & 0xFFF0) | (0x000F << (change_data <<
                                                   2));
    }
    for (j = 0; j < height / 2; j++)
    {
        while (nDecodedPixNum < rownum)
        {
            nRLData =
                GetWordFromPixBuffer(PXDBufferBitPos, pixelIn);
            nBits = DecodeRL(nRLData, &nPixelNum, &nPixelData);
            PXDBufferBitPos += nBits;
            if (PXDBufferBitPos >= 16)
            {
                PXDBufferBitPos -= 16;
                pixelIn++;
            }
            if (nPixelNum == 0)
            {
                nPixelNum = rownum - nDecodedPixNum % rownum;
            }
            if (change_data)
            {
                nPixelData = PixelDatas[nPixelData];
            }
            for (i = 0; i < nPixelNum; i++)
            {
                WrOffset -= 2;
                *ptrPXDWrite |= nPixelData << WrOffset;
                if (WrOffset == 0)
                {
                    WrOffset = 16;
                    ptrPXDWrite++;
                }
            }
            nDecodedPixNum += nPixelNum;
        }
        if (PXDBufferBitPos == 4)   //Rule 6
        {
            PXDBufferBitPos = 8;
        }
        else if (PXDBufferBitPos == 12)
        {
            PXDBufferBitPos = 0;
            pixelIn++;
        }
        if (WrOffset != 16)
        {
            WrOffset = 16;
            ptrPXDWrite++;
        }
        nDecodedPixNum -= rownum;
    }
    return 0;
}

#define str2ms(s) (((s[1]-0x30)*3600*10+(s[2]-0x30)*3600+(s[4]-0x30)*60*10+(s[5]-0x30)*60+(s[7]-0x30)*10+(s[8]-0x30))*1000+(s[10]-0x30)*100+(s[11]-0x30)*10+(s[12]-0x30))
int get_spu(AML_SPUVAR *spu, int read_sub_fd)
{
    int ret, rd_oft, wr_oft, size;
    char *spu_buf = NULL;
    unsigned current_length, current_pts, current_type, duration_pts;
    unsigned short *ptrPXDWrite = 0, *ptrPXDRead = 0;
    DivXSubPictHdr *avihandle = NULL;
    DivXSubPictHdr_HD *avihandle_hd = NULL;
    if (subtitle_status == SUB_INIT)
    {
        subtitle_status = SUB_PLAYING;
    }
    else if (subtitle_status == SUB_STOP)
    {
        LOGI(" subtitle_status == SUB_STOP \n\n");
        return 0;
    }
    if (read_sub_fd < 0)
        return 0;

    if (getIOType() == IO_TYPE_DEV) {
        ret = pollFd(read_sub_fd, 10);
        if (ret == 0) {
            //LOGI("codec_poll_sub_fd fail \n\n");
            ret = -1;
            goto error;
        }
    }

    /*if (get_subtitle_enable() == 0)
    {
        size = getSize(read_sub_fd);
        if (size > 0)
        {
            char *buff = malloc(size);
            if (buff)
            {
                getData(read_sub_fd, buff, size);
                free(buff);
            }
        }
        ret = -1;
        goto error;
    }*/
    //LOGI(" [get_spu] getsubtype:%d \n",get_subtitle_subtype());
    if (get_subtitle_subtype() == 1)
    {
        //pgs subtitle
        sublen = 50;
        size = getSize(read_sub_fd);
        LOGI("start pgs sub buffer size %d\n", size);
        int ret_spu = get_pgs_spu(spu, read_sub_fd);
        size = getSize(read_sub_fd);
        LOGI("end pgs sub buffer size %d\n", size);
        return 0;
    }
    else if (get_subtitle_subtype() == 5)
    {
        sublen = 50;
        size = getSize(read_sub_fd);
        LOGI("start dvb sub buffer size %d\n", size);
        int ret_spu = get_dvb_spu(spu, read_sub_fd);
        if (ret_spu == -1)
        {
            close_subtitle();
            dvbsub_init_decoder();
            subtitle_status = SUB_INIT;
            restlen = 0;
        }
        size = getSize(read_sub_fd);
        LOGI("end dvb buffer size %d\n", size);
        return 0;
    } else if (get_subtitle_subtype() == 9) { //SUBTITLE_DVB_TELETEXT
        sublen = 50;
        size = getSize(read_sub_fd);
        LOGI("start dvb teletext buffer size %d\n", size);
        int ret_spu = get_dvb_teletext_spu(spu, read_sub_fd);
        if (ret_spu == -1)
        {
            close_subtitle();
            teletext_init_decoder();
            subtitle_status = SUB_INIT;
            restlen = 0;
        }
        size = getSize(read_sub_fd);
        LOGI("end dvb teletext buffer size %d\n", size);
        return 0;
    }
    size = getSize(read_sub_fd);
    if (size <= 0)
    {
        ret = -1;
        //LOGI("\n player get sub size less than zero \n\n");
        goto error;
    }
    else
    {
        size += restlen;
        current_type = 0;
        spu_buf = malloc(size);
        LOGI("\n malloc subtitle size %d, restlen=%d, spu_buf=%x, \n\n",
             size, restlen, spu_buf);
    }
    int sizeflag = size;
    char *spu_buf_tmp = spu_buf;
    char *spu_buf_piece = spu_buf_piece;
    while (sizeflag > 30)
    {
        LOGI("\n sizeflag =%u  restlen=%d, \n\n", sizeflag, restlen);
        if (sizeflag <= 16)
        {
            ret = -1;
            LOGI("\n sizeflag is too little \n\n");
            goto error;
        }
        char *spu_buf_piece = spu_buf_tmp;
        if (restlen)
            memcpy(spu_buf_piece, restbuf, restlen);
        if ((current_type == 0x17000 || current_type == 0x1700a)
                && restlen > 0)
        {
            LOGI("decode rest data!\n");
        }
        else
        {
            getData(read_sub_fd, spu_buf_piece + restlen, 16);
            sizeflag -= 16;
            spu_buf_tmp += 16;
        }
        rd_oft = 0;
        if ((spu_buf_piece[rd_oft++] != 0x41)
                || (spu_buf_piece[rd_oft++] != 0x4d)
                || (spu_buf_piece[rd_oft++] != 0x4c)
                || (spu_buf_piece[rd_oft++] != 0x55)
                /*|| ((spu_buf_piece[rd_oft++] & 0xfe) != 0xaa)*/)
        {
            LOGI("\n wrong subtitle header :%x %x %x %x    %x %x %x %x    %x %x %x %x \n", spu_buf_piece[0], spu_buf_piece[1], spu_buf_piece[2], spu_buf_piece[3], spu_buf_piece[4], spu_buf_piece[5], spu_buf_piece[6], spu_buf_piece[7], spu_buf_piece[8], spu_buf_piece[9], spu_buf_piece[10], spu_buf_piece[11]);
            getData(read_sub_fd, spu_buf_piece, sizeflag);
            sizeflag = 0;
            LOGI("\n\n ******* find wrong subtitle header!! ******\n\n");
            ret = -1;
            goto error; // wrong head
        }
        rd_oft++;//0xaa or 0x77
        LOGI("\n\n ******* find correct subtitle header ******\n\n");
        current_type = spu_buf_piece[rd_oft++] << 16;
        current_type |= spu_buf_piece[rd_oft++] << 8;
        current_type |= spu_buf_piece[rd_oft++];
        current_length = spu_buf_piece[rd_oft++] << 24;
        current_length |= spu_buf_piece[rd_oft++] << 16;
        current_length |= spu_buf_piece[rd_oft++] << 8;
        current_length |= spu_buf_piece[rd_oft++];
        current_pts = spu_buf_piece[rd_oft++] << 24;
        current_pts |= spu_buf_piece[rd_oft++] << 16;
        current_pts |= spu_buf_piece[rd_oft++] << 8;
        current_pts |= spu_buf_piece[rd_oft++];
        LOGI("sizeflag=%u, current_type:%x, current_pts is %x, current_length is %d, \n", sizeflag, current_type, current_pts, current_length);
        if (current_length > sizeflag)
        {
            LOGI("current_length > size");
            getData(read_sub_fd, spu_buf_piece, sizeflag);
            sizeflag = 0;
            ret = -1;
            goto error;
        }
        if (current_type == 0x17000 || current_type == 0x1700a)
        {
            getData(read_sub_fd, spu_buf_piece + restlen + 16, sizeflag - restlen);
            restlen = sizeflag;
            sizeflag = 0;
            spu_buf_tmp += current_length;
            LOGI("current_type=0x17000 or 0x1700a! restlen=%d, sizeflag=%d,\n", restlen, sizeflag);
        }
        else
        {
            getData(read_sub_fd, spu_buf_piece + 16, current_length + 4);
            sizeflag -= (current_length + 4);
            spu_buf_tmp += (current_length + 4);
            restlen = 0;
        }
        //FFT: i dont know why we throw the first sub, when pts == 0. remove these codes first.
        /*
           if ((current_pts == 0) && (current_type != 0x17009)) {
           LOGI("current_pts==0\n");

           ret = -1;
           continue;
           }
         */
        switch (current_type)
        {
            case 0x17003:   //XSUB
                duration_pts = spu_buf_piece[rd_oft++] << 24;
                duration_pts |= spu_buf_piece[rd_oft++] << 16;
                duration_pts |= spu_buf_piece[rd_oft++] << 8;
                duration_pts |= spu_buf_piece[rd_oft++];
                int has_alpha = spu_buf_piece[4] & 0x01;
                LOGI("duration_pts is %d, current_length=%d  ,rd_oft is %d, has_alpha=%d\n", duration_pts, current_length, rd_oft, has_alpha);
                avihandle = (DivXSubPictHdr *)(spu_buf_piece + rd_oft);
                spu->spu_data = malloc(VOB_SUB_SIZE);
                memset(spu->spu_data, 0, VOB_SUB_SIZE);
                sublen = 50;
                spu->subtitle_type = SUBTITLE_VOB;
                spu->buffer_size = VOB_SUB_SIZE;
                {
                    unsigned char *s = &(avihandle->duration[0]);
                    spu->pts = str2ms(s) * 90;
                    s = &(avihandle->duration[13]);
                    spu->m_delay = str2ms(s) * 90;
                }
                spu->spu_width = avihandle->width;
                spu->spu_height = avihandle->height;
                LOGI(" spu->pts:%x, spu->spu_width is 0x%x,  spu->spu_height=0x%x\n  spu->spu_width is %u,  spu->spu_height=%u\n", spu->pts, avihandle->width, avihandle->height, spu->spu_width, spu->spu_height);
                spu->rgba_enable = 1;   // XSUB
                //FFT:The background pixels are 100% transparent
                spu->rgba_background =
                    (unsigned)avihandle->background.
                    red << 16 | (unsigned)avihandle->background.
                    green << 8 | (unsigned)avihandle->background.
                    blue | 0 << 24;
                spu->rgba_pattern1 =
                    (unsigned)avihandle->pattern1.
                    red << 16 | (unsigned)avihandle->pattern1.
                    green << 8 | (unsigned)avihandle->pattern1.
                    blue | 0xff << 24;
                spu->rgba_pattern2 =
                    (unsigned)avihandle->pattern2.
                    red << 16 | (unsigned)avihandle->pattern2.
                    green << 8 | (unsigned)avihandle->pattern2.
                    blue | 0xff << 24;
                spu->rgba_pattern3 =
                    (unsigned)avihandle->pattern3.
                    red << 16 | (unsigned)avihandle->pattern3.
                    green << 8 | (unsigned)avihandle->pattern3.
                    blue | 0xff << 24;
                LOGI(" spu->rgba_background == 0x%x,  spu->rgba_pattern1 == 0x%x\n", spu->rgba_background, spu->rgba_pattern1);
                LOGI(" spu->rgba_pattern2 == 0x%x,  spu->rgba_pattern3 == 0x%x\n", spu->rgba_pattern2, spu->rgba_pattern3);
                ptrPXDRead = (unsigned short *) & (avihandle->rleData);
                //if has alpha, header had another 4 bytes
                if (has_alpha)
                    ptrPXDRead += 2;

                FillPixel(ptrPXDRead, spu->spu_data, 1, spu,
                          avihandle->field_offset);
                //ptrPXDRead =
                //    (unsigned long *)((unsigned long)(&avihandle->rleData) +
                //                       (unsigned long)(avihandle->field_offset));
                FillPixel(ptrPXDRead, spu->spu_data + VOB_SUB_SIZE / 2,
                          2, spu, avihandle->field_offset);
                ret = 0;
                break;
            case 0x17008:   //XSUB HD
            case 0x17009:   //XSUB+ (XSUA HD)
                duration_pts = spu_buf_piece[rd_oft++] << 24;
                duration_pts |= spu_buf_piece[rd_oft++] << 16;
                duration_pts |= spu_buf_piece[rd_oft++] << 8;
                duration_pts |= spu_buf_piece[rd_oft++];
                LOGI("duration_pts is %d, current_length=%d  ,rd_oft is %d\n", duration_pts, current_length, rd_oft);
                avihandle_hd =
                    (DivXSubPictHdr_HD *)(spu_buf_piece + rd_oft);
                spu->spu_data = malloc(VOB_SUB_SIZE);
                memset(spu->spu_data, 0, VOB_SUB_SIZE);
                sublen = 50;
                spu->subtitle_type = SUBTITLE_VOB;
                spu->buffer_size = VOB_SUB_SIZE;
                {
                    unsigned char *s = &(avihandle_hd->duration[0]);
                    spu->pts = str2ms(s) * 90;
                    s = &(avihandle_hd->duration[13]);
                    spu->m_delay = str2ms(s) * 90;
                }
                spu->spu_width = avihandle_hd->width;
                spu->spu_height = avihandle_hd->height;
                LOGI(" spu->spu_width is 0x%x,  spu->spu_height=0x%x\n  spu->spu_width is %u,  spu->spu_height=%u\n", avihandle_hd->width, avihandle_hd->height, spu->spu_width, spu->spu_height);
                spu->rgba_enable = 1;   // XSUB
                spu->rgba_background =
                    (unsigned)avihandle_hd->background.
                    red << 16 | (unsigned)avihandle_hd->background.
                    green << 8 | (unsigned)avihandle_hd->background.
                    blue | avihandle_hd->background_transparency << 24;
                spu->rgba_pattern1 =
                    (unsigned)avihandle_hd->pattern1.
                    red << 16 | (unsigned)avihandle_hd->pattern1.
                    green << 8 | (unsigned)avihandle_hd->pattern1.
                    blue | avihandle_hd->pattern1_transparency << 24;
                spu->rgba_pattern2 =
                    (unsigned)avihandle_hd->pattern2.
                    red << 16 | (unsigned)avihandle_hd->pattern2.
                    green << 8 | (unsigned)avihandle_hd->pattern2.
                    blue | avihandle_hd->pattern2_transparency << 24;
                spu->rgba_pattern3 =
                    (unsigned)avihandle_hd->pattern3.
                    red << 16 | (unsigned)avihandle_hd->pattern3.
                    green << 8 | (unsigned)avihandle_hd->pattern3.
                    blue | avihandle_hd->pattern3_transparency << 24;
                LOGI(" avihandle_hd->background.red == 0x%x,  avihandle_hd->background.green == 0x%x\n", avihandle_hd->background.red, avihandle_hd->background.green);
                LOGI(" avihandle_hd->background.blue == 0x%x,  avihandle_hd->background_transparency == 0x%x\n\n", avihandle_hd->background.blue, avihandle_hd->background_transparency);
                LOGI(" avihandle_hd->pattern1.red == 0x%x,  avihandle_hd->pattern1.green == 0x%x\n", avihandle_hd->pattern1.red, avihandle_hd->pattern1.green);
                LOGI(" avihandle_hd->pattern1.blue == 0x%x,	avihandle_hd->pattern1_transparency == 0x%x\n\n", avihandle_hd->pattern1.blue, avihandle_hd->pattern1_transparency);
                LOGI(" avihandle_hd->pattern2.red == 0x%x,  avihandle_hd->pattern2.green == 0x%x\n", avihandle_hd->pattern2.red, avihandle_hd->pattern2.green);
                LOGI(" avihandle_hd->pattern2.blue == 0x%x,	avihandle_hd->pattern@_transparency == 0x%x\n\n", avihandle_hd->pattern2.blue, avihandle_hd->pattern2_transparency);
                LOGI(" avihandle_hd->pattern3.red == 0x%x,  avihandle_hd->pattern3.green == 0x%x\n", avihandle_hd->pattern3.red, avihandle_hd->pattern3.green);
                LOGI(" avihandle_hd->pattern3.blue == 0x%x,	avihandle_hd->pattern3_transparency == 0x%x\n\n", avihandle_hd->pattern3.blue, avihandle_hd->pattern3_transparency);
                LOGI(" spu->rgba_background == 0x%x,  spu->rgba_pattern1 == 0x%x\n", spu->rgba_background, spu->rgba_pattern1);
                LOGI(" spu->rgba_pattern2 == 0x%x,  spu->rgba_pattern3 == 0x%x\n", spu->rgba_pattern2, spu->rgba_pattern3);
                ptrPXDRead = (unsigned short *) & (avihandle_hd->rleData);
                FillPixel(ptrPXDRead, spu->spu_data, 1, spu,
                          avihandle_hd->field_offset);
                ptrPXDRead =
                    (unsigned long *)((unsigned long)(&avihandle_hd->rleData) +
                                       (unsigned long)(avihandle_hd->
                                             field_offset));
                FillPixel(ptrPXDRead, spu->spu_data + VOB_SUB_SIZE / 2,
                          2, spu, avihandle_hd->field_offset);
                ret = 0;
                break;
            case 0x1700a:   //mkv internel image
                duration_pts = spu_buf_piece[rd_oft++] << 24;
                duration_pts |= spu_buf_piece[rd_oft++] << 16;
                duration_pts |= spu_buf_piece[rd_oft++] << 8;
                duration_pts |= spu_buf_piece[rd_oft++];
                restlen -= 4;
                LOGI("duration_pts is %d\n", duration_pts);
            case 0x17000:   //vob internel image
                sublen = 50;
                spu->subtitle_type = SUBTITLE_VOB;
                spu->buffer_size = VOB_SUB_SIZE;
                spu->spu_data = malloc(VOB_SUB_SIZE);
                memset(spu->spu_data, 0, VOB_SUB_SIZE);
                spu->pts = current_pts;
                ret =
                    get_vob_spu(spu_buf_piece + rd_oft, &restlen,
                                current_length, spu);
                if (current_type == 0x17000 || current_type == 0x1700a)
                {
                    LOGI("## ret=%d, restlen=%d, sizeflag=%d, restbuf=%x,%x, ---\n", ret, restlen, sizeflag, restbuf, restbuf ? restbuf[0] : 0);
                    if (restlen < 0)
                    {
                        LOGI("Warning restlen <0, set to 0\n");
                        restlen = 0;
                    }
                    if (restlen)
                    {
                        if (restbuf)
                        {
                            free(restbuf);
                            restbuf = NULL;
                        }
                        restbuf = malloc(restlen);
                        memcpy(restbuf,
                               spu_buf_piece + rd_oft + ret,
                               restlen);
                        //LOGI("## %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,-----\n",
                        //    restbuf[0],restbuf[1],restbuf[2],restbuf[3],restbuf[4],restbuf[5],restbuf[6],restbuf[7],
                        //    restbuf[8],restbuf[9],restbuf[10],restbuf[11],restbuf[12],restbuf[13],restbuf[14],restbuf[15],
                        //    restbuf[16],restbuf[17],restbuf[18],restbuf[19],restbuf[20],restbuf[21],restbuf[22],restbuf[23]);
                        if ((restbuf[0] == 0x41)
                                && (restbuf[1] == 0x4d)
                                && (restbuf[2] == 0x4c)
                                && (restbuf[3] == 0x55)
                                && ((restbuf[4] == 0xaa) || (restbuf[4] == 0x77)))
                        {
                            LOGI("## sub header found ! restbuf=%x,%x, ---\n", restbuf, restbuf[0]);
                            sizeflag = restlen;
                        }
                        else
                        {
                            LOGI("## no header found, free restbuf! ---\n");
                            free(restbuf);
                            restbuf = NULL;
                            restlen = sizeflag = 0;
                        }
                    }
                }
                else
                {
                    restlen = 0;
                }
                //              {
                //                  int fd =open("/sdcard/subtitle.rawdata", O_RDWR|O_CREAT );
                //                  if(fd!=-1)
                //                  {
                //                      write(fd,spu->spu_data,VOB_SUB_SIZE);
                //                      close(fd);
                //                  }
                //
                //              }
                break;
            case 0x17002:   //mkv internel utf-8
            case 0x17004:   //mkv internel ssa
            case 0x17808:   //mkv internel SUBRIP
            case 0x1780d:   //mkv internel ass
                duration_pts = spu_buf_piece[rd_oft++] << 24;
                duration_pts |= spu_buf_piece[rd_oft++] << 16;
                duration_pts |= spu_buf_piece[rd_oft++] << 8;
                duration_pts |= spu_buf_piece[rd_oft++];
                sublen = 1000;
                spu->subtitle_type = SUBTITLE_SSA;
                spu->buffer_size = current_length + 1;  //256*(current_length/256+1);
                spu->spu_data = malloc(spu->buffer_size);
                memset(spu->spu_data, 0, spu->buffer_size);
                spu->pts = current_pts;
                spu->m_delay = duration_pts;
                if (duration_pts != 0)
                {
                    spu->m_delay += current_pts;
                }
                memcpy(spu->spu_data, spu_buf_piece + rd_oft,
                       current_length);
                get_ass_spu(spu->spu_data, spu->buffer_size, spu);
                LOGI("CODEC_ID_SSA   size is:    %u ,data is:    %s, current_length=%d\n", spu->buffer_size, spu->spu_data, current_length);
                ret = 0;
                break;
            case 0x17005:
                duration_pts = spu_buf_piece[rd_oft++] << 24;
                duration_pts |= spu_buf_piece[rd_oft++] << 16;
                duration_pts |= spu_buf_piece[rd_oft++] << 8;
                duration_pts |= spu_buf_piece[rd_oft++];
                sublen = 1000;
                spu->subtitle_type = SUBTITLE_TMD_TXT;
                spu->buffer_size = current_length + 1;
                spu->spu_data = malloc(spu->buffer_size);
                memset(spu->spu_data, 0, spu->buffer_size);
                spu->pts = current_pts;
                spu->m_delay = duration_pts;
                if (duration_pts != 0)
                {
                    spu->m_delay += current_pts;
                }
                rd_oft += 2;
                current_length -= 2;
                if (current_length == 0)
                    ret = -1;
                memcpy(spu->spu_data, spu_buf_piece + rd_oft,
                       current_length);
                LOGI("CODEC_ID_TIME_TEXT   size is:    %u ,data is:    %s, current_length=%d\n", spu->buffer_size, spu->spu_data, current_length);
                ret = 0;
                break;
            default:
                ret = -1;
                break;
        }
        if (ret < 0)
            goto error;
        write_subtitle_file(spu);
        add_file_position();
    }
error:
    //LOGI("[%s::%d] error! spu_buf=%x, \n", __FUNCTION__, __LINE__, spu_buf);
    if (spu_buf)
    {
        free(spu_buf);
        spu_buf = NULL;
    }
    return ret;
}

int release_spu(AML_SPUVAR *spu)
{
    if (spu->spu_data)
        free(spu->spu_data);
    spu->spu_data = NULL;
    return 0;
}

int set_int_value(int value, char *data, int *pos)
{
    data[0] = (value >> 24) & 0xff;
    data[1] = (value >> 16) & 0xff;
    data[2] = (value >> 8) & 0xff;
    data[3] = value & 0xff;
    *pos += 4;
    return 0;
}

int set_short_value(unsigned short value, char *data, int *pos)
{
    data[0] = (value >> 8) & 0xff;
    data[1] = value & 0xff;
    *pos += 2;
    return 0;
}

int init_subtitle_file(int need_close)
{
    if (need_close == 1) {
        close_subtitle();
    }
    init_pgs_subtitle();
    dvbsub_init_decoder();
    dvdsub_init_decoder();
    teletext_init_decoder();
    subtitle_status = SUB_INIT;
    restlen = 0;
    return 0;
}

int add_sub_end_time(int end_time)
{
    if (DEC_SUBTITLE_POSITION(file_position) >= 0
            && inter_subtitle_data[DEC_SUBTITLE_POSITION(file_position)].data)
    {
        inter_subtitle_data[DEC_SUBTITLE_POSITION(file_position)].
        subtitle_delay_pts = end_time;
        LOGI("add file_position %d read_position is %d\n",
             DEC_SUBTITLE_POSITION(file_position), read_position);
    }
    //if(read_position > DEC_SUBTITLE_POSITION(file_position))
    //use jni fun to clear subtitle
    return 0;
}

/*
write subtitle to file:SUBTITLE_FILE
first 4 bytes are sync bytes:0x414d4c55(AMLU)
next  1 byte  is  subtitle type
next  4 bytes are subtitle pts
mext  4 bytes arg subtitle delay
next  2 bytes are subtitle start x pos
next  2 bytes are subtitel start y pos
next  2 bytes are subtitel width
next  2 bytes are subtitel height
next  2 bytes are subtitle alpha
next  4 bytes are subtitel size
next  n bytes are subtitle data
*/
int write_subtitle_file(AML_SPUVAR *spu)
{
    if (spu->pts <
            inter_subtitle_data[DEC_SUBTITLE_POSITION(file_position)].
            subtitle_pts)
    {
        LOGI("inter_subtitle_data[%d].subtitle_pts %d",
             DEC_SUBTITLE_POSITION(file_position),
             inter_subtitle_data[DEC_SUBTITLE_POSITION(file_position)].
             subtitle_pts);
        //      close_subtitle();
    }
    while (ADD_SUBTITLE_POSITION(file_position) == read_position)
    {
        LOGI("## write_subtitle_file wait file_pos=%d,read_pos=%d,-----------\n", ADD_SUBTITLE_POSITION(file_position), read_position);
        usleep(100000);
    }
    lp_lock(&sublock);
    if (inter_subtitle_data[file_position].data)
        free(inter_subtitle_data[file_position].data);
    inter_subtitle_data[file_position].data = NULL;
    inter_subtitle_data[file_position].subtitle_type = spu->subtitle_type;
    inter_subtitle_data[file_position].data = spu->spu_data;
    //  inter_subtitle_data[file_position].data_size = VOB_SUBTITLE_FRAMW_SIZE;    //?? need change for text sub
    inter_subtitle_data[file_position].data_size = spu->buffer_size;    //?? need change for text sub
    inter_subtitle_data[file_position].subtitle_pts = spu->pts;
    inter_subtitle_data[file_position].subtitle_delay_pts = spu->m_delay;
    inter_subtitle_data[file_position].sub_alpha = spu->spu_alpha;
    inter_subtitle_data[file_position].subtitle_width = spu->spu_width;
    inter_subtitle_data[file_position].subtitle_height = spu->spu_height;
    inter_subtitle_data[file_position].resize_width = spu->spu_width;
    inter_subtitle_data[file_position].resize_height = spu->spu_height;
    inter_subtitle_data[file_position].rgba_enable = spu->rgba_enable;
    inter_subtitle_data[file_position].rgba_background = spu->rgba_background;
    inter_subtitle_data[file_position].rgba_pattern1 = spu->rgba_pattern1;
    inter_subtitle_data[file_position].rgba_pattern2 = spu->rgba_pattern2;
    inter_subtitle_data[file_position].rgba_pattern3 = spu->rgba_pattern3;
    LOGI(" write_subtitle_file[%d], sublen=%d, subtitle_type is 0x%x size: %d  subtitle_pts =%u,subtitle_delay_pts=%u \n",file_position,sublen,inter_subtitle_data[read_position].subtitle_type,
        inter_subtitle_data[file_position].data_size,inter_subtitle_data[file_position].subtitle_pts,inter_subtitle_data[file_position].subtitle_delay_pts);
    if (spu->subtitle_type == SUBTITLE_PGS)
    {
        inter_subtitle_type = SUBTITLE_PGS;
        file_position = ADD_SUBTITLE_POSITION(file_position);
    }
    else if(spu->subtitle_type == SUBTITLE_DVB){
        inter_subtitle_data[file_position].spu_origin_display_w = spu->spu_origin_display_w;
        inter_subtitle_data[file_position].spu_origin_display_h = spu->spu_origin_display_h;
        inter_subtitle_data[file_position].spu_start_x = spu->spu_start_x;
        inter_subtitle_data[file_position].spu_start_y = spu->spu_start_y;

        LOGI("spu_origin_display[%d,%d],start[%d,%d]-\n",inter_subtitle_data[file_position].spu_origin_display_w,
            inter_subtitle_data[file_position].spu_origin_display_h,inter_subtitle_data[file_position].spu_start_x,
            inter_subtitle_data[file_position].spu_start_y);
        inter_subtitle_type = SUBTITLE_DVB;
        file_position = ADD_SUBTITLE_POSITION(file_position);
    }
    else if(spu->subtitle_type == SUBTITLE_DVB_TELETEXT){
        inter_subtitle_data[file_position].spu_origin_display_w = spu->spu_origin_display_w;
        inter_subtitle_data[file_position].spu_origin_display_h = spu->spu_origin_display_h;
        inter_subtitle_data[file_position].spu_start_x = spu->spu_start_x;
        inter_subtitle_data[file_position].spu_start_y = spu->spu_start_y;

        LOGI("spu_origin_display[%d,%d],start[%d,%d]-\n",inter_subtitle_data[file_position].spu_origin_display_w,
            inter_subtitle_data[file_position].spu_origin_display_h,inter_subtitle_data[file_position].spu_start_x,
            inter_subtitle_data[file_position].spu_start_y);
        inter_subtitle_type = SUBTITLE_DVB_TELETEXT;
        file_position = ADD_SUBTITLE_POSITION(file_position);
    }
    else if (spu->subtitle_type == SUBTITLE_TMD_TXT)
    {
        inter_subtitle_type = SUBTITLE_TMD_TXT;
    }
    else
    {
        inter_subtitle_type = 0;
    }
    lp_unlock(&sublock);
    return 0;
}

int read_subtitle_file()
{
    LOGI("subtitle data address is %p\n", inter_subtitle_data[file_position].data);
    return 0;
}

int get_inter_spu_packet(int pts)
{
    lp_lock(&sublock);
    LOGI(" search pts %d , s %d \n", pts, pts / 90);
    int storenumber =
        (file_position >=
         read_position) ? file_position - read_position : sublen +
        file_position - 1 - read_position;
    LOGI("inter_subtitle_data[%d].subtitle_pts is %d storenumber=%d end_time %d\n", read_position, inter_subtitle_data[read_position].subtitle_pts, storenumber, inter_subtitle_data[read_position].subtitle_delay_pts);
    int i;
    for (i = 0; i < storenumber - 1; i++)
    {
        if (pts >= inter_subtitle_data[ADD_SUBTITLE_POSITION(read_position)].subtitle_pts
            && (inter_subtitle_data[read_position].subtitle_pts != inter_subtitle_data[ADD_SUBTITLE_POSITION(read_position)].subtitle_pts))
            read_position = ADD_SUBTITLE_POSITION(read_position);
        else
            break;
    }
    if (get_inter_spu_type() != SUBTITLE_PGS)
    {
        if (inter_subtitle_data[read_position].subtitle_pts > pts)
        {
            int tmp_read_position =
                ADD_SUBTITLE_POSITION(read_position);
            for (i = 0; i < storenumber - 2; i++)
            {
                LOGI("get_inter_spu_packet  pts discontinue read_position is %d  file_position is %d, tmp_read_position is %d, tmp_read_position pts is %d\n", read_position, file_position, tmp_read_position, inter_subtitle_data[read_position].subtitle_pts);
                if (pts >=
                        inter_subtitle_data[tmp_read_position].
                        subtitle_pts)
                {
                    if (pts >=
                            inter_subtitle_data
                            [ADD_SUBTITLE_POSITION
                             (tmp_read_position)].
                            subtitle_pts)
                    {
                        tmp_read_position =
                            ADD_SUBTITLE_POSITION
                            (tmp_read_position);
                    }
                    else
                    {
                        read_position =
                            tmp_read_position;
                        break;
                    }
                }
                else
                {
                    tmp_read_position =
                        ADD_SUBTITLE_POSITION
                        (tmp_read_position);
                }
            }
        }
        if ((inter_subtitle_data[read_position].subtitle_pts > pts)
                || (inter_subtitle_data[read_position].subtitle_pts + 10 * 90000) < pts)
        {
            lp_unlock(&sublock);
            return -1;
        }
    }
    if (storenumber == 0)
    {
        lp_unlock(&sublock);
        return -1;
    }
    LOGI("get_inter_spu_packet  read_position is %d  file_position is %d  ,time is %d\n", read_position, file_position, inter_subtitle_data[read_position].subtitle_pts);
    lp_unlock(&sublock);
    return read_position;
}

int get_inter_spu_type()
{
    LOGI(" inter_subtitle_data[%d] subtitle_type is 0x%x\n", read_position,
         inter_subtitle_data[read_position].subtitle_type);
    return inter_subtitle_data[read_position].subtitle_type;
}

int get_inter_sub_type()
{
    return inter_subtitle_type;
}

int get_subtitle_buffer_size()
{
    return getSize(aml_sub_handle);
}

int get_inter_spu_size()
{
    if (get_inter_spu_type() == SUBTITLE_VOB)
    {
        int subtitle_width =
            inter_subtitle_data[read_position].subtitle_width;
        int subtitle_height =
            inter_subtitle_data[read_position].subtitle_height;
        if (subtitle_width * subtitle_height == 0)
            return 0;
        int buffer_width = (subtitle_width + 63) & 0xffffffc0;
        LOGI("buffer width is %d\n", buffer_width);
        LOGI("buffer height is %d\n", subtitle_height);
        return buffer_width * subtitle_height;
    }
    else if (get_inter_spu_type() == SUBTITLE_SSA)
    {
        LOGI(" inter_subtitle_data[%d] data_size is 0x%x\n",
             read_position,
             inter_subtitle_data[read_position].data_size);
        return inter_subtitle_data[read_position].data_size;
    }
    else if (get_inter_spu_type() == SUBTITLE_PGS)
    {
        LOGI(" inter_subtitle_data[%d] data_size is 0x%x\n",
             read_position,
             inter_subtitle_data[read_position].data_size);
        return inter_subtitle_data[read_position].data_size / 4;
    }
    else if (get_inter_spu_type() == SUBTITLE_DVB)
    {
        int subtitle_width =
            inter_subtitle_data[read_position].subtitle_width;
        int subtitle_height =
            inter_subtitle_data[read_position].subtitle_height;
        if (subtitle_width * subtitle_height == 0)
            return 0;
        int buffer_width = (subtitle_width + 63) & 0xffffffc0;
        LOGI("## subtitle width is %d: subtitle height is %d, buffer width is %d, size=%d,\n", subtitle_width, subtitle_height, buffer_width, buffer_width * subtitle_height);
        return buffer_width * subtitle_height;
    }
    else if (get_inter_spu_type() == SUBTITLE_DVB_TELETEXT)
    {
        int subtitle_width =
            inter_subtitle_data[read_position].subtitle_width;
        int subtitle_height =
            inter_subtitle_data[read_position].subtitle_height;
        if (subtitle_width * subtitle_height == 0)
            return 0;
        int buffer_width = (subtitle_width + 63) & 0xffffffc0;
        LOGI("## subtitle width is %d: subtitle height is %d, buffer width is %d, size=%d,\n", subtitle_width, subtitle_height, buffer_width, buffer_width * subtitle_height);
        return buffer_width * subtitle_height;
    }
    else if (get_inter_spu_type() == SUBTITLE_TMD_TXT)
    {
        LOGI(" inter_subtitle_data[%d] data_size is 0x%x\n",
             read_position,
             inter_subtitle_data[read_position].data_size);
        return inter_subtitle_data[read_position].data_size;
    }
    return 0;
}

void free_inter_spu_data()
{
    if (inter_subtitle_data[read_position].data)
    {
        free(inter_subtitle_data[read_position].data);
        inter_subtitle_data[read_position].data = NULL;
    }
    return;
}

void free_last_inter_spu_data()
{
    int tmp_pos = 0;
    if (read_position > 0)
    {
        tmp_pos = read_position - 1;
    }
    else
    {
        tmp_pos = sublen - 1;
    }

    if (inter_subtitle_data[tmp_pos].data)
    {
        free(inter_subtitle_data[tmp_pos].data);
        inter_subtitle_data[tmp_pos].data = NULL;
    }
    return;
}

char *get_inter_spu_data()
{
    return inter_subtitle_data[read_position].data;
}

int get_inter_spu_width()
{
    return inter_subtitle_data[read_position].resize_width;
    //return ((inter_subtitle_data[read_position].subtitle_width+63)&0xffffffc0);
}

int get_inter_spu_height()
{
    return inter_subtitle_data[read_position].resize_height;
    //return inter_subtitle_data[read_position].subtitle_height;
}

int get_inter_spu_origin_width()
{
    return inter_subtitle_data[read_position].spu_origin_display_w;
}

int get_inter_spu_origin_height()
{
    return inter_subtitle_data[read_position].spu_origin_display_h;
}

int get_inter_start_x()
{
    return inter_subtitle_data[read_position].spu_start_x;
}

int get_inter_start_y()
{
    return inter_subtitle_data[read_position].spu_start_y;
}

unsigned get_inter_spu_pts()
{
    return inter_subtitle_data[read_position].subtitle_pts;
}

unsigned get_inter_spu_delay()
{
    return inter_subtitle_data[read_position].subtitle_delay_pts;
}

int get_inter_spu_resize_size()
{
    return inter_subtitle_data[read_position].resize_size;
}

int add_read_position()
{
    lp_lock(&sublock);
    read_position = ADD_SUBTITLE_POSITION(read_position);
    LOGI("read_position is %d\n\n", read_position);
    lp_unlock(&sublock);
    return 0;
}

int add_file_position()
{
    lp_lock(&sublock);
    file_position = ADD_SUBTITLE_POSITION(file_position);
    lp_unlock(&sublock);
    return 0;
}

int fill_resize_data(int *dst_data, int *src_data)
{
    if (inter_subtitle_data[read_position].resize_size ==
            get_inter_spu_size())
    {
        memcpy(dst_data, src_data,
               inter_subtitle_data[read_position].resize_size * 4);
        return 0;
    }
    int y_start = inter_subtitle_data[read_position].resize_ystart;
    int x_start = inter_subtitle_data[read_position].resize_xstart;
    int y_end = y_start + inter_subtitle_data[read_position].resize_height;
    int resize_width = inter_subtitle_data[read_position].resize_width;
    int buffer_width = inter_subtitle_data[read_position].subtitle_width;
    int buffer_height = inter_subtitle_data[read_position].subtitle_height;
    int buffer_width_size = (buffer_width + 63) & 0xffffffc0;
    int *resize_src_data = src_data + buffer_width_size * y_start;
    int i = y_start;
    for (; i < y_end; i++)
    {
        memcpy(dst_data + (resize_width * (i - y_start)),
               resize_src_data + (buffer_width_size * (i - y_start)) +
               x_start, resize_width * 4);
    }
    return 0;
}

int *parser_inter_spu(int *buffer)
{
    LOGI("enter parser_inter_sup \n\n");
    unsigned short i = 0, j = 0;
    unsigned char *data = NULL, *data2 = NULL;
    unsigned char color = 0;
    unsigned *result_buf = (unsigned *)buffer;
    unsigned index = 0, index1 = 0;
    unsigned char n = 0;
    unsigned short buffer_width, buffer_height;
    int start_height = -1, end_height = 0;
    buffer_width = inter_subtitle_data[read_position].subtitle_width;
    buffer_height = inter_subtitle_data[read_position].subtitle_height;
    int resize_width = buffer_width, resize_height;
    int x_start = buffer_width, x_end = 0;
    unsigned data_byte = (((buffer_width * 2) + 15) >> 4) << 1;
    //LOGI("data_byte is %d\n\n",data_byte);
    int buffer_width_size = (buffer_width + 63) & 0xffffffc0;
    //LOGI("buffer_width is %d\n\n",buffer_width_size);
    unsigned short subtitle_alpha =
        inter_subtitle_data[read_position].sub_alpha;
    LOGI("subtitle_alpha is %x\n\n", subtitle_alpha);
    unsigned int RGBA_Pal[4];
    RGBA_Pal[0] = RGBA_Pal[1] = RGBA_Pal[2] = RGBA_Pal[3] = 0;
#if 0
    if ((subtitle_alpha == 0xff0))
    {
        RGBA_Pal[2] = 0xffffffff;
        RGBA_Pal[1] = 0xff0000ff;
    }
    else if ((subtitle_alpha == 0xfff0))
    {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[2] = 0xff000000;
        RGBA_Pal[3] = 0xff000000;
    }
    else if ((subtitle_alpha == 0xf0f0))
    {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[3] = 0xff000000;
    }
    else if (subtitle_alpha == 0xf0ff)
    {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[2] = 0xff000000;
        RGBA_Pal[3] = 0xff000000;
    }
    else if ((subtitle_alpha == 0xff00))
    {
        RGBA_Pal[2] = 0xffffffff;
        RGBA_Pal[3] = 0xff000000;
    }
    else if (subtitle_alpha == 0xfe0)
    {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[2] = 0xff000000;
        RGBA_Pal[3] = 0;
    }
    else
    {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[3] = 0xff000000;
    }
#else
    if (inter_subtitle_data[read_position].rgba_enable)
    {
        RGBA_Pal[0] =
            inter_subtitle_data[read_position].rgba_background;
        RGBA_Pal[1] = inter_subtitle_data[read_position].rgba_pattern1;
        RGBA_Pal[2] = inter_subtitle_data[read_position].rgba_pattern2;
        RGBA_Pal[3] = inter_subtitle_data[read_position].rgba_pattern3;
        LOGI(" RGBA_Pal[0] == 0x%x, RGBA_Pal[1] == 0x%x\n", RGBA_Pal[0],
             RGBA_Pal[1]);
        LOGI(" RGBA_Pal[2] == 0x%x,	RGBA_Pal[3] == 0x%x\n",
             RGBA_Pal[2], RGBA_Pal[3]);
    }
    else if (subtitle_alpha & 0xf000 && subtitle_alpha & 0x0f00
             && subtitle_alpha & 0x00f0)
    {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[2] = 0xff000000;
        RGBA_Pal[3] = 0xff000000;
    }
    else if (subtitle_alpha == 0xfe0)
    {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[2] = 0xff000000;
        RGBA_Pal[3] = 0;
    }
    else
    {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[3] = 0xff000000;
    }
#endif
    for (i = 0; i < buffer_height; i++)
    {
        if (i & 1)
            data =
                inter_subtitle_data[read_position].data +
                (i >> 1) * data_byte + (VOB_SUB_SIZE / 2);
        else
            data =
                inter_subtitle_data[read_position].data +
                (i >> 1) * data_byte;
        index = 0;
        for (j = 0; j < buffer_width; j++)
        {
            index1 = index % 2 ? index - 1 : index + 1;
            n = data[index1];
            index++;
            if (start_height < 0)
            {
                start_height = i;
                //start_height = (start_height%2)?(start_height-1):start_height;
            }
            end_height = i;
            if (j < x_start)
                x_start = j;
            result_buf[i * (buffer_width_size) + j] =
                RGBA_Pal[(n >> 6) & 0x3];
            if (++j >= buffer_width)
                break;
            result_buf[i * (buffer_width_size) + j] =
                RGBA_Pal[(n >> 4) & 0x3];
            if (++j >= buffer_width)
                break;
            result_buf[i * (buffer_width_size) + j] =
                RGBA_Pal[(n >> 2) & 0x3];
            if (++j >= buffer_width)
                break;
            result_buf[i * (buffer_width_size) + j] =
                RGBA_Pal[n & 0x3];
            if (j > x_end)
                x_end = j;
        }
    }
    //end_height = (end_height%2)?(((end_height+1)<=buffer_height)?(end_height+1):end_height):end_height;
    inter_subtitle_data[read_position].resize_xstart = x_start;
    inter_subtitle_data[read_position].resize_ystart = start_height;
    inter_subtitle_data[read_position].resize_width =
        (x_end - x_start + 1 + 63) & 0xffffffc0;
    inter_subtitle_data[read_position].resize_height =
        end_height - start_height + 1;
    inter_subtitle_data[read_position].resize_size =
        inter_subtitle_data[read_position].resize_height *
        inter_subtitle_data[read_position].resize_width;
    LOGI("resize startx is %d\n\n",
         inter_subtitle_data[read_position].resize_xstart);
    LOGI("resize starty is %d\n\n",
         inter_subtitle_data[read_position].resize_ystart);
    LOGI("resize height is %d\n\n",
         inter_subtitle_data[read_position].resize_height);
    LOGI("resize_width is %d\n\n",
         inter_subtitle_data[read_position].resize_width);
    return (result_buf + start_height * buffer_width_size);
    //ADD_SUBTITLE_POSITION(read_position);
    return NULL;
}

int *parser_inter_spu2(int *buffer)
{
    LOGI("enter parser_inter_sup \n\n");
    unsigned char *data = NULL;
    int i = 0, j = 0;
    unsigned int *result_buf = (unsigned int *)buffer;
    unsigned short buffer_width, buffer_height;
    int start_height = -1, end_height = 0;
    int x_start = buffer_width, x_end = 0;
    buffer_width = inter_subtitle_data[read_position].subtitle_width;
    buffer_height = inter_subtitle_data[read_position].subtitle_height;
    int buffer_width_size = (buffer_width + 63) & 0xffffffc0;
    LOGI("(width=%d,height=%d),buffer_width_size=%d, -----\n", buffer_width,
         buffer_height, buffer_width_size);
    for (i = 0; i < buffer_height; i++)
    {
        data =
            inter_subtitle_data[read_position].data +
            i * buffer_width * 4;
        for (j = 0; j < buffer_width; j++)
        {
            data += 4;
            if (start_height < 0)
            {
                start_height = i;
            }
            end_height = i;
            if (j < x_start)
                x_start = j;
            result_buf[i * (buffer_width_size) + j] = data[0] << 24;
            result_buf[i * (buffer_width_size) + j] |=
                data[1] << 16;
            result_buf[i * (buffer_width_size) + j] |= data[2] << 8;
            result_buf[i * (buffer_width_size) + j] |= data[3];
            //if ((i<2) && (j<8))
            //    LOGI("##, parser_inter_spu2:[%d][%d] %d, %4x,-- %4x,%4x,%4x,%4x,%4x,%4x,%4x,%4x,---\n",j,i,(i*(buffer_width_size)+j), result_buf[i*(buffer_width_size)+j],data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
            if (j > x_end)
                x_end = j;
        }
    }
    LOGI("##, parser_inter_spu2:addr:%4x, %4x,%4x,%4x,%4x,%4x,%4x,%4x,%4x,---\n", result_buf, result_buf[0], result_buf[1], result_buf[2], result_buf[3], result_buf[4], result_buf[5], result_buf[6], result_buf[7]);
    inter_subtitle_data[read_position].resize_xstart = x_start;
    inter_subtitle_data[read_position].resize_ystart = start_height;
    inter_subtitle_data[read_position].resize_width =
        (x_end - x_start + 1 + 63) & 0xffffffc0;
    inter_subtitle_data[read_position].resize_height =
        end_height - start_height + 1;
    inter_subtitle_data[read_position].resize_size =
        inter_subtitle_data[read_position].resize_height *
        inter_subtitle_data[read_position].resize_width;
    LOGI("resize startx is %d\n\n",
         inter_subtitle_data[read_position].resize_xstart);
    LOGI("resize starty is %d\n\n",
         inter_subtitle_data[read_position].resize_ystart);
    LOGI("resize height is %d\n\n",
         inter_subtitle_data[read_position].resize_height);
    LOGI("resize_width is %d xstart:%d, xend:%d,\n\n",
         inter_subtitle_data[read_position].resize_width, x_start, x_end);
    LOGI("start_heigth is %d end_height:%d buffer_width_size=%d, \n\n",
         start_height, end_height, buffer_width_size);
    return (result_buf + start_height * buffer_width_size);
}

int get_inter_spu()
{
    lp_lock(&handlelock);
    if (aml_sub_handle < 0 && (subtitle_status == SUB_INIT || subtitle_status == SUB_STOP))
    {
        aml_sub_handle = open(SUBTITLE_READ_DEVICE, O_RDONLY);
        lp_unlock(&handlelock);
        return -1;
    }
    if (aml_sub_handle < 0)
    {
        LOGI("subtitle read device open fail\n");
        lp_unlock(&handlelock);
        return 0;
    }

    if (subtitle_status == SUB_INIT)
    {
        subtitle_status = SUB_PLAYING;
    }
    int read_sub_fd = 0;
    AML_SPUVAR spu;
    memset(&spu, 0x0, sizeof(AML_SPUVAR));
    spu.sync_bytes = 0x414d4c55;
    int ret = get_spu(&spu, aml_sub_handle);
    //LOGI("[get_inter_spu]ret is %d\n\n", ret);
    if (ret < 0) {
        lp_unlock(&handlelock);
        return -1;
    }
    //  if(get_subtitle_subtype()==1){
    //      spu.buffer_size = spu.spu_width*spu.spu_height*4;
    //  }
    //
    //  write_subtitle_file(&spu);
    //  free(spu.spu_data);
    //  file_position = ADD_SUBTITLE_POSITION(file_position);
    LOGI("file_position is %d\n\n", file_position);
    LOGI("end parser subtitle success\n");
    lp_unlock(&handlelock);
    return 0;
}

int close_subtitle()
{
    LOGI("----------------------close_subtitle------------------------------aml_sub_handle:%d\n", aml_sub_handle);
    if (subtitle_status == SUB_STOP) {
        return 0;
    }
    subtitle_status = SUB_STOP;
    if (get_subtitle_subtype() == 5)    // dvb sub
    {
        LOGI("waiting for dvb sub stop \n");
        usleep(100000);
    }
    LOGI("start to close subtitle \n");
    lp_lock(&closelock);
    //resetSocketBuffer();
    if (aml_sub_handle >= 0)
    {
        int ret = close(aml_sub_handle);
        aml_sub_handle = -1;
    }
    dvbsub_close_decoder();
    close_pgs_subtitle();
    teletext_close_decoder();
    if (restbuf)
    {
        free(restbuf);
        restbuf = NULL;
    }
    restlen = 0;
    int i = 0;
    for (i = 0; i < sublen; i++)
    {
        if (inter_subtitle_data[i].data)
            free(inter_subtitle_data[i].data);
        inter_subtitle_data[i].data = NULL;
        memset(&(inter_subtitle_data[i]), 0x0, sizeof(subtitle_data_t));
    }
    file_position = 0;
    read_position = 0;
    lp_unlock(&closelock);
    return 0;
}
