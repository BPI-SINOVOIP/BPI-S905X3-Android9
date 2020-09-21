/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvScanner"

#include "CTvScanner.h"
#include "CTvChannel.h"
#include "CTvProgram.h"
#include "CTvRegion.h"
#include "CFrontEnd.h"

#include <tvconfig.h>

#ifdef SUPPORT_ADTV
#define dvb_fend_para(_p) ((struct dvb_frontend_parameters*)(&_p))
#define IS_DVBT2_TS(_para) (_para.m_type == TV_FE_OFDM && _para.terrestrial.ofdm_mode == OFDM_DVBT2)
#define IS_ISDBT_TS(_para) (_para.m_type == TV_FE_ISDBT)
#define VALID_PID(_pid_) ((_pid_)>0 && (_pid_)<0x1fff)
#endif

CTvScanner *CTvScanner::mInstance;
CTvScanner::ScannerEvent CTvScanner::mCurEv;
CTvScanner::service_list_t CTvScanner::service_list_dummy;

CTvScanner *CTvScanner::getInstance()
{
    if (NULL == mInstance) mInstance = new CTvScanner();
    return mInstance;
}

CTvScanner::CTvScanner()
{
    mbScanStart = false;
    mpObserver = NULL;
    mSource = 0xff;
    mMinFreq = 1;
    mMaxFreq = 100;
    mCurScanStartFreq = 1;
    mCurScanEndFreq = 100;
    mVbi = NULL;
    mScanHandle = NULL;
    mpTvin = NULL;
    mMode = 0;
    mFendID = 0;
    mTvMode = 0;
    mTvOptions = 0;
    mSat_id = 0;
    mTsSourceID = 0;
    mAtvMode = 0;
    mStartFreq = 0;
    mDirection = 0;
    mChannelID = 0;
    tunerStd = 0;
    demuxID = 0;
    user_band = 0;
    ub_freq = 0;
    mFEType = 0;
    mVbiTsId = 0;
    mAtvIsAtsc = 0;
}

CTvScanner::~CTvScanner()
{
#ifdef SUPPORT_ADTV
    AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_PROGRESS, evtCallback, NULL);
    AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_SIGNAL, evtCallback, NULL);
#endif
}

int CTvScanner::Scan(char *feparas, char *scanparas) {
    CFrontEnd::FEParas fe(feparas);
    ScanParas sp(scanparas);
    return Scan(fe, sp);
}

int CTvScanner::Scan(CFrontEnd::FEParas &fp, ScanParas &sp) {
 #ifdef SUPPORT_ADTV
    stopScan();

    if (mbScanStart) {
        LOGW("scan is scanning, need first stop it");
        return -1;
    }

    AM_SCAN_CreatePara_t para;
    AM_DMX_OpenPara_t dmx_para;
    AM_SCAN_Handle_t handle = 0;
    //int i;

    LOGD("Scan fe[%s] scan[%s]", fp.toString().c_str(), sp.toString().c_str());

    mFEParas = fp;
    mScanParas = sp;

    /*for convenient use*/
    mCurScanStartFreq = sp.getAtvFrequency1();
    mCurScanEndFreq = sp.getAtvFrequency2();

    //reset scanner status
    mCurEv.reset();
    mFEType = -1;

    mAtvIsAtsc = 0;
    // Create the scan
    memset(&para, 0, sizeof(para));
    para.fend_dev_id = 0;//default
    para.vlfend_dev_id = 0;
    para.mode = sp.getMode();
    para.proc_mode = sp.getProc();
    if (createAtvParas(para.atv_para, fp, sp) != 0)
        return -1;
    if (createDtvParas(para.dtv_para, fp, sp) != 0) {
        freeAtvParas(para.atv_para);
        return -1;
    }
    const char *db_mode = config_get_str ( CFG_SECTION_TV, SYS_SCAN_TO_PRIVATE_DB, "false");
    if (!strcmp(db_mode, "true")) {
        para.store_cb = NULL;
    } else {
        para.store_cb = storeScanHelper;
    }

    // Start Scan
    memset(&dmx_para, 0, sizeof(dmx_para));
    AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);
    /*get scramble prop value,default value 0 is ca des*/
    int mode = config_get_int(CFG_SECTION_TV, CFG_DTV_CHECK_SCRAMBLE_MODE, 0);
    if (mode == 1) {
        para.dtv_para.mode |= AM_SCAN_DTVMODE_SCRAMB_TSHEAD;
    }
    mode = config_get_int(CFG_SECTION_TV, CFG_DTV_SCAN_STOREMODE_FTA, 0);
    if (mode == 1) {
        para.dtv_para.mode |= AM_SCAN_DTVMODE_FTA;
    }
    mode = config_get_int(CFG_SECTION_TV, CFG_DTV_SCAN_STOREMODE_NOPAL, 0);
    if (mode == 1) {
        para.store_mode |= AM_SCAN_ATV_STOREMODE_NOPAL;
    }

    mode = config_get_int(CFG_SECTION_TV, CFG_DTV_SCAN_STOREMODE_NOVCT, 0);
    if (mode == 1) {
        para.dtv_para.mode |= AM_SCAN_DTVMODE_NOVCT;
    }

    mode = config_get_int(CFG_SECTION_TV, CFG_DTV_SCAN_STOREMODE_NOVCTHIDE, 0);
    if (mode == 1) {
        para.dtv_para.mode |= AM_SCAN_DTVMODE_NOVCTHIDE;
    }

    mode = config_get_int(CFG_SECTION_TV, CFG_DTV_CHECK_SCRAMBLE_AV, 0);
    if (mode == 1) {
        para.dtv_para.mode |= AM_SCAN_DTVMODE_CHECKDATA;
    }

    mode = config_get_int(CFG_SECTION_TV, CFG_DTV_CHECK_DATA_AUDIO, 0);
    if (mode == 1) {
        LOGW("CFG_DTV_CHECK_DATA_AUDIO");
        para.dtv_para.mode |= AM_SCAN_DTVMODE_CHECK_AUDIODATA;
    }

    mode = config_get_int(CFG_SECTION_TV, CFG_DTV_SCAN_STOREMODE_VALIDPID, 0);
    if (mode == 1) {
        para.dtv_para.mode |= AM_SCAN_DTVMODE_INVALIDPID;
    }

    if (AM_SCAN_Create(&para, &handle) != DVB_SUCCESS) {
        LOGD("SCAN CREATE fail");
        handle = NULL;
    } else {
        AM_SCAN_Helper_t ts_type_helper;
        ts_type_helper.id = AM_SCAN_HELPER_ID_FE_TYPE_CHANGE;
        ts_type_helper.user = (void*)this;
        ts_type_helper.cb = FETypeHelperCBHelper;
        AM_SCAN_SetHelper(handle, &ts_type_helper);
        mScanHandle = handle;
        AM_SCAN_SetUserData(handle, (void *)this);
        AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_PROGRESS, evtCallback, NULL);
        AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_SIGNAL, evtCallback, NULL);
        if (AM_SCAN_Start(handle) != DVB_SUCCESS) {
            AM_SCAN_Destroy(handle, false);
            AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_PROGRESS, evtCallback, NULL);
            AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_SIGNAL, evtCallback, NULL);
            handle = NULL;
        }
    }

    freeAtvParas(para.atv_para);
    freeDtvParas(para.dtv_para);

    if (handle == NULL) {
        return -1;
    }
    mbScanStart = true;//start call ok
 #endif
    return 0;
}

int CTvScanner::pauseScan()
{
    LOGD("pauseScan scan started:%d", mbScanStart);
 #ifdef SUPPORT_ADTV
    if (mbScanStart) { //if start ok and not stop
        int ret = AM_SCAN_Pause(mScanHandle);
        LOGD("pauseScan , ret=%d", ret);
        return ret;
    }
 #endif
    return 0;
}

int CTvScanner::resumeScan()
{
    LOGD("resumeScan scan started:%d", mbScanStart);
 #ifdef SUPPORT_ADTV
    if (mbScanStart) { //if start ok and not stop
        int ret = AM_SCAN_Resume(mScanHandle);
        LOGD("resumeScan , ret=%d", ret);
        return ret;
    }
 #endif
    return 0;
}

int CTvScanner::stopScan()
{
    LOGD("StopScan is started:%d", mbScanStart);
 #ifdef SUPPORT_ADTV
    if (mbScanStart) { //if start ok and not stop
        if (needVbiAssist())
            stopVBI();
        AM_SCAN_Destroy(mScanHandle, true);
        AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_PROGRESS, evtCallback, NULL);
        AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_SIGNAL, evtCallback, NULL);
        AM_SEC_Cache_Reset(0);
        //stop loop
        mbScanStart = false;//stop ok
        mFEType = -1;
    }
 #endif
    return 0;
}

int CTvScanner::getScanStatus(int *status)
{
    LOGD("getScanStatus scan started:%d", mbScanStart);
 #ifdef SUPPORT_ADTV
    if (mbScanStart && status) { //if start ok and not stop
        int ret = AM_SCAN_GetStatus(mScanHandle, status);
        LOGD("getScanStatus = [%d], ret=%d", *status, ret);
        return ret;
    }
 #endif
    return 0;
}


int CTvScanner::getParamOption(const char *para) {
    int forcePara = -1;
    char paraForce[64];
    snprintf(paraForce, sizeof(paraForce), "dtv.scan.%s", para);
    const char *paraForced = config_get_str ( CFG_SECTION_TV, paraForce, "null");
    if (sscanf(paraForced, "%i", &forcePara)) {
        LOGD("option %s: %d", para, forcePara);
    }
    return forcePara;
}

int CTvScanner::insertLcnList(lcn_list_t &llist, ScannerLcnInfo *lcn, int idx)
{
    int found = 0;

    for (lcn_list_t::iterator p=llist.begin(); p != llist.end(); p++) {
        ScannerLcnInfo *pl = *p;
        //LOGD("list size:%d, pl:%#x", llist.size(), pl);

        if ((pl->net_id == lcn->net_id)
               && (pl->ts_id == lcn->ts_id)
               && (pl->service_id == lcn->service_id)) {
            pl->lcn[idx] = lcn->lcn[idx];
            pl->visible[idx] = lcn->visible[idx];
            pl->valid[idx] = lcn->valid[idx];
            found = 1;
        }
    }
    if (!found) {
        llist.push_back(lcn);
    }
    return found ? 1 : 0; //found = insert fail.
}

void CTvScanner::notifyLcn(ScannerLcnInfo *lcn)
{
    mCurEv.clear();
    mCurEv.mType = ScannerEvent::EVENT_LCN_INFO_DATA;
    mCurEv.mLcnInfo = *lcn;

    getInstance()->sendEvent(mCurEv);
}

