/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c file
*/
//header file
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <android/log.h>

//#include "codec.h"
#include "sub_subtitle.h"
#include "sub_vob_sub.h"

#define  LOG_TAG    "sub_jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
static uint32_t palette[16];
static int has_palette = 0;

unsigned short doDCSQC(unsigned char *pdata, unsigned char *pend)
{
    unsigned short cmdDelay, cmdDelaynew;
    unsigned short temp;
    unsigned short cmdAddress;
    int Done, stoped;
    cmdDelay = *pdata++;
    cmdDelay <<= 8;
    cmdDelay += *pdata++;
    cmdAddress = *pdata++;
    cmdAddress <<= 8;
    cmdAddress += *pdata++;
    cmdDelaynew = 0;
    Done = 0;
    stoped = 0;
    while (!Done)
    {
        switch (*pdata)
        {
            case FSTA_DSP:
                pdata++;
                break;
            case STA_DSP:
                pdata++;
                break;
            case STP_DSP:
                pdata++;
                stoped = 1;
                break;
            case SET_COLOR:
                pdata += 3;
                break;
            case SET_CONTR:
                pdata += 3;
                break;
            case SET_DAREA:
                pdata += 7;
                break;
            case SET_DSPXA:
                pdata += 7;
                break;
            case CHG_COLCON:
                temp = *pdata++;
                temp = temp << 8;
                temp += *pdata++;
                pdata += temp;
                break;
            case CMD_END:
                pdata++;
                Done = 1;
                break;
            default:
                pdata = pend;
                Done = 1;
                break;
        }
    }
    if ((pdata < pend) && (stoped == 0))
        cmdDelaynew = doDCSQC(pdata, pend);
    return cmdDelaynew > cmdDelay ? cmdDelaynew : cmdDelay;
}

static int get_spu_cmd(AML_SPUVAR *sub_frame)
{
    unsigned short temp;
    unsigned char *pCmdData;
    unsigned char *pCmdEnd;
    unsigned char data_byte0, data_byte1;
    unsigned char spu_cmd;
    if (sub_frame->cmd_offset >= sub_frame->length)
    {
        LOGI("cmd_offset bigger than frame_size\n\n");
        return -1;  //cmd offset > frame size
    }
    pCmdData = (unsigned char *)(sub_frame->spu_data);
    pCmdEnd = pCmdData + sub_frame->length;
    pCmdData += sub_frame->cmd_offset;
    pCmdData += 4;
    while (pCmdData < pCmdEnd)
    {
        spu_cmd = *pCmdData++;
        switch (spu_cmd)
        {
            case FSTA_DSP:
                sub_frame->display_pending = 2;
                break;
            case STA_DSP:
                sub_frame->display_pending = 1;
                break;
            case STP_DSP:
                sub_frame->display_pending = 0;
                break;
            case SET_COLOR:
                temp = *pCmdData++;
                sub_frame->spu_color = temp << 8;
                temp = *pCmdData++;
                sub_frame->spu_color += temp;
                break;
            case SET_CONTR:
                temp = *pCmdData++;
                sub_frame->spu_alpha = temp << 8;
                temp = *pCmdData++;
                sub_frame->spu_alpha += temp;
                break;
            case SET_DAREA:
                data_byte0 = *pCmdData++;
                data_byte1 = *pCmdData++;
                sub_frame->spu_start_x =
                    ((data_byte0 & 0x3f) << 4) | (data_byte1 >> 4);
                data_byte0 = *pCmdData++;
                sub_frame->spu_width =
                    ((data_byte1 & 0x0f) << 8) | (data_byte0);
                sub_frame->spu_width =
                    sub_frame->spu_width - sub_frame->spu_start_x + 1;
                data_byte0 = *pCmdData++;
                data_byte1 = *pCmdData++;
                sub_frame->spu_start_y =
                    ((data_byte0 & 0x3f) << 4) | (data_byte1 >> 4);
                data_byte0 = *pCmdData++;
                sub_frame->spu_height =
                    ((data_byte1 & 0x0f) << 8) | (data_byte0);
                sub_frame->spu_height =
                    sub_frame->spu_height - sub_frame->spu_start_y + 1;
                if (sub_frame->spu_width > VOB_SUB_WIDTH)
                {
                    sub_frame->spu_width = VOB_SUB_WIDTH;
                }
                if (sub_frame->spu_height > VOB_SUB_HEIGHT)
                {
                    sub_frame->spu_height = VOB_SUB_HEIGHT;
                }
                LOGI("sub_frame->spu_width = %d, sub_frame->spu_height = %d \n", sub_frame->spu_width, sub_frame->spu_height);
                break;
            case SET_DSPXA:
                temp = *pCmdData++;
                sub_frame->top_pxd_addr = temp << 8;
                temp = *pCmdData++;
                sub_frame->top_pxd_addr += temp;
                temp = *pCmdData++;
                sub_frame->bottom_pxd_addr = temp << 8;
                temp = *pCmdData++;
                sub_frame->bottom_pxd_addr += temp;
                break;
            case CHG_COLCON:
                temp = *pCmdData++;
                temp = temp << 8;
                temp += *pCmdData++;
                pCmdData += temp;
                /*
                   uVobSPU.disp_colcon_addr = uVobSPU.point + uVobSPU.point_offset;
                   uVobSPU.colcon_addr_valid = 1;
                   temp = uVobSPU.disp_colcon_addr + temp - 2;

                   uSPU.point = temp & 0x1fffc;
                   uSPU.point_offset = temp & 3;
                 */
                break;
            case CMD_END:
                if (pCmdData <= (pCmdEnd - 6))
                {
                    if ((sub_frame->m_delay =
                                doDCSQC(pCmdData, pCmdEnd - 6)) > 0)
                        sub_frame->m_delay =
                            sub_frame->m_delay * 1024 +
                            sub_frame->pts;
                }
                LOGI("get_spu_cmd parser to the end\n\n");
                return 0;
                break;
            default:
                return -1;
        }
    }
    LOGI("get_spu_cmd can not parser complete\n\n");
    return -1;
}

