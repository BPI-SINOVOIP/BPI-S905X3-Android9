/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CFrontEnd"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <time.h>
#include "CFrontEnd.h"
//#include "util.h"
#include "tvin/CTvin.h"
#include "CTvScanner.h"

#include <string>
#include <map>

#ifdef SUPPORT_ADTV
#include "am_misc.h"
#include "am_debug.h"
#include <tvconfig.h>

extern "C" {
#include "am_av.h"
#include "am_dmx.h"
#include "linux/videodev2.h"
#include "am_fend_ctrl.h"
#include "am_vlfend.h"
}
#endif

CFrontEnd *CFrontEnd::mInstance;

CFrontEnd *CFrontEnd::getInstance()
{
    if (NULL == mInstance) mInstance = new CFrontEnd();
    return mInstance;
}

CFrontEnd::CFrontEnd()
{
    mFrontDevID = FE_DEV_ID;
    mVLFrontDevID = VLFE_DEV_ID;
    mpObserver = NULL;
    mCurFineFreq = 0;
    mCurMode = -1;
    mVLCurMode = -1;
    mCurFreq = -1;
    mCurPara1 = -1;
    mCurPara2 = -1;
    mCurPara3 = -1;
    mCurPara4 = -1;
    mbFEOpened = false;
    mbVLFEOpened = false;
    mDemuxDevID = -1;
    mTvPlayDevID = -1;
    mCurPara3 = -1;
}

CFrontEnd::~CFrontEnd()
{
#ifdef SUPPORT_ADTV
    AM_EVT_Unsubscribe(mFrontDevID, AM_FEND_EVT_STATUS_CHANGED, dmd_fend_callback, NULL);
    if (mFrontDevID == FE_DEV_ID)
        AM_FEND_Close(mFrontDevID);
    AM_EVT_Unsubscribe(mVLFrontDevID, AM_VLFEND_EVT_STATUS_CHANGED, v4l2_fend_callback, NULL);
    if (mVLFrontDevID == VLFE_DEV_ID)
        AM_FEND_Close(mVLFrontDevID);
    mFrontDevID = -1;
    mVLFrontDevID = -1;
#endif
}

int CFrontEnd::Open(int mode)
{
#ifdef SUPPORT_ADTV
    AutoMutex _l( mLock );

    AM_FEND_OpenPara_t para;
    int rc = 0;

    LOGD("%s, mode %d", __FUNCTION__, mode);

    if (mbFEOpened && (mCurMode == mode || mode == FE_AUTO)) {
        LOGD("FrontEnd already opened: %d", mCurMode);
        /* return 0; */
    }

    if (mbVLFEOpened && (mVLCurMode == mode || mode == FE_AUTO)) {
        LOGD("VLFrontEnd already opened: %d", mVLCurMode);
        /* return 0; */
    }

    memset(&para, 0, sizeof(AM_FEND_OpenPara_t));
    para.mode = mode;

    if (mbVLFEOpened == false) {
        if (TV_FE_ANALOG == mode || mode == FE_AUTO) {
            LOGD("VLFE Open [%d->%d]", mVLCurMode, mode);

            rc = AM_VLFEND_Open(mVLFrontDevID, &para);
            if ((rc != AM_FEND_ERR_BUSY) && (rc != 0)) {
                LOGD("%s,vlfrontend dev[%d] open failed! v4l2 error id is %d\n",
                        __FUNCTION__, mVLFrontDevID, rc);
                /*return -1;*/
            } else {
                AM_EVT_Subscribe(mVLFrontDevID, AM_VLFEND_EVT_STATUS_CHANGED, v4l2_fend_callback, (void *)this);
                LOGD("%s,vlfrontend dev[%d] open success!\n", __FUNCTION__, mVLFrontDevID);
                mbVLFEOpened = true;
                mVLCurMode = mode;
            }
        }
    } else {
        /* when open with different mode, need set mode. */
        if (mVLCurMode != mode && TV_FE_ANALOG == mode) {
            LOGD("VLFE Open [%d->%d]", mVLCurMode, mode);
            rc = AM_VLFEND_SetMode(mVLFrontDevID, FE_ANALOG);
            mVLCurMode = mode;
        }
    }

    para.mode = mode;
    if (mbFEOpened == false) {
        if (TV_FE_ANALOG != mode) {
            LOGD("FE Open [%d->%d]", mCurMode, mode);

            rc = AM_FEND_Open(mFrontDevID, &para);
            if ((rc != AM_FEND_ERR_BUSY) && (rc != 0)) {
                LOGD("%s,frontend dev[%d] open failed! dvb error id is %d\n",
                        __FUNCTION__, mFrontDevID, rc);
                return -1;
            }
            AM_EVT_Subscribe(mFrontDevID, AM_FEND_EVT_STATUS_CHANGED, dmd_fend_callback, (void *)this);
            LOGD("%s,frontend dev[%d] open success!\n", __FUNCTION__, mFrontDevID);
            mbFEOpened = true;
            mCurMode = mode;
        }
    } else {
        /* when open with different mode, need set mode. */
        if (mCurMode != mode && TV_FE_ANALOG != mode) {
            LOGD("FE Open [%d->%d]", mCurMode, mode);
            rc = AM_FEND_SetMode(mFrontDevID, mode);
            mCurMode = mode;
        }
    }

    mCurFreq = -1;
    mCurPara1 = -1;
    mCurPara2 = -1;
    mCurPara3 = -1;
    mCurPara4 = -1;
#endif
    return 0;
}