void CTvScanner::notifyService(SCAN_ServiceInfo_t *srv)
{
    if (!srv->tsinfo) {
        LOGE("service with no tsinfo.");
        return;
    }

#ifdef SUPPORT_ADTV
    mCurEv.reset();
    mCurEv.mFEParas = srv->tsinfo->fe;
    mCurEv.mFrequency = mCurEv.mFEParas.getFrequency();
    mCurEv.mProgramsInPat = srv->programs_in_pat;

    strncpy(mCurEv.mVct, srv->tsinfo->vct, sizeof(mCurEv.mVct));

    int feType = mCurEv.mFEParas.getFEMode().getBase();
    if (feType != TV_FE_ANALOG) {
        mCurEv.mServiceId = srv->srv_id;
        mCurEv.mONetId = srv->tsinfo->nid;
        mCurEv.mTsId = srv->tsinfo->tsid;
        mCurEv.mPatTsId = srv->tsinfo->pat_ts_id;
        strncpy(mCurEv.mProgramName, srv->name, 1024);
        mCurEv.mprogramType = srv->srv_type;
        mCurEv.mVid = srv->vid;
        mCurEv.mVfmt = srv->vfmt;
        mCurEv.mAcnt = srv->aud_info.audio_count;
        for (int i = 0; i < srv->aud_info.audio_count; i++) {
            mCurEv.mAid[i] = srv->aud_info.audios[i].pid;
            mCurEv.mAfmt[i] = srv->aud_info.audios[i].fmt;
            strncpy(mCurEv.mAlang[i], srv->aud_info.audios[i].lang, 10);
            mCurEv.mAtype[i] = srv->aud_info.audios[i].audio_type;
            mCurEv.mAExt[i] = srv->aud_info.audios[i].audio_exten;
        }
        mCurEv.mPcr = srv->pcr_pid;

        if (srv->tsinfo->dtvstd == TV_SCAN_DTV_STD_ATSC) {
            mCurEv.mAccessControlled = srv->access_controlled;
            mCurEv.mHidden = srv->hidden;
            mCurEv.mHideGuide = srv->hide_guide;
            mCurEv.mSourceId = srv->source_id;
            mCurEv.mMajorChannelNumber = srv->major_chan_num;
            mCurEv.mMinorChannelNumber = srv->minor_chan_num;
            mCurEv.mVctType = srv->vct_type;

            //mCurEv.mScnt = srv->cap_info.caption_count;
            int i, j;
            for (i = 0; i < srv->cap_info.caption_count; i++) {
                //all captions parsed from tables are treated as dtv cc.
                mCurEv.mStype[i] = TYPE_DTV_CC; //srv->cap_info.captions[i].type ? TYPE_DTV_CC : TYPE_ATV_CC;
                mCurEv.mSid[i] = srv->cap_info.captions[i].service_number
                    + (srv->cap_info.captions[i].type ? (AM_CC_CAPTION_SERVICE1-1) : (AM_CC_CAPTION_CC1));
                mCurEv.mSstype[i] = srv->cap_info.captions[i].type ? TYPE_DTV_CC : TYPE_ATV_CC;
                mCurEv.mSid1[i] = srv->cap_info.captions[i].private_data;
                mCurEv.mSid2[i] = srv->cap_info.captions[i].flags;
                strncpy(mCurEv.mSlang[i], srv->cap_info.captions[i].lang, 10);
            }

            for (j = 0; j < srv->scte27_info.subtitle_count; j++) {
                //all captions parsed from tables are treated as dtv cc.
                mCurEv.mStype[i+j] = TYPE_SCTE27;
                mCurEv.mSid[i+j] = srv->scte27_info.subtitles[j].pid;
                mCurEv.mSstype[i+j] = TYPE_SCTE27;
                mCurEv.mSid1[i+j] = 0;
                mCurEv.mSid2[i+j] = 0;
                strncpy(mCurEv.mSlang[i+j], "SCTE", 10);
            }
            mCurEv.mScnt = i+j;

        } else {
            mCurEv.mScnt = srv->sub_info.subtitle_count;
            for (int i = 0; i < srv->sub_info.subtitle_count; i++) {
                mCurEv.mStype[i] = TYPE_DVB_SUBTITLE;
                mCurEv.mSid[i] = srv->sub_info.subtitles[i].pid;
                mCurEv.mSstype[i] = srv->sub_info.subtitles[i].type;
                mCurEv.mSid1[i] = srv->sub_info.subtitles[i].comp_page_id;
                mCurEv.mSid2[i] = srv->sub_info.subtitles[i].anci_page_id;
                strncpy(mCurEv.mSlang[i], srv->sub_info.subtitles[i].lang, 10);
            }
            int scnt = mCurEv.mScnt;
            for (int i = 0; i < srv->ttx_info.teletext_count; i++) {
                if (srv->ttx_info.teletexts[i].type == 0x2 ||
                        srv->ttx_info.teletexts[i].type == 0x5) {
                    if (scnt >= (int)(sizeof(mCurEv.mStype) / sizeof(int)))
                        break;
                    mCurEv.mStype[scnt] = TYPE_DTV_TELETEXT;
                    mCurEv.mSid[scnt] = srv->ttx_info.teletexts[i].pid;
                    mCurEv.mSstype[scnt] = srv->ttx_info.teletexts[i].type;
                    mCurEv.mSid1[scnt] = srv->ttx_info.teletexts[i].magazine_no;
                    mCurEv.mSid2[scnt] = srv->ttx_info.teletexts[i].page_no;
                    strncpy(mCurEv.mSlang[scnt], srv->ttx_info.teletexts[i].lang, 10);
                    scnt++;
                }
            }
            for (int i = 0; i < srv->ttx_info.teletext_count; i++) {
                if (srv->ttx_info.teletexts[i].type != 0x2 &&
                        srv->ttx_info.teletexts[i].type != 0x5) {
                    if (scnt >= (int)(sizeof(mCurEv.mStype) / sizeof(int)))
                        break;
                    mCurEv.mStype[scnt] = TYPE_DTV_TELETEXT_IMG;
                    mCurEv.mSid[scnt] = srv->ttx_info.teletexts[i].pid;
                    mCurEv.mSstype[scnt] = srv->ttx_info.teletexts[i].type;
                    mCurEv.mSid1[scnt] = srv->ttx_info.teletexts[i].magazine_no;
                    mCurEv.mSid2[scnt] = srv->ttx_info.teletexts[i].page_no;
                    strncpy(mCurEv.mSlang[scnt], srv->ttx_info.teletexts[i].lang, 10);
                    scnt++;
                }
            }
            mCurEv.mScnt = scnt;

            mCurEv.mFEParas.setPlp(srv->plp_id);
        }

        mCurEv.mFree_ca = srv->free_ca;
        mCurEv.mScrambled = srv->scrambled_flag;
        mCurEv.mSdtVer = srv->sdt_version;

        mCurEv.mType = ScannerEvent::EVENT_DTV_PROG_DATA;
        LOGD("notifyService freq:%d, sid:%d", mCurEv.mFrequency, srv->srv_id);

    } else {//analog

        mCurEv.mVideoStd = mCurEv.mFEParas.getVideoStd();
        mCurEv.mAudioStd = mCurEv.mFEParas.getAudioStd();
        mCurEv.mVfmt = mCurEv.mFEParas.getVFmt();
        mCurEv.mIsAutoStd = ((mCurEv.mVideoStd & V4L2_COLOR_STD_AUTO) == V4L2_COLOR_STD_AUTO) ? 1 : 0;

        if (mAtvIsAtsc) {
            mCurEv.mAccessControlled = srv->access_controlled;
            mCurEv.mHidden = srv->hidden;
            mCurEv.mHideGuide = srv->hide_guide;
            mCurEv.mSourceId = srv->source_id;
            mCurEv.mMajorChannelNumber = srv->major_chan_num;
            mCurEv.mMinorChannelNumber = srv->minor_chan_num;

            LOGD("add cc info [%d]", srv->cap_info.caption_count);
            mCurEv.mScnt = srv->cap_info.caption_count;
            for (int i = 0; i < srv->cap_info.caption_count; i++) {
                mCurEv.mStype[i] = srv->cap_info.captions[i].type ? TYPE_DTV_CC : TYPE_ATV_CC;
                mCurEv.mSid[i] = srv->cap_info.captions[i].service_number;
                mCurEv.mSstype[i] = srv->cap_info.captions[i].type;
                mCurEv.mSid1[i] = srv->cap_info.captions[i].pid_or_line21;
                mCurEv.mSid2[i] = srv->cap_info.captions[i].flags;
                strncpy(mCurEv.mSlang[i], srv->cap_info.captions[i].lang, 10);
            }

        }

        mCurEv.mType = ScannerEvent::EVENT_ATV_PROG_DATA;
        LOGD("notifyService freq:%d, vstd:%x astd:%x vfmt:%x",
            mCurEv.mFrequency, mCurEv.mFEParas.getVideoStd(), mCurEv.mFEParas.getAudioStd(), mCurEv.mFEParas.getVFmt());
    }

    if ((feType == TV_FE_ANALOG) || (mCurEv.mVid != 0x1fff) || mCurEv.mAcnt)
        getInstance()->sendEvent(mCurEv);
#endif
}

void CTvScanner::sendEvent(ScannerEvent &evt)
{
    if (mpObserver) {
        strcpy(mCurEv.mParas, "{");
        //fe para
        snprintf(mCurEv.mParas, sizeof(mCurEv.mParas), "%s\"fe\":", mCurEv.mParas);
        snprintf(mCurEv.mParas, sizeof(mCurEv.mParas), "%s%s",
            mCurEv.mParas, mCurEv.mFEParas.toString().c_str());
        //other para
        if (mCurEv.mVctType) {
            // service para
            snprintf(mCurEv.mParas, sizeof(mCurEv.mParas), "%s,\"srv\":{\"vct\":%d}",
                mCurEv.mParas, mCurEv.mVctType);
        }
        snprintf(mCurEv.mParas, sizeof(mCurEv.mParas), "%s}", mCurEv.mParas);
        LOGD("Paras:%s", mCurEv.mParas);

        mpObserver->onEvent(evt);
    }
}

#ifdef SUPPORT_ADTV
void CTvScanner::getLcnInfo(AM_SCAN_Result_t *result, AM_SCAN_TS_t *sts, lcn_list_t &llist)
{
    dvbpsi_nit_t *nits = ((sts->type == AM_SCAN_TS_ANALOG) || (result->start_para->dtv_para.standard == TV_SCAN_DTV_STD_ATSC)) ?
            NULL : sts->digital.nits;
    dvbpsi_nit_ts_t *ts;
    dvbpsi_descriptor_t *dr;
    dvbpsi_nit_t *nit;
    ScannerLcnInfo *plcninfo;

    UNUSED(result);

    AM_SI_LIST_BEGIN(nits, nit)
        AM_SI_LIST_BEGIN(nit->p_first_ts, ts)
            AM_SI_LIST_BEGIN(ts->p_first_descriptor, dr)
                if (dr->p_decoded && (dr->i_tag == AM_SI_DESCR_LCN_83)) {
                dvbpsi_logical_channel_number_83_dr_t *lcn_dr = (dvbpsi_logical_channel_number_83_dr_t*)dr->p_decoded;
                dvbpsi_logical_channel_number_83_t *lcn = lcn_dr->p_logical_channel_number;
                int j;
                for (j=0; j<lcn_dr->i_logical_channel_numbers_number; j++) {
                        plcninfo = (ScannerLcnInfo*)calloc(sizeof(ScannerLcnInfo),1);
                        plcninfo->net_id = ts->i_orig_network_id;
                        plcninfo->ts_id = ts->i_ts_id;
                        plcninfo->service_id = lcn->i_service_id;
                        plcninfo->lcn[0] = lcn->i_logical_channel_number;
                        plcninfo->visible[0] = lcn->i_visible_service_flag;
                        plcninfo->valid[0] = 1;
                        LOGD("sd lcn for service [%d:%d:%d] ---> l:%d v:%d",
                                plcninfo->net_id, plcninfo->ts_id, plcninfo->service_id,
                                plcninfo->lcn[0], plcninfo->visible[0]);
                        if (insertLcnList(llist, plcninfo, 0)) {
                            free(plcninfo);
                            LOGD("lcn exists 0.");
                        }
                        lcn++;
                    }
                } else if (dr->p_decoded && dr->i_tag==AM_SI_DESCR_LCN_88) {
                    dvbpsi_logical_channel_number_88_dr_t *lcn_dr = (dvbpsi_logical_channel_number_88_dr_t*)dr->p_decoded;
                    dvbpsi_logical_channel_number_88_t *lcn = lcn_dr->p_logical_channel_number;
                    int j;
                    for (j=0; j<lcn_dr->i_logical_channel_numbers_number; j++) {
                        plcninfo = (ScannerLcnInfo*)calloc(sizeof(ScannerLcnInfo), 1);
                        plcninfo->net_id = ts->i_orig_network_id;
                        plcninfo->ts_id = ts->i_ts_id;
                        plcninfo->service_id = lcn->i_service_id;
                        plcninfo->lcn[1] = lcn->i_logical_channel_number;
                        plcninfo->visible[1] = lcn->i_visible_service_flag;
                        plcninfo->valid[1] = 1;
                        LOGD("hd lcn for service [%d:%d:%d] ---> l:%d v:%d",
                               plcninfo->net_id, plcninfo->ts_id, plcninfo->service_id,
                               plcninfo->lcn[1], plcninfo->visible[1]);
                        if (insertLcnList(llist, plcninfo, 1)) {
                            free(plcninfo);
                            LOGD("lcn exists 1.");
                        }
                        lcn++;
                    }
                }
            AM_SI_LIST_END()
        AM_SI_LIST_END()
    AM_SI_LIST_END()
}

void CTvScanner::processTsInfo(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, SCAN_TsInfo_t *ts_info)
{
    //dvbpsi_nit_t *nit;
    //dvbpsi_descriptor_t *descr;

    ts_info->nid = -1;
    ts_info->tsid = -1;
    ts_info->pat_ts_id = -1;
    ts_info->vct[0] = 0;

    if (ts->type == AM_SCAN_TS_ANALOG) {
        CFrontEnd::FEMode mode(mFEParas.getFEMode());
        mode.setBase(TV_FE_ANALOG);
        ts_info->fe.clear();
        ts_info->fe.setFEMode(mode).setFrequency(ts->analog.freq)
            .setVideoStd(CFrontEnd::stdAndColorToVideoEnum(ts->analog.std))
            .setAudioStd(CFrontEnd::stdAndColorToAudioEnum(ts->analog.audmode))
            .setVFmt(ts->analog.std);
        LOGD("processTsInfo [%d][0x%x][0x%x].", ts->analog.freq, (unsigned int) ts->analog.std, ts->analog.audmode);
    } else {
        /*tsid*/
       dvbpsi_pat_t *pats = getValidPats(ts);
       if (pats != NULL) {
           ts_info->pat_ts_id = pats->i_ts_id;
       }
       if (pats != NULL && !ts->digital.use_vct_tsid) {
            ts_info->tsid = pats->i_ts_id;
            if (ts->digital.sdts)
                ts_info->tsid = ts->digital.sdts->i_ts_id;
            else if (IS_DVBT2_TS(ts->digital.fend_para) && ts->digital.dvbt2_data_plp_num > 0 && ts->digital.dvbt2_data_plps[0].sdts)
                ts_info->tsid = ts->digital.dvbt2_data_plps[0].sdts->i_ts_id;
        } else if (ts->digital.vcts != NULL) {
            ts_info->tsid = ts->digital.vcts->i_extension;
        }

        /*nid*/
        if (result->start_para->dtv_para.standard != TV_SCAN_DTV_STD_ATSC ) {
            if (ts->digital.sdts)
                ts_info->nid = ts->digital.sdts->i_network_id;
            else if (IS_DVBT2_TS(ts->digital.fend_para) && ts->digital.dvbt2_data_plp_num > 0 && ts->digital.dvbt2_data_plps[0].sdts)
                ts_info->nid = ts->digital.dvbt2_data_plps[0].sdts->i_network_id;
        }

        CFrontEnd::FEMode mode(mFEParas.getFEMode());
        mode.setBase(ts->digital.fend_para.m_type);
        ts_info->fe.clear();
        ts_info->fe.fromFENDCTRLParameters(mode, &ts->digital.fend_para);
    }
    ts_info->dtvstd = result->start_para->dtv_para.standard;

    /* Build VCT string.*/
    if (ts_info->dtvstd == TV_SCAN_DTV_STD_ATSC) {
        dvbpsi_atsc_vct_t *vct;
        dvbpsi_atsc_vct_channel_t *vcinfo;
        char *ptr  = ts_info->vct;
        int   left = sizeof(ts_info->vct) - 1;
        int   r;
        LOGD("vct buffer set start");
        AM_SI_LIST_BEGIN(ts->digital.vcts, vct)
        AM_SI_LIST_BEGIN(vct->p_first_channel, vcinfo)
             r = snprintf(ptr, left, (ptr == ts_info->vct) ? "%d:%d-%d" : ",%d:%d-%d", vcinfo->i_source_id, vcinfo->i_major_number, vcinfo->i_minor_number);
             if (r >= left) {
                 LOGD("vct buffer is too small");
             } else {
                 ptr  += r;
                 left -= r;
             }
        AM_SI_LIST_END()
        AM_SI_LIST_END()
        LOGD("vct buffer set end [%s]", ts_info->vct);
        *ptr = 0;
    }
}

