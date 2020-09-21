/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @date     2018/1/16
 *  @par function description:
 *  - 1 droidlogic tvservice interface
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "HIDLIntf"

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>
#include <utils/Errors.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <ITvService.h>
#include <hardware/hardware.h>
#include "DroidTvServiceIntf.h"
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <CTvLog.h>
#include <tvconfig.h>
#include <tvscanconfig.h>
#include <tvutils.h>
#include <tvsetting/CTvSetting.h>
#include <version/version.h>
#include "tvcmd.h"
#include <tvdb/CTvRegion.h>
#include "MemoryLeakTrackUtil.h"
extern "C" {
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#ifdef SUPPORT_ADTV
#include "am_ver.h"
#endif
}

#include "DroidTvServiceIntf.h"

using namespace android;
using namespace std;

DroidTvServiceIntf::DroidTvServiceIntf()
{
    //mpScannerClient = NULL;
    mpTv = new CTv();
    mpTv->setTvObserver(this);
    mpTv->OpenTv();
    mIsStartTv = false;
}

DroidTvServiceIntf::~DroidTvServiceIntf()
{
    //mpScannerClient = NULL;
    /*
    for (int i = 0; i < (int)mClients.size(); i++) {
        wp<Client> client = mClients[i];
        if (client != 0) {
            LOGW("some client still connect it!");
        }
    }*/

    if (mpTv != NULL) {
        delete mpTv;
        mpTv = NULL;
    }
}

void DroidTvServiceIntf::startListener() {
    mpTv->startTvDetect();
}

