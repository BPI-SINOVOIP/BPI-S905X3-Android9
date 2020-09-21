/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "TvService"

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>
#include <utils/Errors.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <ITvService.h>
#include <hardware/hardware.h>
#include "TvService.h"
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <CTvLog.h>
#include <tvconfig.h>
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

static int getCallingPid()
{
    return IPCThreadState::self()->getCallingPid();
}

void TvService::instantiate()
{
    android::status_t ret = defaultServiceManager()->addService(String16("tvservice"), new TvService());
    if (ret != android::OK) {
        LOGE("Couldn't register tv service!");
    }
    LOGD("instantiate add tv service result:%d", ret);
}

TvService::TvService() :BnTvService()
{
    mpScannerClient = NULL;
    mUsers = 0;
    mpTv = new CTv();
    mpTv->setTvObserver(this);
    mCapVidFrame.setObserver(this);
    mpTv->OpenTv();
}

TvService::~TvService()
{
    mpScannerClient = NULL;
    for (int i = 0; i < (int)mClients.size(); i++) {
        wp<Client> client = mClients[i];
        if (client != 0) {
            LOGW("some client still connect it!");
        }
    }

    if (mpTv != NULL) {
        delete mpTv;
        mpTv = NULL;
    }
}

void TvService::onTvEvent(const CTvEv &ev)
{
    int type = ev.getEvType();
    LOGD("TvService::onTvEvent ev type = %d", type);
    switch (type) {
    case CTvEv::TV_EVENT_COMMOM:
        break;

    case CTvEv::TV_EVENT_SCANNER: {
        CTvScanner::ScannerEvent *pScannerEv = (CTvScanner::ScannerEvent *) (&ev);
        if (mpScannerClient != NULL) {
            sp<Client> ScannerClient = mpScannerClient.promote();
            if (ScannerClient != 0) {
                Parcel p;
                LOGD("scanner evt type:%d freq:%d vid:%d acnt:%d scnt:%d",
                     pScannerEv->mType, pScannerEv->mFrequency, pScannerEv->mVid, pScannerEv->mAcnt, pScannerEv->mScnt);
                p.writeInt32(pScannerEv->mType);
                p.writeInt32(pScannerEv->mPercent);
                p.writeInt32(pScannerEv->mTotalChannelCount);
                p.writeInt32(pScannerEv->mLockedStatus);
                p.writeInt32(pScannerEv->mChannelIndex);
                p.writeInt32(pScannerEv->mFrequency);
                p.writeString16(String16(pScannerEv->mProgramName));
                p.writeInt32(pScannerEv->mprogramType);
                p.writeString16(String16(pScannerEv->mParas));
                p.writeInt32(pScannerEv->mStrength);
                p.writeInt32(pScannerEv->mSnr);
                //ATV
                p.writeInt32(pScannerEv->mVideoStd);
                p.writeInt32(pScannerEv->mAudioStd);
                p.writeInt32(pScannerEv->mIsAutoStd);
                //DTV
                p.writeInt32(pScannerEv->mMode);
                p.writeInt32(pScannerEv->mSymbolRate);
                p.writeInt32(pScannerEv->mModulation);
                p.writeInt32(pScannerEv->mBandwidth);
                p.writeInt32(pScannerEv->mReserved);
                p.writeInt32(pScannerEv->mTsId);
                p.writeInt32(pScannerEv->mONetId);
                p.writeInt32(pScannerEv->mServiceId);
                p.writeInt32(pScannerEv->mVid);
                p.writeInt32(pScannerEv->mVfmt);
                p.writeInt32(pScannerEv->mAcnt);
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeInt32(pScannerEv->mAid[i]);
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeInt32(pScannerEv->mAfmt[i]);
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeString16(String16(pScannerEv->mAlang[i]));
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeInt32(pScannerEv->mAtype[i]);
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeInt32(pScannerEv->mAExt[i]);
                p.writeInt32(pScannerEv->mPcr);
                p.writeInt32(pScannerEv->mScnt);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mStype[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mSid[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mSstype[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mSid1[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mSid2[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeString16(String16(pScannerEv->mSlang[i]));
                p.writeInt32(pScannerEv->mFree_ca);
                p.writeInt32(pScannerEv->mScrambled);
                p.writeInt32(pScannerEv->mScanMode);
                p.writeInt32(pScannerEv->mSdtVer);
                p.writeInt32(pScannerEv->mSortMode);

                p.writeInt32(pScannerEv->mLcnInfo.net_id);
                p.writeInt32(pScannerEv->mLcnInfo.ts_id);
                p.writeInt32(pScannerEv->mLcnInfo.service_id);
                for (int i=0; i<MAX_LCN; i++) {
                    p.writeInt32(pScannerEv->mLcnInfo.visible[i]);
                    p.writeInt32(pScannerEv->mLcnInfo.lcn[i]);
                    p.writeInt32(pScannerEv->mLcnInfo.valid[i]);
                }
                p.writeInt32(pScannerEv->mMajorChannelNumber);
                p.writeInt32(pScannerEv->mMinorChannelNumber);
                p.writeInt32(pScannerEv->mSourceId);
                p.writeInt32(pScannerEv->mAccessControlled);
                p.writeInt32(pScannerEv->mHidden);
                p.writeInt32(pScannerEv->mHideGuide);
                p.writeString16(String16(pScannerEv->mVct));
                p.writeInt32(pScannerEv->mProgramsInPat);
                p.writeInt32(pScannerEv->mPatTsId);

                ScannerClient->notifyCallback(SCAN_EVENT_CALLBACK, p);
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_EPG: {
        CTvEpg::EpgEvent *pEpgEvent = (CTvEpg::EpgEvent *) (&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEpgEvent->type);
                    p.writeInt32(pEpgEvent->time);
                    p.writeInt32(pEpgEvent->programID);
                    p.writeInt32(pEpgEvent->channelID);
                    c->getTvClient()->notifyCallback(EPG_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_HDMI_IN_CAP: {
        CTvScreenCapture::CapEvent *pCapEvt = (CTvScreenCapture::CapEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pCapEvt->mFrameNum);
                    p.writeInt32(pCapEvt->mFrameSize);
                    p.writeInt32(pCapEvt->mFrameWide);
                    p.writeInt32(pCapEvt->mFrameHeight);
                    c->getTvClient()->notifyCallback(VFRAME_BMP_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_AV_PLAYBACK: {
        TvEvent::AVPlaybackEvent *pEv = (TvEvent::AVPlaybackEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c!= 0) {
                    Parcel p;
                    p.writeInt32(pEv->mMsgType);
                    p.writeInt32(pEv->mProgramId);
                    c->getTvClient()->notifyCallback(DTV_AV_PLAYBACK_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SIGLE_DETECT: {
        TvEvent::SignalInfoEvent *pEv = (TvEvent::SignalInfoEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mTrans_fmt);
                    p.writeInt32(pEv->mFmt);
                    p.writeInt32(pEv->mStatus);
                    p.writeInt32(pEv->mReserved);
                    c->getTvClient()->notifyCallback(SIGLE_DETECT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SUBTITLE: {
        TvEvent::SubtitleEvent *pEv = (TvEvent::SubtitleEvent *)(&ev);
        sp<Client> c = mpSubClient.promote();
        if (c != NULL) {
            Parcel p;
            p.writeInt32(pEv->pic_width);
            p.writeInt32(pEv->pic_height);
            c->notifyCallback(SUBTITLE_UPDATE_CALLBACK, p);
        }
        break;
    }

    case CTvEv::TV_EVENT_ADC_CALIBRATION: {
        TvEvent::ADCCalibrationEvent *pEv = (TvEvent::ADCCalibrationEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mState);
                    c->getTvClient()->notifyCallback(ADC_CALIBRATION_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_VGA: {//VGA
        TvEvent::VGAEvent *pEv = (TvEvent::VGAEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mState);
                    c->getTvClient()->notifyCallback(VGA_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SOURCE_CONNECT: {
        TvEvent::SourceConnectEvent *pEv = (TvEvent::SourceConnectEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mSourceInput);
                    p.writeInt32(pEv->connectionState);
                    c->getTvClient()->notifyCallback(SOURCE_CONNECT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_HDMIRX_CEC: {
        TvEvent::HDMIRxCECEvent *pEv = (TvEvent::HDMIRxCECEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mDataCount);
                    for (int j = 0; j < pEv->mDataCount; j++) {
                        p.writeInt32(pEv->mDataBuf[j]);
                    }
                    c->getTvClient()->notifyCallback(HDMIRX_CEC_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_UPGRADE_FBC: {
        TvEvent::UpgradeFBCEvent *pEv = (TvEvent::UpgradeFBCEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mState);
                    p.writeInt32(pEv->param);
                    c->getTvClient()->notifyCallback(UPGRADE_FBC_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SERIAL_COMMUNICATION: {
        TvEvent::SerialCommunicationEvent *pEv = (TvEvent::SerialCommunicationEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mDevId);
                    p.writeInt32(pEv->mDataCount);
                    for (int j = 0; j < pEv->mDataCount; j++) {
                        p.writeInt32(pEv->mDataBuf[j]);
                    }
                    c->getTvClient()->notifyCallback(SERIAL_COMMUNICATION_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_2d4G_HEADSET: {
        TvEvent::HeadSetOf2d4GEvent *pEv = (TvEvent::HeadSetOf2d4GEvent *)(&ev);
        LOGD("SendDtvStats status: =%d para2: =%d", pEv->state, pEv->para);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->state);
                    p.writeInt32(pEv->para);
                    c->getTvClient()->notifyCallback(HEADSET_STATUS_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SCANNING_FRAME_STABLE: {
        TvEvent::ScanningFrameStableEvent *pEv = (TvEvent::ScanningFrameStableEvent *)(&ev);
        LOGD("Scanning Frame is stable!");
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->CurScanningFreq);
                    c->getTvClient()->notifyCallback(SCANNING_FRAME_STABLE_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_FRONTEND: {
        TvEvent::FrontendEvent *pEv = (TvEvent::FrontendEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mStatus);
                    p.writeInt32(pEv->mFrequency);
                    p.writeInt32(pEv->mParam1);
                    p.writeInt32(pEv->mParam2);
                    p.writeInt32(pEv->mParam3);
                    p.writeInt32(pEv->mParam4);
                    p.writeInt32(pEv->mParam5);
                    p.writeInt32(pEv->mParam6);
                    p.writeInt32(pEv->mParam7);
                    p.writeInt32(pEv->mParam8);
                    c->getTvClient()->notifyCallback(FRONTEND_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_RECORDER: {
        TvEvent::RecorderEvent *pEv = (TvEvent::RecorderEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c!= 0) {
                    Parcel p;
                    p.writeString16(String16(pEv->mId));
                    p.writeInt32(pEv->mStatus);
                    p.writeInt32(pEv->mError);
                    c->getTvClient()->notifyCallback(RECORDER_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_RRT: {
        CTvRrt::RrtEvent *pRrtEvent = (CTvRrt::RrtEvent *) (&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pRrtEvent->satus);
                    c->getTvClient()->notifyCallback(RRT_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_EAS: {
        CTvEas::EasEvent *pEasEvent = (CTvEas::EasEvent *) (&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    int j, k;
                    p.writeInt32(pEasEvent->eas_section_count);
                    p.writeInt32(pEasEvent->table_id);
                    p.writeInt32(pEasEvent->extension);
                    p.writeInt32(pEasEvent->version);
                    p.writeInt32(pEasEvent->current_next);
                    p.writeInt32(pEasEvent->sequence_num);
                    p.writeInt32(pEasEvent->protocol_version);
                    p.writeInt32(pEasEvent->eas_event_id);
                    p.writeInt32(pEasEvent->eas_orig_code[0]);
                    p.writeInt32(pEasEvent->eas_orig_code[1]);
                    p.writeInt32(pEasEvent->eas_orig_code[2]);
                    p.writeInt32(pEasEvent->eas_event_code_len);
                    for (j=0;j<pEasEvent->eas_event_code_len;j++) {
                        p.writeInt32(pEasEvent->eas_event_code[j]);
                    }
                    p.writeInt32(pEasEvent->alert_message_time_remaining);
                    p.writeInt32(pEasEvent->event_start_time);
                    p.writeInt32(pEasEvent->event_duration);
                    p.writeInt32(pEasEvent->alert_priority);
                    p.writeInt32(pEasEvent->details_OOB_source_ID);
                    p.writeInt32(pEasEvent->details_major_channel_number);
                    p.writeInt32(pEasEvent->details_minor_channel_number);
                    p.writeInt32(pEasEvent->audio_OOB_source_ID);
                    p.writeInt32(pEasEvent->location_count);
                    for (j=0;j<pEasEvent->location_count;j++) {
                        p.writeInt32(pEasEvent->location[j].i_state_code);
                        p.writeInt32(pEasEvent->location[j].i_country_subdiv);
                        p.writeInt32(pEasEvent->location[j].i_country_code);
                    }
                    p.writeInt32(pEasEvent->exception_count);
                    for (j=0;j<pEasEvent->exception_count;j++) {
                        p.writeInt32(pEasEvent->exception[j].i_in_band_refer);
                        p.writeInt32(pEasEvent->exception[j].i_exception_major_channel_number);
                        p.writeInt32(pEasEvent->exception[j].i_exception_minor_channel_number);
                        p.writeInt32(pEasEvent->exception[j].exception_OOB_source_ID);
                    }
                    p.writeInt32(pEasEvent->multi_text_count);
                    for (j=0;j<pEasEvent->multi_text_count;j++) {
                        p.writeInt32(pEasEvent->multi_text[j].lang[0]);
                        p.writeInt32(pEasEvent->multi_text[j].lang[1]);
                        p.writeInt32(pEasEvent->multi_text[j].lang[2]);
                        p.writeInt32(pEasEvent->multi_text[j].i_type);
                        p.writeInt32(pEasEvent->multi_text[j].i_compression_type);
                        p.writeInt32(pEasEvent->multi_text[j].i_mode);
                        p.writeInt32(pEasEvent->multi_text[j].i_number_bytes);
                        for (k=0;k<pEasEvent->multi_text[j].i_number_bytes;k++) {
                            p.writeInt32(pEasEvent->multi_text[j].compressed_str[k]);
                        }
                    }
                    p.writeInt32(pEasEvent->descriptor_text_count);
                    for (j=0;j<pEasEvent->descriptor_text_count;j++) {
                        p.writeInt32(pEasEvent->descriptor[j].i_tag);
                        p.writeInt32(pEasEvent->descriptor[j].i_length);
                        for (k=0;k<pEasEvent->descriptor[j].i_length;k++) {
                            p.writeInt32(pEasEvent->descriptor[j].p_data[k]);
                        }
                    }
                    c->getTvClient()->notifyCallback(EAS_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }

    default:
        break;
    }
}

sp<ITv> TvService::connect(const sp<ITvClient> &tvClient)
{
    int callingPid = getCallingPid();
    LOGD("TvService::connect (pid %d, client %p)", callingPid, IInterface::asBinder(tvClient).get());

    AutoMutex _l(mServiceLock);
    sp<Client> newclient;
    bool haveclient = false;

    int clientSize = mClients.size();
    for (int i = 0; i < clientSize; i++) {
        wp<Client> client = mClients[i];
        if (client != 0) {
            sp<Client> currentClient = client.promote();
            if (currentClient != 0) {
                sp<ITvClient> currentTvClient(currentClient->getTvClient());
                if (IInterface::asBinder(tvClient) == IInterface::asBinder(currentTvClient)) {
                    LOGD("TvService::connect (pid %d, same client %p) is reconnecting...", callingPid, IInterface::asBinder(tvClient).get());
                    newclient = currentClient;
                    haveclient = true;
                }
            } else {
                LOGE("TvService::connect client (pid %d) not exist", callingPid);
                client.clear();
                mClients.removeAt(i);
                clientSize--;
            }
        }
    }

    if ( haveclient ) {
        return newclient;
    }

    newclient = new Client(this, tvClient, callingPid, mpTv);
    mClients.add(newclient);
    return newclient;
}

void TvService::removeClient(const sp<ITvClient> &tvClient)
{
    int callingPid = getCallingPid();

    AutoMutex _l(mServiceLock);
    int clientSize = mClients.size();
    for (int i = 0; i < clientSize; i++) {
        wp<Client> client = mClients[i];
        if (client != 0) {
            sp<Client> currentClient = client.promote();
            if (currentClient != 0) {
                sp<ITvClient> currentTvClient(currentClient->getTvClient());
                if (IInterface::asBinder(tvClient) == IInterface::asBinder(currentTvClient)) {
                    LOGD("find client , and remove it pid = %d, client = %p i=%d", callingPid, IInterface::asBinder(tvClient).get(), i);
                    client.clear();
                    mClients.removeAt(i);
                    break;
                }
            } else {
                LOGW("removeclient currentClient is NULL (pid %d)", callingPid);
                client.clear();
                mClients.removeAt(i);
                clientSize--;
            }
        }
    }

    LOGD("removeClient (pid %d) done", callingPid);
}

void TvService::incUsers()
{
    android_atomic_inc(&mUsers);
}

void TvService::decUsers()
{
    android_atomic_dec(&mUsers);
}

TvService::Client::Client(const sp<TvService> &tvService, const sp<ITvClient> &tvClient, pid_t clientPid, CTv *pTv)
{
    mTvService = tvService;
    mTvClient = tvClient;
    mClientPid = clientPid;
    tvService->incUsers();
    mpTv = pTv;
    mIsStartTv = false;
}

TvService::Client::~Client()
{
    if (mIsStartTv) {
        mpTv->StopTvLock();
        mIsStartTv = false;
    }

    int callingPid = getCallingPid();
    // tear down client
    LOGD("Client::~Client(pid %d, client %p)", callingPid, IInterface::asBinder(getTvClient()).get());
    // make sure we tear down the hardware
    //mClientPid = callingPid;
    //disconnect();
    mTvService->decUsers();
}

status_t TvService::Client::checkPid()
{
    int callingPid = getCallingPid();
    if (mClientPid == callingPid)
        return NO_ERROR;
    LOGD("Attempt to use locked tv (client %p) from different process "
         " (old pid %d, new pid %d)", IInterface::asBinder(getTvClient()).get(), mClientPid, callingPid);
    return -EBUSY;
}

status_t TvService::Client::lock()
{
    int callingPid = getCallingPid();
    LOGD("lock from pid %d (mClientPid %d)", callingPid, mClientPid);
    AutoMutex _l(mLock);
    // lock tv to this client if the the tv is unlocked
    if (mClientPid == 0) {
        mClientPid = callingPid;
        return NO_ERROR;
    }
    // returns NO_ERROR if the client already owns the tv, -EBUSY otherwise
    return checkPid();
}

status_t TvService::Client::unlock()
{
    int callingPid = getCallingPid();
    LOGD("unlock from pid %d (mClientPid %d)", callingPid, mClientPid);
    AutoMutex _l(mLock);
    // allow anyone to use tv
    status_t result = checkPid();
    if (result == NO_ERROR) {
        mClientPid = 0;
        // we need to remove the reference so that when app goes
        // away, the reference count goes to 0.
        mTvClient.clear();
    }
    return result;
}

status_t TvService::Client::connect(const sp<ITvClient> &client)
{
    int callingPid = getCallingPid();
    LOGD("Client::connect E (pid %d, client %p)", callingPid, IInterface::asBinder(client).get());

    {
        AutoMutex _l(mLock);
        if (mClientPid != 0 && checkPid() != NO_ERROR) {
            LOGW("Tried to connect to locked tv (old pid %d, new pid %d)", mClientPid, callingPid);
            return -EBUSY;
        }

        // did the client actually change?
        if ((mTvClient != NULL) && (IInterface::asBinder(client) == IInterface::asBinder(mTvClient))) {
            LOGD("Connect to the same client");
            return NO_ERROR;
        }

        mTvClient = client;
        LOGD("Connect to the new client (pid %d, client %p)", callingPid, IInterface::asBinder(client).get());
    }

    mClientPid = callingPid;
    return NO_ERROR;
}

void TvService::Client::disconnect()
{
    int callingPid = getCallingPid();

    LOGD("Client::disconnect() E (pid %d client %p)", callingPid, IInterface::asBinder(getTvClient()).get());

    AutoMutex _l(mLock);
    if (mClientPid <= 0) {
        LOGE("tv is unlocked (mClientPid = %d), don't tear down hardware", mClientPid);
        return;
    }
    if (checkPid() != NO_ERROR) {
        LOGE("Different client - don't disconnect");
        return;
    }

    mTvService->removeClient(mTvClient);
    mTvService->decUsers();

    LOGD("Client::disconnect() X (pid %d)", callingPid);
}

status_t TvService::Client::createVideoFrame(const sp<IMemory> &shareMem __unused,
    int iSourceMode __unused, int iCapVideoLayerOnly __unused)
{
#if 0
    LOGD(" mem=%d size=%d", shareMem->pointer() == NULL, shareMem->size());
    LOGD("iSourceMode :%d iCapVideoLayerOnly = %d \n", iSourceMode, iCapVideoLayerOnly);
    int Len = 0;
    AutoMutex _l(mLock);
    mTvService->mCapVidFrame.InitVCap(shareMem);

    if ((1 == iSourceMode) && (1 == iCapVideoLayerOnly)) {
        mTvService->mCapVidFrame.CapMediaPlayerVideoLayerOnly(1920, 1080);
#if 0
        mTvService->mCapVidFrame.SetVideoParameter(1920, 1080, 50);
        mTvService->mCapVidFrame.VideoStart();
        mTvService->mCapVidFrame.GetVideoData(&Len);
        mTvService->mCapVidFrame.VideoStop();
#endif
    } else if (2 == iSourceMode && 0 == iCapVideoLayerOnly) {
        mTvService->mCapVidFrame.CapOsdAndVideoLayer(1920, 1080);
    } else if (2 == iSourceMode && 1 == iCapVideoLayerOnly) {
        mTvService->mCapVidFrame.CapMediaPlayerVideoLayerOnly(1920, 1080);
    } else {
        LOGD("=============== NOT SUPPORT=======================\n");
    }
    mTvService->mCapVidFrame.DeinitVideoCap();
#endif
    LOGE("do not support this function");
    return 0;
}

status_t TvService::Client::createSubtitle(const sp<IMemory> &shareMem)
{
    LOGD("createSubtitle pid = %d, mem=%d size=%d", getCallingPid(), shareMem->pointer() == NULL, shareMem->size());
    mpTv->mSubtitle.setBuffer((char *)shareMem->pointer());
    mTvService->mpSubClient = this;
    //pSub = new CTvSubtitle(share_mem, this);
    //pSub->run();
    return 0;
}

status_t TvService::Client::processCmd(const Parcel &p, Parcel *r)
{
    unsigned char dataBuf[512] = {0};
    int *ptrData = NULL;

    int cmd = p.readInt32();

    LOGD("enter client=%d cmd=%d", getCallingPid(), cmd);
    switch (cmd) {
    // Tv function
    case OPEN_TV: {
        break;
    }
    case CLOSE_TV: {
        int ret = mpTv->CloseTv();
        r->writeInt32(ret);
        break;
    }
    case START_TV: {
        int mode = p.readInt32();
        int ret = mpTv->StartTvLock();
        mIsStartTv = true;
        r->writeInt32(ret);
        break;
    }
    case STOP_TV: {
        int ret = mpTv->StopTvLock();
        r->writeInt32(ret);
        mIsStartTv = false;
        break;
    }
    case GET_TV_STATUS: {
        int ret = (int)mpTv->GetTvStatus();
        r->writeInt32(ret);
        break;
    }
    case GET_LAST_SOURCE_INPUT: {
        int ret = (int)mpTv->GetLastSourceInput();
        r->writeInt32(ret);
        break;
    }
    case GET_CURRENT_SOURCE_INPUT: {
        int ret = (int)mpTv->GetCurrentSourceInputLock();
        r->writeInt32(ret);
        break;
    }
    case GET_CURRENT_SOURCE_INPUT_VIRTUAL: {
        int ret = (int)mpTv->GetCurrentSourceInputVirtualLock();
        r->writeInt32(ret);
        break;
    }
    case GET_CURRENT_SIGNAL_INFO: {
        tvin_info_t siginfo = mpTv->GetCurrentSignalInfo();
        int frame_rate = mpTv->getHDMIFrameRate();
        r->writeInt32(siginfo.trans_fmt);
        r->writeInt32(siginfo.fmt);
        r->writeInt32(siginfo.status);
        r->writeInt32(frame_rate);
        break;
    }
    case SET_SOURCE_INPUT: {
        int sourceinput = p.readInt32();
        LOGD(" SetSourceInput sourceId= %x", sourceinput);
        int ret = mpTv->SetSourceSwitchInput((tv_source_input_t)sourceinput);
        r->writeInt32(ret);
        break;
    }
    case SET_SOURCE_INPUT_EXT: {
        int sourceinput = p.readInt32();
        int vsourceinput = p.readInt32();
        LOGD(" SetSourceInputExt vsourceId= %d sourceId=%d", vsourceinput, sourceinput);
        int ret = mpTv->SetSourceSwitchInput((tv_source_input_t)vsourceinput, (tv_source_input_t)sourceinput);
        r->writeInt32(ret);
        break;
    }
    case DO_SUSPEND: {
        int type = p.readInt32();
        int ret = mpTv->DoSuspend(type);
        r->writeInt32(ret);
        break;
    }
    case DO_RESUME: {
        int type = p.readInt32();
        int ret = mpTv->DoResume(type);
        r->writeInt32(ret);
        break;
    }
    case IS_DVI_SIGNAL: {
        int ret = mpTv->IsDVISignal();
        r->writeInt32(ret);
        break;
    }
    case IS_VGA_TIMEING_IN_HDMI: {
        int ret = mpTv->isVgaFmtInHdmi();
        r->writeInt32(ret);
        break;
    }
    case SET_PREVIEW_WINDOW_MODE: {
        bool mode = (p.readInt32() == 1);
        int ret = mpTv->setPreviewWindowMode(mode);
        r->writeInt32(ret);
        break;
    }
    case SET_PREVIEW_WINDOW: {
        tvin_window_pos_t win_pos;
        win_pos.x1 = p.readInt32();
        win_pos.y1 = p.readInt32();
        win_pos.x2 = p.readInt32();
        win_pos.y2 = p.readInt32();
        int ret = (int)mpTv->SetPreviewWindow(win_pos);
        r->writeInt32(ret);
        break;
    }

    case GET_SOURCE_CONNECT_STATUS: {
        int source_input = p.readInt32();
        int ret = mpTv->GetSourceConnectStatus((tv_source_input_t)source_input);
        r->writeInt32(ret);
        break;
    }

    case GET_SOURCE_INPUT_LIST: {
        const char *value = config_get_str(CFG_SECTION_TV, CGF_DEFAULT_INPUT_IDS, "null");
        r->writeString16(String16(value));
        break;
    }
    //Tv function END

    // HDMI
    case SET_HDMI_EDID_VER: {
        int hdmi_port_id = p.readInt32();
        int edid_ver = p.readInt32();
        int tmpRet = mpTv->SetHdmiEdidVersion((tv_hdmi_port_id_t)hdmi_port_id, (tv_hdmi_edid_version_t)edid_ver);
        r->writeInt32(tmpRet);
        break;
    }
    case SET_HDCP_KEY_ENABLE: {
        int enable = p.readInt32();
        int tmpRet = mpTv->SetHdmiHDCPSwitcher((tv_hdmi_hdcpkey_enable_t)enable);
        r->writeInt32(tmpRet);
        break;
    }
    case SET_HDMI_COLOR_RANGE_MODE: {
        int range_mode = p.readInt32();
        int tmpRet = mpTv->SetHdmiColorRangeMode((tvin_color_range_t)range_mode);
        r->writeInt32(tmpRet);
        break;
    }
    case GET_HDMI_COLOR_RANGE_MODE: {
        int tmpRet = mpTv->GetHdmiColorRangeMode();
        r->writeInt32(tmpRet);
        break;
    }
    // HDMI END
    // FACTORY
    case FACTORY_SETTESTPATTERN: {
        int pattern = p.readInt32();
        int ret = mpTv->mFactoryMode.setTestPattern(pattern);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETTESTPATTERN: {
        int ret = mpTv->mFactoryMode.getTestPattern();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPATTERN_YUV: {
        int blend = p.readInt32();
        int y = p.readInt32();
        int u = p.readInt32();
        int v = p.readInt32();
        int ret = mpTv->mFactoryMode.setScreenColor(blend, y, u, v);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETDDRSSC: {
        int setp = p.readInt32();
        int ret = mpTv->mFactoryMode.setDDRSSC(setp);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETDDRSSC: {
        int ret = mpTv->mFactoryMode.getDDRSSC();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETLVDSSSC: {
        int setp = p.readInt32();
        int ret = mpTv->mFactoryMode.setLVDSSSC(setp);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETLVDSSSC: {
        int ret = mpTv->mFactoryMode.getLVDSSSC();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SET_OUT_DEFAULT: {
        int ret = mpTv->Tv_SSMFacRestoreDefaultSetting();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_CLEAN_ALL_TABLE_FOR_PROGRAM: {
        int ret = mpTv->ClearAnalogFrontEnd();
        mpTv->clearDbAllProgramInfoTable();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SET_GAMMA_PATTERN: {
        tcon_gamma_table_t gamma_r, gamma_g, gamma_b;
        memset(gamma_r.data, (unsigned short)(p.readInt32()<<2), 256);
        memset(gamma_g.data, (unsigned short)(p.readInt32()<<2), 256);
        memset(gamma_b.data, (unsigned short)(p.readInt32()<<2), 256);
        int ret = mpTv->mFactoryMode.setGamma(gamma_r, gamma_g, gamma_b);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SET_RGB_PATTERN: {
        int pr = p.readInt32();
        int pg = p.readInt32();
        int pb = p.readInt32();
        int ret = mpTv->mFactoryMode.setRGBPattern(pr, pg, pb);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GET_RGB_PATTERN: {
        int ret = mpTv->mFactoryMode.getRGBPattern();
        r->writeInt32(ret);
        break;
    }
    // FACTORY END

    // AUDIO & AUDIO MUTE
    case SET_AUDIO_MUTE_FOR_TV: {
        int status = p.readInt32();
        break;
    }
    case SET_AUDIO_MUTEKEY_STATUS: {
        break;
    }
    case GET_AUDIO_MUTEKEY_STATUS: {
        break;
    }
    case SET_AUDIO_AVOUT_MUTE_STATUS: {
        break;
    }
    case GET_AUDIO_AVOUT_MUTE_STATUS: {
        break;
    }
    case SET_AUDIO_SPDIF_MUTE_STATUS: {
        break;
    }
    case GET_AUDIO_SPDIF_MUTE_STATUS: {
        break;
    }
    // AUDIO MASTER VOLUME
    case SET_AUDIO_MASTER_VOLUME: {
        break;
    }
    case GET_AUDIO_MASTER_VOLUME: {
        break;
    }
    case SAVE_CUR_AUDIO_MASTER_VOLUME: {
        break;
    }
    case GET_CUR_AUDIO_MASTER_VOLUME: {
        break;
    }
    //AUDIO BALANCE
    case SET_AUDIO_BALANCE: {
        break;
    }
    case GET_AUDIO_BALANCE: {
        break;
    }
    case SAVE_CUR_AUDIO_BALANCE: {
        break;
    }
    case GET_CUR_AUDIO_BALANCE: {
        break;
    }
    //AUDIO SUPPERBASS VOLUME
    case SET_AUDIO_SUPPER_BASS_VOLUME: {
        break;
    }
    case GET_AUDIO_SUPPER_BASS_VOLUME: {
        break;
    }
    case SAVE_CUR_AUDIO_SUPPER_BASS_VOLUME: {
        break;
    }
    case GET_CUR_AUDIO_SUPPER_BASS_VOLUME: {
        break;
    }
    //AUDIO SUPPERBASS SWITCH
    case SET_AUDIO_SUPPER_BASS_SWITCH: {
        break;
    }
    case GET_AUDIO_SUPPER_BASS_SWITCH: {
        break;
    }
    case SAVE_CUR_AUDIO_SUPPER_BASS_SWITCH: {
        break;
    }
    case GET_CUR_AUDIO_SUPPER_BASS_SWITCH: {
        break;
    }
    //AUDIO SRS SURROUND SWITCH
    case SET_AUDIO_SRS_SURROUND: {
        break;
    }
    case GET_AUDIO_SRS_SURROUND: {
        break;
    }
    case SAVE_CUR_AUDIO_SRS_SURROUND: {
        break;
    }
    case GET_CUR_AUDIO_SRS_SURROUND: {
        break;
    }
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
    case GET_CUR_AUDIO_SRS_DIALOG_CLARITY: {
        break;
    }
    //AUDIO SRS TRUBASS
    case SET_AUDIO_SRS_TRU_BASS: {
        break;
    }
    case GET_AUDIO_SRS_TRU_BASS: {
        break;
    }
    case SAVE_CUR_AUDIO_SRS_TRU_BASS: {
        break;
    }
    case GET_CUR_AUDIO_SRS_TRU_BASS: {
        break;
    }
    //AUDIO BASS
    case SET_AUDIO_BASS_VOLUME: {
        break;
    }
    case GET_AUDIO_BASS_VOLUME: {
        break;
    }
    case SAVE_CUR_AUDIO_BASS_VOLUME: {
        break;
    }
    case GET_CUR_AUDIO_BASS_VOLUME: {
        break;
    }
    //AUDIO TREBLE
    case SET_AUDIO_TREBLE_VOLUME: {
        break;
    }
    case GET_AUDIO_TREBLE_VOLUME: {
        break;
    }
    case SAVE_CUR_AUDIO_TREBLE_VOLUME: {
        break;
    }
    case GET_CUR_AUDIO_TREBLE_VOLUME: {
        break;
    }
    //AUDIO SOUND MODE
    case SET_AUDIO_SOUND_MODE: {
        break;
    }
    case GET_AUDIO_SOUND_MODE: {
        break;
    }
    case SAVE_CUR_AUDIO_SOUND_MODE: {
        break;
    }
    case GET_CUR_AUDIO_SOUND_MODE: {
        break;
    }
    //AUDIO WALL EFFECT
    case SET_AUDIO_WALL_EFFECT: {
        break;
    }
    case GET_AUDIO_WALL_EFFECT: {
        break;
    }
    case SAVE_CUR_AUDIO_WALL_EFFECT: {
        break;
    }
    case GET_CUR_AUDIO_WALL_EFFECT: {
        break;
    }
    //AUDIO EQ MODE
    case SET_AUDIO_EQ_MODE: {
        break;
    }
    case GET_AUDIO_EQ_MODE: {
        break;
    }
    case SAVE_CUR_AUDIO_EQ_MODE: {
        break;
    }
    case GET_CUR_AUDIO_EQ_MODE: {
        break;
    }
    //AUDIO EQ GAIN
    case GET_AUDIO_EQ_RANGE: {
        break;
    }
    case GET_AUDIO_EQ_BAND_COUNT: {
        break;
    }
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
    case GET_CUR_AUDIO_SPDIF_SWITCH: {
        break;
    }
    //AUDIO SPDIF MODE
    case SET_AUDIO_SPDIF_MODE: {
        break;
    }
    case SAVE_CUR_AUDIO_SPDIF_MODE: {
        break;
    }
    case GET_CUR_AUDIO_SPDIF_MODE: {
        break;
    }
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
    case GET_AMAUDIO_PRE_MUTE: {
        break;
    }
    case SELECT_LINE_IN_CHANNEL: {
        break;
    }
    case SET_LINE_IN_CAPTURE_VOL: {
        break;
    }
    case SET_AUDIO_VOL_COMP: {
        break;
    }
    case GET_AUDIO_VOL_COMP: {
        int ret = mpTv->GetAudioVolumeCompensationVal(-1);
        r->writeInt32(ret);
        break;
    }
    case SET_AUDIO_VIRTUAL: {
        break;
    }
    case GET_AUDIO_VIRTUAL_ENABLE: {
        break;
    }
    case GET_AUDIO_VIRTUAL_LEVEL: {
        break;
    }
    // AUDIO END

    // SSM
    case SSM_INIT_DEVICE: {
        int tmpRet = 0;
        tmpRet = mpTv->Tv_SSMRestoreDefaultSetting();//mpTv->Tv_SSMInitDevice();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_POWER_ON_OFF_CHANNEL: {
        int tmpPowerChanNum = p.readInt32();
        int tmpRet;
        tmpRet = SSMSavePowerOnOffChannel(tmpPowerChanNum);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_POWER_ON_OFF_CHANNEL: {
        int tmpRet = 0;
        tmpRet = SSMReadPowerOnOffChannel();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_SOURCE_INPUT: {
        int tmpSouceInput = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveSourceInput(tmpSouceInput);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_SOURCE_INPUT: {
        int tmpRet = 0;
        tmpRet = SSMReadSourceInput();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_LAST_SOURCE_INPUT: {
        int tmpLastSouceInput = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveLastSelectSourceInput(tmpLastSouceInput);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_LAST_SOURCE_INPUT: {
        int tmpRet = 0;
        tmpRet = SSMReadLastSelectSourceInput();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_SYS_LANGUAGE: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveSystemLanguage(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_SYS_LANGUAGE: {
        int tmpRet = 0;
        tmpRet = SSMReadSystemLanguage();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_AGING_MODE: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveAgingMode(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_AGING_MODE: {
        int tmpRet = 0;
        tmpRet = SSMReadAgingMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_PANEL_TYPE: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSavePanelType(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_PANEL_TYPE: {
        int tmpRet = 0;
        tmpRet = SSMReadPanelType();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_MAC_ADDR: {
        int size = p.readInt32();
        for (int i = 0; i < size; i++) {
            dataBuf[i] = p.readInt32();
        }
        int ret = KeyData_SaveMacAddress(dataBuf);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_MAC_ADDR: {
        int ret = KeyData_ReadMacAddress(dataBuf);
        int size = KeyData_GetMacAddressDataLen();
        r->writeInt32(size);
        for (int i = 0; i < size; i++) {
            r->writeInt32(dataBuf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_BAR_CODE: {
        int size = p.readInt32();
        for (int i = 0; i < size; i++) {
            dataBuf[i] = p.readInt32();
        }
        int ret = KeyData_SaveBarCode(dataBuf);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_BAR_CODE: {
        int ret = KeyData_ReadBarCode(dataBuf);
        int size = KeyData_GetBarCodeDataLen();
        r->writeInt32(size);
        for (int i = 0; i < size; i++) {
            r->writeInt32(dataBuf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_HDCPKEY: {
        int size = p.readInt32();
        for (int i = 0; i < size; i++) {
            dataBuf[i] = p.readInt32();
        }
        int ret = SSMSaveHDCPKey(dataBuf);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_HDCPKEY: {
        int ret = SSMReadHDCPKey(dataBuf);
        int size = SSMGetHDCPKeyDataLen();
        r->writeInt32(size);
        for (int i = 0; i < size; i++) {
            r->writeInt32(dataBuf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_POWER_ON_MUSIC_SWITCH: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSavePowerOnMusicSwitch(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_POWER_ON_MUSIC_SWITCH: {
        int tmpRet = 0;
        tmpRet = SSMReadPowerOnMusicSwitch();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_POWER_ON_MUSIC_VOL: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSavePowerOnMusicVolume(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_POWER_ON_MUSIC_VOL: {
        int tmpRet = 0;
        tmpRet = SSMReadPowerOnMusicVolume();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_SYS_SLEEP_TIMER: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveSystemSleepTimer(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_SYS_SLEEP_TIMER: {
        int tmpRet = 0;
        tmpRet = SSMReadSystemSleepTimer();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_INPUT_SRC_PARENTAL_CTL: {
        int tmpSourceIndex = p.readInt32();
        int tmpCtlFlag = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveInputSourceParentalControl(tmpSourceIndex, tmpCtlFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_INPUT_SRC_PARENTAL_CTL: {
        int tmpSourceIndex = p.readInt32();
        int tmpRet = 0;
        tmpRet = SSMReadInputSourceParentalControl(tmpSourceIndex);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_PARENTAL_CTL_SWITCH: {
        int tmpSwitchFlag = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveParentalControlSwitch(tmpSwitchFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_PARENTAL_CTL_SWITCH: {
        int tmpRet = 0;
        tmpRet = SSMReadParentalControlSwitch();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_PARENTAL_CTL_PASS_WORD: {
        String16 pass_wd_str = p.readString16();
        int tmpRet = SSMSaveParentalControlPassWord((unsigned char *)pass_wd_str.string(), pass_wd_str.size() * sizeof(unsigned short));
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_CUSTOMER_DATA_START: {
        int tmpRet = SSMGetCustomerDataStart();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_CUSTOMER_DATA_LEN: {
        int tmpRet = SSMGetCustomerDataLen();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_STANDBY_MODE: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveStandbyMode(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_STANDBY_MODE: {
        int tmpRet = SSMReadStandbyMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_LOGO_ON_OFF_FLAG: {
        int tmpSwitchFlag = p.readInt32();
        int tmpRet = SSMSaveLogoOnOffFlag(tmpSwitchFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_LOGO_ON_OFF_FLAG: {
        int tmpRet = SSMReadLogoOnOffFlag();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_HDMIEQ_MODE: {
        int tmpSwitchFlag = p.readInt32();
        int tmpRet = SSMSaveHDMIEQMode(tmpSwitchFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_HDMIEQ_MODE: {
        int tmpRet = SSMReadHDMIEQMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_HDMIINTERNAL_MODE: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveHDMIInternalMode(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_HDMIINTERNAL_MODE: {
        int tmpRet = SSMReadHDMIInternalMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_GLOBAL_OGOENABLE: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveGlobalOgoEnable(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_GLOBAL_OGOENABLE: {
        int tmpRet = SSMReadGlobalOgoEnable();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_NON_STANDARD_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveNonStandardValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_NON_STANDARD_STATUS: {
        int tmpRet = SSMReadNonStandardValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_ADB_SWITCH_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveAdbSwitchValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_ADB_SWITCH_STATUS: {
        int tmpRet = SSMReadAdbSwitchValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SET_HDCP_KEY: {
        int tmpRet = SSMSetHDCPKey();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_REFRESH_HDCPKEY: {
        int tmpRet = SSMRefreshHDCPKey();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_CHROMA_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveChromaStatus(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_CA_BUFFER_SIZE: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveCABufferSizeValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_CA_BUFFER_SIZE: {
        int tmpRet = SSMReadCABufferSizeValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_ATV_DATA_START: {
        int tmpRet = SSMGetATVDataStart();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_ATV_DATA_LEN: {
        int tmpRet = SSMGetATVDataLen();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_VPP_DATA_START: {
        int tmpRet = SSMGetVPPDataStart();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_VPP_DATA_LEN: {
        int tmpRet = SSMGetVPPDataLen();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_NOISE_GATE_THRESHOLD_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveNoiseGateThresholdValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_NOISE_GATE_THRESHOLD_STATUS: {
        int tmpRet = SSMReadNoiseGateThresholdValue();
        r->writeInt32(tmpRet);
        break;
    }

    case SSM_SAVE_HDMI_EDID_VER: {
        int ret = -1;
        int port_id = p.readInt32();
        int ver = p.readInt32();
        ret = SSMSaveHDMIEdidVersion((tv_hdmi_port_id_t)port_id, (tv_hdmi_edid_version_t)ver);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_HDMI_EDID_VER: {
        int ret = -1;
        int port_id = p.readInt32();
        ret = SSMReadHDMIEdidVersion((tv_hdmi_port_id_t)port_id);
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_HDCP_KEY_ENABLE: {
        int ret = -1;
        int enable = p.readInt32();
        ret = SSMSaveHDMIHdcpSwitcher(enable);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_HDCP_KEY_ENABLE: {
        int ret = -1;
        ret = SSMReadHDMIHdcpSwitcher();
        r->writeInt32(ret);
        break;
    }
    // SSM END

    //MISC
    case MISC_CFG_SET: {
        String8 key(p.readString16());
        String8 value(p.readString16());

        int tmpRet = config_set_str(CFG_SECTION_TV, key.string(), value.string());
        r->writeInt32(tmpRet);
        break;
    }
    case MISC_CFG_GET: {
        String8 key(p.readString16());
        String8 def(p.readString16());

        const char *value = config_get_str(CFG_SECTION_TV, key.string(), def.string());
        r->writeString16(String16(value));
        break;
    }
    case MISC_SET_WDT_USER_PET: {
        int counter = p.readInt32();
        int ret = TvMisc_SetUserCounter(counter);
        r->writeInt32(ret);
        break;
    }
    case MISC_SET_WDT_USER_COUNTER: {
        int counter_time_out = p.readInt32();
        int ret = TvMisc_SetUserCounterTimeOut(counter_time_out);
        r->writeInt32(ret);
        break;
    }
    case MISC_SET_WDT_USER_PET_RESET_ENABLE: {
        int enable = p.readInt32();
        int ret = TvMisc_SetUserPetResetEnable(enable);
        r->writeInt32(ret);
        break;
    }
    case MISC_GET_TV_API_VERSION: {
        // write tvapi version info
        const char *str = tvservice_get_git_branch_info();
        r->writeString16(String16(str));

        str = tvservice_get_git_version_info();
        r->writeString16(String16(str));

        str = tvservice_get_last_chaned_time_info();
        r->writeString16(String16(str));

        str = tvservice_get_build_time_info();
        r->writeString16(String16(str));

        str = tvservice_get_build_name_info();
        r->writeString16(String16(str));
        break;
    }
    case MISC_GET_DVB_API_VERSION: {
        // write dvb version info
        const char *str = dvb_get_git_branch_info();
        r->writeString16(String16(str));

        str = dvb_get_git_version_info();
        r->writeString16(String16(str));

        str = dvb_get_last_chaned_time_info();
        r->writeString16(String16(str));

        str = dvb_get_build_time_info();
        r->writeString16(String16(str));

        str = dvb_get_build_name_info();
        r->writeString16(String16(str));
        break;
    }
    //MISC  END

    // EXTAR
    case DTV_SUBTITLE_INIT: {
        int bitmapWidth = p.readInt32();
        int bitmapHeight = p.readInt32();
        r->writeInt32(mpTv->mSubtitle.sub_init(bitmapWidth, bitmapHeight));
        break;
    }
    case DTV_SUBTITLE_LOCK: {
        r->writeInt32(mpTv->mSubtitle.sub_lock());
        break;
    }
    case DTV_SUBTITLE_UNLOCK: {
        r->writeInt32(mpTv->mSubtitle.sub_unlock());
        break;
    }
    case DTV_GET_SUBTITLE_SWITCH: {
        r->writeInt32(mpTv->mSubtitle.sub_switch_status());
        break;
    }
    case DTV_START_SUBTITLE: {
        int dmx_id = p.readInt32();
        int pid = p.readInt32();
        int page_id = p.readInt32();
        int anc_page_id = p.readInt32();
        r->writeInt32(mpTv->mSubtitle.sub_start_dvb_sub(dmx_id, pid, page_id, anc_page_id));
        break;
    }
    case DTV_STOP_SUBTITLE: {
        r->writeInt32(mpTv->mSubtitle.sub_stop_dvb_sub());
        break;
    }
    case DTV_GET_SUBTITLE_INDEX: {
        int progId = p.readInt32();
        CTvProgram prog;
        CTvProgram::selectByID(progId, prog);
        r->writeInt32(prog.getSubtitleIndex(progId));
        break;
    }
    case DTV_SET_SUBTITLE_INDEX: {
        int progId = p.readInt32();
        int index = p.readInt32();
        CTvProgram prog;
        CTvProgram::selectByID(progId, prog);
        r->writeInt32(prog.setSubtitleIndex(progId, index));
        break;
    }
    case ATV_GET_CURRENT_PROGRAM_ID: {
        int atvLastProgramId = mpTv->getATVProgramID();
        r->writeInt32(atvLastProgramId);
        break;
    }
    case DTV_GET_CURRENT_PROGRAM_ID: {
        int dtvLastProgramId = mpTv->getDTVProgramID();
        r->writeInt32(dtvLastProgramId);
        break;
    }
    case ATV_SAVE_PROGRAM_ID: {
        int progID = p.readInt32();
        int retCnt = 0;
        mpTv->saveATVProgramID(progID);
        r->writeInt32(retCnt);
        break;
    }
    case SAVE_PROGRAM_ID: {
        int type = p.readInt32();
        int progID = p.readInt32();
        int retCnt = 0;
        if (type == CTvProgram::TYPE_DTV)
            mpTv->saveDTVProgramID(progID);
        else if (type == CTvProgram::TYPE_RADIO)
            mpTv->saveRadioProgramID(progID);
        else
            mpTv->saveATVProgramID(progID);
        r->writeInt32(retCnt);
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
        r->writeInt32(id);
        break;
    }

    case ATV_GET_MIN_MAX_FREQ: {
        int min, max;
        int tmpRet = mpTv->getATVMinMaxFreq(&min, &max);
        r->writeInt32(min);
        r->writeInt32(max);
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_SCAN_FREQUENCY_LIST: {
        Vector<sp<CTvChannel> > out;
        int tmpRet = CTvRegion::getChannelListByName((char *)"CHINA,Default DTMB ALL", out);
        r->writeInt32(out.size());
        for (int i = 0; i < (int)out.size(); i++) {
            r->writeInt32(out[i]->getID());
            r->writeInt32(out[i]->getFrequency());
            r->writeInt32(out[i]->getLogicalChannelNum());
        }
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_SCAN_FREQUENCY_LIST_MODE: {
        int mode = p.readInt32();
        Vector<sp<CTvChannel> > out;
        int tmpRet = CTvRegion::getChannelListByName((char *)CTvScanner::getDtvScanListName(mode), out);
        r->writeInt32(out.size());
        for (int i = 0; i < (int)out.size(); i++) {
            r->writeInt32(out[i]->getID());
            r->writeInt32(out[i]->getFrequency());
            r->writeInt32(out[i]->getLogicalChannelNum());
        }
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_CHANNEL_INFO: {
        int dbID = p.readInt32();
        channel_info_t chan_info;
        int ret = mpTv->getChannelInfoBydbID(dbID, chan_info);
        r->writeInt32(chan_info.freq);
        r->writeInt32(chan_info.uInfo.dtvChanInfo.strength);
        r->writeInt32(chan_info.uInfo.dtvChanInfo.quality);
        r->writeInt32(chan_info.uInfo.dtvChanInfo.ber);
        r->writeInt32(ret);
        break;
    }
    case ATV_GET_CHANNEL_INFO: {
        int dbID = p.readInt32();
        channel_info_t chan_info;
        int ret = mpTv->getChannelInfoBydbID(dbID, chan_info);
        r->writeInt32(chan_info.freq);
        r->writeInt32(chan_info.uInfo.atvChanInfo.finefreq);
        r->writeInt32(chan_info.uInfo.atvChanInfo.videoStd);
        r->writeInt32(chan_info.uInfo.atvChanInfo.audioStd);
        r->writeInt32(chan_info.uInfo.atvChanInfo.isAutoStd);
        r->writeInt32(ret);
        break;
    }
    case ATV_SCAN_MANUAL: {
        int tmpRet = 0;
        int startFreq = p.readInt32();
        int endFreq = p.readInt32();
        int videoStd = p.readInt32();
        int audioStd = p.readInt32();
        tmpRet = mpTv->atvMunualScan(startFreq, endFreq, videoStd, audioStd);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }

    case ATV_SCAN_AUTO: {
        int videoStd = p.readInt32();
        int audioStd = p.readInt32();
        int searchType = p.readInt32();
        int procMode = p.readInt32();
        int tmpRet = mpTv->atvAutoScan(videoStd, audioStd, searchType, procMode);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_MANUAL: {
        int freq = p.readInt32();
        int tmpRet = mpTv->dtvManualScan(freq, freq);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_MANUAL_BETWEEN_FREQ: {
        int beginFreq = p.readInt32();
        int endFreq = p.readInt32();
        int modulation = p.readInt32();
        int tmpRet = mpTv->dtvManualScan(beginFreq, endFreq, modulation);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_AUTO: {
        int tmpRet = mpTv->dtvAutoScan();
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN: {
        int mode = p.readInt32();
        int scanmode = p.readInt32();
        int freq = p.readInt32();
        int para1 = p.readInt32();
        int para2 = p.readInt32();
        int tmpRet = mpTv->dtvScan(mode, scanmode, freq, freq, para1, para2);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case STOP_PROGRAM_PLAY: {
        int tmpRet = mpTv->stopPlayingLock();
        r->writeInt32(tmpRet);
        break;
    }
    case ATV_DTV_SCAN_PAUSE: {
        int tmpRet = mpTv->pauseScan();
        r->writeInt32(tmpRet);
        break;
    }
    case ATV_DTV_SCAN_RESUME: {
        int tmpRet = mpTv->resumeScan();
        r->writeInt32(tmpRet);
        break;
    }
    case ATV_DTV_GET_SCAN_STATUS: {
        int tmpRet = mpTv->getScanStatus();
        r->writeInt32(tmpRet);
        break;
    }
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

        int tmpRet = mpTv->clearAllProgram(arg0);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }

    case HDMIRX_GET_KSV_INFO: {
        int data[2] = {0, 0};
        int tmpRet = mpTv->GetHdmiHdcpKeyKsvInfo(data);
        r->writeInt32(tmpRet);
        r->writeInt32(data[0]);
        r->writeInt32(data[1]);
        break;
    }
    case FACTORY_FBC_SET_BRIGHTNESS: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetBrightness(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BRIGHTNESS: {
        int ret = mpTv->mFactoryMode.fbcGetBrightness();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_CONTRAST: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetContrast(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_CONTRAST: {
        int ret = mpTv->mFactoryMode.fbcGetContrast();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_SATURATION: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetSaturation(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_SATURATION: {
        int ret = mpTv->mFactoryMode.fbcGetSaturation();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_HUE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetHueColorTint(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_HUE: {
        int ret = mpTv->mFactoryMode.fbcGetHueColorTint();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_BACKLIGHT: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetBacklight(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BACKLIGHT: {
        int ret = mpTv->mFactoryMode.fbcGetBacklight();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_FBC_SET_BACKLIGHT_EN : {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcBacklightOnOffSet(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BACKLIGHT_EN: {
        int ret = mpTv->mFactoryMode.fbcBacklightOnOffGet();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_FBC_SET_LVDS_SSG: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcLvdsSsgSet(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_ELEC_MODE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetElecMode(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_ELEC_MODE: {
        int ret = mpTv->mFactoryMode.fbcGetElecMode();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_PIC_MODE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetPictureMode(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_PIC_MODE: {
        int ret = mpTv->mFactoryMode.fbcGetPictureMode();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_TEST_PATTERN: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetTestPattern(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_TEST_PATTERN: {
        int ret = mpTv->mFactoryMode.fbcGetTestPattern();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_GAIN_RED: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetGainRed(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_GAIN_RED: {
        int ret = mpTv->mFactoryMode.fbcGetGainRed();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_GAIN_GREEN: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetGainGreen(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_GAIN_GREEN: {
        int ret = mpTv->mFactoryMode.fbcGetGainGreen();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_GAIN_BLUE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetGainBlue(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_GAIN_BLUE: {
        int ret = mpTv->mFactoryMode.fbcGetGainBlue();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_OFFSET_RED: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetOffsetRed(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_OFFSET_RED: {
        int ret = mpTv->mFactoryMode.fbcGetOffsetRed();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_OFFSET_GREEN: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetOffsetGreen(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_OFFSET_GREEN: {
        int ret = mpTv->mFactoryMode.fbcGetOffsetGreen();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_OFFSET_BLUE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetOffsetBlue(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_OFFSET_BLUE: {
        int ret = mpTv->mFactoryMode.fbcGetOffsetBlue();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_COLORTEMP_MODE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcColorTempModeSet(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_COLORTEMP_MODE: {
        int ret = mpTv->mFactoryMode.fbcColorTempModeGet();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_WB_INIT: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcWBInitialSet(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_WB_INIT: {
        int ret = mpTv->mFactoryMode.fbcWBInitialGet();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_MAINCODE_VERSION: {
        char sw_version[64];
        char build_time[64];
        char git_version[64];
        char git_branch[64];
        char build_name[64];
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            fbcIns->cfbc_Get_FBC_MAINCODE_Version(COMM_DEV_SERIAL, sw_version, build_time, git_version, git_branch, build_name);
            r->writeString16(String16(sw_version));
            r->writeString16(String16(build_time));
            r->writeString16(String16(git_version));
            r->writeString16(String16(git_branch));
            r->writeString16(String16(build_name));
        } else {
            r->writeString16(String16("No FBC"));
            r->writeString16(String16("No FBC"));
            r->writeString16(String16("No FBC"));
            r->writeString16(String16("No FBC"));
            r->writeString16(String16("No FBC"));
        }
        break;
    }
    case FACTORY_SET_SN: {
        char StrFactSN[256] = {0};
        String16 strTemFactorySn = p.readString16();
        String8 strFactorySn = String8(strTemFactorySn);
        sprintf((char *)StrFactSN, "%s", strFactorySn.string());
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int iRet = fbcIns->cfbc_Set_FBC_Factory_SN(COMM_DEV_SERIAL, (const char *)StrFactSN);
            r->writeInt32(iRet);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_GET_SN: {
        char factorySerialNumber[256] = {0};
        memset((void *)factorySerialNumber, 0, 256);
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            fbcIns->cfbc_Get_FBC_Factory_SN(COMM_DEV_SERIAL, factorySerialNumber);
            r->writeString16(String16(factorySerialNumber));
        } else {
            r->writeString16(String16("No FBC"));
        }
        break;
    }
    case FACTORY_FBC_PANEL_GET_INFO: {
        char panel_model[64];
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            fbcIns->cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_SERIAL, panel_model);
            r->writeString16(String16(panel_model));
        } else {
            r->writeString16(String16(""));
        }
        break;
    }
    case FACTORY_FBC_PANEL_POWER_SWITCH: {
        int value = p.readInt32();
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int ret = fbcIns->cfbc_Set_FBC_panel_power_switch(COMM_DEV_SERIAL, value);
            r->writeInt32(ret);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_FBC_PANEL_SUSPEND: {
        int value = p.readInt32();
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int ret = fbcIns->cfbc_Set_FBC_suspend(COMM_DEV_SERIAL, value);
            r->writeInt32(ret);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_FBC_PANEL_USER_SETTING_DEFAULT: {
        int value = p.readInt32();
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int ret = fbcIns->cfbc_Set_FBC_User_Setting_Default(COMM_DEV_SERIAL, value);
            r->writeInt32(ret);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GAIN_RED: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainRedSet(tv_source_input, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GAIN_RED: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainRedGet(tv_source_input, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GAIN_GREEN: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainGreenSet(tv_source_input, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GAIN_GREEN: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainGreenGet(tv_source_input, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GAIN_BLUE: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainBlueSet(tv_source_input, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GAIN_BLUE: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainBlueGet(tv_source_input, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_OFFSET_RED: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetRedSet(tv_source_input, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_OFFSET_RED: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetRedGet(tv_source_input, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_OFFSET_GREEN: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetGreenSet(tv_source_input, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_OFFSET_GREEN: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetGreenGet(tv_source_input, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_OFFSET_BLUE: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetBlueSet(tv_source_input, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_OFFSET_BLUE: {
        int tv_source_input = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetBlueGet(tv_source_input, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_COLOR_TMP: {
        int tv_source_input = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceColorTempModeGet(tv_source_input);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_COLOR_TMP: {
        int tv_source_input = p.readInt32();
        int Tempmode = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceColorTempModeSet(tv_source_input, Tempmode, is_save);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SAVE_PRAMAS: {
        int tv_source_input = p.readInt32();
        int mode = p.readInt32();
        int r_gain = p.readInt32();
        int g_gain = p.readInt32();
        int b_gain = p.readInt32();
        int r_offset = p.readInt32();
        int g_offset = p.readInt32();
        int b_offset = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalancePramSave(tv_source_input, mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_OPEN_GRAY_PATTERN: {
        int ret = mpTv->mFactoryMode.whiteBalanceGrayPatternOpen();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_CLOSE_GRAY_PATTERN: {
        int ret = mpTv->mFactoryMode.whiteBalanceGrayPatternClose();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GRAY_PATTERN: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGrayPatternSet(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GRAY_PATTERN: {
        int ret = mpTv->mFactoryMode.whiteBalanceGrayPatternGet();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_ALL_PRAMAS: {
        int mode = p.readInt32();
        tcon_rgb_ogo_t params;
        int ret = mpTv->mFactoryMode.getColorTemperatureParams((vpp_color_temperature_mode_t)mode, &params);
        r->writeInt32(ret);
        r->writeInt32(params.r_gain);
        r->writeInt32(params.g_gain);
        r->writeInt32(params.b_gain);
        r->writeInt32(params.r_post_offset);
        r->writeInt32(params.g_post_offset);
        r->writeInt32(params.b_post_offset);
        break;
    }
    case STOP_SCAN: {
        mpTv->stopScanLock();
        break;
    }
    case DTV_GET_SNR: {
        int tmpRet = mpTv->getFrontendSNR();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_BER: {
        int tmpRet = mpTv->getFrontendBER();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_STRENGTH: {
        int tmpRet = mpTv->getFrontendSignalStrength();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_AUDIO_TRACK_NUM: {
        int programId = p.readInt32();
        int retCnt = mpTv->getAudioTrackNum(programId);
        r->writeInt32(retCnt);
        break;
    }
    case DTV_GET_AUDIO_TRACK_INFO: {
        int progId = p.readInt32();
        int aIdx = p.readInt32();
        int aFmt = -1;
        String8 lang;
        int iRet = mpTv->getAudioInfoByIndex(progId, aIdx, &aFmt, lang);
        r->writeInt32(aFmt);
        r->writeString16(String16(lang));
        break;
    }
    case DTV_SWITCH_AUDIO_TRACK: {
        int aPid = p.readInt32();
        int aFmt = p.readInt32();
        int aParam = p.readInt32();
        int ret = mpTv->switchAudioTrack(aPid, aFmt, aParam);
        r->writeInt32(ret);
        break;
    }
    case DTV_SET_AUDIO_AD: {
        int aEnable = p.readInt32();
        int aPid = p.readInt32();
        int aFmt = p.readInt32();
        int ret = mpTv->setAudioAD(aEnable, aPid, aFmt);
        r->writeInt32(ret);
        break;
    }
    case DTV_GET_CURR_AUDIO_TRACK_INDEX: {
        int currAduIdx = -1;
        int progId = p.readInt32();
        CTvProgram prog;
        CTvProgram::selectByID(progId, prog);
        currAduIdx = prog.getCurrAudioTrackIndex();
        r->writeInt32(currAduIdx);
        break;
    }
    case DTV_SET_AUDIO_CHANNEL_MOD: {
        int audioChannelIdx = p.readInt32();
        mpTv->setAudioChannel(audioChannelIdx);
        break;
    }
    case DTV_GET_AUDIO_CHANNEL_MOD: {
        int currChannelMod = mpTv->getAudioChannel();
        r->writeInt32(currChannelMod);
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
        r->writeInt32(freq);
        break;
    }
    case DTV_GET_EPG_UTC_TIME: {
        int utcTime = mpTv->getTvTime();
        r->writeInt32(utcTime);
        break;
    }
    case DTV_GET_EPG_INFO_POINT_IN_TIME: {
        int progid = p.readInt32();
        int utcTime = p.readInt32();
        CTvProgram prog;
        int ret = CTvProgram::selectByID(progid, prog);
        CTvEvent ev;
        ret = ev.getProgPresentEvent(prog.getSrc(), prog.getID(), utcTime, ev);
        r->writeString16(String16(ev.getName()));
        r->writeString16(String16(ev.getDescription()));
        r->writeString16(String16(ev.getExtDescription()));
        r->writeInt32(ev.getStartTime());
        r->writeInt32(ev.getEndTime());
        r->writeInt32(ev.getSubFlag());
        r->writeInt32(ev.getEventId());
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

        r->writeInt32(iObOutSize);
        for (int i = 0; i < iObOutSize; i ++) {
            r->writeString16(String16(epgOut[i]->getName()));
            r->writeString16(String16(epgOut[i]->getDescription()));
            r->writeString16(String16(ev.getExtDescription()));
            r->writeInt32(epgOut[i]->getStartTime());
            r->writeInt32(epgOut[i]->getEndTime());
            r->writeInt32(epgOut[i]->getSubFlag());
            r->writeInt32(epgOut[i]->getEventId());
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
    case GET_AVERAGE_LUMA: {
        int ret = mpTv->getAverageLuma();
        r->writeInt32(ret);
        break;
    }
    case GET_AUTO_BACKLIGHT_DATA: {
        int buf[128] = {0};
        int size = mpTv->getAutoBacklightData(buf);
        r->writeInt32(size);
        for (int i = 0; i < size; i++) {
            r->writeInt32(buf[i]);
        }
        break;
    }
    case SET_AUTO_BACKLIGHT_DATA: {
        int ret = mpTv->setAutobacklightData(String8(p.readString16()));
        r->writeInt32(ret);
        break;
    }

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
        int ret = CTvProgram::selectByID(progid, prog);
        if (ret != 0) return -1;
        CTvChannel channel;
        prog.getChannel(channel);
        freq = channel.getFrequency();
        r->writeInt32(freq);
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
        r->writeInt32(iObOutSize);
        for (int i = 0; i < iObOutSize; i ++) {
            r->writeString16(String16(vTvBookOut[i]->getProgName()));
            r->writeString16(String16(vTvBookOut[i]->getEvtName()));
            r->writeInt32(vTvBookOut[i]->getStartTime());
            r->writeInt32(vTvBookOut[i]->getDurationTime());
            r->writeInt32(vTvBookOut[i]->getBookId());
            r->writeInt32(vTvBookOut[i]->getProgramId());
            r->writeInt32(vTvBookOut[i]->getEventId());
        }
        break;
    }
    case SET_FRONTEND_PARA: {
        int ret = -1;
        frontend_para_set_t feParms;
        feParms.mode = (fe_type_t)p.readInt32();
        feParms.freq = p.readInt32();
        feParms.videoStd = (atv_video_std_t)p.readInt32();
        feParms.audioStd = (atv_audio_std_t)p.readInt32();
        feParms.vfmt = p.readInt32();
        feParms.soundsys = p.readInt32();
        feParms.para1 = p.readInt32();
        feParms.para2 = p.readInt32();
        mpTv->resetFrontEndPara(feParms);
        r->writeInt32(ret);
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
        break;
    }
    case DTV_GET_VIDEO_FMT_INFO: {
        int srcWidth = 0;
        int srcHeight = 0;
        int srcFps = 0;
        int srcInterlace = 0;
        int iRet = -1;

        iRet == mpTv->getVideoFormatInfo(&srcWidth, &srcHeight, &srcFps, &srcInterlace);
        r->writeInt32(srcWidth);
        r->writeInt32(srcHeight);
        r->writeInt32(srcFps);
        r->writeInt32(srcInterlace);
        r->writeInt32(iRet);
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
    }
    break;

    case HDMIAV_HOTPLUGDETECT_ONOFF: {
        int flag = mpTv->GetHdmiAvHotplugDetectOnoff();
        r->writeInt32(flag);
        break;
    }
    case HANDLE_GPIO: {
        String8 strName(p.readString16());
        int is_out = p.readInt32();
        int edge = p.readInt32();
        int ret = mpTv->handleGPIO((char *)strName.string(), is_out, edge);
        r->writeInt32(ret);
        break;
    }
    case SET_LCD_ENABLE: {
        int enable = p.readInt32();
        int ret = mpTv->setLcdEnable(enable);
        r->writeInt32(ret);
        break;
    }
    case GET_ALL_TV_DEVICES: {
        const char *value = config_get_str(CFG_SECTION_TV, CGF_DEFAULT_INPUT_IDS, "null");
        r->writeCString(value);
        break;
    }
    case GET_HDMI_PORTS: {
        int source_input = p.readInt32();
        tvin_port_t port = mpTv->Tv_GetHdmiPortBySourceInput((tv_source_input_t)source_input);
        int value = (port & 0x0f) + 1;
        r->writeInt32(value);
        break;
    }
    case TV_CLEAR_FRONTEND: {
        int para = p.readInt32();
        int tmpRet = mpTv->clearFrontEnd(para);
        r->writeInt32(tmpRet);
        break;
    }
    case TV_SET_FRONTEND: {
        int force = p.readInt32();
        String8 feparas(p.readString16());
        int tmpRet = mpTv->setFrontEnd(feparas, (force != 0));
        r->writeInt32(tmpRet);
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
        int ret = mpTv->Scan(feparas.string(), scanparas.string());
        mTvService->mpScannerClient = this;
        r->writeInt32(ret);
        break;
    }
    case SET_AUDIO_OUTMODE: {
        int mode = p.readInt32();
        int ret = SetAudioOutmode(mode);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_OUTMODE: {
        int ret = GetAudioOutmode();
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_STREAM_OUTMODE: {
        int ret = GetAudioStreamOutmode();
        r->writeInt32(ret);
        break;
    }
    case SET_AMAUDIO_VOLUME: {
        break;
    }
    case GET_AMAUDIO_VOLUME: {
        break;
    }
    case SAVE_AMAUDIO_VOLUME: {
        break;
    }
    case GET_SAVE_AMAUDIO_VOLUME: {
        break;
    }
    // EXTAR END

    //NEWPLAY/RECORDING
    case DTV_RECORDING_CMD: {
        int ret = mpTv->doRecordingCommand(p.readInt32(), String8(p.readString16()).string(), String8(p.readString16()).string());
        r->writeInt32(ret);
    }break;
    case DTV_PLAY_CMD: {
        int ret = mpTv->doPlayCommand(p.readInt32(), String8(p.readString16()).string(), String8(p.readString16()).string());
        r->writeInt32(ret);
    }break;

    default:
        LOGD("default");
        break;
    }

    LOGD("exit client=%d cmd=%d", getCallingPid(), cmd);
    return 0;
}

int TvService::Client::notifyCallback(const int &msgtype, const Parcel &p)
{
    mTvClient->notifyCallback(msgtype, p);
    return 0;
}

status_t TvService::dump(int fd, const Vector<String16>& args)
{
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

            mpTv->dump(result);
        }
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