dvbpsi_pat_t *CTvScanner::getValidPats(AM_SCAN_TS_t *ts)
{
    dvbpsi_pat_t *valid_pat = NULL;
    if (!IS_DVBT2_TS(ts->digital.fend_para)) {
        if (IS_ISDBT_TS(ts->digital.fend_para)) {
            /* process for isdbt one-seg inserted PAT, which ts_id is 0xffff */
            valid_pat = ts->digital.pats;

            while (valid_pat != NULL && valid_pat->i_ts_id == 0xffff) {
                valid_pat = valid_pat->p_next;
            }

            if (valid_pat == NULL && ts->digital.pats != NULL) {
                valid_pat = ts->digital.pats;

                if (ts->digital.sdts != NULL)
                    valid_pat->i_ts_id = ts->digital.sdts->i_ts_id;
            }
        } else {
            valid_pat = ts->digital.pats;
        }
    } else {
        for (int plp = 0; plp < ts->digital.dvbt2_data_plp_num; plp++) {
            if (ts->digital.dvbt2_data_plps[plp].pats != NULL) {
                valid_pat = ts->digital.dvbt2_data_plps[plp].pats;
                break;
            }
        }
    }

    return valid_pat;
}

int CTvScanner::getPmtPid(dvbpsi_pat_t *pats, int program_number)
{
    dvbpsi_pat_t *pat;
    dvbpsi_pat_program_t *prog;

    AM_SI_LIST_BEGIN(pats, pat)
    AM_SI_LIST_BEGIN(pat->p_first_program, prog)
    if (prog->i_number == program_number)
        return prog->i_pid;
    AM_SI_LIST_END()
    AM_SI_LIST_END()

    return 0x1fff;
}

void CTvScanner::extractCaScrambledFlag(dvbpsi_descriptor_t *p_first_descriptor, int *flag)
{
    dvbpsi_descriptor_t *descr;

    AM_SI_LIST_BEGIN(p_first_descriptor, descr)
    if (descr->i_tag == AM_SI_DESCR_CA && ! *flag) {
        LOGD( "Found CA descr, set scrambled flag to 1");
        *flag = 1;
        break;
    }
    AM_SI_LIST_END()
}

void CTvScanner::extractSrvInfoFromSdt(AM_SCAN_Result_t *result, dvbpsi_sdt_t *sdts, SCAN_ServiceInfo_t *srv_info)
{
    dvbpsi_sdt_service_t *srv;
    dvbpsi_sdt_t *sdt;
    dvbpsi_descriptor_t *descr;
    const uint8_t split = 0x80;
    const int name_size = (int)sizeof(srv_info->name);
    int curr_name_len = 0, tmp_len;
    char name[AM_DB_MAX_SRV_NAME_LEN + 1];

    UNUSED(result);

#define COPY_NAME(_s, _slen)\
    AM_MACRO_BEGIN\
    int copy_len = ((curr_name_len+_slen)>=name_size) ? (name_size-curr_name_len) : _slen;\
        if (copy_len > 0) {\
            memcpy(srv_info->name+curr_name_len, _s, copy_len);\
            curr_name_len += copy_len;\
        }\
    AM_MACRO_END


    AM_SI_LIST_BEGIN(sdts, sdt)
    AM_SI_LIST_BEGIN(sdt->p_first_service, srv)

    if (srv->i_service_id == srv_info->srv_id) {
        LOGD("SDT for service %d found!", srv_info->srv_id);
        srv_info->eit_sche = (uint8_t)srv->b_eit_schedule;
        srv_info->eit_pf = (uint8_t)srv->b_eit_present;
        srv_info->rs = srv->i_running_status;
        srv_info->free_ca = (uint8_t)srv->b_free_ca;
        srv_info->sdt_version = sdt->i_version;

        AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
        if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE) {
            dvbpsi_service_dr_t *psd = (dvbpsi_service_dr_t *)descr->p_decoded;
            if (psd->i_service_name_length > 0) {
                name[0] = 0;
                AM_SI_ConvertDVBTextCode((char *)psd->i_service_name, psd->i_service_name_length, \
                                         name, AM_DB_MAX_SRV_NAME_LEN);
                name[AM_DB_MAX_SRV_NAME_LEN] = 0;
                LOGD("found name [%s]", name);

                /*3bytes language code, using xxx to simulate*/
                COPY_NAME("xxx", 3);
                /*following by name text*/
                tmp_len = strlen(name);
                COPY_NAME(name, tmp_len);
            }

            srv_info->srv_type = psd->i_service_type;
            /*service type 0x16 and 0x19 is user defined, as digital television service*/
            /*service type 0xc0 is type of partial reception service in ISDBT*/
            if ((srv_info->srv_type == 0x16) || (srv_info->srv_type == 0x19) || (srv_info->srv_type == 0xc0)) {
                srv_info->srv_type = 0x1;
            }
            break;
        }
        AM_SI_LIST_END()
#if 1   //The upper layer don't have the logic of parse multi service names according to 0x80, Use only one for the time being.
        /* store multilingual service name */
        AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
        bool isChinaStream = (strcmp("CN", CTvRegion::getTvCountry()) == 0);
        if (isChinaStream && descr->p_decoded && descr->i_tag == AM_SI_DESCR_MULTI_SERVICE_NAME) {
            int i;
            dvbpsi_multi_service_name_dr_t *pmsnd = (dvbpsi_multi_service_name_dr_t *)descr->p_decoded;

            for (i = 0; i < pmsnd->i_name_count; i++) {
                /*judge 3bytes language code is current system language or not*/
                const char *iso_639_code = (const char *)(pmsnd->p_service_name[i].i_iso_639_code);
                if (!(strncmp(mCurrentSystemLang.c_str(), iso_639_code, 3) == 0)) {
                    LOGD("not found multi matched current lang [%s] parsed [%3s]", mCurrentSystemLang.c_str(), iso_639_code);
                    continue;
                } else {
                    LOGD("found multi matched current lang [%s] parsed [%3s]", mCurrentSystemLang.c_str(), iso_639_code);
                }
                //clear other info if matched one found
                memset(name, 0, sizeof(name));
                memset(srv_info->name, 0, sizeof(srv_info->name));
                curr_name_len = 0,
                tmp_len = 0;
                name[0] = 0;
                AM_SI_ConvertDVBTextCode((char *)pmsnd->p_service_name[i].i_service_name,
                                         pmsnd->p_service_name[i].i_service_name_length,
                                         name, AM_DB_MAX_SRV_NAME_LEN);
                name[AM_DB_MAX_SRV_NAME_LEN] = 0;
                LOGD("found multi matched name [%s]", name);

                if (curr_name_len > 0) {
                    /*extra split mark*/
                    COPY_NAME(&split, 1);
                }

                /*3bytes language code*/
                COPY_NAME(pmsnd->p_service_name[i].i_iso_639_code, 3);
                /*following by name text*/
                tmp_len = strlen(name);
                COPY_NAME(name, tmp_len);
                LOGD("found multi match lang text [%s]", name);
                break;
            }
        }else {
            LOGD("no multi service name matched");
        }
        AM_SI_LIST_END()
#endif
        /* set the ending null byte */
        if (curr_name_len >= name_size)
            srv_info->name[name_size - 1] = 0;
        else
            srv_info->name[curr_name_len] = 0;

        break;
    }
    AM_SI_LIST_END()
    AM_SI_LIST_END()
}

void CTvScanner::extractSrvInfoFromVc(AM_SCAN_Result_t *result, dvbpsi_atsc_vct_channel_t *vcinfo, SCAN_ServiceInfo_t *srv_info)
{
    char name[22] = {0};

    UNUSED(result);

    srv_info->major_chan_num = vcinfo->i_major_number;
    srv_info->minor_chan_num = vcinfo->i_minor_number;

    srv_info->chan_num = (vcinfo->i_major_number<<16) | (vcinfo->i_minor_number&0xffff);
    srv_info->hidden = vcinfo->b_hidden;
    srv_info->hide_guide = vcinfo->b_hide_guide;
    srv_info->source_id = vcinfo->i_source_id;
    memcpy(srv_info->name, "xxx", 3);

    char const *coding = "utf-16";
    if (AM_SI_ConvertToUTF8((char*)vcinfo->i_short_name, 14, name, 22, (char*)coding) != DVB_SUCCESS)
        strcpy(name, "No Name");
    memcpy(srv_info->name+3, name, sizeof(name));
    srv_info->name[sizeof(name)+3] = 0;
    srv_info->srv_type = vcinfo->i_service_type;

    LOGD("Program(%d)('%s':%d-%d) in current TSID(%d) found!",
        srv_info->srv_id, srv_info->name,
        srv_info->major_chan_num, srv_info->minor_chan_num,
        vcinfo->i_channel_tsid);
}

void CTvScanner::updateServiceInfo(AM_SCAN_Result_t *result, SCAN_ServiceInfo_t *srv_info)
{
#define str(i) (char*)(strings + i)

    //static char strings[14][256];

    if (srv_info->src != TV_FE_ANALOG) {
        int standard = result->start_para->dtv_para.standard;
        int mode = result->start_para->dtv_para.mode;

        /* Transform service types for different dtv standards */
        if (standard != TV_SCAN_DTV_STD_ATSC) {
            if (srv_info->srv_type == 0x1)
                srv_info->srv_type = AM_SCAN_SRV_DTV;
            else if (srv_info->srv_type == 0x2)
                srv_info->srv_type = AM_SCAN_SRV_DRADIO;
        } else {
            if (srv_info->srv_type == 0x2 && VALID_PID(srv_info->vid))
                srv_info->srv_type = AM_SCAN_SRV_DTV;
            else if (srv_info->srv_type == 0x3 || (!VALID_PID(srv_info->vid)))
                srv_info->srv_type = AM_SCAN_SRV_DRADIO;
        }

        /* if video valid, set this program to tv type,
         * if audio valid, but video not found, set it to radio type,
         * if both invalid, but service_type found in SDT/VCT, set to unknown service,
         * this mechanism is OPTIONAL
         */
        if (VALID_PID(srv_info->vid)) {
            srv_info->srv_type = AM_SCAN_SRV_DTV;
        } else if (srv_info->aud_info.audio_count > 0) {
            srv_info->srv_type = AM_SCAN_SRV_DRADIO;
        } else if (srv_info->srv_type == AM_SCAN_SRV_DTV ||
                   srv_info->srv_type == AM_SCAN_SRV_DRADIO) {
            srv_info->srv_type = AM_SCAN_SRV_UNKNOWN;
        }
        /* Skip program for FTA mode */
        if (srv_info->scrambled_flag && (mode & TV_SCAN_DTVMODE_FTA)) {
            LOGD( "Skip program '%s' vid:[%d]for FTA mode", srv_info->name, srv_info->vid);
            return;
        }

        /* Skip program for service_type mode */
        if (srv_info->srv_type == AM_SCAN_SRV_DTV && (mode & TV_SCAN_DTVMODE_NOTV)) {
            LOGD( "Skip program '%s' for NO-TV mode", srv_info->name);
            return;
        }
        if (srv_info->srv_type == AM_SCAN_SRV_DRADIO && (mode & TV_SCAN_DTVMODE_NORADIO)) {
            LOGD( "Skip program '%s' for NO-RADIO mode", srv_info->name);
            return;
        }

        /* Set default name to tv/radio program if no name specified */
        if (!strcmp(srv_info->name, "") &&
                (srv_info->srv_type == AM_SCAN_SRV_DTV ||
                 srv_info->srv_type == AM_SCAN_SRV_DRADIO)) {
            strcpy(srv_info->name, "xxxNo Name");
        }
    }
}

void CTvScanner::addFixedATSCCaption(AM_SI_CaptionInfo_t *cap_info, int service, int cc, int text, int is_digital_cc)
{
    #define DEFAULT_SERVICE_MAX 6
    #define DEFAULT_CC_MAX 4
    #define DEFAULT_TEXT_MAX 4

    if (service) {
        /*service1 ~ service6*/
        int start = cap_info->caption_count;
        int cnt = (service == -1 || service > DEFAULT_SERVICE_MAX)? DEFAULT_SERVICE_MAX : service;
        for (int i = 0; i < cnt; i++) {
            cap_info->captions[start+i].type = 1;
            cap_info->captions[start+i].service_number = AM_CC_CAPTION_SERVICE1 + i;
            sprintf(cap_info->captions[start+i].lang, "CS%d", i+1);
        }
        cap_info->caption_count += cnt;
    }

    if (cc) {
        /*cc1 ~ cc4*/
        int start = cap_info->caption_count;
        int cnt = (cc == -1 || cc > DEFAULT_CC_MAX)? DEFAULT_CC_MAX : cc;
        for (int i = 0; i < cnt; i++) {
            cap_info->captions[start+i].type = is_digital_cc;
            cap_info->captions[start+i].service_number = AM_CC_CAPTION_CC1 + i;
            sprintf(cap_info->captions[start+i].lang, "CC%d", i+1);
        }
        cap_info->caption_count += cnt;
    }

    if (text) {
        /*text1 ~ text4*/
        int start = cap_info->caption_count;
        int cnt = (text == -1 || text > DEFAULT_TEXT_MAX)? DEFAULT_TEXT_MAX : text;
        for (int i = 0; i < cnt; i++) {
            cap_info->captions[start+i].type = is_digital_cc;
            cap_info->captions[start+i].service_number = AM_CC_CAPTION_TEXT1 + i;
            sprintf(cap_info->captions[start+i].lang, "TX%d", i+1);
        }
        cap_info->caption_count += cnt;
    }
}