int CFrontEnd::Close()
{
#ifdef SUPPORT_ADTV
    AutoMutex _l( mLock );

    int rc = 0;
    int rc1 = 0;

    LOGD("FE Close");

    if (!mbFEOpened) {
        LOGD("FrontEnd already closed.");
    }

    if (!mbVLFEOpened) {
        LOGD("VLFrontEnd already closed.");
    }

    mCurMode = -1;
    mVLCurMode = -1;
    mCurFreq = -1;
    mCurPara1 = -1;
    mCurPara2 = -1;
    mCurPara3 = -1;
    mCurPara4 = -1;
    mFEParas.setFrequency(-1);

    if (mbFEOpened) {
        AM_FEND_SetMode(mFrontDevID, FE_ANALOG);
        rc = AM_FEND_Close(mFrontDevID);
        if (rc != 0) {
            LOGD("%s,frontend_close fail! dvb error id is %d\n", __FUNCTION__, rc);
        }
    }

    if (mbVLFEOpened) {
        rc1 = AM_VLFEND_Close(mVLFrontDevID);
        if (rc1 != 0) {
            LOGD("%s,vlfrontend_close fail! dvb error id is %d\n", __FUNCTION__, rc1);
        }
    }

    mbFEOpened = false;
    mbVLFEOpened = false;
#endif
    LOGD("%s,close frontend is ok\n", __FUNCTION__);
    return 0;
}

int CFrontEnd::setMode(int mode)
{
#ifdef SUPPORT_ADTV
    AutoMutex _l( mLock );

    int rc = 0;

    if (mCurMode == mode && mVLCurMode == mode) {
        LOGD("%s, set same mode %d, return.\n", __FUNCTION__, mode);
        return 0;
    }

    LOGD("%s, set mode %d.\n", __FUNCTION__, mode);
    if (mode == TV_FE_ANALOG) {
        rc = AM_FEND_SetMode(mFrontDevID, FE_ANALOG);
        rc = AM_VLFEND_SetMode(mVLFrontDevID, mode);
        mCurMode = FE_ANALOG;
        mVLCurMode = FE_ANALOG;
    } else {
        rc = AM_VLFEND_SetMode(mVLFrontDevID, FE_AUTO);
        rc = AM_FEND_SetMode(mFrontDevID, mode);
        mCurMode = mode;
        mVLCurMode = FE_AUTO;
    }

    if (rc != 0) {
        LOGD("%s,front dev set mode failed! dvb error id is %d\n", __FUNCTION__, rc);
        return -1;
    }
#endif

    return 0;
}

int CFrontEnd::setProp(int cmd, int val)
{
    AutoMutex _l( mLock );
    return setPropLocked(cmd, val);
}

int CFrontEnd::setPropLocked(int cmd, int val)
{
#ifdef SUPPORT_ADTV
    struct dtv_properties props;
    struct dtv_property prop;

    memset(&props, 0, sizeof(props));
    memset(&prop, 0, sizeof(prop));

    prop.cmd = cmd;
    prop.u.data = val;

    props.num = 1;
    props.props = &prop;

    return AM_FEND_SetProp(mFrontDevID, &props);
#else
    return -1;
#endif
}

int CFrontEnd::setVLPropLocked(int cmd, int val)
{
#ifdef SUPPORT_ADTV
    AutoMutex _l( mLock );

    struct dtv_properties props;
    struct dtv_property prop;

    memset(&props, 0, sizeof(props));
    memset(&prop, 0, sizeof(prop));

    prop.cmd = cmd;
    prop.u.data = val;

    props.num = 1;
    props.props = &prop;

    return AM_VLFEND_SetProp(mVLFrontDevID, &props);
 #else
    return -1;
 #endif
}

int CFrontEnd::getVLPropLocked(int cmd, int *val)
{
#ifdef SUPPORT_ADTV
    AutoMutex _l( mLock );

    struct dtv_properties props;
    struct dtv_property prop;

    memset(&props, 0, sizeof(props));
    memset(&prop, 0, sizeof(prop));

    prop.cmd = cmd;
    prop.u.data = 0;

    props.num = 1;
    props.props = &prop;

    AM_VLFEND_GetProp(mVLFrontDevID, &props);

    *val = prop.u.data;

    return 0;
 #else
    return -1;
 #endif
}

int CFrontEnd::convertParas(char *paras, int mode, int freq1, int freq2, int para1, int para2, int para3, int para4)
{
    char p[128] = {0};
    sprintf(paras, "{\"mode\":%d,\"freq\":%d,\"freq2\":%d", mode, freq1, freq2);
    switch (FEMode(mode).getBase())
    {
        case TV_FE_DTMB:
        case TV_FE_OFDM:
            sprintf(p, ",\"bw\":%d}", para1);
            break;
        case TV_FE_QAM:
            sprintf(p, ",\"sr\":%d,\"mod\":%d}", para1, para2);
            break;
        case TV_FE_ATSC:
            sprintf(p, ",\"mod\":%d}", para1);
            break;
        case TV_FE_ANALOG:
            sprintf(p, ",\"vtd\":%d,\"atd\":%d,\"afc\":%d,\"vfmt\":%d,\"soundsys\":%d}",
                stdAndColorToVideoEnum(para1),
                stdAndColorToAudioEnum(para1),
                para2, para3, para4);
            break;
        case TV_FE_ISDBT:
            sprintf(p, ",\"bw\":%d,\"lyr\":%d}", para1, para2);
            break;
        default:
            sprintf(p, "}");
            break;
    }
    strcat(paras, p);
    return 0;
}

