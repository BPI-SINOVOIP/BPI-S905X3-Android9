/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvSubtitle"
#define UNUSED(x) (void)x

#include "CTvSubtitle.h"
#ifdef SUPPORT_ADTV
#include "am_misc.h"
#include "am_dmx.h"
#endif

CTvSubtitle::CTvSubtitle()
{
    mpObser = NULL;
    sub_handle = NULL;
    pes_handle = NULL;
    dmx_id = 0;
    filter_handle = 0;
    bmp_w = 0;
    bmp_h = 0;
    bmp_pitch = 0;
    buffer = NULL;
    sub_w = 0;
    sub_h = 0;
    avchip_chg = 0;
    mSubType = 0;
    isSubOpen = false;
}

CTvSubtitle::~CTvSubtitle()
{
}

void CTvSubtitle::setObserver(IObserver *pObser)
{
    isSubOpen = false;
    mpObser = pObser;
}

void CTvSubtitle::setBuffer(char *share_mem)
{
    pthread_mutex_lock(&lock);
    buffer = (unsigned char *)share_mem;
    pthread_mutex_unlock(&lock);
}

void CTvSubtitle::stopDecoder()
{
}

void CTvSubtitle::startSub()
{
}

void CTvSubtitle::stop()
{
}

void CTvSubtitle::clear()
{
}

void CTvSubtitle::nextPage()
{
}

void CTvSubtitle::previousPage()
{
}

void CTvSubtitle::gotoPage(int page __unused)
{
}

void CTvSubtitle::goHome()
{
}

void CTvSubtitle::colorLink(int color __unused)
{
}

void CTvSubtitle::setSearchPattern(char *pattern __unused, bool casefold __unused)
{
}

void CTvSubtitle::searchNext()
{
}

void CTvSubtitle::searchPrevious()
{
}

int CTvSubtitle::sub_init(int bmp_width, int bmp_height)
{
    pthread_mutex_init(&lock, NULL);
    bmp_w = bmp_width;
    bmp_h = bmp_height;
    sub_w = 720;
    sub_h = 576;
    bmp_pitch =  bmp_w * 4;
    return 0;
}

int CTvSubtitle::sub_destroy()
{
    return 0;
}

int CTvSubtitle::sub_lock()
{
    pthread_mutex_lock(&lock);
    return 0;
}

int CTvSubtitle::sub_unlock()
{
    pthread_mutex_unlock(&lock);
    return 0;
}

int CTvSubtitle::sub_clear()
{
    return 0;
}

static void clear_bitmap(CTvSubtitle *pSub)
{
    unsigned char *ptr = pSub->buffer;
    int y = pSub->bmp_h;

    while (y--) {
        memset(ptr, 0, pSub->bmp_pitch);
        ptr += pSub->bmp_pitch;
    }
}

#ifdef SUPPORT_ADTV
static void show_sub_cb(void * handle, AM_SUB2_Picture_t *pic)
{
    LOGD("dvb callback-----------");

    CTvSubtitle *pSub = ((CTvSubtitle *) AM_SUB2_GetUserData(handle));
    pthread_mutex_lock(&pSub->lock);
    clear_bitmap(pSub);

    if (pic) {
        AM_SUB2_Region_t *rgn = pic->p_region;
        pSub->sub_w = pic->original_width;
        pSub->sub_h = pic->original_height;
        while (rgn) {
            int sx, sy, dx, dy, rw, rh;

            // ensure we have a valid buffer
            if (! rgn->p_buf) {
                rgn = rgn->p_next;
                continue;
            }

            sx = 0;
            sy = 0;
            dx = pic->original_x + rgn->left;
            dy = pic->original_y + rgn->top;
            rw = rgn->width;
            rh = rgn->height;

            if (dx < 0) {
                sx = -dx;
                dx = 0;
                rw += dx;
            }

            if (dx + rw > pSub->bmp_w) {
                rw = pSub->bmp_w - dx;
            }

            if (dy < 0) {
                sy = -dy;
                dy = 0;
                rh += dy;
            }

            if (dy + rh > pSub->bmp_h) {
                rh = pSub->bmp_h - dy;
            }

            if ((rw > 0) && (rh > 0)) {
                unsigned char *sbegin = (unsigned char *)rgn->p_buf + sy * rgn->width + sx;
                unsigned char *dbegin = pSub->buffer + dy * pSub->bmp_pitch + dx * 4;
                unsigned char *src, *dst;
                int size;

                while (rh) {
                    src = sbegin;
                    dst = dbegin;
                    size = rw;
                    while (size--) {
                        int c = src[0];

                        if (c < (int)rgn->entry) {
                            if (rgn->clut[c].a) {
                                *dst++ = rgn->clut[c].r;
                                *dst++ = rgn->clut[c].g;
                                *dst++ = rgn->clut[c].b;
                            } else {
                                dst += 3;
                            }
                            *dst++ = rgn->clut[c].a;
                        } else {
                            dst += 4;
                        }
                        src ++;
                    }
                    sbegin += rgn->width;
                    dbegin += pSub->bmp_pitch;
                    rh--;
                }
            }

            rgn = rgn->p_next;
        }
        pSub->mpObser->updateSubtitle(pic->original_width, pic->original_height);
    }
    pthread_mutex_unlock(&pSub->lock);

}
#endif