void CTvScanner::processDvbTs(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, service_list_t &slist)
{
    LOGD("processDvbTs");

    dvbpsi_pmt_t *pmt;
    dvbpsi_pmt_es_t *es;
    int src = result->start_para->dtv_para.source;
    dvbpsi_pat_t *valid_pat = NULL;
    uint8_t plp_id;
    SCAN_ServiceInfo_t *psrv_info;
    dvbpsi_pat_program_t *prog;
    int programs_in_pat = 0;
    dvbpsi_pat_t *pat;

    valid_pat = getValidPats(ts);
    if (valid_pat == NULL) {
        LOGD("No PAT found in ts");
        return;
    }

    LOGD(" TS: src %d", src);

    AM_SI_LIST_BEGIN(valid_pat, pat) {
        AM_SI_LIST_BEGIN(pat->p_first_program, prog) {
            programs_in_pat ++;
        } AM_SI_LIST_END();
    } AM_SI_LIST_END();

    if (ts->digital.pmts || (IS_DVBT2_TS(ts->digital.fend_para) && ts->digital.dvbt2_data_plp_num > 0)) {
        int loop_count, lc;
        dvbpsi_sdt_t *sdt_list;
        dvbpsi_pmt_t *pmt_list;
        dvbpsi_pat_t *pat_list;

        /* For DVB-T2, search for each PLP, else search in current TS*/
        loop_count = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plp_num : 1;
        LOGD("plp num %d", loop_count);

        for (lc = 0; lc < loop_count; lc++) {
            pat_list = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].pats : ts->digital.pats;
            pmt_list = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].pmts : ts->digital.pmts;
            sdt_list =  IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].sdts : ts->digital.sdts;
            plp_id = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].id : -1;
            LOGD("plp_id %d", plp_id);

            AM_SI_LIST_BEGIN(pmt_list, pmt) {
                if (!(psrv_info = getServiceInfo()))
                    return;
                psrv_info->srv_id = pmt->i_program_number;
                psrv_info->src = src;
                psrv_info->pmt_pid = getPmtPid(pat_list, pmt->i_program_number);
                psrv_info->pcr_pid = pmt->i_pcr_pid;
                psrv_info->plp_id  = plp_id;
                psrv_info->programs_in_pat = programs_in_pat;

                /* looking for CA descr */
                if (! psrv_info->scrambled_flag) {
                    extractCaScrambledFlag(pmt->p_first_descriptor, &psrv_info->scrambled_flag);
                }

                AM_SI_LIST_BEGIN(pmt->p_first_es, es) {
                    AM_SI_ExtractAVFromES(es, &psrv_info->vid, &psrv_info->vfmt, &psrv_info->aud_info);
                    AM_SI_ExtractDVBSubtitleFromES(es, &psrv_info->sub_info);
                    AM_SI_ExtractDVBTeletextFromES(es, &psrv_info->ttx_info);
                    AM_SI_ExtractATSCCaptionFromES(es, &psrv_info->cap_info);
                    if (! psrv_info->scrambled_flag)
                        extractCaScrambledFlag(es->p_first_descriptor, &psrv_info->scrambled_flag);
                } AM_SI_LIST_END()

                extractSrvInfoFromSdt(result, sdt_list, psrv_info);

                /*Store this service*/
                updateServiceInfo(result, psrv_info);

                slist.push_back(psrv_info);
            } AM_SI_LIST_END()

            /* All programs in PMTs added, now trying the programs in SDT but NOT in PMT */
            dvbpsi_sdt_service_t *srv;
            dvbpsi_sdt_t *sdt;

            AM_SI_LIST_BEGIN(sdt_list, sdt) {
                AM_SI_LIST_BEGIN(sdt->p_first_service, srv) {
                    bool found_in_pmt = false;

                    /* Is already added in PMT? */
                    AM_SI_LIST_BEGIN(pmt_list, pmt){
                        if (srv->i_service_id == pmt->i_program_number) {
                            found_in_pmt = true;
                            break;
                        }
                    }AM_SI_LIST_END()

                    if (found_in_pmt)
                        continue;

                    if (!(psrv_info = getServiceInfo()))
                        return;
                    psrv_info->srv_id = srv->i_service_id;
                    psrv_info->src = src;

                    extractSrvInfoFromSdt(result, sdt_list, psrv_info);

                    updateServiceInfo(result, psrv_info);

                    /*as no pmt for this srv, set type to data for invisible*/
                    psrv_info->srv_type = 0;

                    slist.push_back(psrv_info);
                }
                AM_SI_LIST_END()
            }
            AM_SI_LIST_END()
        }
    }
}

void CTvScanner::processAnalogTs(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, SCAN_TsInfo_t *tsinfo, service_list_t &slist)
{
    LOGD("processAnalogTs");

    SCAN_ServiceInfo_t *psrv_info;

    UNUSED(ts);

    if (!(psrv_info=getServiceInfo()))
        return;

    LOGD(" TS: src analog");

    psrv_info->tsinfo = tsinfo;

    /*if atsc, generate the analog channel's channel number*/
    if (mAtvIsAtsc) {

        int tsid = -1;
        int found = 0;

        LOGD("collecting info for %d", tsinfo->fe.getFrequency());

        /*try get channel number from vct*/
        for (tsid_list_t::iterator p=tsid_list.begin(); p != tsid_list.end(); p++) {
            if ((*p)->freq == tsinfo->fe.getFrequency())
                tsid = (*p)->tsid;
        }
        if (tsid != -1) {
            dvbpsi_atsc_vct_t *vct;
            dvbpsi_atsc_vct_channel_t *vcinfo;

            AM_SI_LIST_BEGIN(ts->digital.vcts, vct){
            AM_SI_LIST_BEGIN(vct->p_first_channel, vcinfo){
                if (vcinfo->i_channel_tsid == tsid) {
                    LOGD("found channel info in vct.");
                    extractSrvInfoFromVc(result, vcinfo, psrv_info);
                    found = 1;
                }
            } AM_SI_LIST_END()
            } AM_SI_LIST_END()
        }

        LOGD("tsid:%d, found:%d", tsid, found);
        /*generate by channel id*/
        if (tsid == -1 || found == 0) {
            int mode = mScanParas.getAtvModifier(CFrontEnd::FEParas::FEP_MODE, -1);
            if (mode == -1)
                mode = mFEParas.getFEMode().getMode();
            const char *list_name = getDtvScanListName(mode);
            Vector<sp<CTvChannel>> vcp;
            CTvRegion::getChannelListByName(const_cast<char*>(list_name), vcp);
            for (int i = 0; i < (int)vcp.size(); i++) {
                int diff = abs(vcp[i]->getFrequency() - tsinfo->fe.getFrequency());
                if (diff >= 0 && diff <= 2000000) { // 2M tolerance
                    psrv_info->major_chan_num = vcp[i]->getLogicalChannelNum();
                    psrv_info->minor_chan_num = 0;
                    psrv_info->chan_num = (psrv_info->major_chan_num<<16) | (psrv_info->minor_chan_num&0xffff);
                    psrv_info->hidden = 0;
                    psrv_info->hide_guide = 0;
                    psrv_info->source_id = -1;
                    char name[] = "ATV Program";
                    memcpy(psrv_info->name, "xxx", 3);
                    memcpy(psrv_info->name+3, name, sizeof(name));
                    psrv_info->name[sizeof(name)+3] = 0;
                    psrv_info->srv_type = AM_SCAN_SRV_ATV;

                    LOGD("get channel info by channel id [%d.%d][%s]",
                        psrv_info->major_chan_num, psrv_info->minor_chan_num,
                        psrv_info->name);
                    found = 1;
                    break;
                }
            }
            if (found == 0) { // do not match channel table, major channel number start from table max number + 1
                if (slist.size() <= 0 || slist.back()->major_chan_num <= vcp.size() + 1)
                    psrv_info->major_chan_num = vcp.size() + 2;
                else
                    psrv_info->major_chan_num = slist.back()->major_chan_num + 1;

                psrv_info->minor_chan_num = 0;
                psrv_info->chan_num = (psrv_info->major_chan_num<<16) | (psrv_info->minor_chan_num&0xffff);
                psrv_info->hidden = 0;
                psrv_info->hide_guide = 0;
                psrv_info->source_id = -1;
                char name[] = "ATV Program";
                memcpy(psrv_info->name, "xxx", 3);
                memcpy(psrv_info->name+3, name, sizeof(name));
                psrv_info->name[sizeof(name)+3] = 0;
                psrv_info->srv_type = AM_SCAN_SRV_ATV;
                LOGD("ntsc channel[%d] doesn't match table[%s], set channel id to [%d.%d][%s]",
                        tsinfo->fe.getFrequency(), list_name,
                        psrv_info->major_chan_num, psrv_info->minor_chan_num,
                        psrv_info->name);
            }
        }
    }

    {
        int cc_fixed = getParamOption("cc.fixed");
        bool is_cc_fixed = (cc_fixed == -1)? false : (cc_fixed != 0);

        if (is_cc_fixed) {
            memset(&psrv_info->cap_info, 0, sizeof(psrv_info->cap_info));
            addFixedATSCCaption(&psrv_info->cap_info, 0, -1, -1, 0);
        }
    }
    slist.push_back(psrv_info);
}
int CTvScanner::checkIsSkipProgram(SCAN_ServiceInfo_t *srv_info, int mode)
{
    int pkt_num;
    int i;
    //default skip program;
    int ret = 1;
    /* Skip program for FTA mode */
    if (srv_info->scrambled_flag && (mode & AM_SCAN_DTVMODE_FTA))
    {
        LOGD("Skip program '%s' vid[%d]for FTA mode", srv_info->name, srv_info->vid);
        return ret;
    }
    /* Skip program for vct hide is set 1, we need hide this channel */
    if (srv_info->hidden == 1 && (mode & AM_SCAN_DTVMODE_NOVCTHIDE))
    {
        LOGD("Skip program '%s' vid[%d]for vct hide mode", srv_info->name, srv_info->vid);
        return ret;
    }

    /* Skip program for service_type mode */
    if (srv_info->srv_type == AM_SCAN_SRV_DTV && (mode & AM_SCAN_DTVMODE_NOTV))
    {
        LOGD("Skip program '%s' for NO-TV mode", srv_info->name);
        return ret;
    }
    if (srv_info->srv_type == AM_SCAN_SRV_DRADIO && (mode & AM_SCAN_DTVMODE_NORADIO))
    {
        LOGD("Skip program '%s' for NO-RADIO mode", srv_info->name);
        return ret;
    }
    /*skip invalid video and audio pid programs*/
    if (srv_info->srv_type == AM_SCAN_SRV_UNKNOWN &&
        (mode & AM_SCAN_DTVMODE_INVALIDPID)) {
        LOGD("Skip program '%s' for NO-valid pid mode", srv_info->name);
        return ret;
    }
    if (mode & AM_SCAN_DTVMODE_CHECKDATA) {
        /*check has ts package of video pid */
        AM_Check_Has_Tspackage(srv_info->vid, &pkt_num);
        /*if no video ts package, need check has ts package of audio pid */
        if (!pkt_num) {
            for (i = 0; i < srv_info->aud_info.audio_count; i++) {
                AM_Check_Has_Tspackage(srv_info->aud_info.audios[i].pid, &pkt_num);
            }
        }
        if (!pkt_num) {
            LOGD("Skip program '%s' for NO-ts package", srv_info->name);
            return ret;
        }
    }
    /*only check has ts package of audio pid, remove it that nodata*/
    if (mode & AM_SCAN_DTVMODE_CHECK_AUDIODATA) {
        int pid,fmt,audio_type,audio_exten;
        char lang[10] = {0};
        for (i = 0; i < srv_info->aud_info.audio_count; i++) {
            pkt_num = 0;
            AM_Check_Has_Tspackage(srv_info->aud_info.audios[i].pid, &pkt_num);
                if (pkt_num) {
                if (i == 0) {
                    LOGD("first index hasdata,needn't insert, pid[%d]=%d",i,srv_info->aud_info.audios[i].pid);
                }else {
                    pid = srv_info->aud_info.audios[0].pid;
                    fmt = srv_info->aud_info.audios[0].fmt;
                    memcpy(lang, srv_info->aud_info.audios[0].lang, 10);
                    audio_type = srv_info->aud_info.audios[0].audio_type;
                    audio_exten = srv_info->aud_info.audios[0].audio_exten;
                    srv_info->aud_info.audios[0].pid = srv_info->aud_info.audios[i].pid;
                    srv_info->aud_info.audios[0].fmt = srv_info->aud_info.audios[i].fmt;
                    memcpy(srv_info->aud_info.audios[0].lang, srv_info->aud_info.audios[i].lang, 10);
                    srv_info->aud_info.audios[0].audio_type = srv_info->aud_info.audios[i].audio_type;
                    srv_info->aud_info.audios[0].audio_exten = srv_info->aud_info.audios[i].audio_exten;
                    srv_info->aud_info.audios[i].pid = pid;
                    srv_info->aud_info.audios[i].fmt = fmt;
                    memcpy(srv_info->aud_info.audios[i].lang, lang, 10);
                    srv_info->aud_info.audios[i].audio_type = audio_type;
                    srv_info->aud_info.audios[i].audio_exten = audio_exten;
                }
                break;
             }
        }
    }
    ret = 0;
    return ret;
}

AM_ErrorCode_t AM_SI_ExtractScte27SubtitleFromES(dvbpsi_pmt_es_t *es, AM_SI_Scte27SubtitleInfo_t *sub_info)
{
        dvbpsi_descriptor_t *descr;
        int i, found = 0;

        if (es->i_type == 0x82)
        {
                for (i=0; i<sub_info->subtitle_count; i++)
                {
                        if (es->i_pid == sub_info->subtitles[i].pid)
                        {
                                found = 1;
                                break;
                        }
                }

                if (found != 1)
                {
                        sub_info->subtitles[sub_info->subtitle_count].pid = es->i_pid;
                        sub_info->subtitle_count++;
                }

                LOGE("Scte27 stream found pid 0x%x count %d", es->i_pid, sub_info->subtitle_count);
                AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
                {
                        if (descr->p_decoded)
                        {
                                AM_DEBUG(0, "scte27 i_tag table_id 0x%x", descr->i_tag);
                        }
                }
                AM_SI_LIST_END()
        }
        return AM_SUCCESS;
}