void CFrontEnd::saveCurrentParas(FEParas &paras)
{
    mFEParas = paras;

    /*for compatible*/
    if (mFEParas.getFEMode().getBase() != TV_FE_ANALOG)
        mCurMode = mFEParas.getFEMode().getBase();
    mCurFreq = mFEParas.getFrequency();
    mCurPara1 = mCurPara2 = mCurPara3 = 0;
    switch (mFEParas.getFEMode().getBase())
    {
        case TV_FE_DTMB:
        case TV_FE_OFDM:
            mCurPara1 = mFEParas.getBandwidth();
            break;
        case TV_FE_QAM:
            mCurPara1 = mFEParas.getSymbolrate();
            mCurPara2 = mFEParas.getModulation();
            break;
        case TV_FE_ATSC:
            mCurPara1 = mFEParas.getModulation();
            break;
        case TV_FE_ANALOG:
            mCurPara1 = enumToStdAndColor(mFEParas.getVideoStd(), mFEParas.getAudioStd());
            mCurPara2 = mFEParas.getAfc();
            mCurPara3 = mFEParas.getVFmt();
            mCurPara4 = mFEParas.getSoundsys();
            mVLCurMode = mFEParas.getFEMode().getBase();
            break;
        case TV_FE_ISDBT:
            mCurPara1 = mFEParas.getBandwidth();
            mCurPara2 = mFEParas.getLayer();
            break;
        default:
            break;
    }
}

int CFrontEnd::setPara(int mode, int freq, int para1, int para2, int para3, int para4)
{
    char paras[128];
    convertParas(paras, mode, freq, freq, para1, para2, para3, para4);
    return setPara(paras);
}

int CFrontEnd::setPara(const char *paras)
{
    return setPara(paras, false);
}