static uint64_t get_pts_cb(void *handle __unused, uint64_t pts __unused)
{
#ifdef SUPPORT_ADTV
    char buf[32];
    AM_ErrorCode_t ret;
    uint32_t v;
    uint64_t r;

    ret = AM_FileRead("/sys/class/tsync/pts_pcrscr", buf, sizeof(buf));
    if (!ret) {
        v = strtoul(buf, 0, 16);
        if (pts & (1LL << 32)) {
            r = ((uint64_t)v) | (1LL << 32);
        } else {
            r = (uint64_t)v;
        }
    } else {
        r = 0LL;
    }

    return r;
#else
    return -1;
#endif
}

static void pes_data_cb(int dev_no __unused, int fhandle __unused,
    const uint8_t *data, int len, void *user_data)
{
#ifdef SUPPORT_ADTV
    CTvSubtitle *pSub = ((CTvSubtitle *) user_data);
    AM_PES_Decode(pSub->pes_handle, (uint8_t *)data, len);
#endif
}

static int close_dmx(CTvSubtitle *pSub)
{
#ifdef SUPPORT_ADTV
    AM_DMX_FreeFilter(pSub->dmx_id, pSub->filter_handle);
    AM_DMX_Close(pSub->dmx_id);
    pSub->dmx_id = -1;
    pSub->filter_handle = -1;
#endif
    return 0;
}

static int open_dmx(CTvSubtitle *pSub, int dmx_id, int pid)
{
#ifdef SUPPORT_ADTV
    close_dmx(pSub);
    AM_DMX_OpenPara_t op;
    struct dmx_pes_filter_params pesp;
    AM_ErrorCode_t ret;

    pSub->dmx_id = -1;
    pSub->filter_handle = -1;
    memset(&op, 0, sizeof(op));

    ret = AM_DMX_Open(dmx_id, &op);
    if (ret != DVB_SUCCESS) {
        LOGD("error AM_DMX_Open != DVB_SUCCESS");
        goto error;
    }
    pSub->dmx_id = dmx_id;

    ret = AM_DMX_AllocateFilter(dmx_id, &pSub->filter_handle);
    if (ret != DVB_SUCCESS) {
        LOGD("error AM_DMX_AllocateFilter != DVB_SUCCESS");
        goto error;
    }

    ret = AM_DMX_SetBufferSize(dmx_id, pSub->filter_handle, 0x80000);
    if (ret != DVB_SUCCESS) {
        LOGD("error AM_DMX_SetBufferSize != DVB_SUCCESS");
        goto error;
    }

    memset(&pesp, 0, sizeof(pesp));
    pesp.pid = pid;
    pesp.output = DMX_OUT_TAP;
    pesp.pes_type = DMX_PES_TELETEXT0;

    ret = AM_DMX_SetPesFilter(dmx_id, pSub->filter_handle, &pesp);
    if (ret != DVB_SUCCESS) {
        LOGD("error AM_DMX_SetPesFilter != DVB_SUCCESS, err = %s", strerror(errno));
        goto error;
    }

    ret = AM_DMX_SetCallback(dmx_id, pSub->filter_handle, pes_data_cb, pSub);
    if (ret != DVB_SUCCESS) {
        LOGD("error AM_DMX_SetCallback != DVB_SUCCESS");
        goto error;
    }

    ret = AM_DMX_StartFilter(dmx_id, pSub->filter_handle);
    if (ret != DVB_SUCCESS) {
        LOGD("error AM_DMX_StartFilter != DVB_SUCCESS,dmx_id=%d,filter_handle=%d, ret = %d", dmx_id, pSub->filter_handle, ret);
        goto error;
    }

    return 0;
error:
    if (pSub->filter_handle != -1) {
        AM_DMX_FreeFilter(dmx_id, pSub->filter_handle);
    }
    if (pSub->dmx_id != -1) {
        AM_DMX_Close(dmx_id);
    }
#endif
    return -1;
}

static void pes_sub_cb(void* handle, uint8_t *buf, int size)
{
#ifdef SUPPORT_ADTV
    CTvSubtitle *pSub = ((CTvSubtitle *) AM_SUB2_GetUserData(handle));
    AM_SUB2_Decode(pSub->sub_handle, buf, size);
#endif
}