void CTvScanner::processAtscTs(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, SCAN_TsInfo_t *tsinfo, service_list_t &slist)
{
    #define VALID_PID(_pid_) ((_pid_)>0 && (_pid_)<0x1fff)
    if(ts->digital.vcts
        && ts->digital.pats
        && (ts->digital.vcts->i_extension != ts->digital.pats->i_ts_id)
        && (ts->digital.pats->i_ts_id == 0))
        ts->digital.use_vct_tsid = 1;

    dvbpsi_atsc_vct_t *vct;
    dvbpsi_atsc_vct_channel_t *vcinfo;
    dvbpsi_pat_t *pat;
    dvbpsi_pat_program_t *prog;
    dvbpsi_pmt_t *pmt;
    dvbpsi_pmt_es_t *es;
    int mode = result->start_para->dtv_para.mode;
    int src = result->start_para->dtv_para.source;
    //bool stream_found_in_vct = false;
    bool program_found_in_vct = false;
    SCAN_ServiceInfo_t *psrv_info;
    int cc_fixed = getParamOption("cc.fixed");
    bool is_cc_fixed = (cc_fixed == -1)? false : (cc_fixed != 0);
    bool mNeedCheck_tsid = true;
    int programs_in_pat = 0;
    int vpid = 0;
    /*if do store the programs in VCT but NOT in PMT,we need set store_cvt_mode true*/
    bool store_vct_notin_pmt = false;/*!!!!for  CVTE, need set false*/
    if (!ts->digital.pats && !ts->digital.vcts)
    {
        LOGD("No PAT or VCT found in ts");
        return;
    }

    AM_SI_LIST_BEGIN(ts->digital.pats, pat) {
        AM_SI_LIST_BEGIN(pat->p_first_program, prog) {
            programs_in_pat ++;
        } AM_SI_LIST_END();
    } AM_SI_LIST_END();

    AM_SI_LIST_BEGIN(ts->digital.pmts, pmt) {
        if (!(psrv_info = getServiceInfo()))
            return;
        psrv_info->programs_in_pat = programs_in_pat;
        psrv_info->srv_id = pmt->i_program_number;
        psrv_info->src = src;
        psrv_info->pmt_pid = getPmtPid(ts->digital.pats, pmt->i_program_number);
        psrv_info->pcr_pid = pmt->i_pcr_pid;
        /* looking for CA descr */
        if (! psrv_info->scrambled_flag) {
            if (mode & TV_SCAN_DTVMODE_SCRAMB_TSHEAD) {
                psrv_info->scrambled_flag = pmt->i_scramble_flag;
            } else {
              extractCaScrambledFlag(pmt->p_first_descriptor, &psrv_info->scrambled_flag);
            }
        }

        AM_SI_LIST_BEGIN(pmt->p_first_es, es) {
            vpid = 0;
            AM_SI_ExtractScte27SubtitleFromES(es, &psrv_info->scte27_info);
            AM_SI_ExtractAVFromES(es, &vpid, &psrv_info->vfmt, &psrv_info->aud_info);
            if (!VALID_PID(psrv_info->vid))
                psrv_info->vid = vpid;
            if (!is_cc_fixed)
                AM_SI_ExtractATSCCaptionFromES(es, &psrv_info->cap_info);
            if (! psrv_info->scrambled_flag && !(mode & TV_SCAN_DTVMODE_SCRAMB_TSHEAD))
                extractCaScrambledFlag(es->p_first_descriptor, &psrv_info->scrambled_flag);
        }AM_SI_LIST_END()
        /* Skip program for FTA mode */
        if (psrv_info->scrambled_flag && (mode & TV_SCAN_DTVMODE_FTA)) {
            LOGD( "Skip program '%s' vid:[%d]for FTA mode", psrv_info->name, psrv_info->vid);
            continue;
        }
        program_found_in_vct = false;
        mNeedCheck_tsid = true;
 VCT_REPEAT:
        AM_SI_LIST_BEGIN(ts->digital.vcts, vct) {
        AM_SI_LIST_BEGIN(vct->p_first_channel, vcinfo) {
            /*Skip inactive program*/
            if (vcinfo->i_program_number == 0  || vcinfo->i_program_number == 0xffff)
                continue;

            if ((ts->digital.use_vct_tsid || (vct->i_extension == ts->digital.pats->i_ts_id) || mNeedCheck_tsid == false)
                && vcinfo->i_channel_tsid == vct->i_extension) {
                if (vcinfo->i_program_number == pmt->i_program_number) {
                    if (mNeedCheck_tsid == true) {
                        if (vct->b_cable_vct)
                            psrv_info->vct_type = 1;

                            int vpid = 0;
                            int vfmt = 0;
                            AM_SI_ExtractAVFromVC(vcinfo, &vpid, &vfmt, &psrv_info->aud_info);
                            if (!VALID_PID(psrv_info->vid)) {
                                psrv_info->vid = vpid;
                                psrv_info->vfmt = vfmt;
                            }
                    }
                    psrv_info->sdt_version = vct->i_version;//temp use mSdtVersion to store vctVersion.
                    extractSrvInfoFromVc(result, vcinfo, psrv_info);
                    program_found_in_vct = true;
                    goto VCT_END;
                }
            } else {
                LOGD("Program(%d ts:%d) in VCT(ts:%d) found, current (ts:%d)",
                    vcinfo->i_program_number, vcinfo->i_channel_tsid,
                    vct->i_extension, ts->digital.pats->i_ts_id);
                continue;
            }
        } AM_SI_LIST_END()
        } AM_SI_LIST_END()
        /*if not find in vct on check tsid mode,we need repeat check on not check tsid*/
        if (program_found_in_vct == false && mNeedCheck_tsid == true) {
            mNeedCheck_tsid = false;
            goto VCT_REPEAT;
        }
VCT_END:
        /*Store this service*/
        updateServiceInfo(result, psrv_info);
        if (checkIsSkipProgram(psrv_info, mode)) {
            continue;
        }
        if (is_cc_fixed) {
            memset(&psrv_info->cap_info, 0, sizeof(psrv_info->cap_info));
            addFixedATSCCaption(&psrv_info->cap_info, -1, -1, -1, 1);
        }

        if (!program_found_in_vct) {
            const char *list_name = getDtvScanListName(mFEParas.getFEMode().getMode());
            Vector<sp<CTvChannel>> vcp;
            CTvRegion::getChannelListByName((char *)list_name, vcp);
            for (int i = 0; i < (int)vcp.size(); i++) {
                if ((tsinfo->fe.getFrequency()/1000) == (vcp[i]->getFrequency()/1000)) {
                    psrv_info->major_chan_num = vcp[i]->getLogicalChannelNum();
                    psrv_info->minor_chan_num = psrv_info->srv_id;
                }
            }
        }

        slist.push_back(psrv_info);

    } AM_SI_LIST_END()

    if (mode & AM_SCAN_DTVMODE_NOVCT == AM_SCAN_DTVMODE_NOVCT) {
        LOGD("NOVCT is enbale, so don't store channel that in VCT but not in PMT");
        return;
    }

    /* All programs in PMTs added, now trying the programs in VCT but NOT in PMT */
    if (store_vct_notin_pmt == true) {
        AM_SI_LIST_BEGIN(ts->digital.vcts, vct) {
        AM_SI_LIST_BEGIN(vct->p_first_channel, vcinfo) {
            bool found_in_pmt = false;

            if (!(psrv_info = getServiceInfo()))
                return;
            psrv_info->srv_id = vcinfo->i_program_number;
            psrv_info->src = src;

            /*Skip inactive program*/
            if (vcinfo->i_program_number == 0  || vcinfo->i_program_number == 0xffff)
                continue;

            /* Is already added in PMT? */
            AM_SI_LIST_BEGIN(ts->digital.pmts, pmt) {
                if (vcinfo->i_program_number == pmt->i_program_number) {
                    found_in_pmt = true;
                    break;
                }
            } AM_SI_LIST_END()

            if (found_in_pmt)
                continue;

            if (vcinfo->i_channel_tsid == vct->i_extension) {
                AM_SI_ExtractAVFromVC(vcinfo, &psrv_info->vid, &psrv_info->vfmt, &psrv_info->aud_info);
                psrv_info->sdt_version = vct->i_version;//temp use mSdtVersion to store vctVersion.
                extractSrvInfoFromVc(result, vcinfo, psrv_info);
                updateServiceInfo(result, psrv_info);
                if (checkIsSkipProgram(psrv_info, mode)) {
                    continue;
                }
                if (is_cc_fixed) {
                    memset(&psrv_info->cap_info, 0, sizeof(psrv_info->cap_info));
                addFixedATSCCaption(&psrv_info->cap_info, -1, -1, -1, 1);
            }

            slist.push_back(psrv_info);

            } else {
                LOGD("Program(%d ts:%d) in VCT(ts:%d) found",
                vcinfo->i_program_number, vcinfo->i_channel_tsid,
                vct->i_extension);
                continue;
            }
        } AM_SI_LIST_END()
        } AM_SI_LIST_END()
    }
}

void CTvScanner::storeScanHelper(AM_SCAN_Result_t *result)
{
    if (mInstance)
        mInstance->storeScan(result, NULL);
    else
        LOGE("no Scanner running, ignore");
}

void CTvScanner::storeScan(AM_SCAN_Result_t *result, AM_SCAN_TS_t *curr_ts)
{
    AM_SCAN_TS_t *ts;
    service_list_t service_list;
    ts_list_t ts_list;

    LOGD("Storing tses ...");

    UNUSED(curr_ts);

    service_list_t slist;
    AM_SI_LIST_BEGIN(result->tses, ts) {
        if (ts->type == AM_SCAN_TS_DIGITAL) {
            SCAN_TsInfo_t *tsinfo = (SCAN_TsInfo_t*)calloc(sizeof(SCAN_TsInfo_t), 1);
            if (!tsinfo) {
                LOGE("No Memory for Scanner.");
                return;
            }
            processTsInfo(result, ts, tsinfo);
            ts_list.push_back(tsinfo);

            if (result->start_para->dtv_para.standard == TV_SCAN_DTV_STD_ATSC)
                processAtscTs(result, ts, tsinfo, slist);
            else
                processDvbTs(result, ts, slist);
            for (service_list_t::iterator p=slist.begin(); p != slist.end(); p++) {
                (*p)->tsinfo = tsinfo;
            }
            service_list.merge(slist);
            slist.clear();
        }
    }
    AM_SI_LIST_END()

    AM_SI_LIST_BEGIN(result->tses, ts) {
        if (ts->type == AM_SCAN_TS_ANALOG) {
            int mode = result->start_para->store_mode;
            //not store pal type program
            if (((ts->analog.std & V4L2_COLOR_STD_PAL) == V4L2_COLOR_STD_PAL) &&
                (mode & AM_SCAN_ATV_STOREMODE_NOPAL) == AM_SCAN_ATV_STOREMODE_NOPAL)
            {
                LOGE("skip pal type program");
                continue;
            }
            //not store secam type program
            if (((ts->analog.std & V4L2_COLOR_STD_SECAM) == V4L2_COLOR_STD_SECAM) &&
                (mode & AM_SCAN_DTV_STOREMODE_NOSECAM) == AM_SCAN_DTV_STOREMODE_NOSECAM)
            {
                LOGE("skip secam type program");
                continue;
            }
            SCAN_TsInfo_t *tsinfo = (SCAN_TsInfo_t*)calloc(sizeof(SCAN_TsInfo_t), 1);
            if (!tsinfo) {
                LOGE("No memory for Scanner");
                return;
            }
            processTsInfo(result, ts, tsinfo);
            ts_list.push_back(tsinfo);

            processAnalogTs(result, ts, tsinfo, service_list);
        }
    }
    AM_SI_LIST_END()
    for (tsid_list_t::iterator p=tsid_list.begin(); p != tsid_list.end(); p++)
        free(*p);
    tsid_list.clear();

    if (result->start_para->dtv_para.sort_method == AM_SCAN_SORT_BY_LCN) {
        lcn_list_t lcn_list;
        AM_SI_LIST_BEGIN(result->tses, ts) {
            lcn_list_t llist;
            getLcnInfo(result, ts, llist);
            lcn_list.merge(llist);
            llist.clear();
        }
        AM_SI_LIST_END()

        /*notify lcn info*/
        LOGD("notify lcn info.");
        for (lcn_list_t::iterator p=lcn_list.begin(); p != lcn_list.end(); p++)
            notifyLcn(*p);

        /*free lcn list*/
        for (lcn_list_t::iterator p=lcn_list.begin(); p != lcn_list.end(); p++)
            free(*p);
        lcn_list.clear();
    }

    /*notify services info*/
    LOGD("notify service info.");
    for (service_list_t::iterator p=service_list.begin(); p != service_list.end(); p++)
        notifyService(*p);

    /*free services in list*/
    for (service_list_t::iterator p=service_list.begin(); p != service_list.end(); p++)
        free(*p);
    service_list.clear();

    /*free ts in list*/
    for (ts_list_t::iterator p=ts_list.begin(); p != ts_list.end(); p++)
        free(*p);
    ts_list.clear();
}