int CFrontEnd::setPara(const char *paras, bool force )
{
#ifdef SUPPORT_ADTV
    AutoMutex _l( mLock );
    int ret = 0;
    FEParas feparas(paras);

    LOGD("fe setpara [%s]", paras);
    if (mFEParas == feparas && !force) {
        LOGD("fe setpara  is same return");
        return 0;
    }

    saveCurrentParas(feparas);

    AM_FENDCTRL_DVBFrontendParameters_t dvbfepara;
    memset(&dvbfepara, 0, sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

    dvbfepara.m_type = mFEParas.getFEMode().getBase();
    switch (dvbfepara.m_type) {
    case TV_FE_OFDM:
        dvbfepara.terrestrial.para.frequency = mFEParas.getFrequency();
        dvbfepara.terrestrial.para.u.ofdm.bandwidth = (fe_bandwidth_t)mFEParas.getBandwidth();
        dvbfepara.terrestrial.ofdm_mode = (fe_ofdm_mode_t)mFEParas.getFEMode().getGen();
        break;
    case TV_FE_DTMB:
        dvbfepara.dtmb.para.frequency = mFEParas.getFrequency();
        dvbfepara.dtmb.para.u.ofdm.bandwidth = (fe_bandwidth_t)mFEParas.getBandwidth();
        break;
    case TV_FE_ATSC:
        dvbfepara.atsc.para.frequency = mFEParas.getFrequency();
        dvbfepara.atsc.para.u.vsb.modulation = (fe_modulation_t)mFEParas.getModulation();
        break;
    case TV_FE_QAM:
        dvbfepara.cable.para.frequency = mFEParas.getFrequency();
        dvbfepara.cable.para.u.qam.symbol_rate = mFEParas.getSymbolrate();
        dvbfepara.cable.para.u.qam.modulation  = (fe_modulation_t)mFEParas.getModulation();
        break;
    case TV_FE_ISDBT:
        dvbfepara.isdbt.para.frequency = mFEParas.getFrequency();
        dvbfepara.isdbt.para.u.ofdm.bandwidth = (fe_bandwidth_t)mFEParas.getBandwidth();
        break;
    case TV_FE_ANALOG: {
        int buff_size = 32;
        char VideoStdBuff[buff_size];
        char audioStdBuff[buff_size];
        /*para2 is finetune data */
        dvbfepara.analog.para.frequency = mFEParas.getFrequency();
        dvbfepara.analog.para.u.analog.std = enumToStdAndColor(mFEParas.getVideoStd(), mFEParas.getAudioStd());
        dvbfepara.analog.para.u.analog.audmode = dvbfepara.analog.para.u.analog.std & 0x00FFFFFF;
        dvbfepara.analog.para.u.analog.afc_range = AFC_RANGE;
        dvbfepara.analog.para.u.analog.soundsys = (mFEParas.getSoundsys() >= 0 ? mFEParas.getSoundsys() : 0xFF);
        if (mFEParas.getAfc() == 0) {
            dvbfepara.analog.para.u.analog.flag |= ANALOG_FLAG_ENABLE_AFC;
        } else {
            dvbfepara.analog.para.u.analog.flag &= ~ANALOG_FLAG_ENABLE_AFC;
            dvbfepara.analog.para.u.analog.afc_range = 0;
        }
        printAudioStdStr(dvbfepara.analog.para.u.analog.std, audioStdBuff, buff_size);
        printVideoStdStr(dvbfepara.analog.para.u.analog.std, VideoStdBuff, buff_size);
        LOGD("%s,freq = %dHz, video_std = %s, audio_std = %s, afc = %d\n", __FUNCTION__,
             dvbfepara.analog.para.frequency, VideoStdBuff, audioStdBuff, mFEParas.getAfc());
        }
        break;
    }

    if (dvbfepara.m_type == TV_FE_ANALOG)
    {
        struct dvb_frontend_parameters atv_para;
        atv_para.frequency = dvbfepara.analog.para.frequency;
        atv_para.u.analog.audmode = dvbfepara.analog.para.u.analog.audmode;
        atv_para.u.analog.soundsys = dvbfepara.analog.para.u.analog.soundsys;
        atv_para.u.analog.std = dvbfepara.analog.para.u.analog.std;
        atv_para.u.analog.flag = dvbfepara.analog.para.u.analog.flag;
        atv_para.u.analog.afc_range = dvbfepara.analog.para.u.analog.afc_range;
        //AM_FEND_SetMode(mFrontDevID, FE_ANALOG);
        //ret = AM_VLFEND_SetMode(mVLFrontDevID, dvbfepara.m_type);
        ret = AM_VLFEND_SetPara(mVLFrontDevID, &atv_para);
        mCurMode = FE_ANALOG;
        mVLCurMode = dvbfepara.m_type;
    }
    else
    {
        //AM_VLFEND_SetMode(mVLFrontDevID, 0);
        ret = AM_FENDCTRL_SetPara(mFrontDevID, &dvbfepara);
        mCurMode = dvbfepara.m_type;
        mVLCurMode = FE_AUTO;
    }

    if (ret != 0) {
        LOGD("%s,fend set para failed! dvb error id is %d\n", __FUNCTION__, ret);
        return -1;
    }

    if (mFEParas.getFEMode().getBase() == TV_FE_OFDM
            && mFEParas.getFEMode().getGen()) {
        ret = setPropLocked(DTV_DVBT2_PLP_ID, mFEParas.getPlp());
        if (ret != 0)
            return -1;
    }
    if (dvbfepara.m_type == TV_FE_ISDBT) {
        ret = setPropLocked(DTV_ISDBT_LAYER_ENABLED, mFEParas.getLayer());
        if (ret != 0)
            return -1;
    }
#endif
    return 0;
}

int CFrontEnd::fineTune(int fineFreq)
{
    return fineTune(fineFreq, false);
}

int CFrontEnd::fineTune(int fineFreq, bool force)
{
#ifdef SUPPORT_ADTV
    AutoMutex _l( mLock );

    int ret = 0;
    if (mCurFineFreq == fineFreq && !force) return -1;

    mCurFineFreq = fineFreq;
    ret = AM_FEND_FineTune(FE_DEV_ID, fineFreq);

    if (ret != 0) {
        LOGD("%s, fail! dvb error id is %d", __FUNCTION__, ret);
        return -1;
    }
#endif
    return 0;
}

void CFrontEnd::dmd_fend_callback(long dev_no __unused, int event_type __unused, void *param, void *user_data)
{
#ifdef SUPPORT_ADTV
    CFrontEnd *pFront = (CFrontEnd *)user_data;
    if (NULL == pFront) {
        LOGD("%s,warnning : dmd_fend_callback NULL == pFront\n", __FUNCTION__);
        return ;
    }
    if (pFront->mpObserver == NULL) {
        LOGD("%s,warnning : mpObserver NULL == mpObserver\n", __FUNCTION__);
        return;
    }
    struct dvb_frontend_event *evt = (struct dvb_frontend_event *) param;
    if (!evt)
        return;

    LOGD("fend callback: status[0x%x]\n", evt->status);
    if (evt->status &  TV_FE_HAS_LOCK) {
        pFront->mCurSigEv.mCurSigStaus = FEEvent::EVENT_FE_HAS_SIG;
        pFront->mCurSigEv.mCurFreq = evt->parameters.frequency;
        pFront->mpObserver->onEvent(pFront->mCurSigEv);
    } else if (evt->status & TV_FE_TIMEDOUT) {
        pFront->mCurSigEv.mCurSigStaus = FEEvent::EVENT_FE_NO_SIG;
        pFront->mCurSigEv.mCurFreq = evt->parameters.frequency;
        pFront->mpObserver->onEvent(pFront->mCurSigEv);
    }
#endif
}

void CFrontEnd::v4l2_fend_callback(long dev_no __unused, int event_type __unused, void *param, void *user_data)
{
#ifdef SUPPORT_ADTV
    CFrontEnd *pFront = (CFrontEnd *)user_data;
    if (NULL == pFront) {
        LOGD("%s,warnning : v4l2_fend_callback NULL == pFront\n", __FUNCTION__);
        return ;
    }
    if (pFront->mpObserver == NULL) {
        LOGD("%s,warnning : mpObserver NULL == mpObserver\n", __FUNCTION__);
        return;
    }

    struct dvb_frontend_event *evt = (struct dvb_frontend_event *) param;
    if (!evt)
        return;

    LOGD("vlfend callback: status[0x%x]\n", evt->status);
    if (evt->status &  TV_FE_HAS_LOCK) {
        pFront->mCurSigEv.mCurSigStaus = FEEvent::EVENT_VLFE_HAS_SIG;
        pFront->mCurSigEv.mCurFreq = evt->parameters.frequency;
        pFront->mpObserver->onEvent(pFront->mCurSigEv);
    } else if (evt->status & TV_FE_TIMEDOUT) {
        pFront->mCurSigEv.mCurSigStaus = FEEvent::EVENT_VLFE_NO_SIG;
        pFront->mCurSigEv.mCurFreq = evt->parameters.frequency;
        pFront->mpObserver->onEvent(pFront->mCurSigEv);
    }
#endif
}

int CFrontEnd::stdAndColorToAudioEnum(int data)
{
    atv_audio_std_t std = CC_ATV_AUDIO_STD_DK;
#ifdef SUPPORT_ADTV
    if (((data & V4L2_STD_PAL_DK) == V4L2_STD_PAL_DK) ||
        ((data & V4L2_STD_SECAM_DK) == V4L2_STD_SECAM_DK)) {
        std =  CC_ATV_AUDIO_STD_DK;
    } else if ((data & V4L2_STD_PAL_I) == V4L2_STD_PAL_I) {
        std =  CC_ATV_AUDIO_STD_I;
    } else if (((data & V4L2_STD_PAL_BG) == V4L2_STD_PAL_BG) ||
               ((data & V4L2_STD_SECAM_B) == V4L2_STD_SECAM_B) ||
               ((data & V4L2_STD_SECAM_G) == V4L2_STD_SECAM_G )) {
        std = CC_ATV_AUDIO_STD_BG;
    } else if (((data & V4L2_STD_PAL_M) == V4L2_STD_PAL_M) ||
               ((data & V4L2_STD_NTSC_M) == V4L2_STD_NTSC_M)) {
        std = CC_ATV_AUDIO_STD_M;
    } else if ((data & V4L2_STD_SECAM_L) == V4L2_STD_SECAM_L) {
        std = CC_ATV_AUDIO_STD_L;
    } else if ((data & V4L2_STD_SECAM_LC) == V4L2_STD_SECAM_LC) {
        std = CC_ATV_AUDIO_STD_LC;
    } else if (!data) {
        std = CC_ATV_AUDIO_STD_AUTO;
    }
#endif
    return  std ;
}

int CFrontEnd::stdAndColorToVideoEnum(int std)
{
    atv_video_std_t video_standard = CC_ATV_VIDEO_STD_PAL;
    if ((std & V4L2_COLOR_STD_PAL) == V4L2_COLOR_STD_PAL) {
        video_standard = CC_ATV_VIDEO_STD_PAL;
    } else if ((std & V4L2_COLOR_STD_NTSC) == V4L2_COLOR_STD_NTSC) {
        video_standard = CC_ATV_VIDEO_STD_NTSC;
    } else if ((std & V4L2_COLOR_STD_SECAM) == V4L2_COLOR_STD_SECAM) {
        video_standard = CC_ATV_VIDEO_STD_SECAM;
    } else if (!std) {
        video_standard = CC_ATV_VIDEO_STD_AUTO;
    }
    return video_standard;
}

bool CFrontEnd::stdIsColorAuto(int std)
{
    return  ((std & V4L2_COLOR_STD_AUTO) == V4L2_COLOR_STD_AUTO) ? true : false;
}

int CFrontEnd::addColorAutoFlag(int std)
{
    return std | V4L2_COLOR_STD_AUTO;
}

unsigned long CFrontEnd::enumToStdAndColor(int videoStd, int audioStd)
{
    unsigned long tmpTunerStd = 0;
#ifdef SUPPORT_ADTV
    if (videoStd == CC_ATV_VIDEO_STD_PAL) {
        tmpTunerStd |= V4L2_COLOR_STD_PAL;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_PAL_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= V4L2_STD_PAL_BG;
        } else if (audioStd == CC_ATV_AUDIO_STD_M) {
            tmpTunerStd |= V4L2_STD_PAL_M;
        }
    } else if (videoStd == CC_ATV_VIDEO_STD_NTSC) {
        tmpTunerStd |= V4L2_COLOR_STD_NTSC;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_PAL_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= V4L2_STD_PAL_BG;
        } else if (audioStd == CC_ATV_AUDIO_STD_M) {
            tmpTunerStd |= V4L2_STD_NTSC_M;
        }
    } else if (videoStd == CC_ATV_VIDEO_STD_SECAM) {
        tmpTunerStd |= V4L2_COLOR_STD_SECAM;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_SECAM_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= (V4L2_STD_SECAM_B | V4L2_STD_SECAM_G);
        } else if (audioStd == CC_ATV_AUDIO_STD_M) {
            tmpTunerStd |= V4L2_STD_NTSC_M;
        } else if (audioStd == CC_ATV_AUDIO_STD_L) {
            tmpTunerStd |= V4L2_STD_SECAM_L;
        } else if (audioStd == CC_ATV_AUDIO_STD_LC) {
            tmpTunerStd |= V4L2_STD_SECAM_LC;
        }
    }