int CTvSubtitle::sub_switch_status()
{
    return isSubOpen ? 1 : 0;
}

int CTvSubtitle::sub_start_dvb_sub(int dmx_id, int pid, int page_id, int anc_page_id)
{
    LOGD("start dvb subtitle=----------------");
#ifdef SUPPORT_ADTV
    AM_PES_Para_t pesp;
    AM_SUB2_Para_t subp;
    int ret;

    memset(&pesp, 0, sizeof(pesp));
    pesp.packet    = pes_sub_cb;
    pesp.user_data = this;
    ret = AM_PES_Create(&pes_handle, &pesp);
    if (ret != DVB_SUCCESS) {
        LOGD("error AM_PES_Create != DVB_SUCCESS");
        goto error;
    }

    memset(&subp, 0, sizeof(subp));
    subp.show      = show_sub_cb;
    subp.get_pts   = get_pts_cb;
    subp.composition_id = page_id;
    subp.ancillary_id   = anc_page_id;
    subp.user_data = this;
    ret = AM_SUB2_Create(&sub_handle, &subp);
    if (ret != DVB_SUCCESS) {
        LOGD("error AM_SUB2_Create != DVB_SUCCESS");
        goto error;
    }

    ret = AM_SUB2_Start(sub_handle);
    if (ret != DVB_SUCCESS) {
        LOGD("error AM_SUB2_Start != DVB_SUCCESS");
        goto error;
    }

    ret = open_dmx(this, dmx_id, pid);
    if (ret < 0) {
        LOGD("error open_dmx != DVB_SUCCESS");
        goto error;
    }
    isSubOpen = true;
    return 0;
error:
    if (sub_handle) {
        AM_SUB2_Destroy(sub_handle);
        sub_handle = NULL;
    }
    if (pes_handle) {
        AM_PES_Destroy(pes_handle);
        pes_handle = NULL;
    }
#endif
    return -1;
}

int CTvSubtitle::sub_start_dtv_tt(int dmx_id __unused, int region_id __unused, int pid __unused,
    int page __unused, int sub_page __unused, bool is_sub __unused)
{
    return 0;
}

int CTvSubtitle::sub_stop_dvb_sub()
{
#ifdef SUPPORT_ADTV
    pthread_mutex_lock(&lock);
    close_dmx(this);
    AM_SUB2_Destroy(sub_handle);
    AM_PES_Destroy(pes_handle);

    clear_bitmap(this);
    mpObser->updateSubtitle(0, 0);

    sub_handle = NULL;
    pes_handle = NULL;
    isSubOpen = false;
    pthread_mutex_unlock(&lock);
#endif
    return 0;
}

int CTvSubtitle::sub_stop_dtv_tt()
{
    return 0;
}

int CTvSubtitle::sub_tt_goto(int page __unused)
{
    return 0;
}

int CTvSubtitle::sub_tt_color_link(int color __unused)
{
    return 0;
}

int CTvSubtitle::sub_tt_home_link()
{
    return 0;
}

int CTvSubtitle::sub_tt_next(int dir __unused)
{
    return 0;
}

int CTvSubtitle::sub_tt_set_search_pattern(char *pattern __unused, bool casefold __unused)
{
    return 0;
}

int CTvSubtitle::sub_tt_search(int dir __unused)
{
    return 0;
}

/*
 * 1, Set the country first and parameters should be either USA or KOREA
#define CMD_SET_COUNTRY_USA                 0x5001
#define CMD_SET_COUNTRY_KOREA            0x5002

2, Set the source type which including
    a)VBI data(for analog program only)
    b)USER data(for AIR or Cable service)
CMD_CC_SET_VBIDATA   = 0x7001,
CMD_CC_SET_USERDATA = 0x7002,
2.1 If the frontend type is Analog we must set the channel Index
     with command 'CMD_CC_SET_CHAN_NUM' and the parameter is like 57M
     we set 0x20000, this should according to USA standard frequency
     table.

3, Next is to set the CC service type

#define CMD_CC_1                        0x3001
#define CMD_CC_2                        0x3002
#define CMD_CC_3                        0x3003
#define CMD_CC_4                        0x3004

//this doesn't support currently
#define CMD_TT_1                        0x3005
#define CMD_TT_2                        0x3006
#define CMD_TT_3                        0x3007
#define CMD_TT_4                        0x3008

#define CMD_SERVICE_1                 0x4001
#define CMD_SERVICE_2                 0x4002
#define CMD_SERVICE_3                 0x4003
#define CMD_SERVICE_4                 0x4004
#define CMD_SERVICE_5                 0x4005
#define CMD_SERVICE_6                 0x4006

4, Then set CMD_CC_START to start the CC service, and you needn't to stop

CC service while switching services

5, CMD_CC_STOP should be called in some cases like switch source, change

program, no signal, blocked...*/