int CTvScanner::createAtvParas(AM_SCAN_ATVCreatePara_t &atv_para, CFrontEnd::FEParas &fp, ScanParas &scp) {
    atv_para.mode = scp.getAtvMode();
    if (atv_para.mode == TV_SCAN_ATVMODE_NONE)
        return 0;

    //int analog_auto = getParamOption("analog.auto");
    //bool is_analog_auto = (analog_auto == -1)? false : (analog_auto != 0);
    bool is_analog_auto = (atv_para.mode == TV_SCAN_ATVMODE_AUTO || atv_para.mode == TV_SCAN_ATVMODE_MANUAL);

    int freq1, freq2;
    freq1 = scp.getAtvFrequency1();
    freq2 = scp.getAtvFrequency2();
    if ((atv_para.mode != TV_SCAN_ATVMODE_FREQ && (freq1 <= 0 || freq2 <= 0) && is_analog_auto)
            || (atv_para.mode == TV_SCAN_ATVMODE_AUTO && freq1 > freq2)
            || (atv_para.mode == TV_SCAN_ATVMODE_FREQ && freq1 > freq2)) {
        LOGW(" freq error start:%d end=%d amode:%d",  freq1,  freq2, atv_para.mode);
        return -1;
    }

    if (TV_FE_ATSC == fp.getFEMode().getBase()) {
        mAtvIsAtsc = 1;
        atv_para.afc_range = 2000000;
        LOGD("create ATV scan param: ATSC");
    } else {
        atv_para.afc_range = 2000000;
    }

    atv_para.am_scan_atv_cvbs_lock =  &checkAtvCvbsLockHelper;
    atv_para.default_std = CFrontEnd::getInstance()->enumToStdAndColor(fp.getVideoStd(), fp.getAudioStd());
    atv_para.channel_id = -1;
    //atv_para.afc_range = 2000000;

    if (!is_analog_auto) {// atsc scan with list-mode, not auto mode
        int mode = scp.getAtvModifier(CFrontEnd::FEParas::FEP_MODE, -1);
        if (mode == -1)
            mode = fp.getFEMode().getMode();
        const char *list_name = getDtvScanListName(mode);
        LOGD("Using Region List [%s] for ATV", list_name);

        int f1 = scp.getAtvModifier(CFrontEnd::FEParas::FEP_FREQ, -1);
        if (f1 != -1)
             freq1 = freq2 = f1;

        Vector<sp<CTvChannel>> vcp;

        //for FREQ_MODE
        //freq1 == freq2 == 0 : full list auto search
        //freq1 == freq2 != 0 : single freq manual search
        //freq1 != freq2 : list(range) freq manual search
        if (freq1 == freq2 && freq1 == 0)
            CTvRegion::getChannelListByName((char *)list_name, vcp);
        else
            CTvRegion::getChannelListByNameAndFreqRange((char *)list_name, freq1, freq2, vcp);

        int size = vcp.size();
        LOGD("channel list size = %d", size);

        if (size == 0) {
            CTvDatabase::GetTvDb()->importXmlToDB(CTV_DATABASE_DEFAULT_XML);
            CTvRegion::getChannelListByName((char *)list_name, vcp);
            size = vcp.size();
        }

        if (!(atv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        int i;
        for (i = 0; i < size; i++) {
            atv_para.fe_paras[i].m_type = TV_FE_ANALOG;
            atv_para.fe_paras[i].analog.para.u.analog.std = atv_para.default_std;
            atv_para.fe_paras[i].analog.para.u.analog.audmode = atv_para.default_std & 0x00FFFFFF;
            atv_para.fe_paras[i].analog.para.frequency = vcp[i]->getFrequency();
            atv_para.fe_paras[i].m_logicalChannelNum = vcp[i]->getLogicalChannelNum();
        }
        atv_para.fe_cnt = size;

        return 0;
    }

    atv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(3, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)));
    if (atv_para.fe_paras != NULL) {
        memset(atv_para.fe_paras, 0, 3 * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));
        atv_para.fe_paras[0].m_type = TV_FE_ANALOG;
        atv_para.fe_paras[0].analog.para.frequency = scp.getAtvFrequency1();
        atv_para.fe_paras[0].analog.para.u.analog.std = atv_para.default_std;
        atv_para.fe_paras[0].analog.para.u.analog.audmode = atv_para.default_std & 0x00FFFFFF;
        atv_para.fe_paras[1].m_type = TV_FE_ANALOG;
        atv_para.fe_paras[1].analog.para.frequency = scp.getAtvFrequency2();
        atv_para.fe_paras[1].analog.para.u.analog.std = atv_para.default_std;
        atv_para.fe_paras[1].analog.para.u.analog.audmode = atv_para.default_std & 0x00FFFFFF;
        atv_para.fe_paras[2].m_type = TV_FE_ANALOG;
        atv_para.fe_paras[2].analog.para.frequency = scp.getAtvFrequency1();
        atv_para.fe_paras[2].analog.para.u.analog.std = atv_para.default_std;
        atv_para.fe_paras[2].analog.para.u.analog.audmode = atv_para.default_std & 0x00FFFFFF;
    }
    atv_para.fe_cnt = 3;
    if (atv_para.mode == TV_SCAN_ATVMODE_AUTO) {
        atv_para.afc_unlocked_step = 4500000;
        atv_para.cvbs_unlocked_step = 1500000;
        atv_para.cvbs_locked_step = 6000000;
        atv_para.afc_range = 1500000;
    } else {
        if (atv_para.fe_paras != NULL)
        {
            atv_para.direction = (atv_para.fe_paras[1].analog.para.frequency >= atv_para.fe_paras[0].analog.para.frequency)? 1 : 0;
        }
        atv_para.cvbs_unlocked_step = 1000000;
        atv_para.cvbs_locked_step = 3000000;
        atv_para.afc_range = 1000000;
    }
    return 0;
}

int CTvScanner::freeAtvParas(AM_SCAN_ATVCreatePara_t &atv_para) {
    if (atv_para.fe_paras != NULL)
        free(atv_para.fe_paras);
    return 0;
}

int CTvScanner::createDtvParas(AM_SCAN_DTVCreatePara_t &dtv_para, CFrontEnd::FEParas &fp, ScanParas &scp) {

    dtv_para.mode = scp.getDtvMode();
    if (dtv_para.mode == TV_SCAN_DTVMODE_NONE)
        return 0;

    dtv_para.source = fp.getFEMode().getBase();
    dtv_para.dmx_dev_id = 0;//default 0
    dtv_para.standard = (AM_SCAN_DTVStandard_t) TV_SCAN_DTV_STD_DVB;
    if (dtv_para.source == TV_FE_ATSC)
        dtv_para.standard = (AM_SCAN_DTVStandard_t) TV_SCAN_DTV_STD_ATSC;
    else if (dtv_para.source == TV_FE_ISDBT)
        dtv_para.standard = (AM_SCAN_DTVStandard_t) TV_SCAN_DTV_STD_ISDB;

    int forceDtvStd = getScanDtvStandard(scp);
    if (forceDtvStd != -1) {
        dtv_para.standard = (AM_SCAN_DTVStandard_t)forceDtvStd;
        LOGD("force dtv std: %d", forceDtvStd);
    }

    const char *list_name = getDtvScanListName(fp.getFEMode().getMode());
    LOGD("Using Region List [%s] for DTV", list_name);

    Vector<sp<CTvChannel>> vcp;

    if (scp.getDtvMode() == TV_SCAN_DTVMODE_ALLBAND) {
        CTvRegion::getChannelListByName((char *)list_name, vcp);
    } else if (scp.getDtvMode() == TV_SCAN_DTVMODE_AUTO) {
        Vector<sp<CTvChannel>> vcptemp;
        CTvRegion::getChannelListByName((char *)list_name, vcptemp);
        //add two channel at most
        if (vcptemp.size() >= 2) {
            vcptemp[0]->setFrequency(scp.getDtvFrequency1());
            vcptemp[0]->setSymbolRate(fp.getSymbolrate());
            vcp.add(vcptemp[0]);
            if (scp.getDtvFrequency1() != scp.getDtvFrequency2()) {
                vcptemp[1]->setFrequency(scp.getDtvFrequency2());
                vcptemp[1]->setSymbolRate(fp.getSymbolrate());
                vcp.add(vcptemp[1]);
            }
        }
    } else {
        CTvRegion::getChannelListByNameAndFreqRange((char *)list_name, scp.getDtvFrequency1(), scp.getDtvFrequency2(), vcp);
    }
    int size = vcp.size();
    LOGD("channel list size = %d", size);

    if (size == 0) {
        if (scp.getMode() != TV_SCAN_DTVMODE_ALLBAND) {
            LOGD("frequncy: %d not found in channel list [%s], break", scp.getDtvFrequency1(), list_name);
            return -1;
        }
        CTvDatabase::GetTvDb()->importXmlToDB(CTV_DATABASE_DEFAULT_XML);
        CTvRegion::getChannelListByName((char *)list_name, vcp);
        size = vcp.size();
    }

    int air_on_cable = getParamOption("air_on_cable.enable");
    bool is_air_on_cable = (air_on_cable == -1)? false : (air_on_cable != 0);
    if (dtv_para.source == TV_FE_ATSC && is_air_on_cable && size) {
        int cnt = 0;
        int i;
        for (i = 0; i < size; i++) {
            fe_modulation_t mod = (fe_modulation_t)(vcp[i]->getModulation());
            if (mod >= QAM_16 && mod <= QAM_AUTO) {
                vcp.push_back(new CTvChannel(
                        vcp[i]->getID(), vcp[i]->getMode(), vcp[i]->getFrequency(),
                        vcp[i]->getBandwidth(), VSB_8,
                        vcp[i]->getSymbolRate(), 0, 0));
                cnt++;
            }
        }
        size = vcp.size();
        LOGD("Feature[air on cable] on, list size more: %d, total:%d", cnt, size);
    }

    if (!(dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
        return -1;

    int i;
    for (i = 0; i < size; i++) {
        dtv_para.fe_paras[i].m_type = dtv_para.source;
        switch (dtv_para.fe_paras[i].m_type) {
            case TV_FE_DTMB:
                dtv_para.fe_paras[i].dtmb.para.frequency = vcp[i]->getFrequency();
                dtv_para.fe_paras[i].dtmb.para.inversion = INVERSION_OFF;
                dtv_para.fe_paras[i].dtmb.para.u.ofdm.bandwidth = (fe_bandwidth_t)(vcp[i]->getBandwidth());
                if (fp.getBandwidth() != -1)
                    dtv_para.fe_paras[i].dtmb.para.u.ofdm.bandwidth = (fe_bandwidth_t)fp.getBandwidth();
                break;
            case TV_FE_QAM:
                dtv_para.fe_paras[i].cable.para.frequency = vcp[i]->getFrequency();
                dtv_para.fe_paras[i].cable.para.inversion = INVERSION_OFF;
                dtv_para.fe_paras[i].cable.para.u.qam.symbol_rate = vcp[i]->getSymbolRate();
                dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)vcp[i]->getModulation();
                if (fp.getSymbolrate() != -1)
                    dtv_para.fe_paras[i].cable.para.u.qam.symbol_rate = fp.getSymbolrate();
                if (fp.getModulation() != -1)
                    dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)fp.getModulation();
                break;
            case TV_FE_OFDM:
                dtv_para.fe_paras[i].terrestrial.para.frequency = vcp[i]->getFrequency();
                dtv_para.fe_paras[i].terrestrial.para.inversion = INVERSION_OFF;
                dtv_para.fe_paras[i].terrestrial.para.u.ofdm.bandwidth = (fe_bandwidth_t)(vcp[i]->getBandwidth());
                dtv_para.fe_paras[i].terrestrial.ofdm_mode = (fe_ofdm_mode_t)fp.getFEMode().getGen();
                if (fp.getBandwidth() != -1)
                    dtv_para.fe_paras[i].terrestrial.para.u.ofdm.bandwidth = (fe_bandwidth_t)fp.getBandwidth();
                break;
            case TV_FE_ATSC:
                dtv_para.fe_paras[i].atsc.para.frequency = vcp[i]->getFrequency();
                dtv_para.fe_paras[i].atsc.para.inversion = INVERSION_OFF;
                dtv_para.fe_paras[i].atsc.para.u.vsb.modulation = (fe_modulation_t)(vcp[i]->getModulation());
                if (fp.getModulation() != -1)
                    dtv_para.fe_paras[i].atsc.para.u.vsb.modulation = (fe_modulation_t)(fe_modulation_t)fp.getModulation();
                break;
            case TV_FE_ISDBT:
                dtv_para.fe_paras[i].isdbt.para.frequency = vcp[i]->getFrequency();
                dtv_para.fe_paras[i].isdbt.para.inversion = INVERSION_OFF;
                dtv_para.fe_paras[i].isdbt.para.u.ofdm.bandwidth = (fe_bandwidth_t)(vcp[i]->getBandwidth());
                if (fp.getBandwidth() != -1)
                    dtv_para.fe_paras[i].isdbt.para.u.ofdm.bandwidth = (fe_bandwidth_t)fp.getBandwidth();
                break;
        }
    }

    dtv_para.fe_cnt = size;
    dtv_para.resort_all = false;

    const char *sort_mode = config_get_str ( CFG_SECTION_TV, CFG_DTV_SCAN_SORT_MODE, "null");
    if (!strcmp(sort_mode, "lcn") && (dtv_para.standard != TV_SCAN_DTV_STD_ATSC))
        dtv_para.sort_method = AM_SCAN_SORT_BY_LCN;
    else
        dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;

    return 0;
}

int CTvScanner::freeDtvParas(AM_SCAN_DTVCreatePara_t &dtv_para) {
    if (dtv_para.fe_paras != NULL)
        free(dtv_para.fe_paras);
    return 0;
}

void CTvScanner::CC_VBINetworkCb(void* handle, vbi_network *n)
{
    void *userData = AM_CC_GetUserData(handle);
    if (userData != (void*)this)
        return;
    mVbiTsId = n->ts_id;
}

void CTvScanner::CC_VBINetworkCbHelper(void* handle, vbi_network *n)
{
    if (mInstance)
        mInstance->CC_VBINetworkCb(handle, n);
    else
        LOGE("no scanner running, ignore CC_VBINetworkCb");
}

#endif

CTvScanner::SCAN_ServiceInfo_t* CTvScanner::getServiceInfo()
{
    SCAN_ServiceInfo_t *srv_info = (SCAN_ServiceInfo_t*)calloc(sizeof(SCAN_ServiceInfo_t), 1);
    if (!srv_info) {
       LOGE("No Memory for Scanner.");
       return NULL;
    }

    memset(srv_info, 0, sizeof(SCAN_ServiceInfo_t));
    srv_info->vid = 0x1fff;
    srv_info->vfmt = -1;
    srv_info->free_ca = 1;
    srv_info->srv_id = 0xffff;
    srv_info->pmt_pid = 0x1fff;
    srv_info->plp_id = -1;
    srv_info->sdt_version = 0xff;
    return srv_info;
}