/**
 * Locale-independent conversion of ASCII isspace.
 */
static int av_isspace(int c)
{
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t'
           || c == '\v';
}

static void parse_palette(char *p)
{
    int i;
    has_palette = 1;
    for (i = 0; i < 16; i++)
    {
        palette[i] = strtoul(p, &p, 16);
        while (*p == ',' || av_isspace(*p))
            p++;
    }
}

static int dvdsub_parse_extradata(char *extradata, int extradata_size)
{
    char *dataorig, *data;
    if (!extradata || !extradata_size)
        return 1;
    extradata += 5;
    extradata_size -= 5;
    dataorig = data = malloc(extradata_size + 1);
    if (!data)
        return 1;
    memcpy(data, extradata, extradata_size);
    data[extradata_size] = '\0';
    for (;;)
    {
        int pos = strcspn(data, "\n\r");
        if (pos == 0 && *data == 0)
            break;
        if (strncmp("palette:", data, 8) == 0)
        {
            parse_palette(data + 8);
        }
        data += pos;
        data += strspn(data, "\n\r");
    }
    free(dataorig);
    return 1;
}

int get_vob_spu(char *spu_buf, int *bufsize, unsigned length, AML_SPUVAR *spu)
{
    //  LOGI("spubuf  %x %x %x %x %x %x %x %x   %x %x %x %x %x %x %x %x  \n %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
    //      spu_buf[0], spu_buf[1],spu_buf[2],spu_buf[3],spu_buf[4],spu_buf[5],spu_buf[6],spu_buf[7],
    //      spu_buf[8],spu_buf[9],spu_buf[10],spu_buf[11],spu_buf[12],spu_buf[13],spu_buf[14],spu_buf[15],
    //      spu_buf[16],spu_buf[17],spu_buf[18],spu_buf[19],spu_buf[20],spu_buf[21],spu_buf[22],spu_buf[23],
    //      spu_buf[24],spu_buf[25],spu_buf[26],spu_buf[27],spu_buf[28],spu_buf[29],spu_buf[30],spu_buf[31] );
    int rd_oft, wr_oft, i;
    unsigned current_length = length;
    int ret = -1;
    char *pixDataOdd = NULL;
    char *pixDataEven = NULL;
    unsigned short *ptrPXDRead;
    rd_oft = 0;
    if (spu_buf[0] == 'E' && spu_buf[1] == 'X' && spu_buf[2] == 'T'
            && spu_buf[3] == 'R' && spu_buf[4] == 'A')
    {
        LOGI("## extradata %x,%x,%x,%x,%x,%x,%x,%x, x,%x,%x,%x,%x,%x,%x,%x, ----------\n", spu_buf[0], spu_buf[1], spu_buf[2], spu_buf[3], spu_buf[4], spu_buf[5], spu_buf[6], spu_buf[7], spu_buf[8], spu_buf[9], spu_buf[10], spu_buf[11], spu_buf[12], spu_buf[13], spu_buf[14], spu_buf[15]);
        dvdsub_parse_extradata(spu_buf, current_length);
        rd_oft += current_length;
        current_length = 0;
        *bufsize -= rd_oft;
        return 0;
    }
    spu->length = spu_buf[0] << 8;
    spu->length |= spu_buf[1];
    spu->cmd_offset = spu_buf[2] << 8;
    spu->cmd_offset |= spu_buf[3];
    memset(spu->spu_data, 0, VOB_SUB_SIZE);
    wr_oft = 0;
    while (spu->length - wr_oft > 0)
    {
        if (!current_length)
        {
            LOGI("current_length is zero\n\n");
            if ((spu_buf[rd_oft++] != 0x41)
                    || (spu_buf[rd_oft++] != 0x4d)
                    || (spu_buf[rd_oft++] != 0x4c)
                    || (spu_buf[rd_oft++] != 0x55)
                    /*|| (spu_buf[rd_oft++] != 0xaa)*/)
            {
                LOGI("## goto error ---------\n");
                goto error; // wrong head
            }
            rd_oft++;//0xaa or 0x77
            rd_oft += 3;    // 3 bytes for type
            current_length = spu_buf[rd_oft++] << 24;
            current_length |= spu_buf[rd_oft++] << 16;
            current_length |= spu_buf[rd_oft++] << 8;
            current_length |= spu_buf[rd_oft++];
            rd_oft += 4;    // 4 bytes for pts
        }
        if ((wr_oft + current_length) <= spu->length)
        {
            memcpy(spu->spu_data + wr_oft, spu_buf + rd_oft,
                   current_length);
            rd_oft += current_length;
            wr_oft += current_length;
            current_length = 0;
        }
        if (wr_oft == spu->length)
        {
            get_spu_cmd(spu);
            spu->frame_rdy = 1;
            break;
        }
    }
    *bufsize -= rd_oft;
    if (has_palette)
    {
        spu->rgba_background =
            (palette[(spu->spu_color) & 0x0f] & 0x00ffffff)
            | ((((spu->spu_alpha) & 0x0f) * 17U) << 24);
        spu->rgba_pattern1 =
            (palette[(spu->spu_color >> 4) & 0x0f] & 0x00ffffff)
            | ((((spu->spu_alpha >> 4) & 0x0f) * 17U) << 24);
        spu->rgba_pattern2 =
            (palette[(spu->spu_color >> 8) & 0x0f] & 0x00ffffff)
            | ((((spu->spu_alpha >> 8) & 0x0f) * 17U) << 24);
        spu->rgba_pattern3 =
            (palette[(spu->spu_color >> 12) & 0x0f] & 0x00ffffff)
            | ((((spu->spu_alpha >> 12) & 0x0f) * 17U) << 24);
        spu->rgba_enable = 1;
    }
    // if one frame data is ready, decode it.
    LOGI("spu->frame_rdy is %d, restlen=%d, rd_oft=%d, \n\n",
         spu->frame_rdy, *bufsize, rd_oft);
    if (spu->frame_rdy == 1)
    {
        pixDataOdd = malloc(VOB_SUB_SIZE / 2);
        LOGI("pixDataOdd is %x\n\n", pixDataOdd);
        if (!pixDataOdd)
        {
            LOGI("pixDataOdd malloc fail\n\n");
            goto error; //not enough memory
        }
        ptrPXDRead =
            (unsigned short *)(spu->spu_data + spu->top_pxd_addr);
        spu_fill_pixel(ptrPXDRead, pixDataOdd, spu, 1);
        pixDataEven = malloc(VOB_SUB_SIZE / 2);
        LOGI("pixDataEven is %x\n\n", pixDataEven);
        if (!pixDataEven)
        {
            LOGI("pixDataEven malloc fail\n\n");
            goto error; //not enough memory
        }
        ptrPXDRead =
            (unsigned short *)(spu->spu_data + spu->bottom_pxd_addr);
        spu_fill_pixel(ptrPXDRead, pixDataEven, spu, 2);
        memset(spu->spu_data, 0, VOB_SUB_SIZE);
#if 0
        for (i = 0; i < VOB_SUB_SIZE; i += spu->spu_width / 2)
        {
            memcpy(spu->spu_data + i, pixDataOdd + i / 2,
                   spu->spu_width / 4);
            memcpy(spu->spu_data + i + spu->spu_width / 4,
                   pixDataEven + i / 2, spu->spu_width / 4);
        }
#else
        memcpy(spu->spu_data, pixDataOdd, VOB_SUB_SIZE / 2);
        memcpy(spu->spu_data + VOB_SUB_SIZE / 2, pixDataEven,
               VOB_SUB_SIZE / 2);
#endif
        ret = 0;
    }
error:
    if (pixDataOdd)
    {
        LOGI("start free pixDataOdd\n\n");
        free(pixDataOdd);
        LOGI("end free pixDataOdd\n\n");
    }
    if (pixDataEven)
    {
        LOGI("start free pixDataEven\n\n");
        free(pixDataEven);
        LOGI("end free pixDataEven\n\n");
    }
    return rd_oft;
}

void dvdsub_init_decoder(void)
{
    int i = 0;
    has_palette = 0;
    for (i = 0; i < 16; i++)
    {
        palette[i] = 0;
    }
}