//channel_num == 0 ,if frontend is dtv
//else != 0
int CTvSubtitle::sub_start_atsc_cc(enum cc_param_country country, enum cc_param_source_type src_type, int channel_num, enum cc_param_caption_type caption_type)
{
    LOGD("%s:country=%d,src=%d,channel_num=%D,ctype=%d", __FUNCTION__, country, src_type, channel_num, caption_type);
/*
    switch (country) {
    case CC_PARAM_COUNTRY_USA:
        AM_CC_Cmd(CMD_SET_COUNTRY_USA);
        break;
    case CC_PARAM_COUNTRY_KOREA:
        AM_CC_Cmd(CMD_SET_COUNTRY_KOREA);
        break;
    default:
        AM_CC_Cmd(CMD_SET_COUNTRY_USA);
        break;
    }

    switch (src_type) {
    case CC_PARAM_SOURCE_VBIDATA:
        AM_CC_Cmd(CMD_CC_SET_VBIDATA);
        break;
    case CC_PARAM_SOURCE_USERDATA:
        AM_CC_Cmd(CMD_CC_SET_USERDATA);
        break;
    default:
        AM_CC_Cmd(CMD_CC_SET_USERDATA);
        break;
    }

    //just for test
    if (channel_num == 0) {
    } else {
        //AM_CC_Cmd(CMD_CC_SET_CHAN_NUM);
    }

    AM_CLOSECAPTION_cmd_t cc_t_cmd;
    switch (caption_type) {
    case CC_PARAM_ANALOG_CAPTION_TYPE_CC1:
        cc_t_cmd = CMD_CC_1;
        break;
    case CC_PARAM_ANALOG_CAPTION_TYPE_CC2:
        cc_t_cmd = CMD_CC_2;
        break;
    case CC_PARAM_ANALOG_CAPTION_TYPE_CC3:
        cc_t_cmd = CMD_CC_3;
        break;
    case CC_PARAM_ANALOG_CAPTION_TYPE_CC4:
        cc_t_cmd = CMD_CC_4;
        break;
    case CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE1:
        cc_t_cmd = CMD_SERVICE_1;
        break;
    case CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE2:
        cc_t_cmd = CMD_SERVICE_2;
        break;
    case CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE3:
        cc_t_cmd = CMD_SERVICE_3;
        break;
    case CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE4:
        cc_t_cmd = CMD_SERVICE_4;
        break;
    default:
        cc_t_cmd = CMD_SERVICE_1;
        break;
    }
    AM_CC_Cmd(cc_t_cmd);

    AM_CC_Set_CallBack(close_caption_callback, this);
    AM_VCHIP_Set_CallBack(atv_vchip_callback, this);
    //start
    AM_CC_Cmd(CMD_CC_START);
    LOGD("----sub_start_atsc_cc-2--- country=%d,src=%d,ctype=%d", country, src_type, caption_type);
*/
    return 0;
}

int CTvSubtitle::sub_stop_atsc_cc()
{
/*
    LOGD("----sub_stop_atsc_cc----");
    AM_CC_Cmd(CMD_CC_STOP);
*/
    return 0;
}

int CTvSubtitle::ResetVchipChgStat()
{
/*
    avchip_chg = 0;
    AM_CC_Cmd(CMD_VCHIP_RST_CHGSTAT);
*/
    return 0;
}

int CTvSubtitle::IsVchipChange()
{
    return avchip_chg;
}

//cnt :data buf len
//databuf len  is max 512
//cmdbuf len is max 128
void CTvSubtitle::close_caption_callback(char *str, int cnt, int data_buf[], int cmd_buf[], void *user_data)
{
    /*
    CTvSubtitle *pSub = (CTvSubtitle *)user_data;

    if (pSub == NULL)
    {
        LOGD("sub cc callback is null user data for this");
        return;
    }

    if (pSub->mpObser == NULL) return;

    pSub->mCurCCEv.mDataBufSize = cnt;
    pSub->mCurCCEv.mpDataBuffer = data_buf;
    pSub->mCurCCEv.mCmdBufSize = 128;//max
    pSub->mCurCCEv.mpCmdBuffer = cmd_buf;

    pSub->mpObser->onEvent(pSub->mCurCCEv);
    */
    //TODO
    UNUSED(str);
    UNUSED(cnt);
    UNUSED(data_buf);
    UNUSED(cmd_buf);
    UNUSED(user_data);
}

void CTvSubtitle::atv_vchip_callback(int Is_chg,  void *user_data)
{
    CTvSubtitle *pSub = (CTvSubtitle *)user_data;
    pSub->avchip_chg = Is_chg;
}