const char *CTvScanner::getDtvScanListName(int mode)
{
    char *list_name = NULL;
#ifdef SUPPORT_ADTV
    CFrontEnd::FEMode feMode(mode);
    int base = feMode.getBase();
    int list = feMode.getList();
    const char* pCountry = CTvRegion::getTvCountry();

    LOGD(" pCountry = %s, base = %d", pCountry, base);
    switch (base) {
        case TV_FE_ANALOG:
            if (strcmp(pCountry, "IN") == 0) {
                list_name = (char *)"IN,Default ATV";
            }
            else if (strcmp(pCountry, "ID") == 0) {
                list_name = (char *)"ID,Default ATV";
            }
            else if (strcmp(pCountry, "CN") == 0) {
                list_name = (char *)"CN,Default ATV";
            }
            else {
                list_name = (char *)"IN,Default ATV";
            }

            break;
        case TV_FE_DTMB:
            list_name = (char *)"CN,Default DTMB EXTRA ALL";
            break;
        case TV_FE_QAM:
            list_name = (char *)"CN,DVB-C allband";
            break;
        case TV_FE_OFDM:
            if (strcmp(pCountry, "ID") == 0) {
                list_name = (char *)"ID,Default DVB-T";
            }
            else {
                list_name = (char *)"GB,Default DVB-T";
            }
            break;
        case TV_FE_ATSC:
            switch (list) {
            case 1:
                list_name = (char *)"US,ATSC Cable Standard";
                break;
            case 2:
                list_name = (char *)"US,ATSC Cable IRC";
                break;
            case 3:
                list_name = (char *)"US,ATSC Cable HRC";
                break;
            case 4:
                list_name = (char *)"US,ATSC Cable Auto";
                break;
            case 5:
                list_name = (char *)"US,ATSC Air";
                break;
            case 6:
                list_name = (char *)"US,NTSC Cable Standard";
                break;
            case 7:
                list_name = (char *)"US,NTSC Cable IRC";
                break;
            case 8:
                list_name = (char *)"US,NTSC Cable HRC";
                break;
            case 9:
                list_name = (char *)"US,NTSC Cable Auto";
                break;
            case 10:
                list_name = (char *)"US,NTSC Air";
                break;
            default:
                list_name = (char *)"US,ATSC Air";
                break;
            }break;
        case TV_FE_ISDBT:
            list_name = (char *)"BR,Default ISDBT";
            break;
        default:
            //list_name = (char *)"CN,Default DTMB ALL";
            list_name = (char *)"CN,Default DTMB EXTRA ALL";
            LOGD("unknown scan mode %d, using default[%s]", mode, list_name);
            break;
    }
#endif
    return list_name;
}

AM_Bool_t CTvScanner::checkAtvCvbsLock(unsigned long  *colorStd)
{
    tvafe_cvbs_video_t cvbs_lock_status;
    int ret, i = 0;

    *colorStd = 0;
    while (i < 20) {
        ret = CTvin::getInstance()->AFE_GetCVBSLockStatus(&cvbs_lock_status);

        if (cvbs_lock_status == TVAFE_CVBS_VIDEO_HV_LOCKED)
            /*||cvbs_lock_status == TVAFE_CVBS_VIDEO_V_LOCKED
            ||cvbs_lock_status == TVAFE_CVBS_VIDEO_H_LOCKED)*/ {
            //usleep(2000 * 1000);
            tvin_info_t info;
            CTvin::getInstance()->VDIN_GetSignalInfo(&info);
            *colorStd = CTvin::CvbsFtmToV4l2ColorStd(info.fmt);
            LOGD("checkAtvCvbsLock locked and cvbs fmt = %d std = 0x%x", info.fmt, (unsigned int) *colorStd);
            return AM_TRUE;
        }
        usleep(50 * 1000);
        i++;
    }
    return AM_FALSE;
}

AM_Bool_t CTvScanner::checkAtvCvbsLockHelper(void *data)
{
    if (data == NULL) return false;
#ifdef SUPPORT_ADTV
    AM_SCAN_ATV_LOCK_PARA_t *pAtvPara = (AM_SCAN_ATV_LOCK_PARA_t *)data;
    CTvScanner *pScan = (CTvScanner *)(pAtvPara->pData);
    if (mInstance != pScan)
        return AM_FALSE;
    unsigned long std;
    AM_Bool_t isLock = pScan->checkAtvCvbsLock(&std);
    pAtvPara->pOutColorSTD = std;
    return isLock;
#else
    return AM_FALSE;
#endif
}

int CTvScanner::getScanDtvStandard(ScanParas &scp) {
#ifdef SUPPORT_ADTV
    int forceDtvStd = scp.getDtvStandard();
    const char *dtvStd = config_get_str ( CFG_SECTION_TV, CFG_DTV_SCAN_STD_FORCE, "null");
    if (!strcmp(dtvStd, "atsc"))
        forceDtvStd = TV_SCAN_DTV_STD_ATSC;
    else if (!strcmp(dtvStd, "dvb"))
        forceDtvStd = TV_SCAN_DTV_STD_DVB;
    else if (!strcmp(dtvStd, "isdb"))
        forceDtvStd = TV_SCAN_DTV_STD_ISDB;

    if (forceDtvStd != -1) {
        LOGD("force dtv std: %d", forceDtvStd);
        return forceDtvStd;
    }
#endif
    return -1;
}

void CTvScanner::reconnectDmxToFend(int dmx_no, int fend_no __unused)
{
#ifdef SUPPORT_ADTV
    int isTV = config_get_int(CFG_SECTION_TV, FRONTEND_TS_SOURCE, 0);

    if (isTV == 2) {
        AM_DMX_SetSource(dmx_no, AM_DMX_SRC_TS2);
        LOGD("TV Set demux%d source to AM_DMX_SRC_TS2", dmx_no);
    }else if (isTV == 1) {
        AM_DMX_SetSource(dmx_no, AM_DMX_SRC_TS1);
        LOGD("TV Set demux%d source to AM_DMX_SRC_TS1", dmx_no);
    }else{
        AM_DMX_SetSource(dmx_no, AM_DMX_SRC_TS0);
        LOGD("NON-TV Set demux%d source to AM_DMX_SRC_TS0", dmx_no);
    }
#endif
}

bool CTvScanner::needVbiAssist() {
    return (getScanDtvStandard(mScanParas) == TV_SCAN_DTV_STD_ATSC
        && mScanParas.getAtvMode() != TV_SCAN_ATVMODE_NONE);
}

int CTvScanner::startVBI()
{
#ifdef SUPPORT_ADTV
    AM_CC_CreatePara_t cc_para;
    AM_CC_StartPara_t spara;
    int ret;

    if (mVbi)
        return 0;

    mVbiTsId = -1;

    memset(&cc_para, 0, sizeof(cc_para));
    cc_para.input = AM_CC_INPUT_VBI;
    cc_para.network_cb = CC_VBINetworkCbHelper;
    cc_para.user_data = this;
    ret = AM_CC_Create(&cc_para, &mVbi);
    if (ret != DVB_SUCCESS)
        goto error;

    memset(&spara, 0, sizeof(spara));
    spara.caption1 = AM_CC_CAPTION_XDS;
    spara.caption2 = AM_CC_CAPTION_NONE;
    ret = AM_CC_Start(mVbi, &spara);
    if (ret != DVB_SUCCESS)
        goto error;

#endif
    LOGD("start cc successfully!");
    return 0;
error:
    stopVBI();
    LOGD("start cc failed!");
    return -1;

}

void CTvScanner::stopVBI()
{
#ifdef SUPPORT_ADTV
    if (mVbi != NULL) {
        AM_CC_Destroy(mVbi);
        mVbi = NULL;
    }
    mVbiTsId = -1;
#endif
}

void CTvScanner::resetVBI()
{
    stopVBI();
    startVBI();
}

bool CTvScanner::checkVbiDataReady(int freq) {
    for (int i = 0; i < 5; i++) {
        if (mVbiTsId != -1)
            break;
        usleep(50 * 1000);
    }

    if (mVbiTsId == -1) {
        LOGD("VbiDataNotReady");
        return false;
    }

    LOGD("VbiDataReady");
    SCAN_TsIdInfo_t *tsid_info = (SCAN_TsIdInfo_t*)calloc(sizeof(SCAN_TsIdInfo_t), 1);
    if (!tsid_info) {
       LOGE("No Memory for Scanner.");
       return false;
    }

    tsid_info->freq = freq;
    tsid_info->tsid = mVbiTsId;
    tsid_list.push_back(tsid_info);
    return true;
}

int CTvScanner::FETypeHelperCBHelper(int id, void *para, void *user) {
    if (mInstance)
        mInstance->FETypeHelperCB(id, para, user);
    else
        LOGE("no scanner running, ignore FETypeHelperCB");
    return -1;
}