void DroidTvServiceIntf::onTvEvent(const CTvEv &ev)
{
    int type = ev.getEvType();
    LOGD("DroidTvServiceIntf::onTvEvent ev type = %d", type);
    switch (type) {
    case CTvEv::TV_EVENT_SCANNER: {
        CTvScanner::ScannerEvent *pScannerEv = (CTvScanner::ScannerEvent *) (&ev);
        //if (mpScannerClient != NULL) {
            //sp<Client> ScannerClient = mpScannerClient.promote();
            //if (ScannerClient != 0) {
                TvHidlParcel hidlParcel;
                LOGD("scanner evt type:%d freq:%d vid:%d acnt:%d scnt:%d",
                     pScannerEv->mType, pScannerEv->mFrequency, pScannerEv->mVid, pScannerEv->mAcnt, pScannerEv->mScnt);
                hidlParcel.msgType = SCAN_EVENT_CALLBACK;
                hidlParcel.bodyInt.resize(MAX_LCN*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+41);
                hidlParcel.bodyString.resize(pScannerEv->mScnt+pScannerEv->mAcnt+3);
                hidlParcel.bodyInt[0] = pScannerEv->mType;
                hidlParcel.bodyInt[1] = pScannerEv->mPercent;
                hidlParcel.bodyInt[2] = pScannerEv->mTotalChannelCount;
                hidlParcel.bodyInt[3] = pScannerEv->mLockedStatus;
                hidlParcel.bodyInt[4] = pScannerEv->mChannelIndex;
                hidlParcel.bodyInt[5] = pScannerEv->mFrequency;
                hidlParcel.bodyString[0] = pScannerEv->mProgramName;
                hidlParcel.bodyInt[6] = pScannerEv->mprogramType;
                hidlParcel.bodyString[1] = pScannerEv->mParas;
                hidlParcel.bodyInt[7] = pScannerEv->mStrength;
                hidlParcel.bodyInt[8] = pScannerEv->mSnr;
                //ATV
                hidlParcel.bodyInt[9] = pScannerEv->mVideoStd;
                hidlParcel.bodyInt[10] = pScannerEv->mAudioStd;
                hidlParcel.bodyInt[11] = pScannerEv->mIsAutoStd;
                //DTV
                hidlParcel.bodyInt[12] = pScannerEv->mMode;
                hidlParcel.bodyInt[13] = pScannerEv->mSymbolRate;
                hidlParcel.bodyInt[14] = pScannerEv->mModulation;
                hidlParcel.bodyInt[15] = pScannerEv->mBandwidth;
                hidlParcel.bodyInt[16] = pScannerEv->mReserved;
                hidlParcel.bodyInt[17] = pScannerEv->mTsId;
                hidlParcel.bodyInt[18] = pScannerEv->mONetId;
                hidlParcel.bodyInt[19] = pScannerEv->mServiceId;
                hidlParcel.bodyInt[20] = pScannerEv->mVid;
                hidlParcel.bodyInt[21] = pScannerEv->mVfmt;
                hidlParcel.bodyInt[22] = pScannerEv->mAcnt;
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    hidlParcel.bodyInt[i+23] = pScannerEv->mAid[i];
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    hidlParcel.bodyInt[i+pScannerEv->mAcnt+23] = pScannerEv->mAfmt[i];
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    hidlParcel.bodyString[i+2] = pScannerEv->mAlang[i];
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    hidlParcel.bodyInt[i+2*pScannerEv->mAcnt+23] = pScannerEv->mAtype[i];
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    hidlParcel.bodyInt[i+3*pScannerEv->mAcnt+23] = pScannerEv->mAExt[i];
                hidlParcel.bodyInt[4*pScannerEv->mAcnt+23] = pScannerEv->mPcr;
                hidlParcel.bodyInt[4*pScannerEv->mAcnt+24] = pScannerEv->mScnt;
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    hidlParcel.bodyInt[i+4*pScannerEv->mAcnt+25] = pScannerEv->mStype[i];
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    hidlParcel.bodyInt[i+pScannerEv->mScnt+4*pScannerEv->mAcnt+25] = pScannerEv->mSid[i];
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    hidlParcel.bodyInt[i+2*pScannerEv->mScnt+4*pScannerEv->mAcnt+25] = pScannerEv->mSstype[i];
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    hidlParcel.bodyInt[i+3*pScannerEv->mScnt+4*pScannerEv->mAcnt+25] = pScannerEv->mSid1[i];
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    hidlParcel.bodyInt[i+4*pScannerEv->mScnt+4*pScannerEv->mAcnt+25] = pScannerEv->mSid2[i];
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    hidlParcel.bodyString[i+pScannerEv->mAcnt+2] = pScannerEv->mSlang[i];
                hidlParcel.bodyInt[5*pScannerEv->mScnt+4*pScannerEv->mAcnt+25] = pScannerEv->mFree_ca;
                hidlParcel.bodyInt[5*pScannerEv->mScnt+4*pScannerEv->mAcnt+26] = pScannerEv->mScrambled;
                hidlParcel.bodyInt[5*pScannerEv->mScnt+4*pScannerEv->mAcnt+27] = pScannerEv->mScanMode;
                hidlParcel.bodyInt[5*pScannerEv->mScnt+4*pScannerEv->mAcnt+28] = pScannerEv->mSdtVer;
                hidlParcel.bodyInt[5*pScannerEv->mScnt+4*pScannerEv->mAcnt+29] = pScannerEv->mSortMode;

                hidlParcel.bodyInt[5*pScannerEv->mScnt+4*pScannerEv->mAcnt+30] = pScannerEv->mLcnInfo.net_id;
                hidlParcel.bodyInt[5*pScannerEv->mScnt+4*pScannerEv->mAcnt+31] = pScannerEv->mLcnInfo.ts_id;
                hidlParcel.bodyInt[5*pScannerEv->mScnt+4*pScannerEv->mAcnt+32] = pScannerEv->mLcnInfo.service_id;
                for (int i=0; i<MAX_LCN; i++) {
                    hidlParcel.bodyInt[i*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+33] = pScannerEv->mLcnInfo.visible[i];
                    hidlParcel.bodyInt[i*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+34] = pScannerEv->mLcnInfo.lcn[i];
                    hidlParcel.bodyInt[i*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+35] = pScannerEv->mLcnInfo.valid[i];
                }
                hidlParcel.bodyInt[MAX_LCN*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+33] = pScannerEv->mMajorChannelNumber;
                hidlParcel.bodyInt[MAX_LCN*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+34] = pScannerEv->mMinorChannelNumber;
                hidlParcel.bodyInt[MAX_LCN*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+35] = pScannerEv->mSourceId;
                hidlParcel.bodyInt[MAX_LCN*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+36] = pScannerEv->mAccessControlled;
                hidlParcel.bodyInt[MAX_LCN*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+37] = pScannerEv->mHidden;
                hidlParcel.bodyInt[MAX_LCN*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+38] = pScannerEv->mHideGuide;
                hidlParcel.bodyString[pScannerEv->mScnt+pScannerEv->mAcnt+2] = pScannerEv->mVct;
                hidlParcel.bodyInt[MAX_LCN*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+39] = pScannerEv->mProgramsInPat;
                hidlParcel.bodyInt[MAX_LCN*3+5*pScannerEv->mScnt+4*pScannerEv->mAcnt+40] = pScannerEv->mPatTsId;

                mNotifyListener->onEvent(hidlParcel);
            //}
        //}
        break;
    }

    case CTvEv::TV_EVENT_EPG: {
        CTvEpg::EpgEvent *pEpgEvent = (CTvEpg::EpgEvent *) (&ev);
        TvHidlParcel hidlParcel;

        hidlParcel.msgType = EPG_EVENT_CALLBACK;
        hidlParcel.bodyInt.resize(4);
        hidlParcel.bodyInt[0] = pEpgEvent->type;
        hidlParcel.bodyInt[1] = pEpgEvent->time;
        hidlParcel.bodyInt[2] = pEpgEvent->programID;
        hidlParcel.bodyInt[3] = pEpgEvent->channelID;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_SOURCE_SWITCH: {
        TvEvent::SourceSwitchEvent *pEv = (TvEvent::SourceSwitchEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = SOURCE_SWITCH_CALLBACK;
        hidlParcel.bodyInt.resize(2);
        hidlParcel.bodyInt[0] = pEv->DestSourceInput;
        hidlParcel.bodyInt[1] = pEv->DestSourcePortNum;
        LOGD("source switch: destSource = %d, portID = %d\n", pEv->DestSourceInput, pEv->DestSourcePortNum);
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_HDMI_IN_CAP: {
        /*
        TvHidlParcel hidlParcel;

        CTvScreenCapture::CapEvent *pCapEvt = (CTvScreenCapture::CapEvent *)(&ev);

        hidlParcel.msgType = VFRAME_BMP_EVENT_CALLBACK;

        ALOGI("TV_EVENT_HDMI_IN_CAP size:%d", sizeof(CTvScreenCapture::CapEvent));

        hidlParcel.bodyInt.resize(4);
        hidlParcel.bodyInt[0] = pCapEvt->mFrameNum;
        hidlParcel.bodyInt[1] = pCapEvt->mFrameSize;
        hidlParcel.bodyInt[2] = pCapEvt->mFrameWide;
        hidlParcel.bodyInt[3] = pCapEvt->mFrameHeight;

        mNotifyListener->onEvent(hidlParcel);
        */
        break;
    }

    case CTvEv::TV_EVENT_AV_PLAYBACK: {
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = DTV_AV_PLAYBACK_CALLBACK;

        TvEvent::AVPlaybackEvent *pEv = (TvEvent::AVPlaybackEvent *)(&ev);

        hidlParcel.bodyInt.resize(2);
        hidlParcel.bodyInt[0] = pEv->mMsgType;
        hidlParcel.bodyInt[1] = pEv->mProgramId;

        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_SIGLE_DETECT: {
        TvEvent::SignalInfoEvent *pEv = (TvEvent::SignalInfoEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = SIGLE_DETECT_CALLBACK;
        hidlParcel.bodyInt.resize(4);
        hidlParcel.bodyInt[0] = pEv->mTrans_fmt;
        hidlParcel.bodyInt[1] = pEv->mFmt;
        hidlParcel.bodyInt[2] = pEv->mStatus;
        hidlParcel.bodyInt[3] = pEv->mReserved;
        LOGD("Tv Event signal detect mTrans_fmt =%d, mFmt = %d, mStatus = %d, mReserved = %d", pEv->mTrans_fmt, pEv->mFmt, pEv->mStatus, pEv->mReserved);
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_SUBTITLE: {
        TvEvent::SubtitleEvent *pEv = (TvEvent::SubtitleEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = SUBTITLE_UPDATE_CALLBACK;
        hidlParcel.bodyInt.resize(2);
        hidlParcel.bodyInt[0] = pEv->pic_width;
        hidlParcel.bodyInt[1] = pEv->pic_height;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_ADC_CALIBRATION: {
        TvEvent::ADCCalibrationEvent *pEv = (TvEvent::ADCCalibrationEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = ADC_CALIBRATION_CALLBACK;
        hidlParcel.bodyInt.resize(1);
        hidlParcel.bodyInt[0] = pEv->mState;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_VGA: {//VGA
        TvEvent::VGAEvent *pEv = (TvEvent::VGAEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = VGA_CALLBACK;
        hidlParcel.bodyInt.resize(1);
        hidlParcel.bodyInt[0] = pEv->mState;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_SOURCE_CONNECT: {
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = SOURCE_CONNECT_CALLBACK;

        TvEvent::SourceConnectEvent *pEv = (TvEvent::SourceConnectEvent *)(&ev);

        hidlParcel.bodyInt.resize(2);
        hidlParcel.bodyInt[0] = pEv->mSourceInput;
        hidlParcel.bodyInt[1] = pEv->connectionState;

        LOGD("source connect input: =%d state: =%d", pEv->mSourceInput, pEv->connectionState);
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_HDMIRX_CEC: {
        TvEvent::HDMIRxCECEvent *pEv = (TvEvent::HDMIRxCECEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = HDMIRX_CEC_CALLBACK;
        hidlParcel.bodyInt.resize(pEv->mDataCount + 1);
        hidlParcel.bodyInt[0] = pEv->mDataCount;
        for (int j = 0; j < pEv->mDataCount; j++) {
            hidlParcel.bodyInt[j+1] = pEv->mDataBuf[j];
        }
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_UPGRADE_FBC: {
        TvEvent::UpgradeFBCEvent *pEv = (TvEvent::UpgradeFBCEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = UPGRADE_FBC_CALLBACK;
        hidlParcel.bodyInt.resize(2);
        hidlParcel.bodyInt[0] = pEv->mState;
        hidlParcel.bodyInt[1] = pEv->param;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_SERIAL_COMMUNICATION: {
        TvEvent::SerialCommunicationEvent *pEv = (TvEvent::SerialCommunicationEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = SERIAL_COMMUNICATION_CALLBACK;
        hidlParcel.bodyInt.resize(pEv->mDataCount+2);
        hidlParcel.bodyInt[0] = pEv->mDevId;
        hidlParcel.bodyInt[1] = pEv->mDataCount;
        for (int j = 0; j < pEv->mDataCount; j++) {
            hidlParcel.bodyInt[j+2] = pEv->mDataBuf[j];
        }
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_2d4G_HEADSET: {
        TvEvent::HeadSetOf2d4GEvent *pEv = (TvEvent::HeadSetOf2d4GEvent *)(&ev);
        LOGD("SendDtvStats status: =%d para2: =%d", pEv->state, pEv->para);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = HEADSET_STATUS_CALLBACK;
        hidlParcel.bodyInt.resize(2);
        hidlParcel.bodyInt[0] = pEv->state;
        hidlParcel.bodyInt[1] = pEv->para;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_SCANNING_FRAME_STABLE: {
        TvEvent::ScanningFrameStableEvent *pEv = (TvEvent::ScanningFrameStableEvent *)(&ev);
        LOGD("Scanning Frame is stable!");
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = SCANNING_FRAME_STABLE_CALLBACK;
        hidlParcel.bodyInt.resize(1);
        hidlParcel.bodyInt[0] = pEv->CurScanningFreq;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_FRONTEND: {
        TvEvent::FrontendEvent *pEv = (TvEvent::FrontendEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = FRONTEND_EVENT_CALLBACK;
        hidlParcel.bodyInt.resize(10);
        hidlParcel.bodyInt[0] = pEv->mStatus;
        hidlParcel.bodyInt[1] = pEv->mFrequency;
        hidlParcel.bodyInt[2] = pEv->mParam1;
        hidlParcel.bodyInt[3] = pEv->mParam2;
        hidlParcel.bodyInt[4] = pEv->mParam3;
        hidlParcel.bodyInt[5] = pEv->mParam4;
        hidlParcel.bodyInt[6] = pEv->mParam5;
        hidlParcel.bodyInt[7] = pEv->mParam6;
        hidlParcel.bodyInt[8] = pEv->mParam7;
        hidlParcel.bodyInt[9] = pEv->mParam8;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_RECORDER: {
        TvEvent::RecorderEvent *pEv = (TvEvent::RecorderEvent *)(&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = RECORDER_EVENT_CALLBACK;
        hidlParcel.bodyInt.resize(2);
        hidlParcel.bodyString.resize(1);
        hidlParcel.bodyString[0]= pEv->mId;
        hidlParcel.bodyInt[0] = pEv->mStatus;
        hidlParcel.bodyInt[1] = pEv->mError;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_RRT: {
        CTvRrt::RrtEvent *pRrtEvent = (CTvRrt::RrtEvent *) (&ev);
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = RRT_EVENT_CALLBACK;
        hidlParcel.bodyInt.resize(1);
        hidlParcel.bodyInt[0] = pRrtEvent->satus;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_EAS: {
        CTvEas::EasEvent *pEasEvent = (CTvEas::EasEvent *) (&ev);

        TvHidlParcel hidlParcel;
        int index = 0;
        hidlParcel.msgType = EAS_EVENT_CALLBACK;
        hidlParcel.bodyInt.resize(1024);
        hidlParcel.bodyInt[index++] = pEasEvent->eas_section_count;
        hidlParcel.bodyInt[index++] = pEasEvent->table_id;
        hidlParcel.bodyInt[index++] = pEasEvent->extension;
        hidlParcel.bodyInt[index++] = pEasEvent->version;
        hidlParcel.bodyInt[index++] = pEasEvent->current_next;
        hidlParcel.bodyInt[index++] = pEasEvent->sequence_num;
        hidlParcel.bodyInt[index++] = pEasEvent->protocol_version;
        hidlParcel.bodyInt[index++] = pEasEvent->eas_event_id;
        hidlParcel.bodyInt[index++] = pEasEvent->eas_orig_code[0];
        hidlParcel.bodyInt[index++] = pEasEvent->eas_orig_code[1];
        hidlParcel.bodyInt[index++] = pEasEvent->eas_orig_code[2];
        hidlParcel.bodyInt[index++] = pEasEvent->eas_event_code_len;
        for (int j=0;j<pEasEvent->eas_event_code_len;j++) {
            hidlParcel.bodyInt[index++] = pEasEvent->eas_event_code[j];
        }
        hidlParcel.bodyInt[index++] = pEasEvent->alert_message_time_remaining;
        hidlParcel.bodyInt[index++] = pEasEvent->event_start_time;
        hidlParcel.bodyInt[index++] = pEasEvent->event_duration;
        hidlParcel.bodyInt[index++] = pEasEvent->alert_priority;
        hidlParcel.bodyInt[index++] = pEasEvent->details_OOB_source_ID;
        hidlParcel.bodyInt[index++] = pEasEvent->details_major_channel_number;
        hidlParcel.bodyInt[index++] = pEasEvent->details_minor_channel_number;
        hidlParcel.bodyInt[index++] = pEasEvent->audio_OOB_source_ID;
        hidlParcel.bodyInt[index++] = pEasEvent->location_count;
        for (int j=0;j<pEasEvent->location_count;j++) {
            hidlParcel.bodyInt[index++] = pEasEvent->location[j].i_state_code;
            hidlParcel.bodyInt[index++] = pEasEvent->location[j].i_country_subdiv;
            hidlParcel.bodyInt[index++] = pEasEvent->location[j].i_country_code;
        }
        hidlParcel.bodyInt[index++] = pEasEvent->exception_count;
        for (int j=0;j<pEasEvent->exception_count;j++) {
            hidlParcel.bodyInt[index++] = pEasEvent->exception[j].i_in_band_refer;
            hidlParcel.bodyInt[index++] = pEasEvent->exception[j].i_exception_major_channel_number;
            hidlParcel.bodyInt[index++] = pEasEvent->exception[j].i_exception_minor_channel_number;
            hidlParcel.bodyInt[index++] = pEasEvent->exception[j].exception_OOB_source_ID;
        }
        hidlParcel.bodyInt[index++] = pEasEvent->multi_text_count;
        for (int j=0;j<pEasEvent->multi_text_count;j++) {
            hidlParcel.bodyInt[index++] = pEasEvent->multi_text[j].lang[0];
            hidlParcel.bodyInt[index++] = pEasEvent->multi_text[j].lang[1];
            hidlParcel.bodyInt[index++] = pEasEvent->multi_text[j].lang[2];
            hidlParcel.bodyInt[index++] = pEasEvent->multi_text[j].i_type;
            hidlParcel.bodyInt[index++] = pEasEvent->multi_text[j].i_compression_type;
            hidlParcel.bodyInt[index++] = pEasEvent->multi_text[j].i_mode;
            hidlParcel.bodyInt[index++] = pEasEvent->multi_text[j].i_number_bytes;
            for (int k=0;k<pEasEvent->multi_text[j].i_number_bytes;k++) {
                hidlParcel.bodyInt[index++] = pEasEvent->multi_text[j].compressed_str[k];
            }
        }
        hidlParcel.bodyInt[index++] = pEasEvent->descriptor_text_count;
        for (int j=0;j<pEasEvent->descriptor_text_count;j++) {
            hidlParcel.bodyInt[index++] = pEasEvent->descriptor[j].i_tag;
            hidlParcel.bodyInt[index++] = pEasEvent->descriptor[j].i_length;
            for (int k=0;k<pEasEvent->descriptor[j].i_length;k++) {
                hidlParcel.bodyInt[index++] = pEasEvent->descriptor[j].p_data[k];
            }
        }
        mNotifyListener->onEvent(hidlParcel);
        break;
    }
    case CTvEv::TV_EVENT_AUDIO_CB: {
        TvEvent::AVAudioCBEvent *pEv = (TvEvent::AVAudioCBEvent *)(&ev);

        TvHidlParcel hidlParcel;
        hidlParcel.msgType = AUDIO_EVENT_CALLBACK;
        hidlParcel.bodyInt.resize(3);
        hidlParcel.bodyInt[0] = pEv->cmd;
        hidlParcel.bodyInt[1] = pEv->param1;
        hidlParcel.bodyInt[2] = pEv->param2;
        mNotifyListener->onEvent(hidlParcel);
        break;
    }


    default:
        break;
    }
}

int DroidTvServiceIntf::startTv() {
    return mpTv->StartTvLock();
}

int DroidTvServiceIntf::stopTv() {
    return mpTv->StopTvLock();
}

int DroidTvServiceIntf::switchInputSrc(int32_t inputSrc) {
    LOGD("switchInputSrc sourceId= 0x%x", inputSrc);
    return mpTv->SetSourceSwitchInput((tv_source_input_t)inputSrc);
}

int DroidTvServiceIntf::getInputSrcConnectStatus(int32_t inputSrc) {
    return mpTv->GetSourceConnectStatus((tv_source_input_t)inputSrc);
}

int DroidTvServiceIntf::getCurrentInputSrc() {
    return (int)mpTv->GetCurrentSourceInputVirtualLock();
}

int DroidTvServiceIntf::getHdmiAvHotplugStatus() {
    return mpTv->GetHdmiAvHotplugDetectOnoff();
}

std::string DroidTvServiceIntf::getSupportInputDevices() {
    const char *valueStr = config_get_str(CFG_SECTION_TV, CGF_DEFAULT_INPUT_IDS, "null");
    LOGD("get support input devices = %s", valueStr);
    return std::string(valueStr);
}

int DroidTvServiceIntf::getHdmiPorts(int32_t inputSrc) {
    int value = mpTv->Tv_GetHdmiPortBySourceInput((tv_source_input_t)inputSrc);
    return ((value & 0x0f) + 1);
}

void DroidTvServiceIntf::getCurSignalInfo(int &fmt, int &transFmt, int &status, int &frameRate) {
    tvin_info_t siginfo = mpTv->GetCurrentSignalInfo();
    int frame_rate = mpTv->getHDMIFrameRate();

    fmt = siginfo.fmt;
    transFmt = siginfo.trans_fmt;
    status = siginfo.status;
    frameRate = frame_rate;
}

int DroidTvServiceIntf::setMiscCfg(const std::string& key, const std::string& val) {
    //LOGD("setMiscCfg key = %s, val = %s", key.c_str(), val.c_str());
    return config_set_str(CFG_SECTION_TV, key.c_str(), val.c_str());
}

std::string DroidTvServiceIntf::getMiscCfg(const std::string& key, const std::string& def) {
    const char *value = config_get_str(CFG_SECTION_TV, key.c_str(), def.c_str());
    //LOGD("getMiscCfg key = %s, def = %s, value = %s", key.c_str(), def.c_str(), value);
    return std::string(value);
}

int DroidTvServiceIntf::isDviSIgnal() {
    return mpTv->IsDVISignal();
}

int DroidTvServiceIntf::isVgaTimingInHdmi() {
    return mpTv->isVgaFmtInHdmi();
}

int DroidTvServiceIntf::setHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mpTv->SetHdmiEdidVersion((tv_hdmi_port_id_t)port_id, (tv_hdmi_edid_version_t)ver);
}

int DroidTvServiceIntf::getHdmiEdidVersion(int32_t port_id) {
    return mpTv->GetHdmiEdidVersion((tv_hdmi_port_id_t)port_id);
}

int DroidTvServiceIntf::saveHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mpTv->SaveHdmiEdidVersion((tv_hdmi_port_id_t)port_id, (tv_hdmi_edid_version_t)ver);
}

int DroidTvServiceIntf::setHdmiColorRangeMode(int32_t range_mode)
{
    return mpTv->SetHdmiColorRangeMode((tvin_color_range_t)range_mode);
}

int DroidTvServiceIntf::getHdmiColorRangeMode()
{
    return mpTv->GetHdmiColorRangeMode();
}

int DroidTvServiceIntf::handleGPIO(const std::string& key, int is_out, int edge) {
    return mpTv->handleGPIO(key.c_str(), is_out, edge);
}
int DroidTvServiceIntf::setSourceInput(int32_t inputSrc) {
    return mpTv->SetSourceSwitchInput((tv_source_input_t)inputSrc);
}

int DroidTvServiceIntf::setSourceInput(int32_t inputSrc, int32_t vInputSrc) {
    return mpTv->SetSourceSwitchInput((tv_source_input_t)vInputSrc, (tv_source_input_t)inputSrc);
}

int DroidTvServiceIntf::setBlackoutEnable(int32_t status, int32_t is_save) {
    return mpTv->setBlackoutEnable(status, is_save);
}

int DroidTvServiceIntf::getBlackoutEnable() {
    return mpTv->getBlackoutEnable();
}

int DroidTvServiceIntf::getATVMinMaxFreq(int32_t &scanMinFreq, int32_t &scanMaxFreq) {
    return mpTv->getATVMinMaxFreq(&scanMinFreq, &scanMaxFreq);
}

int DroidTvServiceIntf::setAmAudioPreMute(int32_t mute) {
    return 0;//mpTv->setAmAudioPreMute(mute);//wxl for hidl compile
}

int DroidTvServiceIntf::setDvbTextCoding(const std::string& coding) {
    mpTv->setDvbTextCoding((char *)coding.c_str());
    return 0;
}

int DroidTvServiceIntf::operateDeviceForScan(int32_t type) {
    mpTv->operateDeviceForScan(type);
    return 0;
}

int DroidTvServiceIntf::atvAutoScan(int32_t videoStd, int32_t audioStd, int32_t searchType, int32_t procMode) {
    return mpTv->atvAutoScan(videoStd, audioStd, searchType, procMode);
}

int DroidTvServiceIntf::atvMunualScan(int32_t startFreq, int32_t endFreq, int32_t videoStd, int32_t audioStd) {
    return mpTv->atvMunualScan(startFreq, endFreq, videoStd, audioStd);
}

int DroidTvServiceIntf::Scan(const std::string& feparas, const std::string& scanparas) {
    return mpTv->Scan(feparas.c_str(), scanparas.c_str());
}

int DroidTvServiceIntf::dtvScan(int32_t mode, int32_t scan_mode, int32_t beginFreq, int32_t endFreq, int32_t para1, int32_t para2) {
    return mpTv->dtvScan(mode, scan_mode, beginFreq, endFreq, para1, para2);
}

int DroidTvServiceIntf::pauseScan() {
    return mpTv->pauseScan();
}

int DroidTvServiceIntf::resumeScan() {
    return mpTv->resumeScan();
}

int DroidTvServiceIntf::dtvStopScan() {
    return mpTv->stopScanLock();
}

int DroidTvServiceIntf::tvSetFrontEnd(const std::string& feparas, int32_t force) {
    return mpTv->setFrontEnd(feparas.c_str(), (force != 0));
}

int DroidTvServiceIntf::dtvGetSignalStrength() {
    return mpTv->getFrontendSignalStrength();
}

int DroidTvServiceIntf::tvSetFrontendParms(int32_t feType, int32_t freq, int32_t vStd, int32_t aStd, int32_t vfmt, int32_t soundsys, int32_t p1, int32_t p2) {
    frontend_para_set_t feParms;
#ifdef SUPPORT_ADTV
    feParms.mode = (fe_type_t)feType;
#endif
    feParms.freq = freq;
    feParms.videoStd = (atv_video_std_t)vStd;
    feParms.audioStd = (atv_audio_std_t)aStd;
    feParms.vfmt = vfmt;
    feParms.soundsys = soundsys;
    feParms.para1 = p1;
    feParms.para2 = p2;
    return mpTv->resetFrontEndPara(feParms);
}
int DroidTvServiceIntf::DtvSetAudioAD(int32_t enable, int32_t audio_pid, int32_t audio_format) {
    return mpTv->setAudioAD(enable,audio_pid,audio_format);
}

int DroidTvServiceIntf::DtvSwitchAudioTrack(int32_t prog_id, int32_t audio_track_id) {
    return mpTv->switchAudioTrack(prog_id,audio_track_id);
}

int DroidTvServiceIntf::DtvSwitchAudioTrack(int audio_pid, int audio_format, int audio_param) {
    return mpTv->switchAudioTrack(audio_pid,audio_format,audio_param);
}

int DroidTvServiceIntf::sendPlayCmd(int32_t cmd, const std::string& id, const std::string& param) {
    return mpTv->doPlayCommand(cmd, id.c_str(), param.c_str());
}

int DroidTvServiceIntf::getCurrentSourceInput() {
    return (int)mpTv->GetCurrentSourceInputLock();
}

int DroidTvServiceIntf::getCurrentVirtualSourceInput() {
    return (int)mpTv->GetCurrentSourceInputVirtualLock();
}

int DroidTvServiceIntf::dtvSetAudioChannleMod(int32_t audioChannelIdx) {
    return mpTv->setAudioChannel(audioChannelIdx);
}

int DroidTvServiceIntf::dtvGetVideoFormatInfo(int &srcWidth, int &srcHeight, int &srcFps, int &srcInterlace) {
    return mpTv->getVideoFormatInfo(&srcWidth, &srcHeight, &srcFps, &srcInterlace);
}

void DroidTvServiceIntf::dtvGetScanFreqListMode(int mode, std::vector<FreqList> &freqlist) {
    Vector<sp<CTvChannel>> out;
    int ret = CTvRegion::getChannelListByName((char *)CTvScanner::getDtvScanListName(mode), out);
    //r->writeInt32(out.size());
    int size = out.size();
    LOGD("DroidTvServiceIntf dtvGetScanFreqListMode size = %d, mode = %d", size, mode);

    std::vector<FreqList> freqlisttmp(size);
    for (int i = 0; i < (int)out.size(); i++) {
        freqlisttmp[i].ID = out[i]->getID();
        freqlisttmp[i].freq = out[i]->getFrequency();
        freqlisttmp[i].channelNum = out[i]->getLogicalChannelNum();
        freqlisttmp[i].physicalNumDisplayName = out[i]->getPhysicalNumDisplayName();
        //r->writeInt32(out[i]->getID());
        //r->writeInt32(out[i]->getFrequency());
        //r->writeInt32(out[i]->getLogicalChannelNum());
    }
    freqlist = freqlisttmp;
}

int DroidTvServiceIntf::atvdtvGetScanStatus() {
    return mpTv->getScanStatus();
}

int DroidTvServiceIntf::SSMInitDevice() {
    return mpTv->Tv_SSMRestoreDefaultSetting();
}

int DroidTvServiceIntf::FactoryCleanAllTableForProgram() {
    int ret = mpTv->ClearAnalogFrontEnd();
    mpTv->clearDbAllProgramInfoTable();
    return ret;
}

std::string DroidTvServiceIntf::getTvSupportCountries() {
    string strValue;
    const char *str = get_tv_support_country_list();
    if (str == NULL) {
        LOGE("get tv support country list fail");
        return strValue;
    }
    strValue = str;
    LOGD("get tv support country: %s", str);
    return strValue;
}

std::string DroidTvServiceIntf::getTvDefaultCountry() {
    string strValue;
    const char *str = get_tv_current_country();
    if (str == NULL) {
        LOGE("get tv default country fail!");
        return strValue;
    }
    strValue = str;
    LOGD("get tv default country: %s", str);
    return strValue;
}

std::string DroidTvServiceIntf::getTvCountryName(const std::string& country_code) {
    string strValue;
    const char *str = get_tv_country_name(country_code.c_str());
    if (str == NULL) {
        LOGE("get tv country name fail!");
        return strValue;
    }
    strValue = str;
    LOGD("get tv country name: %s", str);
    return strValue;
}

std::string DroidTvServiceIntf::getTvSearchMode(const std::string& country_code) {
    string strValue;
    const char *str = get_tv_search_mode(country_code.c_str());
    if (str == NULL) {
        LOGE("get tv search mode fail!");
        return strValue;
    }
    strValue = str;
    LOGD("get tv search mode: %s", str);
    return strValue;
}

bool DroidTvServiceIntf::getTvDtvSupport(const std::string& country_code) {
    bool ret = get_tv_dtv_support(country_code.c_str());
    LOGD("get tv dtv support: %d", ret);
    return ret;
}

std::string DroidTvServiceIntf::getTvDtvSystem(const std::string& country_code) {
    string strValue;
    const char *str = get_tv_dtv_system(country_code.c_str());
    if (str == NULL) {
        LOGE("get tv dtv system fail!");
        return strValue;
    }
    strValue = str;
    LOGD("get tv dtv system: %s", str);
    return strValue;
}

bool DroidTvServiceIntf::getTvAtvSupport(const std::string& country_code) {
    bool ret = get_tv_atv_support(country_code.c_str());
    LOGD("get tv atv support: %d", ret);
    return ret;
}

std::string DroidTvServiceIntf::getTvAtvColorSystem(const std::string& country_code) {
    string strValue;
    const char *str = get_tv_atv_color_system(country_code.c_str());
    if (str == NULL) {
        LOGE("get tv atv color system fail!");
        return strValue;
    }
    strValue = str;
    LOGD("get tv atv color system: %s", str);
    return strValue;
}

std::string DroidTvServiceIntf::getTvAtvSoundSystem(const std::string& country_code) {
    string strValue;
    const char *str = get_tv_atv_sound_system(country_code.c_str());
    if (str == NULL) {
        LOGE("get tv atv sound system fail!");
        return strValue;
    }
    strValue = str;
    LOGD("get tv atv sound system: %s", str);
    return strValue;
}

std::string DroidTvServiceIntf::getTvAtvMinMaxFreq(const std::string& country_code) {
    string strValue;
    const char *str = get_tv_atv_min_max_freq(country_code.c_str());
    if (str == NULL) {
        LOGE("get tv atv min max freq fail!");
        return strValue;
    }
    strValue = str;
    LOGD("get tv atv min max freq: %s", str);
    return strValue;
}

bool DroidTvServiceIntf::getTvAtvStepScan(const std::string& country_code) {
    bool ret = get_tv_atv_step_scan(country_code.c_str());
    LOGD("get tv atv step scan: %d", ret);
    return ret;
}

void DroidTvServiceIntf::setTvCountry(const std::string& country) {
    CTvRegion::setTvCountry(country.c_str());
    LOGD("set tv country = %s", country.c_str());
}

void DroidTvServiceIntf::setCurrentLanguage(const std::string& lang) {
    mpTv->SetCurrentLanguage(lang);
    LOGD("set tv current lang = %s", lang.c_str());
}

int DroidTvServiceIntf::setAudioOutmode(int32_t mode) {
    int ret = mpTv->SetAtvAudioOutmode(mode);
    return ret;
}

int DroidTvServiceIntf::getAudioOutmode() {
    int ret = mpTv->GetAtvAudioOutmode();
    return ret;
}

int DroidTvServiceIntf::getAudioStreamOutmode() {
    int ret = mpTv->GetAtvAudioInputmode();
    return ret;
}

int DroidTvServiceIntf::getAtvAutoScanMode() {
    int ret = mpTv->GetAtvAutoScanMode();
    return ret;
}

int DroidTvServiceIntf::vdinUpdateForPQ(int32_t gameStatus, int32_t pcStatus, int32_t autoSwitchFlag) {
    return mpTv->Tv_SetVdinForPQ(gameStatus, pcStatus, autoSwitchFlag);
}

int DroidTvServiceIntf::setWssStatus (int status) {
    return mpTv->Tv_SetWssStatus(status);
}

int DroidTvServiceIntf::sendRecordingCmd(int32_t cmd, const std::string& id, const std::string& param) {
    LOGD("sendRecordingCmd in tvserver cmd = %d, id = %s", cmd, id.c_str());
    return mpTv->doRecordingCommand(cmd, id.c_str(), param.c_str());
}

rrt_select_info_t DroidTvServiceIntf::searchRrtInfo(int rating_region_id, int dimension_id, int value_id, int program_id) {
    return mpTv->Tv_RrtSearch(rating_region_id, dimension_id, value_id, program_id);
}

int DroidTvServiceIntf::updateRRT(int freq, int moudle, int mode) {
    return mpTv->Tv_RrtUpdate(freq, moudle, mode);
}

int DroidTvServiceIntf::updateEAS(int freq, int moudle, int mode) {
    return mpTv->Tv_Easupdate();
}

int DroidTvServiceIntf::setDeviceIdForCec(int DeviceId) {
    return mpTv->Tv_SetDeviceIdForCec(DeviceId);
}

int DroidTvServiceIntf::getTvRunStatus(void) {
    return mpTv->GetTvStatus();
}

int DroidTvServiceIntf::getTvAction(void) {
    return mpTv->GetTvAction();
}

int DroidTvServiceIntf::setLcdEnable(int enable) {
    return mpTv->setLcdEnable(enable);
}

int DroidTvServiceIntf::readMacAddress(unsigned char *dataBuf) {
    unsigned char tempBuf[CC_MAC_LEN] = {0};
    int ret = KeyData_ReadMacAddress(tempBuf);
    if (ret != 0) {
        for (int i = 0; i < CC_MAC_LEN; i++) {
            dataBuf[i] = tempBuf[i];
        }
        ret = 0;
    } else {
        ret = -1;
    }

    return ret;
}

int DroidTvServiceIntf::saveMacAddress(unsigned char *dataBuf) {
    unsigned char tempBuf[CC_MAC_LEN] = {0};
    for (int i = 0; i < CC_MAC_LEN; i++) {
        tempBuf[i] = dataBuf[i];
    }

    return KeyData_SaveMacAddress(tempBuf);
}

int DroidTvServiceIntf::getIwattRegs() {
    return mpTv->Tv_GetIwattRegs();
}

int DroidTvServiceIntf::setSameSourceEnable(bool isEnable) {
    return mpTv->TV_SetSameSourceEnable(isEnable);
}

int DroidTvServiceIntf::setPreviewWindow(int x1, int y1, int x2, int y2) {
    tvin_window_pos_t win_pos;
    win_pos.x1 = x1;
    win_pos.y1 = y1;
    win_pos.x2 = x2;
    win_pos.y2 = y2;
    return mpTv->SetPreviewWindow(win_pos);
}

int DroidTvServiceIntf::setPreviewWindowMode(bool enable) {
    return mpTv->setPreviewWindowMode(enable);
}

int DroidTvServiceIntf::processCmd(const Parcel &p) {
    unsigned char dataBuf[512] = {0};
    int ret = -1;
    int cmd = p.readInt32();

    LOGD("processCmd cmd=%d", cmd);
    switch (cmd) {
        // Tv function
        case OPEN_TV:
            break;

        case CLOSE_TV:
            ret = mpTv->CloseTv();
            break;

        case START_TV:
            //int mode = p.readInt32();
            ret = mpTv->StartTvLock();
            mIsStartTv = true;
            break;

        case STOP_TV:
            ret = mpTv->StopTvLock();
            mIsStartTv = false;
            break;
        case GET_TV_STATUS:
            ret = (int)mpTv->GetTvStatus();
            break;
        case GET_LAST_SOURCE_INPUT:
            ret = (int)mpTv->GetLastSourceInput();
            break;
        case GET_CURRENT_SOURCE_INPUT:
            ret = (int)mpTv->GetCurrentSourceInputLock();
            break;

        case GET_CURRENT_SOURCE_INPUT_VIRTUAL:
            ret = (int)mpTv->GetCurrentSourceInputVirtualLock();
            break;

        case GET_CURRENT_SIGNAL_INFO:
            /*
            tvin_info_t siginfo = mpTv->GetCurrentSignalInfo();
            int frame_rate = mpTv->getHDMIFrameRate();
            r->writeInt32(siginfo.trans_fmt);
            r->writeInt32(siginfo.fmt);
            r->writeInt32(siginfo.status);
            r->writeInt32(frame_rate);
            */
            break;

        case SET_SOURCE_INPUT: {
            int sourceinput = p.readInt32();
            LOGD(" SetSourceInput sourceId= %x", sourceinput);
            ret = mpTv->SetSourceSwitchInput((tv_source_input_t)sourceinput);
            break;
        }
        case SET_SOURCE_INPUT_EXT: {
            int sourceinput = p.readInt32();
            int vsourceinput = p.readInt32();
            LOGD(" SetSourceInputExt vsourceId= %d sourceId=%d", vsourceinput, sourceinput);
            ret = mpTv->SetSourceSwitchInput((tv_source_input_t)vsourceinput, (tv_source_input_t)sourceinput);
            break;
        }
        case DO_SUSPEND: {
            int type = p.readInt32();
            ret = mpTv->DoSuspend(type);
            break;
        }
        case DO_RESUME: {
            int type = p.readInt32();
            ret = mpTv->DoResume(type);
            break;
        }
        case IS_DVI_SIGNAL:
            ret = mpTv->IsDVISignal();
            break;

        case IS_VGA_TIMEING_IN_HDMI:
            ret = mpTv->isVgaFmtInHdmi();
            break;

        case SET_PREVIEW_WINDOW_MODE:
            ret = mpTv->setPreviewWindowMode(p.readInt32() == 1);
            break;

        case SET_PREVIEW_WINDOW: {
            tvin_window_pos_t win_pos;
            win_pos.x1 = p.readInt32();
            win_pos.y1 = p.readInt32();
            win_pos.x2 = p.readInt32();
            win_pos.y2 = p.readInt32();
            ret = (int)mpTv->SetPreviewWindow(win_pos);
            break;
        }

        case GET_SOURCE_CONNECT_STATUS: {
            int source_input = p.readInt32();
            ret = mpTv->GetSourceConnectStatus((tv_source_input_t)source_input);
            break;
        }

        case GET_SOURCE_INPUT_LIST:
            /*
            const char *value = config_get_str(CFG_SECTION_TV, CGF_DEFAULT_INPUT_IDS, "null");
            r->writeString16(String16(value));
            */
            break;

        //Tv function END

        // HDMI
        case SET_HDMI_EDID_VER: {
            int hdmi_port_id = p.readInt32();
            int edid_ver = p.readInt32();
            ret = mpTv->SetHdmiEdidVersion((tv_hdmi_port_id_t)hdmi_port_id, (tv_hdmi_edid_version_t)edid_ver);
            break;
        }
        case SET_HDCP_KEY_ENABLE: {
            int enable = p.readInt32();
            ret = mpTv->SetHdmiHDCPSwitcher((tv_hdmi_hdcpkey_enable_t)enable);
            break;
        }
        case SET_HDMI_COLOR_RANGE_MODE: {
            int range_mode = p.readInt32();
            ret = mpTv->SetHdmiColorRangeMode((tvin_color_range_t)range_mode);
            break;
        }
        case GET_HDMI_COLOR_RANGE_MODE:
            ret = mpTv->GetHdmiColorRangeMode();
            break;

        // HDMI END

        // AUDIO & AUDIO MUTE
        case SET_AUDIO_MUTE_FOR_TV: {
            int status = p.readInt32();
            break;
        }
        case SET_AUDIO_MUTEKEY_STATUS: {
            int status = p.readInt32();
            break;
        }
        case GET_AUDIO_MUTEKEY_STATUS:
            break;

        case SET_AUDIO_AVOUT_MUTE_STATUS: {
            break;
        }
        case GET_AUDIO_AVOUT_MUTE_STATUS:
            break;

        case SET_AUDIO_SPDIF_MUTE_STATUS: {
            break;
        }
        case GET_AUDIO_SPDIF_MUTE_STATUS:
            break;

        // AUDIO MASTER VOLUME
        case SET_AUDIO_MASTER_VOLUME: {
            break;
        }
        case GET_AUDIO_MASTER_VOLUME:
            break;
        case SAVE_CUR_AUDIO_MASTER_VOLUME: {
            break;
        }
        case GET_CUR_AUDIO_MASTER_VOLUME:
            break;
        //AUDIO BALANCE
        case SET_AUDIO_BALANCE: {
            break;
        }
        case GET_AUDIO_BALANCE:
            break;

        case SAVE_CUR_AUDIO_BALANCE: {
            break;
        }
        case GET_CUR_AUDIO_BALANCE:
            break;

        //AUDIO SUPPERBASS VOLUME
        case SET_AUDIO_SUPPER_BASS_VOLUME: {
            break;
        }
        case GET_AUDIO_SUPPER_BASS_VOLUME:
            break;

        case SAVE_CUR_AUDIO_SUPPER_BASS_VOLUME: {
            break;
        }
        case GET_CUR_AUDIO_SUPPER_BASS_VOLUME:
            break;

        //AUDIO SUPPERBASS SWITCH
        case SET_AUDIO_SUPPER_BASS_SWITCH: {
            break;
        }
        case GET_AUDIO_SUPPER_BASS_SWITCH:
            break;

        case SAVE_CUR_AUDIO_SUPPER_BASS_SWITCH: {
            break;
        }
        case GET_CUR_AUDIO_SUPPER_BASS_SWITCH:
            break;

        //AUDIO SRS SURROUND SWITCH
        case SET_AUDIO_SRS_SURROUND: {
            break;
        }
        case GET_AUDIO_SRS_SURROUND:
            break;

        case SAVE_CUR_AUDIO_SRS_SURROUND: {
            break;
        }
        case GET_CUR_AUDIO_SRS_SURROUND:
            break;

        //AUDIO SRS DIALOG CLARITY
        case SET_AUDIO_SRS_DIALOG_CLARITY: {
            break;
        }
        case GET_AUDIO_SRS_DIALOG_CLARITY: {
            break;
        }
        case SAVE_CUR_AUDIO_SRS_DIALOG_CLARITY: {
            break;
        }
        case GET_CUR_AUDIO_SRS_DIALOG_CLARITY:
            break;

        //AUDIO SRS TRUBASS
        case SET_AUDIO_SRS_TRU_BASS: {
            break;
        }
        case GET_AUDIO_SRS_TRU_BASS:
            break;

        case SAVE_CUR_AUDIO_SRS_TRU_BASS: {
            break;
        }
        case GET_CUR_AUDIO_SRS_TRU_BASS:
            break;

        //AUDIO BASS
        case SET_AUDIO_BASS_VOLUME: {
            break;
        }
        case GET_AUDIO_BASS_VOLUME:
            break;

        case SAVE_CUR_AUDIO_BASS_VOLUME: {
            break;
        }

        case GET_CUR_AUDIO_BASS_VOLUME:
            break;

        //AUDIO TREBLE
        case SET_AUDIO_TREBLE_VOLUME: {
            break;
        }
        case GET_AUDIO_TREBLE_VOLUME:
            break;

        case SAVE_CUR_AUDIO_TREBLE_VOLUME: {
            break;
        }
        case GET_CUR_AUDIO_TREBLE_VOLUME:
            break;

        //AUDIO SOUND MODE
        case SET_AUDIO_SOUND_MODE: {
            break;
        }
        case GET_AUDIO_SOUND_MODE:
            break;

        case SAVE_CUR_AUDIO_SOUND_MODE: {
            break;
        }
        case GET_CUR_AUDIO_SOUND_MODE:
            break;

        //AUDIO WALL EFFECT
        case SET_AUDIO_WALL_EFFECT: {
            break;
        }
        case GET_AUDIO_WALL_EFFECT:
            break;

        case SAVE_CUR_AUDIO_WALL_EFFECT: {
            break;
        }
        case GET_CUR_AUDIO_WALL_EFFECT:
            break;

        //AUDIO EQ MODE
        case SET_AUDIO_EQ_MODE: {
            break;
        }
        case GET_AUDIO_EQ_MODE:
            break;

        case SAVE_CUR_AUDIO_EQ_MODE: {
            break;
        }
        case GET_CUR_AUDIO_EQ_MODE:
            break;

        //AUDIO EQ GAIN
        case GET_AUDIO_EQ_RANGE: {
            break;
        }
        case GET_AUDIO_EQ_BAND_COUNT:
            break;

        case SET_AUDIO_EQ_GAIN: {
            break;
        }
        case GET_AUDIO_EQ_GAIN: {
            break;
        }
        case SAVE_CUR_AUDIO_EQ_GAIN: {
            break;
        }
        case GET_CUR_EQ_GAIN: {
            break;
        }
        case SET_AUDIO_EQ_SWITCH: {
            break;
        }
        // AUDIO SPDIF SWITCH
        case SET_AUDIO_SPDIF_SWITCH: {
            break;
        }
        case SAVE_CUR_AUDIO_SPDIF_SWITCH: {
            break;
        }
        case GET_CUR_AUDIO_SPDIF_SWITCH:
            break;

        //AUDIO SPDIF MODE
        case SET_AUDIO_SPDIF_MODE: {
            break;
        }
        case SAVE_CUR_AUDIO_SPDIF_MODE: {
            break;
        }
        case GET_CUR_AUDIO_SPDIF_MODE:
            break;

        case SET_AMAUDIO_OUTPUT_MODE: {
            break;
        }
        case SET_AMAUDIO_MUSIC_GAIN: {
            break;
        }
        case SET_AMAUDIO_LEFT_GAIN: {
            break;
        }
        case SET_AMAUDIO_RIGHT_GAIN: {
            break;
        }
        case SET_AMAUDIO_PRE_GAIN: {
            break;
        }
        case SET_AMAUDIO_PRE_MUTE: {
            break;
        }
        case GET_AMAUDIO_PRE_MUTE:
            break;

        case SELECT_LINE_IN_CHANNEL: {
            break;
        }
        case SET_LINE_IN_CAPTURE_VOL: {
            int l_vol = p.readInt32();
            int r_vol = p.readInt32();
            break;
        }
        case SET_AUDIO_VOL_COMP: {
            break;
        }
        case GET_AUDIO_VOL_COMP:
            ret = mpTv->GetAudioVolumeCompensationVal(-1);
            break;

        case SET_AUDIO_VIRTUAL: {
            break;
        }
        case GET_AUDIO_VIRTUAL_ENABLE:
            break;

        case GET_AUDIO_VIRTUAL_LEVEL:
            break;
        // AUDIO END

        // SSM
        case SSM_INIT_DEVICE:
            ret = mpTv->Tv_SSMRestoreDefaultSetting();//mpTv->Tv_SSMInitDevice();
            break;

        case SSM_SAVE_POWER_ON_OFF_CHANNEL: {
            int tmpPowerChanNum = p.readInt32();
            ret = SSMSavePowerOnOffChannel(tmpPowerChanNum);
            break;
        }
        case SSM_READ_POWER_ON_OFF_CHANNEL: {
            ret = SSMReadPowerOnOffChannel();
            break;
        }
        case SSM_SAVE_SOURCE_INPUT: {
            int tmpSouceInput = p.readInt32();
            ret = SSMSaveSourceInput(tmpSouceInput);
            break;
        }
        case SSM_READ_SOURCE_INPUT:
            ret = SSMReadSourceInput();
            break;

        case SSM_SAVE_LAST_SOURCE_INPUT: {
            int tmpLastSouceInput = p.readInt32();
            ret = SSMSaveLastSelectSourceInput(tmpLastSouceInput);
            break;
        }
        case SSM_READ_LAST_SOURCE_INPUT:
            ret = SSMReadLastSelectSourceInput();
            break;

        case SSM_SAVE_SYS_LANGUAGE: {
            int tmpVal = p.readInt32();
            ret = SSMSaveSystemLanguage(tmpVal);
            break;
        }
        case SSM_READ_SYS_LANGUAGE: {
            ret = SSMReadSystemLanguage();
            break;
        }
        case SSM_SAVE_AGING_MODE: {
            int tmpVal = p.readInt32();
            ret = SSMSaveAgingMode(tmpVal);
            break;
        }
        case SSM_READ_AGING_MODE:
            ret = SSMReadAgingMode();
            break;

        case SSM_SAVE_PANEL_TYPE: {
            int tmpVal = p.readInt32();
            ret = SSMSavePanelType(tmpVal);
            break;
        }
        case SSM_READ_PANEL_TYPE:
            ret = SSMReadPanelType();
            break;

        case SSM_SAVE_MAC_ADDR: {
            int size = p.readInt32();
            for (int i = 0; i < size; i++) {
                dataBuf[i] = p.readInt32();
            }
            ret = KeyData_SaveMacAddress(dataBuf);
            break;
        }
        case SSM_READ_MAC_ADDR: {
            ret = KeyData_ReadMacAddress(dataBuf);
            int size = KeyData_GetMacAddressDataLen();
            //r->writeInt32(size);
            for (int i = 0; i < size; i++) {
                //r->writeInt32(dataBuf[i]);
            }
            //r->writeInt32(ret);
            break;
        }
        case SSM_SAVE_BAR_CODE: {
            int size = p.readInt32();
            for (int i = 0; i < size; i++) {
                dataBuf[i] = p.readInt32();
            }
            ret = KeyData_SaveBarCode(dataBuf);
            break;
        }
        case SSM_READ_BAR_CODE: {
            ret = KeyData_ReadBarCode(dataBuf);
            int size = KeyData_GetBarCodeDataLen();
            //r->writeInt32(size);
            for (int i = 0; i < size; i++) {
                //r->writeInt32(dataBuf[i]);
            }
            break;
        }
        case SSM_SAVE_HDCPKEY: {
            int size = p.readInt32();
            for (int i = 0; i < size; i++) {
                dataBuf[i] = p.readInt32();
            }
            ret = SSMSaveHDCPKey(dataBuf);
            break;
        }
        case SSM_READ_HDCPKEY: {
            ret = SSMReadHDCPKey(dataBuf);
            int size = SSMGetHDCPKeyDataLen();
            //r->writeInt32(size);
            for (int i = 0; i < size; i++) {
                //r->writeInt32(dataBuf[i]);
            }
            break;
        }
        case SSM_SAVE_POWER_ON_MUSIC_SWITCH: {
            int tmpVal = p.readInt32();
            ret = SSMSavePowerOnMusicSwitch(tmpVal);
            break;
        }
        case SSM_READ_POWER_ON_MUSIC_SWITCH:
            ret = SSMReadPowerOnMusicSwitch();
            break;

        case SSM_SAVE_POWER_ON_MUSIC_VOL: {
            int tmpVal = p.readInt32();
            ret = SSMSavePowerOnMusicVolume(tmpVal);
            break;
        }
        case SSM_READ_POWER_ON_MUSIC_VOL:
            ret = SSMReadPowerOnMusicVolume();
            break;
        case SSM_SAVE_SYS_SLEEP_TIMER: {
            int tmpVal = p.readInt32();
            ret = SSMSaveSystemSleepTimer(tmpVal);
            break;
        }
        case SSM_READ_SYS_SLEEP_TIMER:
            ret = SSMReadSystemSleepTimer();
            break;

        case SSM_SAVE_INPUT_SRC_PARENTAL_CTL: {
            int tmpSourceIndex = p.readInt32();
            int tmpCtlFlag = p.readInt32();
            ret = SSMSaveInputSourceParentalControl(tmpSourceIndex, tmpCtlFlag);
            break;
        }
        case SSM_READ_INPUT_SRC_PARENTAL_CTL: {
            int tmpSourceIndex = p.readInt32();
            ret = SSMReadInputSourceParentalControl(tmpSourceIndex);
            break;
        }
        case SSM_SAVE_PARENTAL_CTL_SWITCH: {
            int tmpSwitchFlag = p.readInt32();
            ret = SSMSaveParentalControlSwitch(tmpSwitchFlag);
            break;
        }
        case SSM_READ_PARENTAL_CTL_SWITCH: {
            ret = SSMReadParentalControlSwitch();
            break;
        }
        case SSM_SAVE_PARENTAL_CTL_PASS_WORD: {
            String16 pass_wd_str = p.readString16();
            ret = SSMSaveParentalControlPassWord((unsigned char *)pass_wd_str.string(), pass_wd_str.size() * sizeof(unsigned short));
            break;
        }
        case SSM_GET_CUSTOMER_DATA_START:
            ret = SSMGetCustomerDataStart();
            break;

        case SSM_GET_CUSTOMER_DATA_LEN:
            ret = SSMGetCustomerDataLen();
            break;

        case SSM_SAVE_STANDBY_MODE: {
            int tmp_val = p.readInt32();
            ret = SSMSaveStandbyMode(tmp_val);
            break;
        }
        case SSM_READ_STANDBY_MODE:
            ret = SSMReadStandbyMode();
            break;

        case SSM_SAVE_LOGO_ON_OFF_FLAG: {
            int tmpSwitchFlag = p.readInt32();
            ret = SSMSaveLogoOnOffFlag(tmpSwitchFlag);
            break;
        }
        case SSM_READ_LOGO_ON_OFF_FLAG:
            ret = SSMReadLogoOnOffFlag();
            break;

        case SSM_SAVE_HDMIEQ_MODE: {
            int tmpSwitchFlag = p.readInt32();
            ret = SSMSaveHDMIEQMode(tmpSwitchFlag);
            break;
        }
        case SSM_READ_HDMIEQ_MODE:
            ret = SSMReadHDMIEQMode();
            break;

        case SSM_SAVE_HDMIINTERNAL_MODE: {
            int tmp_val = p.readInt32();
            ret = SSMSaveHDMIInternalMode(tmp_val);
            break;
        }
        case SSM_READ_HDMIINTERNAL_MODE:
            ret = SSMReadHDMIInternalMode();
            break;

        case SSM_SAVE_GLOBAL_OGOENABLE: {
            int tmp_val = p.readInt32();
            ret = SSMSaveGlobalOgoEnable(tmp_val);
            break;
        }
        case SSM_READ_GLOBAL_OGOENABLE:
            ret = SSMReadGlobalOgoEnable();
            break;

        case SSM_SAVE_NON_STANDARD_STATUS: {
            int tmp_val = p.readInt32();
            ret = SSMSaveNonStandardValue(tmp_val);
            break;
        }
        case SSM_READ_NON_STANDARD_STATUS:
            ret = SSMReadNonStandardValue();
            break;

        case SSM_SAVE_ADB_SWITCH_STATUS: {
            int tmp_val = p.readInt32();
            ret = SSMSaveAdbSwitchValue(tmp_val);
            break;
        }
        case SSM_READ_ADB_SWITCH_STATUS:
            ret = SSMReadAdbSwitchValue();
            break;

        case SSM_SET_HDCP_KEY:
            ret = SSMSetHDCPKey();
            break;

        case SSM_REFRESH_HDCPKEY:
            ret = SSMRefreshHDCPKey();
            break;

        case SSM_SAVE_CHROMA_STATUS: {
            int tmp_val = p.readInt32();
            ret = SSMSaveChromaStatus(tmp_val);
            break;
        }
        case SSM_SAVE_CA_BUFFER_SIZE: {
            int tmp_val = p.readInt32();
            ret = SSMSaveCABufferSizeValue(tmp_val);
            break;
        }
        case SSM_READ_CA_BUFFER_SIZE:
            ret= SSMReadCABufferSizeValue();
            break;

        case SSM_GET_ATV_DATA_START:
            ret = SSMGetATVDataStart();
            break;

        case SSM_GET_ATV_DATA_LEN:
            ret = SSMGetATVDataLen();
            break;

        case SSM_GET_VPP_DATA_START:
            ret = SSMGetVPPDataStart();
            break;

        case SSM_GET_VPP_DATA_LEN:
            ret = SSMGetVPPDataLen();
            break;

        case SSM_SAVE_NOISE_GATE_THRESHOLD_STATUS: {
            int tmp_val = p.readInt32();
            ret = SSMSaveNoiseGateThresholdValue(tmp_val);
            break;
        }
        case SSM_READ_NOISE_GATE_THRESHOLD_STATUS:
            ret = SSMReadNoiseGateThresholdValue();
            break;

        case SSM_SAVE_HDMI_EDID_VER: {
            int port_id = p.readInt32();
            int ver = p.readInt32();
            ret = SSMSaveHDMIEdidVersion((tv_hdmi_port_id_t)port_id, (tv_hdmi_edid_version_t)ver);
            break;
        }
        case SSM_READ_HDMI_EDID_VER: {
            int port_id = p.readInt32();
            ret = SSMReadHDMIEdidVersion((tv_hdmi_port_id_t)port_id);
            break;
        }
        case SSM_SAVE_HDCP_KEY_ENABLE: {
            int enable = p.readInt32();
            ret = SSMSaveHDMIHdcpSwitcher(enable);
            break;
        }
        case SSM_READ_HDCP_KEY_ENABLE:
            ret = SSMReadHDMIHdcpSwitcher();
            break;
        // SSM END

        //MISC
        case MISC_CFG_SET: {
            String8 key(p.readString16());
            String8 value(p.readString16());

            ret = config_set_str(CFG_SECTION_TV, key.string(), value.string());
            break;
        }
        case MISC_CFG_GET: {
            String8 key(p.readString16());
            String8 def(p.readString16());

            const char *value = config_get_str(CFG_SECTION_TV, key.string(), def.string());
            //r->writeString16(String16(value));
            break;
        }
        case MISC_SET_WDT_USER_PET: {
            int counter = p.readInt32();
            ret = TvMisc_SetUserCounter(counter);
            break;
        }
        case MISC_SET_WDT_USER_COUNTER: {
            int counter_time_out = p.readInt32();
            ret = TvMisc_SetUserCounterTimeOut(counter_time_out);
            break;
        }
        case MISC_SET_WDT_USER_PET_RESET_ENABLE: {
            int enable = p.readInt32();
            ret = TvMisc_SetUserPetResetEnable(enable);
            break;
        }
        case MISC_GET_TV_API_VERSION: {
            // write tvapi version info
            const char *str = tvservice_get_git_branch_info();
            //r->writeString16(String16(str));

            str = tvservice_get_git_version_info();
            //r->writeString16(String16(str));

            str = tvservice_get_last_chaned_time_info();
            //r->writeString16(String16(str));

            str = tvservice_get_build_time_info();
            //r->writeString16(String16(str));

            str = tvservice_get_build_name_info();
            //r->writeString16(String16(str));
            break;
        }
        case MISC_GET_DVB_API_VERSION: {
            // write dvb version info
            //const char *str = dvb_get_git_branch_info();
            //r->writeString16(String16(str));

            //str = dvb_get_git_version_info();
            //r->writeString16(String16(str));

            //str = dvb_get_last_chaned_time_info();
            //r->writeString16(String16(str));

            //str = dvb_get_build_time_info();
            //r->writeString16(String16(str));

            //str = dvb_get_build_name_info();
            //r->writeString16(String16(str));
            break;
        }
        //MISC  END

        // EXTAR
        case ATV_GET_CURRENT_PROGRAM_ID:
            ret = mpTv->getATVProgramID();
            break;

        case DTV_GET_CURRENT_PROGRAM_ID:
            ret = mpTv->getDTVProgramID();
            break;

        case ATV_SAVE_PROGRAM_ID: {
            int progID = p.readInt32();
            mpTv->saveATVProgramID(progID);
            break;
        }
        case SAVE_PROGRAM_ID: {
            int type = p.readInt32();
            int progID = p.readInt32();
            if (type == CTvProgram::TYPE_DTV)
                mpTv->saveDTVProgramID(progID);
            else if (type == CTvProgram::TYPE_RADIO)
                mpTv->saveRadioProgramID(progID);
            else
                mpTv->saveATVProgramID(progID);
            break;
        }
        case GET_PROGRAM_ID: {
            int type = p.readInt32();
            int id;
            if (type == CTvProgram::TYPE_DTV)
                id = mpTv->getDTVProgramID();
            else if (type == CTvProgram::TYPE_RADIO)
                id = mpTv->getRadioProgramID();
            else
                id = mpTv->getATVProgramID();
            ret = id;
            break;
        }

        case ATV_GET_MIN_MAX_FREQ: {
            int min, max;
            int tmpRet = mpTv->getATVMinMaxFreq(&min, &max);
            //r->writeInt32(min);
            //r->writeInt32(max);
            //r->writeInt32(tmpRet);
            break;
        }
        case DTV_GET_SCAN_FREQUENCY_LIST: {
            Vector<sp<CTvChannel> > out;
            ret = CTvRegion::getChannelListByName((char *)"CHINA,Default DTMB ALL", out);
            //r->writeInt32(out.size());
            for (int i = 0; i < (int)out.size(); i++) {
                //r->writeInt32(out[i]->getID());
                //r->writeInt32(out[i]->getFrequency());
                //r->writeInt32(out[i]->getLogicalChannelNum());
            }
            break;
        }
        case DTV_GET_SCAN_FREQUENCY_LIST_MODE: {
            int mode = p.readInt32();
            Vector<sp<CTvChannel> > out;
            ret = CTvRegion::getChannelListByName((char *)CTvScanner::getDtvScanListName(mode), out);
            //r->writeInt32(out.size());
            for (int i = 0; i < (int)out.size(); i++) {
                //r->writeInt32(out[i]->getID());
                //r->writeInt32(out[i]->getFrequency());
                //r->writeInt32(out[i]->getLogicalChannelNum());
            }
            break;
        }
        case DTV_GET_CHANNEL_INFO: {
            int dbID = p.readInt32();
            channel_info_t chan_info;
            ret = mpTv->getChannelInfoBydbID(dbID, chan_info);
            //r->writeInt32(chan_info.freq);
            //r->writeInt32(chan_info.uInfo.dtvChanInfo.strength);
            //r->writeInt32(chan_info.uInfo.dtvChanInfo.quality);
            //r->writeInt32(chan_info.uInfo.dtvChanInfo.ber);
            break;
        }
        case ATV_GET_CHANNEL_INFO: {
            int dbID = p.readInt32();
            channel_info_t chan_info;
            ret = mpTv->getChannelInfoBydbID(dbID, chan_info);
            //r->writeInt32(chan_info.freq);
            //r->writeInt32(chan_info.uInfo.atvChanInfo.finefreq);
            //r->writeInt32(chan_info.uInfo.atvChanInfo.videoStd);
            //r->writeInt32(chan_info.uInfo.atvChanInfo.audioStd);
            //r->writeInt32(chan_info.uInfo.atvChanInfo.isAutoStd);
            break;
        }
        case ATV_SCAN_MANUAL: {
            int startFreq = p.readInt32();
            int endFreq = p.readInt32();
            int videoStd = p.readInt32();
            int audioStd = p.readInt32();
            ret = mpTv->atvMunualScan(startFreq, endFreq, videoStd, audioStd);
            //mTvService->mpScannerClient = this;
            break;
        }

        case ATV_SCAN_AUTO: {
            int videoStd = p.readInt32();
            int audioStd = p.readInt32();
            int searchType = p.readInt32();
            int procMode = p.readInt32();
            ret = mpTv->atvAutoScan(videoStd, audioStd, searchType, procMode);
            //mTvService->mpScannerClient = this;
            break;
        }
        case DTV_SCAN_MANUAL: {
            int freq = p.readInt32();
            ret = mpTv->dtvManualScan(freq, freq);
            //mTvService->mpScannerClient = this;
            break;
        }
        case DTV_SCAN_MANUAL_BETWEEN_FREQ: {
            int beginFreq = p.readInt32();
            int endFreq = p.readInt32();
            int modulation = p.readInt32();
            ret = mpTv->dtvManualScan(beginFreq, endFreq, modulation);
            //mTvService->mpScannerClient = this;
            break;
        }
        case DTV_SCAN_AUTO: {
            ret = mpTv->dtvAutoScan();
            //mTvService->mpScannerClient = this;
            break;
        }
        case DTV_SCAN: {
            int mode = p.readInt32();
            int scanmode = p.readInt32();
            int freq = p.readInt32();
            int para1 = p.readInt32();
            int para2 = p.readInt32();
            ret = mpTv->dtvScan(mode, scanmode, freq, freq, para1, para2);
            //mTvService->mpScannerClient = this;
            break;
        }
        case STOP_PROGRAM_PLAY:
            ret = mpTv->stopPlayingLock();
            break;

        case ATV_DTV_SCAN_PAUSE:
            ret = mpTv->pauseScan();
            break;

        case ATV_DTV_SCAN_RESUME:
            ret = mpTv->resumeScan();
            break;

        case ATV_DTV_GET_SCAN_STATUS:
            ret = mpTv->getScanStatus();
            break;

        case ATV_DTV_SCAN_OPERATE_DEVICE : {
            int type = p.readInt32();
            mpTv->operateDeviceForScan(type);
            break;
        }
        case DTV_SET_TEXT_CODING: {
            String8 coding(p.readString16());
            mpTv->setDvbTextCoding((char *)coding.string());
            break;
        }

        case TV_CLEAR_ALL_PROGRAM: {
            int arg0 = p.readInt32();

            ret = mpTv->clearAllProgram(arg0);
            //mTvService->mpScannerClient = this;
            break;
        }

        case HDMIRX_GET_KSV_INFO: {
            int data[2] = {0, 0};
            ret = mpTv->GetHdmiHdcpKeyKsvInfo(data);
            //r->writeInt32(data[0]);
            //r->writeInt32(data[1]);
            break;
        }

        case STOP_SCAN:
            mpTv->stopScanLock();
            break;

        case DTV_GET_SNR:
            ret = mpTv->getFrontendSNR();
            break;

        case DTV_GET_BER:
            ret = mpTv->getFrontendBER();
            break;

        case DTV_GET_STRENGTH:
            ret = mpTv->getFrontendSignalStrength();
            break;

        case DTV_GET_AUDIO_TRACK_NUM: {
            int programId = p.readInt32();
            ret = mpTv->getAudioTrackNum(programId);
            break;
        }
        case DTV_GET_AUDIO_TRACK_INFO: {
            int progId = p.readInt32();
            int aIdx = p.readInt32();
            int aFmt = -1;
            String8 lang;
            ret = mpTv->getAudioInfoByIndex(progId, aIdx, &aFmt, lang);
            //r->writeInt32(aFmt);
            //r->writeString16(String16(lang));
            break;
        }
        case DTV_SWITCH_AUDIO_TRACK: {
            int aPid = p.readInt32();
            int aFmt = p.readInt32();
            int aParam = p.readInt32();
            ret = mpTv->switchAudioTrack(aPid, aFmt, aParam);
            break;
        }
        case DTV_SET_AUDIO_AD: {
            int aEnable = p.readInt32();
            int aPid = p.readInt32();
            int aFmt = p.readInt32();
            ret = mpTv->setAudioAD(aEnable, aPid, aFmt);
            break;
        }
        case DTV_GET_CURR_AUDIO_TRACK_INDEX: {
            int currAduIdx = -1;
            int progId = p.readInt32();
            CTvProgram prog;
            CTvProgram::selectByID(progId, prog);
            currAduIdx = prog.getCurrAudioTrackIndex();
            //r->writeInt32(currAduIdx);
            break;
        }
        case DTV_SET_AUDIO_CHANNEL_MOD: {
            int audioChannelIdx = p.readInt32();
            mpTv->setAudioChannel(audioChannelIdx);
            break;
        }
        case DTV_GET_AUDIO_CHANNEL_MOD: {
            int currChannelMod = mpTv->getAudioChannel();
            //r->writeInt32(currChannelMod);
            break;
        }
        case DTV_GET_CUR_FREQ: {
            int progId = p.readInt32();
            CTvProgram prog;
            CTvChannel channel;

            int iRet = CTvProgram::selectByID(progId, prog);
            if (0 != iRet) return -1;
            prog.getChannel(channel);
            int freq = channel.getFrequency();
            //r->writeInt32(freq);
            break;
        }
        case DTV_GET_EPG_UTC_TIME: {
            int utcTime = mpTv->getTvTime();
            //r->writeInt32(utcTime);
            break;
        }
        case DTV_GET_EPG_INFO_POINT_IN_TIME: {
            int progid = p.readInt32();
            int utcTime = p.readInt32();
            CTvProgram prog;
            int ret = CTvProgram::selectByID(progid, prog);
            CTvEvent ev;
            ret = ev.getProgPresentEvent(prog.getSrc(), prog.getID(), utcTime, ev);
            //r->writeString16(String16(ev.getName()));
            //r->writeString16(String16(ev.getDescription()));
            //r->writeString16(String16(ev.getExtDescription()));
            //r->writeInt32(ev.getStartTime());
            //r->writeInt32(ev.getEndTime());
            //r->writeInt32(ev.getSubFlag());
            //r->writeInt32(ev.getEventId());
            break;
        }
        case DTV_GET_EPG_INFO_DURATION: {
            Vector<sp<CTvEvent> > epgOut;
            int progid = p.readInt32();
            int iUtcStartTime = p.readInt32();
            int iDurationTime = p.readInt32();
            CTvProgram prog;
            CTvEvent ev;
            int iRet = CTvProgram::selectByID(progid, prog);
            if (0 != iRet) {
                break;
            }
            iRet = ev.getProgScheduleEvents(prog.getSrc(), prog.getID(), iUtcStartTime, iDurationTime, epgOut);
            if (0 != iRet) {
                break;
            }
            int iObOutSize = epgOut.size();
            if (0 == iObOutSize) {
                break;
            }

            //r->writeInt32(iObOutSize);
            for (int i = 0; i < iObOutSize; i ++) {
                //r->writeString16(String16(epgOut[i]->getName()));
                //r->writeString16(String16(epgOut[i]->getDescription()));
                //r->writeString16(String16(ev.getExtDescription()));
                //r->writeInt32(epgOut[i]->getStartTime());
                //r->writeInt32(epgOut[i]->getEndTime());
                //r->writeInt32(epgOut[i]->getSubFlag());
                //r->writeInt32(epgOut[i]->getEventId());
            }
            break;
        }
        case DTV_SET_PROGRAM_NAME: {
            CTvProgram prog;
            int progid = p.readInt32();
            String16 tmpName = p.readString16();
            String8 strName = String8(tmpName);
            prog.updateProgramName(progid, strName);
            break;
        }
        case DTV_SET_PROGRAM_SKIPPED: {
            CTvProgram prog;
            int progid = p.readInt32();
            bool bSkipFlag = p.readInt32();
            prog.setSkipFlag(progid, bSkipFlag);
            break;
        }
        case DTV_SET_PROGRAM_FAVORITE: {
            CTvProgram prog;
            int progid = p.readInt32();
            bool bFavorite = p.readInt32();
            prog.setFavoriteFlag(progid, bFavorite);
            break;
        }
        case DTV_DETELE_PROGRAM: {
            CTvProgram prog;
            int progid = p.readInt32();
            prog.deleteProgram(progid);
            break;
        }
        case GET_AVERAGE_LUMA:
            ret = mpTv->getAverageLuma();
            break;

        case GET_AUTO_BACKLIGHT_DATA: {
            int buf[128] = {0};
            int size = mpTv->getAutoBacklightData(buf);
            //r->writeInt32(size);
            for (int i = 0; i < size; i++) {
                //r->writeInt32(buf[i]);
            }
            break;
        }
        case SET_AUTO_BACKLIGHT_DATA:
            ret = mpTv->setAutobacklightData(String8(p.readString16()));
            break;

        case DTV_SWAP_PROGRAM: {
            CTvProgram prog;
            int firstProgId = p.readInt32();
            int secondProgId = p.readInt32();
            CTvProgram::selectByID(firstProgId, prog);
            int firstChanOrderNum = prog.getChanOrderNum();
            CTvProgram::selectByID(secondProgId, prog);
            int secondChanOrderNum = prog.getChanOrderNum();
            prog.swapChanOrder(firstProgId, firstChanOrderNum, secondProgId, secondChanOrderNum);
            break;
        }
        case DTV_SET_PROGRAM_LOCKED: {
            CTvProgram prog;
            int progid = p.readInt32();
            bool bLocked = p.readInt32();
            prog.setLockFlag(progid, bLocked);
            break;
        }
        case DTV_GET_FREQ_BY_PROG_ID: {
            int freq = 0;
            int progid = p.readInt32();
            CTvProgram prog;
            ret = CTvProgram::selectByID(progid, prog);
            if (ret != 0) return -1;
            CTvChannel channel;
            prog.getChannel(channel);
            freq = channel.getFrequency();
            break;
        }
        case DTV_GET_BOOKED_EVENT: {
            CTvBooking tvBook;
            Vector<sp<CTvBooking> > vTvBookOut;
            tvBook.getBookedEventList(vTvBookOut);
            int iObOutSize = vTvBookOut.size();
            if (0 == iObOutSize) {
                break;
            }
            //r->writeInt32(iObOutSize);
            for (int i = 0; i < iObOutSize; i ++) {
                //r->writeString16(String16(vTvBookOut[i]->getProgName()));
                //r->writeString16(String16(vTvBookOut[i]->getEvtName()));
                //r->writeInt32(vTvBookOut[i]->getStartTime());
                //r->writeInt32(vTvBookOut[i]->getDurationTime());
                //r->writeInt32(vTvBookOut[i]->getBookId());
                //r->writeInt32(vTvBookOut[i]->getProgramId());
                //r->writeInt32(vTvBookOut[i]->getEventId());
            }
            break;
        }
        case SET_FRONTEND_PARA: {
            frontend_para_set_t feParms;
            //feParms.mode = (fe_type_t)p.readInt32();
            feParms.freq = p.readInt32();
            feParms.videoStd = (atv_video_std_t)p.readInt32();
            feParms.audioStd = (atv_audio_std_t)p.readInt32();
            feParms.vfmt = p.readInt32();
            feParms.soundsys = p.readInt32();
            feParms.para1 = p.readInt32();
            feParms.para2 = p.readInt32();
            mpTv->resetFrontEndPara(feParms);
            break;
        }
        case PLAY_PROGRAM: {
            int mode = p.readInt32();
            int freq = p.readInt32();
            if (mode == TV_FE_ANALOG) {
                int videoStd = p.readInt32();
                int audioStd = p.readInt32();
                int vfmt = p.readInt32();
                int soundsys = p.readInt32();
                int fineTune = p.readInt32();
                int audioCompetation = p.readInt32();
                mpTv->playAtvProgram(freq, videoStd, audioStd, vfmt, soundsys, fineTune, audioCompetation);
            } else {
                int para1 = p.readInt32();
                int para2 = p.readInt32();
                int vid = p.readInt32();
                int vfmt = p.readInt32();
                int aid = p.readInt32();
                int afmt = p.readInt32();
                int pcr = p.readInt32();
                int audioCompetation = p.readInt32();
                mpTv->playDtvProgram(mode, freq, para1, para2, vid, vfmt, aid, afmt, pcr, audioCompetation);
            }
            break;
        }
        case GET_PROGRAM_LIST: {
            Vector<sp<CTvProgram> > out;
            int type = p.readInt32();
            int skip = p.readInt32();
            CTvProgram::selectByType(type, skip, out);
            /*
            r->writeInt32(out.size());
            for (int i = 0; i < (int)out.size(); i++) {
                r->writeInt32(out[i]->getID());
                r->writeInt32(out[i]->getChanOrderNum());
                r->writeInt32(out[i]->getMajor());
                r->writeInt32(out[i]->getMinor());
                r->writeInt32(out[i]->getProgType());
                r->writeString16(String16(out[i]->getName()));
                r->writeInt32(out[i]->getProgSkipFlag());
                r->writeInt32(out[i]->getFavoriteFlag());
                r->writeInt32(out[i]->getVideo()->getFormat());
                CTvChannel ch;
                out[i]->getChannel(ch);
                r->writeInt32(ch.getDVBTSID());
                r->writeInt32(out[i]->getServiceId());
                r->writeInt32(out[i]->getVideo()->getPID());
                r->writeInt32(out[i]->getVideo()->getPID());

                int audioTrackSize = out[i]->getAudioTrackSize();
                r->writeInt32(audioTrackSize);
                for (int j = 0; j < audioTrackSize; j++) {
                    r->writeString16(String16(out[i]->getAudio(j)->getLang()));
                    r->writeInt32(out[i]->getAudio(j)->getFormat());
                    r->writeInt32(out[i]->getAudio(j)->getPID());
                }
                Vector<CTvProgram::Subtitle *> mvSubtitles = out[i]->getSubtitles();
                int subTitleSize = mvSubtitles.size();
                r->writeInt32(subTitleSize);
                if (subTitleSize > 0) {
                    for (int k = 0; k < subTitleSize; k++) {
                        r->writeInt32(mvSubtitles[k]->getPID());
                        r->writeString16(String16(mvSubtitles[k]->getLang()));
                        r->writeInt32(mvSubtitles[k]->getCompositionPageID());
                        r->writeInt32(mvSubtitles[k]->getAncillaryPageID());
                    }
                }
                r->writeInt32(ch.getFrequency());
            }
            */
            break;
        }
        case DTV_GET_VIDEO_FMT_INFO: {
            int srcWidth = 0;
            int srcHeight = 0;
            int srcFps = 0;
            int srcInterlace = 0;
            int iRet = -1;

    /*
            iRet == mpTv->getVideoFormatInfo(&srcWidth, &srcHeight, &srcFps, &srcInterlace);
            r->writeInt32(srcWidth);
            r->writeInt32(srcHeight);
            r->writeInt32(srcFps);
            r->writeInt32(srcInterlace);
            r->writeInt32(iRet);
            */
        }
        break;

        case DTV_GET_AUDIO_FMT_INFO: {
            int iRet = -1;
            int fmt[2];
            int sample_rate[2];
            int resolution[2];
            int channels[2];
            int lpepresent[2];
            int frames;
            int ab_size;
            int ab_data;
            int ab_free;
            iRet == mpTv->getAudioFormatInfo(fmt, sample_rate, resolution, channels,
                lpepresent, &frames, &ab_size, &ab_data, &ab_free);
            /*
            r->writeInt32(fmt[0]);
            r->writeInt32(fmt[1]);
            r->writeInt32(sample_rate[0]);
            r->writeInt32(sample_rate[1]);
            r->writeInt32(resolution[0]);
            r->writeInt32(resolution[1]);
            r->writeInt32(channels[0]);
            r->writeInt32(channels[1]);
            r->writeInt32(lpepresent[0]);
            r->writeInt32(lpepresent[1]);
            r->writeInt32(frames);
            r->writeInt32(ab_size);
            r->writeInt32(ab_data);
            r->writeInt32(ab_free);
            r->writeInt32(iRet);
            */
        }
        break;

        case HDMIAV_HOTPLUGDETECT_ONOFF:
            ret = mpTv->GetHdmiAvHotplugDetectOnoff();
            break;

        case HANDLE_GPIO: {
            String8 strName(p.readString16());
            int is_out = p.readInt32();
            int edge = p.readInt32();
            ret = mpTv->handleGPIO((char *)strName.string(), is_out, edge);
            break;
        }
        case SET_LCD_ENABLE: {
            int enable = p.readInt32();
            ret = mpTv->setLcdEnable(enable);
            break;
        }
        case GET_ALL_TV_DEVICES:
            //const char *value = config_get_str(CFG_SECTION_TV, CGF_DEFAULT_INPUT_IDS, "null");
            //r->writeCString(value);
            break;

        case GET_HDMI_PORTS: {
            int source = p.readInt32();
            int value = mpTv->Tv_GetHdmiPortBySourceInput((tv_source_input_t)source);
            ret = (value & 0x0f) + 1;
            break;
        }
        case TV_CLEAR_FRONTEND: {
            int para = p.readInt32();
            ret = mpTv->clearFrontEnd(para);
            break;
        }
        case TV_SET_FRONTEND: {
            int force = p.readInt32();
            String8 feparas(p.readString16());
            ret = mpTv->setFrontEnd(feparas, (force != 0));
            break;
        }
        case PLAY_PROGRAM_2: {
            String8 feparas(p.readString16());
            int vid = p.readInt32();
            int vfmt = p.readInt32();
            int aid = p.readInt32();
            int afmt = p.readInt32();
            int pcr = p.readInt32();
            int audioCompetation = p.readInt32();
            mpTv->playDtvProgram(feparas.string(), vid, vfmt, aid, afmt, pcr, audioCompetation);
            break;
        }
        case TV_SCAN_2: {
            String8 feparas(p.readString16());
            String8 scanparas(p.readString16());
            ret = mpTv->Scan(feparas.string(), scanparas.string());
            //mTvService->mpScannerClient = this;
            break;
        }
        case SET_AUDIO_OUTMODE: {
            int mode = p.readInt32();
            ret = mpTv->SetAtvAudioOutmode(mode);
            break;
        }

        case GET_AUDIO_OUTMODE:
            ret = mpTv->GetAtvAudioOutmode();
            break;

        case GET_AUDIO_STREAM_OUTMODE:
            ret = mpTv->GetAtvAudioInputmode();
            break;

        case SET_AMAUDIO_VOLUME: {
            break;
        }
        case GET_AMAUDIO_VOLUME:
            break;

        case SAVE_AMAUDIO_VOLUME: {
            break;
        }
        case GET_SAVE_AMAUDIO_VOLUME: {
            break;
        }
        // EXTAR END

        //NEWPLAY/RECORDING
        case DTV_RECORDING_CMD:
            ret = mpTv->doRecordingCommand(p.readInt32(), String8(p.readString16()).string(), String8(p.readString16()).string());
            break;

        case DTV_PLAY_CMD:
            ret = mpTv->doPlayCommand(p.readInt32(), String8(p.readString16()).string(), String8(p.readString16()).string());
            break;

        default:
            break;
    }

    return ret;
}

void DroidTvServiceIntf::setListener(const sp<TvServiceNotify>& listener) {
    mNotifyListener = listener;
}



status_t DroidTvServiceIntf::dump(int fd, const Vector<String16>& args)
{
#if 0
    String8 result;
    if (!checkCallingPermission(String16("android.permission.DUMP"))) {
        char buffer[256];
        snprintf(buffer, 256, "Permission Denial: "
                "can't dump system_control from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        AutoMutex _l(mServiceLock);
        if (args.size() > 0) {
            for (int i = 0; i < (int)args.size(); i ++) {
                if (args[i] == String16("-s")) {
                    String8 section(args[i+1]);
                    String8 key(args[i+2]);
                    const char *value = config_get_str(section, key, "");
                    result.appendFormat("section:[%s] key:[%s] value:[%s]\n", section.string(), key.string(), value);
                }
                else if (args[i] == String16("-h")) {
                    result.append(
                        "\ntv service use to control the tv logical \n\n"
                        "usage: dumpsys tvservice [-s <SECTION> <KEY>][-m][-h] \n"
                        "-s: get config string \n"
                        "   SECTION:[TV|ATV|SourceInputMap|SETTING|FBCUART]\n"
                        "-m: track native heap memory\n"
                        "-h: help \n\n");
                }
                else if (args[i] == String16("-m")) {
                    dumpMemoryAddresses(fd);
                }
            }
        }
        else {
            /*
            result.appendFormat("client num = %d\n", mUsers);
            for (int i = 0; i < (int)mClients.size(); i++) {
                wp<Client> client = mClients[i];
                if (client != 0) {
                    sp<Client> c = client.promote();
                    if (c != 0) {
                        result.appendFormat("client[%d] pid = %d\n", i, c->getPid());
                    }
                }
            }
            */

            mpTv->dump(result);
        }
    }
    write(fd, result.string(), result.size());
#endif
    return NO_ERROR;
}