#endif
    return tmpTunerStd;
}

int CFrontEnd::getPara(int *mode, int *freq, int *para1, int *para2)
{
#ifdef SUPPORT_ADTV
    struct dvb_frontend_parameters para;
    memset(&para, 0, sizeof(struct dvb_frontend_parameters));
    /*if((ret1=AM_FEND_GetPara(mFrontDevID, &para))<0) {
        LOGD("%s,getPara  faiture\n", __FUNCTION__ );
        return -1;

        //fpara->mode = fend_mode ;

    }*/
#endif
    *mode = mCurMode;
    *freq = mCurFreq;
    *para1 = mCurPara1;
    *para2 = mCurPara2;
    return 0;
}

int CFrontEnd::getSNR()
{
    int snr = 0;
#ifdef SUPPORT_ADTV
    AM_FEND_GetSNR(mFrontDevID, &snr);
#endif
    return snr;
}

int CFrontEnd::getBER()
{
    int ber = 0;
#ifdef SUPPORT_ADTV
    AM_FEND_GetBER(mFrontDevID, &ber);
#endif
    return ber;
}

int CFrontEnd::getStrength()
{
    int Strength = 0;
#ifdef SUPPORT_ADTV
    AM_FEND_GetStrength(mFrontDevID, &Strength);
#endif
    return Strength;
}