int CTvScanner::FETypeHelperCB(int id, void *para, void *user) {
#ifdef SUPPORT_ADTV
    if ((id != AM_SCAN_HELPER_ID_FE_TYPE_CHANGE)
        || (user != (void*)this))
        return -1;

    fe_type_t type = static_cast<fe_type_t>(reinterpret_cast<intptr_t>(para));
    LOGD("FE set mode %d", type);

    if (type == mFEType)
        return 0;

    mFEType = type;

    CFrontEnd *fe = CFrontEnd::getInstance();
    CTvin *tvin = CTvin::getInstance();
    if (type == TV_FE_ANALOG) {
        fe->setMode(type);
        tvin->Tvin_StopDecoder();
        tvin->VDIN_ClosePort();
        tvin->VDIN_OpenPort(tvin->Tvin_GetSourcePortBySourceInput(SOURCE_TV));
        if (needVbiAssist())
            resetVBI();
    } else {
        if (needVbiAssist())
            stopVBI();
        tvin->Tvin_StopDecoder();
        tvin->VDIN_ClosePort();
        //tvin->VDIN_OpenPort(tvin->Tvin_GetSourcePortBySourceInput(SOURCE_DTV));
        //tvin->VDIN_CloseModule();
        //tvin->AFE_CloseModule();
        fe->setMode(type);
        //CTvScanner *pScanner = (CTvScanner *)user;
        reconnectDmxToFend(0, 0);
    }
#endif
    return 0;
}


 void CTvScanner::evtCallback(long dev_no, int event_type, void *param, void *data __unused)
{
#ifdef SUPPORT_ADTV
    CTvScanner *pT = NULL;
    long long tmpFreq = 0;

    LOGD("evt evt:%d", event_type);
    AM_SCAN_GetUserData((void*)dev_no, (void **)&pT);
    if (pT == NULL) {
        return;
    }
    int AdtvMixed = (pT->mScanParas.getAtvMode() != TV_SCAN_ATVMODE_NONE
        && pT->mScanParas.getDtvMode() != TV_SCAN_DTVMODE_NONE)? 1 : 0;
    int factor =  (AdtvMixed && pT->mScanParas.getAtvMode() != TV_SCAN_ATVMODE_FREQ)? 50 : 100;

    pT->mCurEv.clear();
    memset(pT->mCurEv.mProgramName, '\0', sizeof(pT->mCurEv.mProgramName));
    memset(pT->mCurEv.mParas, '\0', sizeof(pT->mCurEv.mParas));
    memset(pT->mCurEv.mVct, '\0', sizeof(pT->mCurEv.mVct));
    if (event_type == AM_SCAN_EVT_PROGRESS) {
        AM_SCAN_Progress_t *evt = (AM_SCAN_Progress_t *)param;
        LOGD("progress evt:%d [%d%%]", evt->evt, pT->mCurEv.mPercent);
        switch (evt->evt) {
        case AM_SCAN_PROGRESS_SCAN_BEGIN: {
            AM_SCAN_CreatePara_t *cp = (AM_SCAN_CreatePara_t*)evt->data;
            pT->mCurEv.mPercent = 0;
            pT->mCurEv.mScanMode = (cp->mode<<24)|((cp->atv_para.mode&0xFF)<<16)|(cp->dtv_para.mode&0xFFFF);
            pT->mCurEv.mSortMode = (cp->dtv_para.standard<<16)|(cp->dtv_para.sort_method&0xFFFF);
            if (TV_FE_ATSC == pT->mFEParas.getFEMode().getBase())
                pT->mCurEv.mSortMode = pT->mCurEv.mSortMode | (TV_SCAN_DTV_STD_ATSC<<16);
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_BEGIN;
            pT->sendEvent(pT->mCurEv);
            }
            break;
        case AM_SCAN_PROGRESS_NIT_BEGIN:
            break;
        case AM_SCAN_PROGRESS_NIT_END:
            break;
        case AM_SCAN_PROGRESS_TS_BEGIN: {
            AM_SCAN_TSProgress_t *tp = (AM_SCAN_TSProgress_t *)evt->data;
            if (tp == NULL)
                break;
            pT->mCurEv.mChannelIndex = tp->index;
            if (pT->mFEParas.getFEMode().getBase() == tp->fend_para.m_type)
                pT->mCurEv.mMode = pT->mFEParas.getFEMode().getMode();
            else
                pT->mCurEv.mMode = tp->fend_para.m_type;
            pT->mCurEv.mFrequency = ((struct dvb_frontend_parameters *)(&tp->fend_para))->frequency;
            pT->mCurEv.mSymbolRate = tp->fend_para.cable.para.u.qam.symbol_rate;
            pT->mCurEv.mModulation = tp->fend_para.cable.para.u.qam.modulation;
            pT->mCurEv.mBandwidth = tp->fend_para.terrestrial.para.u.ofdm.bandwidth;

            pT->mCurEv.mFEParas.fromFENDCTRLParameters(CFrontEnd::FEMode(pT->mCurEv.mMode), &tp->fend_para);

            if (tp->fend_para.m_type == TV_FE_ANALOG) {
                if (AdtvMixed || pT->mScanParas.getAtvMode() == TV_SCAN_ATVMODE_FREQ) {//mix
                    pT->mCurEv.mPercent = (tp->index * factor) / tp->total;
                } else {
                    pT->mCurEv.mPercent = 0;
                }
            } else {
                pT->mCurEv.mPercent = (tp->index * factor) / tp->total;
            }

            if (pT->mCurEv.mTotalChannelCount == 0)
                pT->mCurEv.mTotalChannelCount = tp->total;
            if (pT->mCurEv.mPercent >= factor)
                pT->mCurEv.mPercent = factor -1;

            pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mStrength = 0;
            pT->mCurEv.mSnr = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_TS_END: {
            /*pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
            pT->sendEvent(pT->mCurEv);*/
        }
        break;

        case AM_SCAN_PROGRESS_PAT_DONE: /*{
                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
                    pT->sendEvent(pT->mCurEv);
                }
            }*/
            break;
        case AM_SCAN_PROGRESS_SDT_DONE: /*{
                dvbpsi_sdt_t *sdts = (dvbpsi_sdt_t *)evt->data;
                dvbpsi_sdt_t *sdt;

                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mPercent += 25;
                    if (pT->mCurEv.mPercent >= 100)
                        pT->mCurEv.mPercent = 99;
                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

                    pT->sendEvent(pT->mCurEv);
                }
            }*/
            break;
        case AM_SCAN_PROGRESS_CAT_DONE: /*{
                dvbpsi_cat_t *cat = (dvbpsi_cat_t *)evt->data;
                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mPercent += 25;
                    if (pT->mCurEv.mPercent >= 100)
                        pT->mCurEv.mPercent = 99;

                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

                    pT->sendEvent(pT->mCurEv);
                }
            }*/
            break;
        case AM_SCAN_PROGRESS_PMT_DONE: /*{
                dvbpsi_pmt_t *pmt = (dvbpsi_pmt_t *)evt->data;
                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mPercent += 25;
                    if (pT->mCurEv.mPercent >= 100)
                        pT->mCurEv.mPercent = 99;

                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
                    pT->sendEvent(pT->mCurEv);
                }
            }*/
            break;
        case AM_SCAN_PROGRESS_MGT_DONE: {
            //mgt_section_info_t *mgt = (mgt_section_info_t *)evt->data;

            if (pT->mCurEv.mTotalChannelCount == 1) {
                pT->mCurEv.mPercent += 10;
                if (pT->mCurEv.mPercent >= 100)
                    pT->mCurEv.mPercent = 99;

                pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
                pT->sendEvent(pT->mCurEv);
            }
        }
        break;
        case AM_SCAN_PROGRESS_VCT_DONE: {
            /*ATSC TVCT*/
            if (pT->mCurEv.mTotalChannelCount == 1) {
                pT->mCurEv.mPercent += 30;
                if (pT->mCurEv.mPercent >= 100)
                    pT->mCurEv.mPercent = 99;
                pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
                pT->sendEvent(pT->mCurEv);
            }
        }
        break;
        case AM_SCAN_PROGRESS_NEW_PROGRAM: {
            /* Notify the new searched programs */
            AM_SCAN_ProgramProgress_t *pp = (AM_SCAN_ProgramProgress_t *)evt->data;
            if (pp != NULL) {
                pT->mCurEv.mprogramType = pp->service_type;
                snprintf(pT->mCurEv.mProgramName, sizeof(pT->mCurEv.mProgramName), "%s", pp->name);
                pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
                pT->sendEvent(pT->mCurEv);
            }
        }
        break;
        case AM_SCAN_PROGRESS_NEW_PROGRAM_MORE: {
            AM_SCAN_NewProgram_Data_t *npd = (AM_SCAN_NewProgram_Data_t *)evt->data;
            if (npd != NULL) {
                switch (npd->newts->type) {
                    case AM_SCAN_TS_ANALOG:
                        if (npd->result->start_para->atv_para.mode & TV_SCAN_ATVMODE_AUTO == TV_SCAN_ATVMODE_AUTO
                            || npd->result->start_para->atv_para.mode & TV_SCAN_ATVMODE_FREQ == TV_SCAN_ATVMODE_FREQ)
                            pT->storeScan(npd->result, npd->newts);
                    break;
                    case AM_SCAN_TS_DIGITAL:
                        if ((npd->result->start_para->dtv_para.mode & TV_SCAN_DTVMODE_AUTO == TV_SCAN_DTVMODE_AUTO)
                            || (npd->result->start_para->dtv_para.mode & TV_SCAN_DTVMODE_ALLBAND == TV_SCAN_DTVMODE_ALLBAND))
                            pT->storeScan(npd->result, npd->newts);
                    break;
                    default:
                    break;
                }
            }
        }
        break;
        case AM_SCAN_PROGRESS_BLIND_SCAN: {
            AM_SCAN_DTVBlindScanProgress_t *bs_prog = (AM_SCAN_DTVBlindScanProgress_t *)evt->data;

            if (bs_prog) {
                pT->mCurEv.mPercent = bs_prog->progress;

                /*snprintf(pT->mCurEv.mMSG, sizeof(pT->mCurEv.mMSG), "%s/%s %dMHz",
                         bs_prog->polar == AM_FEND_POLARISATION_H ? "H" : "V",
                         bs_prog->lo == AM_FEND_LOCALOSCILLATORFREQ_L ? "L-LOF" : "H-LOF",
                         bs_prog->freq / 1000);
                */
                pT->mCurEv.mType = ScannerEvent::EVENT_BLINDSCAN_PROGRESS;

                pT->sendEvent(pT->mCurEv);

                if (bs_prog->new_tp_cnt > 0) {
                    int i = 0;
                    for (i = 0; i < bs_prog->new_tp_cnt; i++) {
                        LOGD("New tp: %dkS/s %d====", bs_prog->new_tps[i].frequency,
                             bs_prog->new_tps[i].u.qpsk.symbol_rate);

                        pT->mCurEv.mFrequency = bs_prog->new_tps[i].frequency;
                        pT->mCurEv.mSymbolRate = bs_prog->new_tps[i].u.qpsk.symbol_rate;
                        pT->mCurEv.mSat_polarisation = bs_prog->polar;
                        pT->mCurEv.mType = ScannerEvent::EVENT_BLINDSCAN_NEWCHANNEL;
                        pT->sendEvent(pT->mCurEv);
                    }
                }
                if (bs_prog->progress >= 100) {
                    pT->mCurEv.mType = ScannerEvent::EVENT_BLINDSCAN_END;
                    pT->sendEvent(pT->mCurEv);
                    pT->mCurEv.mPercent = 0;
                }
            }
        }
        break;
        case AM_SCAN_PROGRESS_STORE_BEGIN: {
            pT->mCurEv.mType = ScannerEvent::EVENT_STORE_BEGIN;
            pT->mCurEv.mLockedStatus = 0;
            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_STORE_END: {
            pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_STORE_END;
            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_SCAN_END: {
            pT->mCurEv.mPercent = 100;
            pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_END;
            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_SCAN_EXIT: {
            pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_EXIT;
            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_ATV_TUNING: {
            CFrontEnd::FEMode femode(TV_FE_ANALOG);
            pT->mCurEv.mFEParas.setFEMode(femode);
            pT->mCurEv.mFEParas.setFrequency((long)evt->data);
            pT->mCurEv.mFrequency = (long)evt->data;
            pT->mCurEv.mLockedStatus = 0;
            tmpFreq = (pT->mCurEv.mFrequency - pT->mCurScanStartFreq) / 1000000;
            pT->mCurEv.mPercent = tmpFreq * factor / ((pT->mCurScanEndFreq - pT->mCurScanStartFreq) / 1000000)
                                                + ((factor == 50)? 50 : 0);
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

            pT->sendEvent(pT->mCurEv);
        }
        break;

        default:
            break;
        }
    } else if (event_type == AM_SCAN_EVT_SIGNAL) {
        AM_SCAN_DTVSignalInfo_t *evt = (AM_SCAN_DTVSignalInfo_t *)param;
        //pT->mCurEv.mprogramType = 0xff;
        pT->mCurEv.mFrequency = (int)evt->frequency;
        pT->mCurEv.mFEParas.setFrequency(evt->frequency);
        pT->mCurEv.mLockedStatus = (evt->locked ? 1 : 0);

        if (pT->mCurEv.mFEParas.getFEMode().getBase() == TV_FE_ANALOG && evt->locked) {//trick here for atv new prog
            pT->mCurEv.mLockedStatus |= 0x10;
            if (pT->needVbiAssist())
                pT->checkVbiDataReady(evt->frequency);
        }

        pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
        if (pT->mCurEv.mFEParas.getFEMode().getBase() != TV_FE_ANALOG && evt->locked) {
            pT->mCurEv.mStrength = evt->strength;
            pT->mCurEv.mSnr = evt->snr;
        } else {
            pT->mCurEv.mStrength = 0;
            pT->mCurEv.mSnr = 0;
        }

        //if (pT->mCurEv.mMode == TV_FE_ANALOG)
        pT->sendEvent(pT->mCurEv);
        pT->mCurEv.mLockedStatus &= ~0x10;
    }
#endif
}


const char* CTvScanner::ScanParas::SCP_MODE = "m";
const char* CTvScanner::ScanParas::SCP_ATVMODE = "am";
const char* CTvScanner::ScanParas::SCP_DTVMODE = "dm";
const char* CTvScanner::ScanParas::SCP_ATVFREQ1 = "af1";
const char* CTvScanner::ScanParas::SCP_ATVFREQ2 = "af2";
const char* CTvScanner::ScanParas::SCP_DTVFREQ1 = "df1";
const char* CTvScanner::ScanParas::SCP_DTVFREQ2 = "df2";
const char* CTvScanner::ScanParas::SCP_PROC = "prc";
const char* CTvScanner::ScanParas::SCP_DTVSTD = "dstd";

CTvScanner::ScanParas& CTvScanner::ScanParas::operator = (const ScanParas &spp)
{
    this->mparas = spp.mparas;
    return *this;
}

int CTvScanner::getAtscChannelPara(int attennaType, Vector<sp<CTvChannel> > &vcp)
{
    switch (attennaType) { //region name should be remove to config file and read here
    case 1:
        CTvRegion::getChannelListByName((char *)"US,ATSC Air", vcp);
        break;
    case 2:
        CTvRegion::getChannelListByName((char *)"US,ATSC Cable Standard", vcp);
        break;
    case 3:
        CTvRegion::getChannelListByName((char *)"US,ATSC Cable IRC", vcp);
        break;
    case 4:
        CTvRegion::getChannelListByName((char *)"US,ATSC Cable HRC", vcp);
        break;
    default:
        return -1;
    }

    return 0;
}

int CTvScanner::ATVManualScan(int min_freq, int max_freq, int std, int store_Type, int channel_num)
{
#ifdef SUPPORT_ADTV
    UNUSED(store_Type);
    UNUSED(channel_num);

    char paras[128];
    CFrontEnd::convertParas(paras, TV_FE_ANALOG, min_freq, max_freq, std, 0, 0, 0);

    CFrontEnd::FEParas fe(paras);

    ScanParas sp;
    sp.setMode(TV_SCAN_MODE_ATV_DTV);
    sp.setDtvMode(TV_SCAN_DTVMODE_NONE);
    sp.setAtvMode(TV_SCAN_ATVMODE_MANUAL);
    sp.setAtvFrequency1(min_freq);
    sp.setAtvFrequency2(max_freq);
    return Scan(fe, sp);
#else
    return -1;
#endif
}

int CTvScanner::autoAtvScan(int min_freq, int max_freq, int std, int search_type, int proc_mode)
{
#ifdef SUPPORT_ADTV
    UNUSED(search_type);

    char paras[128];
    CFrontEnd::convertParas(paras, TV_FE_ANALOG, min_freq, max_freq, std, 0, 0, 0);

    CFrontEnd::FEParas fe(paras);

    ScanParas sp;
    sp.setMode(TV_SCAN_MODE_ATV_DTV);
    sp.setDtvMode(TV_SCAN_DTVMODE_NONE);
    sp.setAtvMode(TV_SCAN_ATVMODE_AUTO);
    sp.setAtvFrequency1(min_freq);
    sp.setAtvFrequency2(max_freq);
    sp.setProc(proc_mode);
    return Scan(fe, sp);
#else
    return -1;
#endif
}

int CTvScanner::autoAtvScan(int min_freq, int max_freq, int std, int search_type)
{
    return autoAtvScan(min_freq, max_freq, std, search_type, 0);
}

int CTvScanner::dtvScan(int mode, int scan_mode, int beginFreq, int endFreq, int para1, int para2)
{
    char feparas[128];
    CFrontEnd::convertParas(feparas, mode, beginFreq, endFreq, para1, para2, 0, 0);
    return dtvScan(feparas, scan_mode, beginFreq, endFreq);
}

int CTvScanner::dtvScan(char *feparas, int scan_mode, int beginFreq, int endFreq)
{
    LOGE("dtvScan fe:[%s]", feparas);
    CFrontEnd::FEParas fe(feparas);
    ScanParas sp;
    sp.setMode(TV_SCAN_MODE_DTV_ATV);
    sp.setAtvMode(TV_SCAN_ATVMODE_NONE);
    sp.setDtvMode(scan_mode);
    sp.setDtvFrequency1(beginFreq);
    sp.setDtvFrequency2(endFreq);
    return Scan(fe, sp);
}

int CTvScanner::dtvAutoScan(int mode)
{
    return dtvScan(mode, TV_SCAN_DTVMODE_ALLBAND, 0, 0, -1, -1);
}

int CTvScanner::dtvManualScan(int mode, int beginFreq, int endFreq, int para1, int para2)
{
    return dtvScan(mode, TV_SCAN_DTVMODE_MANUAL, beginFreq, endFreq, para1, para2);
}

int CTvScanner::manualDtmbScan(int beginFreq, int endFreq, int modulation)
{
#ifdef SUPPORT_ADTV
    return dtvManualScan(TV_FE_DTMB, beginFreq, endFreq, modulation, -1);
#else
    return -1;
#endif
}

//only test for dtv allbland auto
int CTvScanner::autoDtmbScan()
{
#ifdef SUPPORT_ADTV
    return dtvAutoScan(TV_FE_DTMB);
#else
    return -1;
#endif
}