int CFrontEnd::formatATVFreq(int tmp_freq)
{
    //const int ATV_1MHZ = 1000000;
    const int ATV_50KHZ = 50 * 1000;
    const int ATV_25KHZ = 25 * 1000;
    int mastar = (tmp_freq / ATV_50KHZ) * ATV_50KHZ;
    int follow = tmp_freq % ATV_50KHZ;
    if (follow >= ATV_25KHZ) {
        follow = ATV_50KHZ;
    } else {
        follow = 0;
    }
    return mastar + follow;
}

int CFrontEnd::printVideoStdStr(int compStd, char strBuffer[], int buff_size)
{
    memset(strBuffer, 0, buff_size);
    int videoStd = stdAndColorToVideoEnum(compStd);
    switch (videoStd) {
    case CC_ATV_VIDEO_STD_AUTO:
        strcpy(strBuffer, "AUTO");
        break;
    case CC_ATV_VIDEO_STD_PAL:
        strcpy(strBuffer, "PAL");
        break;
    case CC_ATV_VIDEO_STD_NTSC:
        strcpy(strBuffer, "NTSC");
        break;
    case CC_ATV_VIDEO_STD_SECAM:
        strcpy(strBuffer, "SECAM");
        break;
    default:
        strcpy(strBuffer, "UnkownVideo");
        break;
    }

    return 0;
}

int CFrontEnd::printAudioStdStr(int compStd, char strBuffer[], int buff_size)
{
    memset(strBuffer, 0, buff_size);
    int audioStd = stdAndColorToAudioEnum(compStd);
    switch (audioStd) {
    case CC_ATV_AUDIO_STD_DK:
        strcpy(strBuffer, "DK");
        break;
    case CC_ATV_AUDIO_STD_I:
        strcpy(strBuffer, "I");
        break;
    case CC_ATV_AUDIO_STD_BG:
        strcpy(strBuffer, "BG");
        break;
    case CC_ATV_AUDIO_STD_M:
        strcpy(strBuffer, "M");
        break;
    case CC_ATV_AUDIO_STD_L:
        strcpy(strBuffer, "L");
        break;
    case CC_ATV_AUDIO_STD_AUTO:
        strcpy(strBuffer, "AUTO");
        break;
    default:
        strcpy(strBuffer, "UnkownAudio");
        break;
    }

    return 0;
}

int CFrontEnd::autoLoadFE()
{
    FILE *fp = NULL;
    int ret = -1;

    fp = fopen ( "/sys/class/amlfe/setting", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/amlfe/setting error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    ret = fprintf ( fp, "%s", "autoload" );

    if ( ret < 0 ) {
        LOGW ( "autoload FE error(%s)!\n", strerror ( errno ) );
    }
    LOGD("autoLoadFE");
    fclose ( fp );
    fp = NULL;

    return ret;
}

int CFrontEnd::setCvbsAmpOut(int tmp)
{
    int rc = DVB_SUCCESS;
#ifdef SUPPORT_ADTV
    rc = AM_FEND_SetCvbsAmpOut(mFrontDevID, tmp);
    if (rc == DVB_SUCCESS) {
        LOGD("%s, sucessful!", __FUNCTION__);
        rc =  0;
    } else {
        LOGD("%s, fail!", __FUNCTION__);
        rc =  -1;
    }
#endif
    return rc;
}

int CFrontEnd::setThreadDelay(int delay)
{
    int rc = DVB_SUCCESS;
#ifdef SUPPORT_ADTV
    rc = AM_FEND_SetThreadDelay(mFrontDevID, delay);
    if (rc == DVB_SUCCESS) {
        LOGD("frontend_setThreadDelay sucessful!\n");
        return 0;
    }
#endif
    LOGD("frontend_setThreadDelay fail! %d\n\n", rc);
    return -1;
}

int CFrontEnd::lock(int frequency, int symbol_rate, int modulation, int bandwidth __unused)
{
    int rt = -1;
#ifdef SUPPORT_ADTV
    struct dvb_frontend_parameters fparam;
    struct dvb_frontend_info finfo;

    memset(&fparam, 0, sizeof(struct dvb_frontend_parameters));
    memset(&finfo, 0, sizeof(struct dvb_frontend_info));

    AM_FEND_GetInfo(mFrontDevID, &finfo);

    if (frequency == 0)
        return -1;
    fparam.frequency = frequency;

    switch (finfo.type) {

    case TV_FE_QAM:
    default:

        if (symbol_rate == 0)
            return -1;
        fparam.u.qam.symbol_rate = symbol_rate;


        if (modulation == 0)
            return -1;
        fparam.u.qam.modulation = (fe_modulation_t)modulation;

        LOGD("mFrontDevID = %d;fre=%d;sym=%d;qam=%d ",
             mFrontDevID, fparam.frequency, fparam.u.qam.symbol_rate, fparam.u.qam.modulation);
        break;
    case TV_FE_OFDM:
    case TV_FE_ATSC:
        LOGD("mFrontDevID = %d;fre=%d;bw=%d\n ",
             mFrontDevID, fparam.frequency, fparam.u.ofdm.bandwidth);
        break;
    case TV_FE_QPSK:
        LOGD("QPSK are not supported.\n ");
        break;
    case TV_FE_ANALOG:
        LOGD("ANALOG is supported.\n ");
        fparam.u.analog.std =   V4L2_STD_PAL_I;
        break;
    }

    fe_status_t status;

    rt =  AM_FEND_Lock(FE_DEV_ID, &fparam, &status);

    if ((!rt) && (status & TV_FE_HAS_LOCK)) {
        LOGD("frontend_lock sucessful!\n");
        return true;
    }
#endif
    LOGD("frontend_lock fail %d!\n", rt);
    return false;
}

int CFrontEnd::getInfo()
{
#ifdef SUPPORT_ADTV
    struct dvb_frontend_info finfo;

    AM_FEND_GetInfo(mFrontDevID, &finfo);
#endif
    //return fend_info;  noitfy FrontEnd message
    return 0;
}

int CFrontEnd::setTunerAfc(int afc)
{
#ifdef SUPPORT_ADTV
    AM_FEND_SetAfc(FE_DEV_ID, afc);
#endif
    return 0;
}

int CFrontEnd::getStatus()
{
#ifdef SUPPORT_ADTV
    fe_status_t status;
    AM_FEND_GetStatus(mFrontDevID, &status);

    return status;
#else
    return -1;
#endif
}

int CFrontEnd::checkStatusOnce()
{
#ifdef SUPPORT_ADTV
    fe_status_t status;
    AM_FEND_GetStatus(mFrontDevID, &status);
    LOGD("%s,get status = %x", __FUNCTION__, status);
    if (status &  TV_FE_HAS_LOCK) {
        mCurSigEv.mCurSigStaus = FEEvent::EVENT_FE_HAS_SIG;
        mCurSigEv.mCurFreq = 0;
        mpObserver->onEvent(mCurSigEv);
    } else if (status & TV_FE_TIMEDOUT) {
        mCurSigEv.mCurSigStaus = FEEvent::EVENT_FE_NO_SIG;
        mCurSigEv.mCurFreq = 0;
        mpObserver->onEvent(mCurSigEv);
    }
#endif
    return 0;
}

int CFrontEnd::stdEnumToCvbsFmt (int vfmt, unsigned long std)
{
    tvin_sig_fmt_e cvbs_fmt = TVIN_SIG_FMT_NULL;
#ifdef SUPPORT_ADTV
    if ((vfmt & 0xff000000) == V4L2_COLOR_STD_NTSC) {
        switch (vfmt & 0x00ffffff) {
        case V4L2_STD_NTSC_M:
        case V4L2_STD_NTSC_443:
        case V4L2_STD_PAL_I:
            if ((std & V4L2_STD_PAL_M) || (std & V4L2_STD_NTSC_M)) {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
            } else if ((std & V4L2_STD_PAL_DK) || (std & V4L2_STD_PAL_BG)
                    || (std & V4L2_STD_PAL_I)) {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_443;
            } else {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
            }
            break;
        default:
            cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
            break;
        }
    } else if ((vfmt & 0xff000000) == V4L2_COLOR_STD_PAL) {
        switch (vfmt & 0x00ffffff) {
        case V4L2_STD_PAL_DK:
        case V4L2_STD_PAL_BG:
        case V4L2_STD_PAL_I:
        case V4L2_STD_PAL_M:
        case V4L2_STD_NTSC_M:
            if ((std & V4L2_STD_PAL_M) || (std & V4L2_STD_NTSC_M)) {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_M;
            } else {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
            }
            break;
        case V4L2_STD_PAL_60:
            if ((std & V4L2_STD_PAL_M) || (std & V4L2_STD_NTSC_M)) {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_60;
            } else {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
            }
            break;
        case V4L2_STD_PAL_Nc:
            if ((std & V4L2_STD_PAL_M) || (std & V4L2_STD_NTSC_M)) {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_CN;
            } else {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
            }
            break;
        default:
            cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
            break;
        }
    } else if ((vfmt & 0xff000000) == V4L2_COLOR_STD_SECAM) {
        switch (vfmt & 0x00ffffff) {
        case V4L2_STD_SECAM_L:
        case V4L2_STD_SECAM_LC:
            cvbs_fmt = TVIN_SIG_FMT_CVBS_SECAM;
            break;
        default:
            cvbs_fmt = TVIN_SIG_FMT_CVBS_SECAM;
            break;
        }
    }

    LOGD("stdEnumToCvbsFmt vfmt:0x%x, cvbs_fmt:0x%x, std: 0x%x", vfmt, cvbs_fmt, std);
#endif
    return cvbs_fmt;
}

int CFrontEnd::ClearAnalogFrontEnd()
{
#ifdef SUPPORT_ADTV
    return this->setPara ( TV_FE_ANALOG, 44250000, V4L2_COLOR_STD_PAL | V4L2_STD_PAL_DK, 0 );
#else
    return -1;
#endif
}

/*to control afc function*/
int CFrontEnd::SetAnalogFrontEndTimerSwitch(int onOff)
{
    FILE *fp = NULL;
    int ret = -1;

    fp = fopen ( "/sys/module/aml_fe/parameters/aml_timer_en", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/aml_fe/parameters/aml_timer_en error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    if (onOff)
        ret = fprintf(fp, "%s", "1");
    else
        ret = fprintf(fp, "%s", "0");
    if ( ret < 0 ) {
        LOGW ( "set aml_timer_en error(%s)!\n", strerror ( errno ) );
    }
    LOGD("SetAnalogFrontEndTimerSwitch %d", onOff);
    fclose ( fp );
    fp = NULL;

    return ret;
}

int CFrontEnd::SetAnalogFrontEndSearhSlowMode(int onOff)
{
    FILE *fp = NULL;
    int ret = -1;

    fp = fopen ( "/sys/module/aml_fe/parameters/slow_mode", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/aml_fe/parameters/slow_mode error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    if (onOff)
        ret = fprintf(fp, "%s", "1");
    else
        ret = fprintf(fp, "%s", "0");
    if ( ret < 0 ) {
        LOGW ( "set aml_timer_en error(%s)!\n", strerror ( errno ) );
    }
    LOGD("SetAnalogFrontEndSearhSlowMode %d", onOff);
    fclose ( fp );
    fp = NULL;

    return ret;
}

CFrontEnd::FEMode& CFrontEnd::FEMode::set8(int n, int v)
{
    mMode = ((mMode & ~(0xff << (8 * n))) | ((v & 0xff) << (8 * n)));
    return *this;
}

int CFrontEnd::FEMode::get8(int n) const
{
    return (mMode >> (8 * n)) & 0xff;
}

bool CFrontEnd::FEMode::operator == (const FEMode &femode) const
{
    return ((getBase() == femode.getBase()) && (getGen() == femode.getGen()));
}


const char* CFrontEnd::FEParas::FEP_MODE = "mode";
const char* CFrontEnd::FEParas::FEP_FREQ = "freq";
const char* CFrontEnd::FEParas::FEP_FREQ2 = "freq2";
const char* CFrontEnd::FEParas::FEP_BW = "bw";
const char* CFrontEnd::FEParas::FEP_SR = "sr";
const char* CFrontEnd::FEParas::FEP_MOD = "mod";
const char* CFrontEnd::FEParas::FEP_PLP = "plp";
const char* CFrontEnd::FEParas::FEP_LAYR = "layr";
const char* CFrontEnd::FEParas::FEP_VSTD = "vtd";
const char* CFrontEnd::FEParas::FEP_ASTD = "atd";
const char* CFrontEnd::FEParas::FEP_AFC = "afc";
const char* CFrontEnd::FEParas::FEP_VFMT = "vfmt";
const char* CFrontEnd::FEParas::FEP_SOUNDSYS = "soundsys";

bool CFrontEnd::FEParas::operator == (const FEParas &fep) const
{
    if (!(getFEMode() == fep.getFEMode()))
        return false;
    if (getFrequency() != fep.getFrequency())
        return false;
    if (getFEMode().getGen() && (getPlp() != fep.getPlp()))
        return false;

    switch (getFEMode().getBase()) {
        case TV_FE_DTMB:
        case TV_FE_OFDM:
            if (getBandwidth() != fep.getBandwidth())
                return false;
            break;
        case TV_FE_QAM:
            if (getSymbolrate() != fep.getSymbolrate() || getModulation() != fep.getModulation())
                return false;
            break;
        case TV_FE_ATSC:
            if (getModulation() != fep.getModulation())
                return false;
            break;
        case TV_FE_ISDBT:
            if (getModulation() != fep.getModulation() || getLayer() != fep.getLayer())
                return false;
            break;
        case TV_FE_ANALOG:
            if (getVideoStd() != fep.getVideoStd() || getAudioStd() != fep.getAudioStd() || getAfc() != fep.getAfc()
                 || getVFmt() != fep.getVFmt() || getSoundsys() != fep.getSoundsys())
                return false;
            break;
        default:
            return false;
            break;
    }
    return true;
}

#ifdef SUPPORT_ADTV
int CFrontEnd::GetTSSource(AM_DMX_Source_t *src)
{
    int isTV = config_get_int(CFG_SECTION_TV, FRONTEND_TS_SOURCE, 0);

    if (isTV == 2) {
        *src = AM_DMX_SRC_TS2;
    } else if (isTV == 1) {
        *src = AM_DMX_SRC_TS1;
    }else{
        *src = AM_DMX_SRC_TS0;
    }
    return 0;
}

CFrontEnd::FEParas& CFrontEnd::FEParas::fromDVBParameters(const CFrontEnd::FEMode& mode,
        const struct dvb_frontend_parameters *dvb)
{
    setFEMode(mode).setFrequency(dvb->frequency);
    switch (mode.getBase()) {
    case TV_FE_OFDM:
    case TV_FE_DTMB:
    default:
        setBandwidth(dvb->u.ofdm.bandwidth);
    break;
    case TV_FE_QAM:
        setSymbolrate(dvb->u.qam.symbol_rate);
        setModulation(dvb->u.qam.modulation);
    break;
    case TV_FE_ATSC:
        setModulation(dvb->u.vsb.modulation);
    break;
    case TV_FE_ANALOG:
        setVideoStd(stdAndColorToVideoEnum(dvb->u.analog.std));
        setAudioStd(stdAndColorToAudioEnum(dvb->u.analog.std));
    break;
    }
    return *this;
}

CFrontEnd::FEParas& CFrontEnd::FEParas::fromFENDCTRLParameters(const CFrontEnd::FEMode& mode,
        const AM_FENDCTRL_DVBFrontendParameters_t *fendctrl)
{
    return fromDVBParameters(mode, (struct dvb_frontend_parameters *)fendctrl);
}
#endif

