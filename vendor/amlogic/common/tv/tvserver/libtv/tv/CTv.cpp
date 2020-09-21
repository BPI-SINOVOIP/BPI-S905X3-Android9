/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTv"
#define UNUSED(x) (void)x

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include <dlfcn.h>
#include <cutils/properties.h>
#include <cutils/android_reboot.h>
#include <utils/threads.h>
#include <time.h>
#include <sys/prctl.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef ANDROID
#include <termios.h>
#endif
#include <string.h>
#include <signal.h>
#include <hardware_legacy/power.h>
#include <media/AudioSystem.h>
#include <sys/wait.h>

#include <tvutils.h>
#include <tvconfig.h>
#include <tvscanconfig.h>
#include <CFile.h>
#include <serial_operate.h>

#include "CTvDatabase.h"
#include "../version/version.h"
#include "../tvsetting/CTvSetting.h"

#include <hardware_legacy/power.h>

#ifdef SUPPORT_ADTV
#include <am_epg.h>
#include <am_mem.h>

extern "C" {
#include "am_ver.h"
#include "am_misc.h"
#include "am_debug.h"
#include "am_fend.h"
}
#endif

#include <math.h>
#include <sys/ioctl.h>

#include "CTv.h"

using namespace android;

CTv::CTv():mTvDmx(0), mTvDmx1(1), mTvDmx2(2), mTvMsgQueue(this)
{
    /*copy file to param
    char buf[PROPERTY_VALUE_MAX] = {0};
    int len = property_get("tv.tvconfig.force_copy", buf, "true");*/

    if (isFileExist(TV_RRT_DEFINE_SYSTEM_PATH)) {
        if (!isFileExist(TV_RRT_DEFINE_PARAM_PATH)) {
            CFile file ( TV_RRT_DEFINE_SYSTEM_PATH );

            if ( file.copyTo ( TV_RRT_DEFINE_PARAM_PATH ) != 0 ) {
                LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_RRT_DEFINE_PARAM_PATH );
            }

            char sysCmd[1024];
            sprintf(sysCmd, "chmod 644 %s", TV_RRT_DEFINE_PARAM_PATH);
            if (system(sysCmd)) {
               LOGE("exec cmd:%s fail\n", sysCmd);
            }
        }
    }

#ifdef SUPPORT_ADTV
    AM_EVT_Init();
#endif
    mpObserver = NULL;
    mAutoSetDisplayFreq = false;
    mPreviewEnabled = false;
    mTvMsgQueue.startMsgQueue();
    tv_config_load (TV_CONFIG_FILE_PATH);
    tv_scan_config_load(TV_SCAN_CONFIG_FILE_PATH);
    mFrontDev = CFrontEnd::getInstance();
    mpTvin = CTvin::getInstance();
    mTvScanner = CTvScanner::getInstance();
    mTvRrt = CTvRrt::getInstance();
    mTvRrt->setObserver(&mTvMsgQueue);
    sqlite3_config (SQLITE_CONFIG_SERIALIZED);
    sqlite3_soft_heap_limit(8 * 1024 * 1024);
    // Initialize SQLite.
    sqlite3_initialize();
    CTvDatabase::GetTvDb()->InitTvDb ( TV_DB_PATH );

    if ( CTvDimension::isDimensionTblExist() == false ) {
        CTvDimension::builtinAtscDimensions();
    }

    mFactoryMode.init();
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;

    SetHdmiEdidForUboot();
    mSetHdmiEdid = false;

    mDevicesPollStatusDetectThread.setObserver ( this );
    mFrontDev->setObserver ( &mTvMsgQueue );
    mTvEas = CTvEas::GetInstance();
    mTvEas->setObserver(&mTvMsgQueue);
    mSubtitle.setObserver(this);
    mHeadSet.setObserver(this);
    mAv.setObserver(&mTvMsgQueue);

    mTvAction &= TV_ACTION_NULL;
    mTvStatus = TV_INIT_ED;
    pGpio = new CTvGpio();
    print_version_info();
    mbSameSourceEnableStatus = false;
}

CTv::~CTv()
{
    mpObserver = NULL;
    CTvDatabase::deleteTvDb();
    tv_config_unload();
    tv_scan_config_unload();
    mAv.Close();
    mTvStatus = TV_INIT_ED;
    mFrontDev->Close();

    if (pGpio != NULL) {
        delete pGpio;
        pGpio = NULL;
    }
}

void CTv::onEvent ( const CTvScanner::ScannerEvent &ev )
{
    LOGD ( "%s, CTv::onEvent lockStatus = %d type = %d\n", __FUNCTION__,  ev.mLockedStatus, ev.mType );

    if ( mDtvScanRunningStatus == DTV_SCAN_RUNNING_ANALYZE_CHANNEL ) {
        if ( ev.mType == CTvScanner::ScannerEvent::EVENT_SCAN_END ) {
            CMessage msg;
            msg.mType = CTvMsgQueue::TV_MSG_STOP_ANALYZE_TS;
            msg.mpData = this;
            mTvMsgQueue.sendMsg ( msg );
        } else if ( ev.mType == CTvScanner::ScannerEvent::EVENT_STORE_END ) {
            CTvEpg::EpgEvent epgev;
            epgev.type = CTvEpg::EpgEvent::EVENT_CHANNEL_UPDATE_END;
            mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
            sendTvEvent ( epgev );
        }
    } else {
        sendTvEvent ( ev );
    }
}

void CTv::onEvent ( const CTvEas::EasEvent &ev )
{
    LOGD("EAS event!\n");
    sendTvEvent(ev);
}

void CTv::onEvent ( const CFrontEnd::FEEvent &ev )
{
    LOGD ( "%s, FE event type = %d tvaction=%x", __FUNCTION__, ev.mCurSigStaus, mTvAction);
    if (mTvAction & TV_ACTION_SCANNING) return;


    if ( ev.mCurSigStaus == CFrontEnd::FEEvent::EVENT_FE_HAS_SIG ) {
        LOGD("onEvent fe lock");
        if (/*m_source_input == SOURCE_TV || */m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //atv and other tvin source    not to use it, and if not playing, not use have sig
            TvEvent::SignalInfoEvent ev;
            ev.mStatus = TVIN_SIG_STATUS_STABLE;
            ev.mTrans_fmt = TVIN_TFMT_2D;
            ev.mFmt = TVIN_SIG_FMT_NULL;
            ev.mReserved = 0;
            sendTvEvent ( ev );
        }
    } else if ( ev.mCurSigStaus == CFrontEnd::FEEvent::EVENT_FE_NO_SIG ) {
        LOGD("onEvent fe unlock");
        if ((!(mTvAction & TV_ACTION_STOPING)) && m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //just playing
            if (iSBlackPattern) {
                mAv.DisableVideoWithBlackColor();
            } else {
                mAv.DisableVideoWithBlueColor();
            }
            TvEvent::SignalInfoEvent ev;
            ev.mStatus = TVIN_SIG_STATUS_NOSIG;
            ev.mTrans_fmt = TVIN_TFMT_2D;
            ev.mFmt = TVIN_SIG_FMT_NULL;
            ev.mReserved = 0;
            sendTvEvent ( ev );
        }
    } else if (ev.mCurSigStaus == CFrontEnd::FEEvent::EVENT_VLFE_HAS_SIG) {
        LOGD("onEvent vlfe lock");
    } else if (ev.mCurSigStaus == CFrontEnd::FEEvent::EVENT_VLFE_NO_SIG) {
        LOGD("onEvent vlfe unlock");
    }
}

void CTv::onEvent ( const CTvEpg::EpgEvent &ev )
{
    switch ( ev.type ) {
    case CTvEpg::EpgEvent::EVENT_TDT_END:
        LOGD ( "%s, CTv::onEvent epg time = %ld", __FUNCTION__, ev.time );
        mTvTime.setTime ( ev.time );
        break;

    case CTvEpg::EpgEvent::EVENT_CHANNEL_UPDATE: {
        LOGD ( "%s, CTv:: onEvent channel update", __FUNCTION__ );
        CMessage msg;
        msg.mType = CTvMsgQueue::TV_MSG_START_ANALYZE_TS;
        msg.mpData = this;
        mCurAnalyzeTsChannelID = ev.channelID;
        mTvMsgQueue.sendMsg ( msg );
        break;
    }

    default:
        break;
    }

    sendTvEvent ( ev );
}

void CTv::onEvent(const CTvRecord::RecEvent &ev)
{
    LOGD ( "RecEvent = %d", ev.type);
    switch ( ev.type ) {
        case CTvRecord::RecEvent::EVENT_REC_START: {
            TvEvent::RecorderEvent RecorderEvent;
            RecorderEvent.mStatus = TvEvent::RecorderEvent::EVENT_RECORD_START;
            RecorderEvent.mError = (int)ev.error;
            RecorderEvent.mId = String8(ev.id.c_str());
            sendTvEvent(RecorderEvent);
            }break;
        case CTvRecord::RecEvent::EVENT_REC_STOP: {
            TvEvent::RecorderEvent RecorderEvent;
            RecorderEvent.mStatus = TvEvent::RecorderEvent::EVENT_RECORD_STOP;
            RecorderEvent.mError = (int)ev.error;
            RecorderEvent.mId = String8(ev.id.c_str());
            sendTvEvent(RecorderEvent);
            }break;
        default:
            break;
    }
}

void CTv::onEvent (const CTvRrt::RrtEvent &ev)
{
    LOGD("RRT event!\n");
    sendTvEvent ( ev );
}

void CTv::onEvent(const CAv::AVEvent &ev)
{
    LOGD ( "onEvent AVEvent = %d", ev.type);
    switch ( ev.type ) {
    case CAv::AVEvent::EVENT_AV_STOP: {
        if ( mTvAction & TV_ACTION_STOPING ) {
            LOGD("tv stopping, no need CAv::AVEvent::EVENT_AV_STOP");
            break;
        }
        if (iSBlackPattern) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }

        TvEvent::SignalInfoEvent ev;
        ev.mStatus = TVIN_SIG_STATUS_NOSIG;
        ev.mTrans_fmt = TVIN_TFMT_2D;
        ev.mFmt = TVIN_SIG_FMT_NULL;
        ev.mReserved = 0;
        sendTvEvent ( ev );
        break;
    }

    case CAv::AVEvent::EVENT_AV_RESUEM: {
        TvEvent::AVPlaybackEvent AvPlayBackEvt;
        AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_PLAYBACK_RESUME;
        AvPlayBackEvt.mProgramId = (int)ev.param;
        sendTvEvent(AvPlayBackEvt);
        break;
    }

    case CAv::AVEvent::EVENT_AV_SCAMBLED: {
        if (iSBlackPattern) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        TvEvent::AVPlaybackEvent AvPlayBackEvt;
        AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_SCAMBLED;
        AvPlayBackEvt.mProgramId = (int)ev.param;
        sendTvEvent(AvPlayBackEvt);
        break;
    }
    case CAv::AVEvent::EVENT_AV_VIDEO_AVAILABLE: {
        LOGD("EVENT_AV_VIDEO_AVAILABLE, video available");
        if (m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //atv and other tvin source    not to use it, and if not playing, not use have sig
            if ( m_win_mode == PREVIEW_WONDOW ) {
                mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
            }
            if (mAutoSetDisplayFreq && !mPreviewEnabled) {
                mpTvin->VDIN_SetDisplayVFreq(50);
            }
        }
        isVideoFrameAvailable();
        TvEvent::AVPlaybackEvent AvPlayBackEvt;
        AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_VIDEO_AVAILABLE;
        AvPlayBackEvt.mProgramId = (int)ev.param;
        sendTvEvent(AvPlayBackEvt);
        break;
    }
    case CAv::AVEvent::EVENT_AV_UNSUPPORT: {
        LOGD("To AVS or AVS+ format");//just avs format,  not unsupport, and avs avs+
        break;
    }
    case CAv::AVEvent::EVENT_PLAY_UPDATE: {
        //fix me: get the player, [only one player can run concurrently]
        //the av event should be handled by player..
        //need refactoring here
        CDTVTvPlayer *player = (CDTVTvPlayer *)PlayerManager::getInstance().getFirstDevWithIdContains("atsc");
        if (player)
            player->onPlayUpdate(ev);
        else {
            LOGD("no atsc player found");
        }
        break;
    }
    case CAv::AVEvent::EVENT_AUDIO_CB: {
        TvEvent::AVAudioCBEvent AvAudiocbEvt;

        LOGD("cmd:%d, param1:%d,param2:%d\n",ev.status,ev.param,ev.param1);

        AvAudiocbEvt.cmd = ev.status;
        AvAudiocbEvt.param1 = ev.param;
        AvAudiocbEvt.param2 = ev.param1;
        sendTvEvent(AvAudiocbEvt);
        break;
    }
    default:
        break;
    }
}

CTv::CTvMsgQueue::CTvMsgQueue(CTv *tv)
{
    mpTv = tv;
}

CTv::CTvMsgQueue::~CTvMsgQueue()
{
}

void CTv::CTvMsgQueue::handleMessage ( CMessage &msg )
{
    LOGD ("%s, CTv::CTvMsgQueue::handleMessage type = %d", __FUNCTION__,  msg.mType);

    switch ( msg.mType ) {
    case TV_MSG_COMMON:
        break;

    case TV_MSG_STOP_ANALYZE_TS:
        //mpTv->Tv_Stop_Analyze_Ts();
        break;

    case TV_MSG_START_ANALYZE_TS:
        //mpTv->Tv_Start_Analyze_Ts ( pTv->mCurAnalyzeTsChannelID );
        break;

    case TV_MSG_CHECK_FE_DELAY: {
        mpTv->mFrontDev->checkStatusOnce();
        break;
    }

    case TV_MSG_AV_EVENT: {
        mpTv->onEvent(*((CAv::AVEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_FE_EVENT: {
        mpTv->onEvent(*((CFrontEnd::FEEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_SCAN_EVENT: {
        mpTv->onEvent(*((CTvScanner::ScannerEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_EPG_EVENT: {
        mpTv->onEvent(*((CTvEpg::EpgEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_ENABLE_VIDEO_LATER: {
        //int fc = msg.mpPara[0];
        mpTv->isVideoFrameAvailable();
        break;
    }

    case TV_MSG_VIDEO_AVAILABLE_LATER: {
        int fc = msg.mpPara[0];
        mpTv->onVideoAvailableLater(fc);
        break;
    }

    case TV_MSG_RECORD_EVENT: {
        mpTv->onEvent(*((CTvRecord::RecEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_RRT_EVENT: {
        mpTv->onEvent(*((CTvRrt::RrtEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_EAS_EVENT: {
        mpTv->onEvent(*((CTvEas::EasEvent *)(msg.mpPara)));
        break;
    }

    default:
        break;
    }
}

void CTv::CTvMsgQueue::onEvent ( const CTvScanner::ScannerEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_SCAN_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CFrontEnd::FEEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_FE_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CTvEpg::EpgEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_EPG_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent(const CAv::AVEvent &ev)
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_AV_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent(const CTvRecord::RecEvent &ev)
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_RECORD_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CTvRrt::RrtEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_RRT_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CTvEas::EasEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_EAS_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

int CTv::setTvObserver ( TvIObserver *ob )
{
    mpObserver = ob;
    return 0;
}

void CTv::sendTvEvent ( const CTvEv &ev )
{
    /*if (ev.getEvType() == CTvEv::TV_EVENT_SIGLE_DETECT) {
        TvEvent::SignalInfoEvent *pEv = (TvEvent::SignalInfoEvent *)(&ev);
    }*/

    LOGD ( "%s, tvaction=%x", __FUNCTION__, mTvAction);
    if ((mTvAction & TV_ACTION_SCANNING) && (ev.getEvType() == CTvEv::TV_EVENT_SIGLE_DETECT)) {
        LOGD("%s, when scanning, not send sig detect event", __FUNCTION__);
        return;
    }
    if ( mpObserver != NULL ) {
        mpObserver->onTvEvent ( ev );
    }
}

int CTv::ClearAnalogFrontEnd()
{
    return mFrontDev->ClearAnalogFrontEnd ();
}

int CTv::clearFrontEnd(int para)
{
    return mFrontDev->setPara(para, 470000000, 0, 0);
}

int CTv::Scan(const char *feparas, const char *scanparas) {
    AutoMutex _l(mLock);
    mTvAction = mTvAction | TV_ACTION_SCANNING;
    LOGD("%s: mTvAction = 0x%x\n", __FUNCTION__, mTvAction);
    LOGD("%s: fe[%s], scan[%s]!\n", __FUNCTION__, feparas, scanparas);

    mAv.StopTS();
    mAv.DisableVideoWithBlackColor();
    mpTvin->Tvin_StopDecoder();

    CTvScanner::ScanParas sp(scanparas);
    CFrontEnd::FEParas fp(feparas);

    int AtvMode = sp.getAtvMode();
    int DtvMode = sp.getDtvMode();
    LOGD("%s: AtvMode = %d, DtvMode = %d\n", __FUNCTION__, AtvMode, DtvMode);
    if (AtvMode != TV_SCAN_ATVMODE_NONE) {
        m_source_input = SOURCE_TV;
    } else {
        m_source_input = SOURCE_INVALID;
    }

    if (AtvMode & TV_SCAN_ATVMODE_AUTO) {
        LOGD("%s: clean all ATV channal!\n", __FUNCTION__);
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_ATV );
    }

    if (DtvMode & TV_SCAN_DTVMODE_MANUAL) {
        LOGD("%s: clean appointed DTV channal!\n", __FUNCTION__);
        CTvChannel::DeleteBetweenFreq(sp.getDtvFrequency1(), sp.getDtvFrequency2());
    } else {
        LOGD("%s: clean All DTV channal!\n", __FUNCTION__);
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO );
        CTvEvent::CleanAllEvent();
    }

    mFrontDev->Open(TV_FE_AUTO);
    mTvScanner->setObserver(&mTvMsgQueue);
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
    mTvScanner->SetCurrentLanguage(GetCurrentLanguage());

    return mTvScanner->Scan(fp, sp);
}

int CTv::dtvScan(int mode, int scan_mode, int beginFreq, int endFreq, int para1, int para2) {
    AutoMutex _l(mLock);
    mTvAction = mTvAction | TV_ACTION_SCANNING;
    LOGD("mTvAction = %#x, %s", mTvAction, __FUNCTION__);
    LOGD("mode[%#x], scan_mode[%#x], freq[%d-%d], para[%d,%d] %s",
        mode, scan_mode, beginFreq, endFreq, para1, para2, __FUNCTION__);
    //mTvEpg.leaveChannel();
    mAv.StopTS();
    mAv.DisableVideoWithBlackColor();

    if (scan_mode == TV_SCAN_DTVMODE_MANUAL) {
        CTvChannel::DeleteBetweenFreq(beginFreq, endFreq);
    } else {
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO );
        CTvEvent::CleanAllEvent();
    }
    mTvScanner->setObserver(&mTvMsgQueue);
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
    mTvScanner->SetCurrentLanguage(GetCurrentLanguage());

    return mTvScanner->dtvScan(mode, scan_mode, beginFreq, endFreq, para1, para2);
}

int CTv::dtvMode(const char *mode)
{
    if ( strcmp ( mode, DTV_DTMB_MODE ) == 0 )
        return (TV_FE_DTMB);
    else if ( strcmp ( mode, DTV_DVBC_MODE ) == 0 )
        return (TV_FE_QAM);
    else if ( strcmp ( mode, DTV_DVBS_MODE ) == 0 )
        return (TV_FE_QPSK);
    else if ( strcmp ( mode, DTV_ATSC_MODE ) == 0 )
        return (TV_FE_ATSC);
    else if ( strcmp ( mode, DTV_DVBT_MODE ) == 0 )
        return (TV_FE_OFDM);
    else if ( strcmp ( mode, DTV_ISDBT_MODE ) == 0 )
        return (TV_FE_ISDBT);

    return (TV_FE_DTMB);
}

int CTv::dtvAutoScan()
{
    const char *dtv_mode = config_get_str ( CFG_SECTION_TV, CFG_DTV_MODE, DTV_DTMB_MODE);
    return dtvScan(dtvMode(dtv_mode), TV_SCAN_DTVMODE_ALLBAND, 0, 0, -1, -1);
}

int CTv::dtvCleanProgramByFreq ( int freq )
{
    int iOutRet = 0;
    CTvChannel channel;
    iOutRet = CTvChannel::SelectByFreq ( freq, channel );
    if ( -1 == iOutRet ) {
        LOGD("%s, Warnning: Current Freq not have program info in the ts_table\n", __FUNCTION__);
    }

    iOutRet = CTvProgram::deleteChannelsProgram ( channel );
    return iOutRet;
}

int CTv::dtvManualScan (int beginFreq, int endFreq, int modulation)
{
    const char *dtv_mode = config_get_str ( CFG_SECTION_TV, CFG_DTV_MODE, "dtmb");
    return dtvScan(dtvMode(dtv_mode), TV_SCAN_DTVMODE_MANUAL, beginFreq, endFreq, modulation, -1);

}

//searchType 0:not 256 1:is 256 Program
int CTv::atvAutoScan(int videoStd, int audioStd, int searchType, int procMode)
{
    int minScanFreq, maxScanFreq, vStd, aStd;
    AutoMutex _l( mLock );
    if (!mATVDisplaySnow)
        mAv.DisableVideoWithBlackColor();
    mTvAction |= TV_ACTION_SCANNING;
    stopPlaying(false);
    mTvScanner->setObserver ( &mTvMsgQueue );
    getATVMinMaxFreq (&minScanFreq, &maxScanFreq );
    if ( minScanFreq == 0 || maxScanFreq == 0 || minScanFreq > maxScanFreq ) {
        LOGE ( "%s, auto scan  freq set is error min=%d, max=%d", __FUNCTION__, minScanFreq, maxScanFreq );
        return -1;
    }
    if ( !mATVDisplaySnow ) {
        mpTvin->Tvin_StopDecoder();
    } else {
        SetSnowShowEnable( true );
    }

    vStd = videoStd;
    aStd = audioStd;
    tvin_port_t source_port = mpTvin->Tvin_GetSourcePortBySourceInput(SOURCE_TV);
    mpTvin->VDIN_OpenPort ( source_port );
    unsigned long stdAndColor = mFrontDev->enumToStdAndColor(vStd, aStd);

    //int fmt = CFrontEnd::stdEnumToCvbsFmt (0, 0);
    mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) TVIN_SIG_FMT_NULL );

    if (searchType == 0) {
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_ATV );
    } else if (searchType == 1) { //type for skyworth, and insert 256 prog, and just update ts table
    }
    minScanFreq = mFrontDev->formatATVFreq ( minScanFreq );
    maxScanFreq = mFrontDev->formatATVFreq ( maxScanFreq );
    LOGD("%s, atv auto scan vstd=%d, astd=%d stdandcolor=0x%x", __FUNCTION__, vStd, aStd, stdAndColor);
    mTvScanner->SetCurrentLanguage(GetCurrentLanguage());

    mTvScanner->autoAtvScan ( minScanFreq, maxScanFreq, stdAndColor, searchType, procMode);
    return 0;
}

int CTv::atvAutoScan(int videoStd __unused, int audioStd __unused, int searchType)
{
    return atvAutoScan(videoStd, audioStd, searchType, 0);
}

int CTv::clearAllProgram(int arg0 __unused)
{
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_ATV );
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_TV );
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO);
    return 0;
}

int CTv::atvMunualScan ( int startFreq, int endFreq, int videoStd, int audioStd,
                         int store_Type , int channel_num )
{
    int minScanFreq, maxScanFreq, vStd, aStd;

    minScanFreq = mFrontDev->formatATVFreq ( startFreq );
    maxScanFreq = mFrontDev->formatATVFreq ( endFreq );
    if ( minScanFreq == 0 || maxScanFreq == 0 ) {
        LOGE ( "%s, munual scan  freq set is error min=%d, max=%d", __FUNCTION__, minScanFreq, maxScanFreq );
        return -1;
    }

    //if  set std null AUTO, use default PAL/DK
    //if(videoStd == CC_ATV_VIDEO_STD_AUTO) {
    //    vStd = CC_ATV_VIDEO_STD_PAL;
    //    aStd = CC_ATV_AUDIO_STD_DK;
    // } else {
    vStd = videoStd;
    aStd = audioStd;
    // }

    AutoMutex _l( mLock );
    if ( !mATVDisplaySnow )
        mAv.DisableVideoWithBlackColor();
    mTvAction |= TV_ACTION_SCANNING;
    mTvScanner->setObserver ( &mTvMsgQueue );
    unsigned long stdAndColor = mFrontDev->enumToStdAndColor(vStd, aStd);
    tvin_port_t source_port = mpTvin->Tvin_GetSourcePortBySourceInput(SOURCE_TV);
    mpTvin->VDIN_OpenPort ( source_port );

    //int fmt = CFrontEnd::stdEnumToCvbsFmt (0, 0);
    mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) TVIN_SIG_FMT_NULL );
    LOGD("%s, atv manual scan vstd=%d, astd=%d stdandcolor=0x%x", __FUNCTION__, vStd, aStd, stdAndColor);
    mTvScanner->SetCurrentLanguage(GetCurrentLanguage());

    return mTvScanner->ATVManualScan ( minScanFreq, maxScanFreq, stdAndColor, store_Type, channel_num);
}

int CTv::getVideoFormatInfo ( int *pWidth, int *pHeight, int *pFPS, int *pInterlace )
{
    int  iOutRet = -1;
    do {
        if ( NULL == pWidth || NULL == pHeight || NULL == pFPS || NULL == pInterlace ) {
            break;
        }

        iOutRet = mAv.GetVideoStatus (pWidth, pHeight, pFPS, pInterlace);

        if ( DVB_SUCCESS != iOutRet ) {
            LOGD ( "%s, ERROR: Cann't Get video format info\n", __FUNCTION__);
            break;
        }
        //LOGD("%s, w : %d h : %d fps : %d  interlaced: %d\n", __FUNCTION__, video_status.src_w,video_status.src_h,video_status.fps,video_status.interlaced);
    } while ( false );
    return iOutRet;
}

int CTv::getAudioFormatInfo ( int fmt[2], int sample_rate[2], int resolution[2], int channels[2],
    int lfepresent[2], int *frames, int *ab_size, int *ab_data, int *ab_free)
{
    int  iOutRet = -1;

    do {
        iOutRet = mAv.GetAudioStatus (fmt, sample_rate, resolution, channels, lfepresent, frames, ab_size, ab_data, ab_free);
        if ( DVB_SUCCESS != iOutRet ) {
            LOGD ( "%s, ERROR: Cann't Get audio format info\n", __FUNCTION__);
            break;
        }
    } while ( false );

    return iOutRet;
}

int CTv::stopScanLock()
{
    AutoMutex _l( mLock );
    return stopScan();
}

int CTv::stopScan()
{
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
        return 0;
    }

    LOGD("%s, tv scanning , stop it\n", __FUNCTION__);
    if ((SOURCE_TV == m_source_input) && mATVDisplaySnow) {
        SetSnowShowEnable( false );
        mAv.DisableVideoWithBlackColor();
    } else {
        mAv.DisableVideoWithBlackColor();
    }
    mpTvin->Tvin_StopDecoder();
    //mTvEpg.leaveChannel();
    mTvScanner->stopScan();
    mFrontDev->Close();
    m_source_input = SOURCE_INVALID;
    mTvAction &= ~TV_ACTION_SCANNING;
    return 0;
}

int CTv::pauseScan()
{
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
        return 0;
    }
    return mTvScanner->pauseScan();
}

int CTv::resumeScan()
{
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
        return 0;
    }
    return mTvScanner->resumeScan();
}

int CTv::getScanStatus()
{
    int status;
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
        return 0;
    }
    mTvScanner->getScanStatus(&status);
    return status;
}
void CTv::operateDeviceForScan(int type)
{
    LOGD("%s : type:%d\n", __FUNCTION__, type);
    if (type & OPEN_DEV_FOR_SCAN_ATV) {
            mFrontDev->Open(TV_FE_AUTO);
            mFrontDev->SetAnalogFrontEndTimerSwitch(1);
     }else if  (type & OPEN_DEV_FOR_SCAN_DTV) {
            mFrontDev->Open(TV_FE_AUTO);
            mFrontDev->SetAnalogFrontEndTimerSwitch(0);
     }else if  (type & CLOSE_DEV_FOR_SCAN) {
            mFrontDev->SetAnalogFrontEndTimerSwitch(0);
     }
}
void CTv::setDvbTextCoding(char *coding)
{
#ifdef SUPPORT_ADTV
    if (!strcmp(coding, "standard")) {
        AM_SI_SetDefaultDVBTextCoding("");
    } else {
        AM_SI_SetDefaultDVBTextCoding(coding);
    }
#endif
}

int CTv::playDvbcProgram ( int progId )
{
    LOGD("%s, progId = %d\n", __FUNCTION__,  progId );

    CTvProgram prog;
    int ret = CTvProgram::selectByID ( progId, prog );
    if ( ret != 0 ) {
        return -1;
    }

    LOGD("%s, prog name = %s\n", __FUNCTION__,  prog.getName().string() );
    CTvChannel channel;
    prog.getChannel ( channel );

    mFrontDev->setPara ( channel.getMode(), channel.getFrequency(), channel.getSymbolRate(), channel.getModulation() );

    int vpid = 0x1fff, apid = 0x1fff, vfmt = -1, afmt = -1;

    CTvProgram::Video *pV = prog.getVideo();
    if ( pV != NULL ) {
        vpid = pV->getPID();
        vfmt = pV->getFormat();
    }

    int aindex = prog.getCurrentAudio ( String8 ( "eng" ) );
    if ( aindex >= 0 ) {
        CTvProgram::Audio *pA = prog.getAudio ( aindex );
        if ( pA != NULL ) {
            apid = pA->getPID();
            afmt = pA->getFormat();
        }
    }
    mTvEpg.leaveProgram();
    mTvEpg.leaveChannel();
    startPlayTv ( SOURCE_DTV, vpid, apid, prog.getPcrId(), vfmt, afmt );

    mTvEpg.enterChannel ( channel.getID() );
    mTvEpg.enterProgram ( prog.getID() );
    return 0;
}

int CTv::getAudioTrackNum ( int progId )
{
    int iRet, iOutCnt = 0;
    CTvProgram prog;

    do {
        if ( 0 > progId ) {
            break;
        }

        iRet = CTvProgram::selectByID ( progId, prog );
        if ( 0 != iRet ) {
            break;
        }

        iOutCnt = prog.getAudioTrackSize();
    } while ( false );

    return iOutCnt;
}

int CTv::getAudioInfoByIndex ( int progId, int idx, int *pAFmt, String8 &lang )
{
    int iRet, iOutRet = -1;
    CTvProgram prog;
    CTvProgram::Audio *pA = NULL;

    do {
        if ( NULL == pAFmt || idx < 0 ) {
            break;
        }

        iRet = CTvProgram::selectByID ( progId, prog );
        if ( 0 != iRet ) {
            break;
        }

        if ( idx >= prog.getAudioTrackSize() ) {
            break;
        }

        pA = prog.getAudio ( idx );
        if ( NULL == pA ) {
            break;
        }

        *pAFmt = pA->getFormat();
        lang = pA->getLang();
    } while ( false );

    return iOutRet;
}

int CTv::switchAudioTrack (int aPid, int aFmt, int aParam __unused)
{
    int iOutRet = 0;
    do {
        if ( aPid < 0 || aFmt < 0 ) {
            break;
        }

        iOutRet = mAv.SwitchTSAudio (( unsigned short ) aPid, aFmt );
        LOGD ("%s, iOutRet = %d AM_AV_SwitchTSAudio\n", __FUNCTION__,  iOutRet );
    } while ( false );

    return iOutRet;
}

int CTv::switchAudioTrack ( int progId, int idx )
{
    int iOutRet = 0;
    CTvProgram prog;
    CTvProgram::Audio *pAudio = NULL;
    int aPid = -1;
    int aFmt = -1;

    do {
        if ( idx < 0 ) {
            break;
        }

        iOutRet = CTvProgram::selectByID ( progId, prog );
        if ( 0 != iOutRet ) {
            break;
        }
        /*if (prog.getAudioTrackSize() <= 1) {
            LOGD("%s, just one audio track, not to switch", __FUNCTION__);
            break;
        }*/
        if ( idx >= prog.getAudioTrackSize() ) {
            break;
        }

        pAudio = prog.getAudio ( idx );
        if ( NULL == pAudio ) {
            break;
        }

        aPid = pAudio->getPID();
        aFmt = pAudio->getFormat();
        if ( aPid < 0 || aFmt < 0 ) {
            break;
        }

        iOutRet = mAv.SwitchTSAudio (( unsigned short ) aPid, aFmt );
        LOGD ( "%s, iOutRet = %d AM_AV_SwitchTSAudio\n", __FUNCTION__,  iOutRet );
    } while ( false );

    return iOutRet;
}

int CTv::ResetAudioDecoderForPCMOutput()
{
    int iOutRet = mAv.ResetAudioDecoder ();
    LOGD ( "%s, iOutRet = %d AM_AV_ResetAudioDecoder\n", __FUNCTION__,  iOutRet );
    return iOutRet;
}

int CTv::setAudioAD (int enable, int aPid, int aFmt)
{
    int iOutRet = 0;
    if ( aPid < 0 || aFmt < 0 ) {
        return iOutRet;
    }

    iOutRet = mAv.SetADAudio (enable, ( unsigned short ) aPid, aFmt );
    LOGD ("%s, iOutRet = %d (%d,%d,%d)\n", __FUNCTION__,  iOutRet, enable, aPid, aFmt );

    return iOutRet;
}

int CTv::playDtvProgramUnlocked (const char *feparas, int mode, int freq, int para1, int para2,
                          int vpid, int vfmt, int apid, int afmt, int pcr, int audioCompetation __unused)
{
    int ret = 0;
    int FeMode = 0;
    LOGD("[%s] Start SwitchSourceTime = %fs", __FUNCTION__, getUptimeSeconds());

    CFrontEnd::FEParas fp(feparas);

    SetSourceSwitchInputLocked(m_source_input_virtual, SOURCE_DTV);

    FeMode = fp.getFEMode().getBase();
    LOGD("[%s] FeMode = %d", __FUNCTION__, FeMode);
    mFrontDev->Open(FeMode);
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        if (!feparas) {
            if ( SOURCE_ADTV == m_source_input_virtual ) {
                mFrontDev->setPara (FeMode, freq, para1, para2);
            } else {
                mFrontDev->setPara (mode, freq, para1, para2);
            }
        } else {
            mFrontDev->setPara(feparas);
        }
    }
    mTvAction |= TV_ACTION_PLAYING;
    ret = startPlayTv ( SOURCE_DTV, vpid, apid, pcr, vfmt, afmt );

    CMessage msg;
    msg.mDelayMs = 2000;
    msg.mType = CTvMsgQueue::TV_MSG_CHECK_FE_DELAY;
    mTvMsgQueue.sendMsg ( msg );
    LOGD("[%s] End SwitchSourceTime = %fs", __FUNCTION__, getUptimeSeconds());
    return 0;
}

int CTv::playDtvProgram (const char *feparas, int mode, int freq, int para1, int para2,
                          int vpid, int vfmt, int apid, int afmt, int pcr, int audioCompetation)
{
    AutoMutex _l( mLock );
    return playDtvProgramUnlocked(feparas, mode, freq, para1, para2,
                            vpid, vfmt, apid, afmt, pcr, audioCompetation);
}

int CTv::playDtvProgramUnlocked(int mode, int freq, int para1, int para2, int vpid, int vfmt, int apid,
        int afmt, int pcr, int audioCompetation) {
    return playDtvProgramUnlocked(NULL, mode, freq, para1, para2, vpid, vfmt, apid, afmt, pcr, audioCompetation);
}
int CTv::playDtvProgram(int mode, int freq, int para1, int para2, int vpid, int vfmt, int apid,
        int afmt, int pcr, int audioCompetation) {
    return playDtvProgram(NULL, mode, freq, para1, para2, vpid, vfmt, apid, afmt, pcr, audioCompetation);
}
int CTv::playDtvProgramUnlocked(const char* feparas, int vpid, int vfmt, int apid,
        int afmt, int pcr, int audioCompetation) {
    return playDtvProgramUnlocked(feparas, 0, 0, 0, 0, vpid, vfmt, apid, afmt, pcr, audioCompetation);
}
int CTv::playDtvProgram(const char* feparas, int vpid, int vfmt, int apid,
        int afmt, int pcr, int audioCompetation) {
    return playDtvProgram(feparas, 0, 0, 0, 0, vpid, vfmt, apid, afmt, pcr, audioCompetation);
}


int CTv::playDtvTimeShiftUnlocked (const char *feparas, void *para, int audioCompetation __unused)
{
    int ret = 0;
    int FeMode = 0;
    if (feparas) {
        SetSourceSwitchInputLocked(m_source_input_virtual, SOURCE_DTV);
        CFrontEnd::FEParas fp(feparas);
        FeMode = fp.getFEMode().getBase();
        LOGD("[%s] FeMode = %d", __FUNCTION__, FeMode);
        mFrontDev->Open(FeMode);
        if (!(mTvAction & TV_ACTION_SCANNING)) {
            mFrontDev->setPara(feparas);
        }
    }

    mTvAction |= TV_ACTION_PLAYING;
    tvWriteSysfs ( DEVICE_CLASS_TSYNC_AV_THRESHOLD_MIN, AV_THRESHOLD_MIN_MS );
    ret = mAv.startTimeShift(para);

    return ret;
}


int CTv::playDtvTimeShift (const char *feparas, void *para, int audioCompetation)
{
    AutoMutex _l( mLock );
    return playDtvTimeShiftUnlocked(feparas, para, audioCompetation);
}

int CTv::playDtmbProgram ( int progId )
{
    CTvProgram prog;
    CTvChannel channel;
    int vpid = 0x1fff, apid = 0x1fff, vfmt = -1, afmt = -1;
    int aindex;
    CTvProgram::Audio *pA;
    CTvProgram::Video *pV;
    CTvProgram::selectByID ( progId, prog );
    saveDTVProgramID ( progId );
    prog.getChannel ( channel );
    int chanVolCompValue = 0;
    chanVolCompValue = GetAudioVolumeCompensationVal(progId);

    mFrontDev->setPara ( channel.getMode(), channel.getFrequency(), channel.getBandwidth(), 0 );

    pV = prog.getVideo();
    if ( pV != NULL ) {
        vpid = pV->getPID();
        vfmt = pV->getFormat();
    }

    aindex = prog.getCurrAudioTrackIndex();
    if ( -1 == aindex ) { //db is default
        aindex = prog.getCurrentAudio ( String8 ( "eng" ) );
        if ( aindex >= 0 ) {
            prog.setCurrAudioTrackIndex ( progId, aindex );
        }
    }

    if ( aindex >= 0 ) {
        pA = prog.getAudio ( aindex );
        if ( pA != NULL ) {
            apid = pA->getPID();
            afmt = pA->getFormat();
        }
    }

    mTvEpg.leaveProgram();
    mTvEpg.leaveChannel();
    startPlayTv ( SOURCE_DTV, vpid, apid, prog.getPcrId(), vfmt, afmt );
    mTvEpg.enterChannel ( channel.getID() );
    mTvEpg.enterProgram ( prog.getID() );
    return 0;
}

int CTv::playAtvProgram (int freq, int videoStd, int audioStd, int vfmt, int soundsys, int fineTune __unused, int audioCompetation __unused)
{
    LOGD("%s Start SwitchSourceTime = %fs",__FUNCTION__,getUptimeSeconds());
    SetSourceSwitchInputLocked(m_source_input_virtual, SOURCE_TV);
    mTvAction |= TV_ACTION_IN_VDIN;
    mTvAction |= TV_ACTION_PLAYING;

    /* mpTvin->Tvin_StopDecoder(); */ /* No, it will affect the handling of onVdinSignalChange */
    mFrontDev->Open(TV_FE_ANALOG);
    unsigned long stdAndColor = mFrontDev->enumToStdAndColor (videoStd, audioStd);

    //set CVBS
    int fmt = CFrontEnd::stdEnumToCvbsFmt (vfmt, stdAndColor);
    mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

    //set TUNER
    mFrontDev->setPara (TV_FE_ANALOG, freq, stdAndColor, -1, vfmt, soundsys);
    LOGD("%s End SwitchSourceTime = %fs",__FUNCTION__,getUptimeSeconds());
    return 0;
}

int CTv::resetFrontEndPara ( frontend_para_set_t feParms )
{
    LOGD("mTvAction = %#x, %s", mTvAction, __FUNCTION__);
#ifdef SUPPORT_ADTV
    if (mTvAction & TV_ACTION_SCANNING) {
        return -1;
    }

    if ( feParms.mode == TV_FE_ANALOG ) {
        //int progID = -1;
        int tmpFreq = feParms.freq;
        int tmpfineFreq = feParms.para2;
        //int mode = feParms.mode;

        //get tunerStd from videoStd and audioStd
        unsigned long stdAndColor = mFrontDev->enumToStdAndColor (feParms.videoStd, feParms.audioStd);

        LOGD("%s, resetFrontEndPara- vstd=%d astd=%d stdandcolor=0x%x", __FUNCTION__, feParms.videoStd, feParms.audioStd, stdAndColor);

        //set frontend parameters to tuner dev
        mpTvin->Tvin_StopDecoder();

        //set CVBS
        int fmt = CFrontEnd::stdEnumToCvbsFmt (feParms.vfmt, stdAndColor);
        mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

        //set TUNER
        //usleep(400 * 1000);
        mFrontDev->Open(TV_FE_AUTO);
        mFrontDev->setPara ( TV_FE_ANALOG, tmpFreq, stdAndColor, -1, feParms.vfmt);
        //usleep(400 * 1000);
        if ( tmpfineFreq != 0 ) {
            mFrontDev->fineTune ( tmpfineFreq / 1000 );
        }
    } else {
        mFrontDev->Open(TV_FE_AUTO);
        mFrontDev->setPara ( feParms.mode, feParms.freq, feParms.para1, feParms.para2 );
    }
#endif
    return 0;
}

int CTv::setFrontEnd ( const char *paras, bool force )
{
    AutoMutex _l( mLock );
    LOGD("%s: mTvAction = %#x, paras = %s, m_source_input = %d. SwitchSourceTime = %fs",
            __FUNCTION__, mTvAction, paras, m_source_input,getUptimeSeconds());

    if (mTvAction & TV_ACTION_SCANNING) {
        return -1;
    }

    CFrontEnd::FEParas fp(paras);

    if (mBlackoutEnable) {
        mAv.EnableVideoBlackout();
    } else {
        mAv.DisableVideoBlackout();
    }

    int FEMode_Base = fp.getFEMode().getBase();
    LOGD("%s: fp.getFEMode.getbase = %d", __FUNCTION__, FEMode_Base);
    if ( FEMode_Base == TV_FE_ANALOG ) {
        mTvAction |= TV_ACTION_IN_VDIN;
        if (SOURCE_DTV == m_source_input) {
            /* DTV --> ATV */
            stopPlaying(false);
        }
        //int tmpFreq = fp.getFrequency();
        int tmpfineFreq = fp.getFrequency2();

        //get tunerStd from videoStd and audioStd
        unsigned long stdAndColor = mFrontDev->enumToStdAndColor (fp.getVideoStd(), fp.getAudioStd());

        LOGD("%s: vstd=%d astd=%d stdandcolor=0x%x", __FUNCTION__, fp.getVideoStd(), fp.getAudioStd(), stdAndColor);
        //set frontend parameters to tuner dev
        if ((SOURCE_TV == m_source_input) && mATVDisplaySnow && mpTvin->getSnowStatus()) {
            SetSnowShowEnable( false );
        }
        mpTvin->Tvin_StopDecoder();
        //set CVBS
        int fmt = CFrontEnd::stdEnumToCvbsFmt (fp.getVFmt(), stdAndColor);
        mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

        //set TUNER
        //usleep(400 * 1000);
        mFrontDev->Open(TV_FE_AUTO);
        mFrontDev->setMode(TV_FE_ANALOG);
        mFrontDev->setPara ( paras, force );
        //usleep(400 * 1000);
        if ( tmpfineFreq != 0 ) {
            mFrontDev->fineTune ( tmpfineFreq / 1000, force );
        }
    } else {
        mTvAction &= ~TV_ACTION_IN_VDIN;
        if ((SOURCE_TV == m_source_input) && mATVDisplaySnow && mpTvin->getSnowStatus()) {
            SetSnowShowEnable( false );
        }
        mpTvin->Tvin_StopDecoder();
        mpTvin->VDIN_ClosePort();

        mFrontDev->Open(TV_FE_AUTO);
        mFrontDev->setMode(FEMode_Base);
        mFrontDev->setPara (paras, force);
    }

    int mode, freq, para1, para2;
    mFrontDev->getPara(&mode, &freq, &para1, &para2);
    mode = (FEMode_Base == TV_FE_ANALOG);
    Tv_RrtUpdate(freq, mode, 0);
    Tv_Easupdate();
    LOGD("%s setFrontEnd End SwitchSourceTime = %fs",__FUNCTION__,getUptimeSeconds());
    return 0;
}

int CTv::resetDmxAndAvSource()
{
#ifdef SUPPORT_ADTV
    AM_DMX_Source_t curdmxSource;
    mFrontDev->GetTSSource ( &curdmxSource );
    LOGD ( "%s, AM_FEND_GetTSSource %d", __FUNCTION__, curdmxSource );
    mTvDmx.Close();
    AM_DMX_OpenPara_t para;
    memset ( &para, 0, sizeof ( para ) );
    mTvDmx.Open (para);
    mTvDmx.SetSource(curdmxSource );
    mTvDmx1.Open(para);
    mTvDmx1.SetSource(curdmxSource);
    mTvDmx2.Open(para);
    mTvDmx2.SetSource(curdmxSource);
    int ts_source = ( int ) curdmxSource;
    mAv.SetTSSource (ts_source );
#endif
    return 0;
}

int CTv::GetAudioVolumeCompensationVal(int progxxId __unused)
{
    int tmpVolValue = 0;
    CTvProgram prog;
    if ( CTvProgram::selectByID ( -1, prog ) != 0 ) {
        LOGE ( "%s, atv progID is not matching the db's  ret = 0\n", __FUNCTION__ );
        return tmpVolValue;
    }
    tmpVolValue = prog.getChanVolume ();
    LOGD ( "%s,progid = -1 CompensationVal = %d\n", __FUNCTION__, tmpVolValue );
    if (tmpVolValue > 20 || tmpVolValue < -20) tmpVolValue = 0;
    return tmpVolValue;
}

int CTv::startPlayTv ( int source, int vid, int aid, int pcrid, int vfat, int afat )
{
    LOGD ("%s\n", __FUNCTION__);
    if ( source == SOURCE_DTV ) {
        tvWriteSysfs ( DEVICE_CLASS_TSYNC_AV_THRESHOLD_MIN, AV_THRESHOLD_MIN_MS );
        setDvbLogLevel();
        LOGD ( "%s, start Play DTV SwitchSourceTime Time = %fs!\n", __FUNCTION__,getUptimeSeconds());
        return mAv.StartTS (vid, aid, pcrid, vfat, afat );
    }
    return -1;
}

int CTv::stopPlayingLock()
{
    AutoMutex _l( mLock );
    if (mSubtitle.sub_switch_status() == 1)
        mSubtitle.sub_stop_dvb_sub();
    return stopPlaying(true);
}

int CTv::stopPlaying(bool isShowTestScreen) {
    return stopPlaying(isShowTestScreen, true);
}

int CTv::stopPlaying(bool isShowTestScreen, bool resetFE)
{
    LOGD("%s", __FUNCTION__);
    if (!(mTvAction & TV_ACTION_PLAYING)) {
        LOGD("%s, stopplay cur action = %x not playing , return", __FUNCTION__, mTvAction);
        return 0;
    }

    LOGD("%s, isShowTestScreen = %d, resetFE = %d", __FUNCTION__, isShowTestScreen, resetFE);
    if (m_source_input == SOURCE_TV) {
        if (resetFE)
            ClearAnalogFrontEnd();
    } else if (m_source_input ==  SOURCE_DTV) {
        mAv.StopTS ();
    }

    mTvAction &= ~TV_ACTION_PLAYING;
    return 0;
}

int CTv::getAudioChannel()
{
    int iRet = -1;
    int audioChanneleMod;
    do {
        iRet = mAv.AudioGetOutputMode (&audioChanneleMod );
        if ( DVB_SUCCESS != iRet ) {
            LOGD ( "%s, audio GetOutputMode is FAILED %d\n", __FUNCTION__,  iRet );
            break;
        }
        LOGD ( "%s, jianfei.lan getAudioChannel iRet : %d audioChanneleMod %d\n", __FUNCTION__,  iRet, audioChanneleMod );
    } while ( 0 );
    return audioChanneleMod;
}

int CTv::setAudioChannel ( int channelIdx )
{
    int iOutRet = 0;
    int audioChanneleMod;
    LOGD ( "%s, channelIdx : %d\n", __FUNCTION__,  channelIdx );
    audioChanneleMod = ( int ) channelIdx;
    iOutRet = mAv.AudioSetOutputMode (audioChanneleMod );
    int aud_ch = 1;
    if (audioChanneleMod == TV_AOUT_OUTPUT_STEREO) {
        aud_ch = 1;
    } else if (audioChanneleMod == TV_AOUT_OUTPUT_DUAL_LEFT) {
        aud_ch = 2;
    } else if (audioChanneleMod == TV_AOUT_OUTPUT_DUAL_RIGHT) {
        aud_ch = 3;
    }
    CTvProgram::updateAudioChannel(-1, aud_ch);
    if ( DVB_SUCCESS != iOutRet) {
        LOGD ( "%s, TV AM_AOUT_SetOutputMode device is FAILED %d\n", __FUNCTION__, iOutRet );
    }
    return 0;
}

int CTv::getFrontendSignalStrength()
{
    return mFrontDev->getStrength();
}

int CTv::getFrontendSNR()
{
    return mFrontDev->getSNR();
}

int CTv::getFrontendBER()
{
    return mFrontDev->getBER();
}

int CTv::getChannelInfoBydbID ( int dbID, channel_info_t &chan_info )
{
    CTvProgram prog;
    CTvChannel channel;
    Vector<sp<CTvProgram> > out;
    memset ( &chan_info, 0, sizeof(chan_info));
    chan_info.freq = 44250000;
    chan_info.uInfo.atvChanInfo.videoStd = CC_ATV_VIDEO_STD_PAL;
    chan_info.uInfo.atvChanInfo.audioStd = CC_ATV_AUDIO_STD_DK;
    static const int CVBS_ID = -1;
    if (dbID == CVBS_ID) {
        unsigned char std;
        SSMReadCVBSStd(&std);
        chan_info.uInfo.atvChanInfo.videoStd = (atv_video_std_t)std;
        chan_info.uInfo.atvChanInfo.audioStd = CC_ATV_AUDIO_STD_DK;
        chan_info.uInfo.atvChanInfo.isAutoStd = (std == CC_ATV_VIDEO_STD_AUTO) ? 1 : 0;
        return 0;
    }

    int ret = CTvProgram::selectByID ( dbID, prog );
    if ( ret != 0 ) {
        LOGD ( "getChannelinfo , select dbid=%d,is not exist", dbID);
        return -1;
    }
    prog.getChannel ( channel );

    if ( CTvProgram::TYPE_ATV == prog.getProgType() ) {
        chan_info.freq = channel.getFrequency();
        chan_info.uInfo.atvChanInfo.finefreq  =  channel.getAfcData();
        LOGD("%s, get channel std = %d", __FUNCTION__, channel.getStd());
        chan_info.uInfo.atvChanInfo.videoStd =
            ( atv_video_std_t ) CFrontEnd::stdAndColorToVideoEnum ( channel.getStd() );
        chan_info.uInfo.atvChanInfo.audioStd =
            ( atv_audio_std_t ) CFrontEnd::stdAndColorToAudioEnum ( channel.getStd() );
        chan_info.uInfo.atvChanInfo.isAutoStd = ((channel.getStd() & V4L2_COLOR_STD_AUTO) ==  V4L2_COLOR_STD_AUTO) ? 1 : 0;
    } else if ( CTvProgram::TYPE_DTV == prog.getProgType()  || CTvProgram::TYPE_RADIO == prog.getProgType()) {
        chan_info.freq = channel.getFrequency();
        chan_info.uInfo.dtvChanInfo.strength = getFrontendSignalStrength();
        chan_info.uInfo.dtvChanInfo.quality = getFrontendSNR();
        chan_info.uInfo.dtvChanInfo.ber = getFrontendBER();
    }

    return 0;
}

bool CTv::Tv_Start_Analyze_Ts ( int channelID )
{
    int freq, ret;
    CTvChannel channel;

    AutoMutex _l( mLock );
    mAv.StopTS ();
    mAv.DisableVideoWithBlueColor();
    ret = CTvChannel::selectByID ( channelID, channel );
    if ( ret != 0 ) {
        LOGD ( "%s, CTv tv_updatats can not get freq by channel ID", __FUNCTION__ );
        return false;
    }

    mTvAction |= TV_ACTION_SCANNING;
    freq = channel.getFrequency();
    LOGD ( "%s, the freq = %d", __FUNCTION__,  freq );
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_ANALYZE_CHANNEL;
    mTvScanner->setObserver ( &mTvMsgQueue );
    mTvScanner->manualDtmbScan ( freq, freq ); //dtmb
    return true;
}

bool CTv::Tv_Stop_Analyze_Ts()
{
    stopScanLock();
    return true;
}

int CTv::saveATVProgramID ( int dbID )
{
    config_set_int ( CFG_SECTION_TV, "atv.get.program.id", dbID );
    return 0;
}

int CTv::getATVProgramID ( void )
{
    return config_get_int ( CFG_SECTION_TV, "atv.get.program.id", -1 );
}

int CTv::saveDTVProgramID ( int dbID )
{
    config_set_int ( CFG_SECTION_TV, "dtv.get.program.id", dbID );
    return 0;
}

int CTv::getDTVProgramID ( void )
{
    return config_get_int ( CFG_SECTION_TV, "dtv.get.program.id", -1 );
}

int CTv::saveRadioProgramID ( int dbID )
{
    config_set_int ( CFG_SECTION_TV, "radio.get.program.id", dbID );
    return 0;
}

int CTv::getRadioProgramID ( void )
{
    return config_get_int ( CFG_SECTION_TV, "radio.get.program.id", -1 );
}


int CTv::getATVMinMaxFreq ( int *scanMinFreq, int *scanMaxFreq )
{
    const char *strDelimit = ",";
    char *token = NULL;

    *scanMinFreq = 44250000;
    *scanMaxFreq = 868250000;

    const char *freqList = config_get_str ( CFG_SECTION_ATV, CFG_ATV_FREQ_LIST, "null" );
    if ( strcmp ( freqList, "null" ) == 0 ) {
        LOGE( "[ctv]%s, atv freq list is null \n", __FUNCTION__ );
        return -1;
    }

    char data_str[512] = {0};
    strncpy ( data_str, freqList, sizeof ( data_str ) - 1 );

    char *pSave;
    token = strtok_r ( data_str, strDelimit, &pSave);
    if (token != NULL)
    {
        sscanf ( token, "%d", scanMinFreq );
    }
    token = strtok_r ( NULL, strDelimit , &pSave);
    if ( token != NULL ) {
        sscanf ( token, "%d", scanMaxFreq );
    }
    return 0;
}

int CTv::IsDVISignal()
{
    return m_cur_sig_info.is_dvi;
}

int CTv::getHDMIFrameRate()
{
    int ConstRate[5] = {24, 25, 30, 50, 60};
    float ConstRateDiffHz[5] = {0.5, 0.5, 0.5, 2, 2};
    int fps = m_cur_sig_info.fps;
    for (int i = 0; i < 5; i++) {
        if (abs(ConstRate[i] - fps) < ConstRateDiffHz[i])
            fps = ConstRate[i];
    }
    return fps;
}

tv_source_input_t CTv::GetLastSourceInput ( void )
{
    return m_last_source_input;
}

int CTv::isVgaFmtInHdmi ( void )
{
    if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_HDMI ) {
        return -1;
    }
    return CTvin::isVgaFmtInHdmi (m_cur_sig_info.fmt);
}

void CTv::print_version_info ( void )
{
    // print tvapi version info
    LOGD ( "libtvservice git branch:%s\n", tvservice_get_git_branch_info() );
    LOGD ( "libtvservice git version:%s\n",  tvservice_get_git_version_info() );
    LOGD ( "libtvservice Last Changed:%s\n", tvservice_get_last_chaned_time_info() );
    LOGD ( "libtvservice Last Build:%s\n",  tvservice_get_build_time_info() );
    LOGD ( "libtvservice Builer Name:%s\n", tvservice_get_build_name_info() );
    LOGD ( "libtvservice board version:%s\n", tvservice_get_board_version_info() );
    LOGD ( "\n\n");
#ifdef SUPPORT_ADTV
    // print dvb version info
    LOGD ( "libdvb git branch:%s\n", dvb_get_git_branch_info() );
    LOGD ( "libdvb git version:%s\n", dvb_get_git_version_info() );
    LOGD ( "libdvb Last Changed:%s\n", dvb_get_last_chaned_time_info() );
    LOGD ( "libdvb Last Build:%s\n", dvb_get_build_time_info() );
    LOGD ( "libdvb Builer Name:%s\n", dvb_get_build_name_info() );
#endif
}

int CTv::Tvin_GetTvinConfig ( void )
{
    const char *config_value;

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_KERNELPET_DISABLE, "null" );
    if ( strcmp ( config_value, "disable" ) == 0 ) {
        gTvinConfig.kernelpet_disable = true;
    } else {
        gTvinConfig.kernelpet_disable = false;
    }

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_KERNELPET_TIMEROUT, "null" );
    gTvinConfig.userpet_timeout = ( unsigned int ) strtol ( config_value, NULL, 10 );

    if ( gTvinConfig.kernelpet_timeout <= 0 || gTvinConfig.kernelpet_timeout > 40 ) {
        gTvinConfig.kernelpet_timeout = 10;
    }

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_USERPET, "null" );
    if ( strcmp ( config_value, "enable" ) == 0 ) {
        gTvinConfig.userpet = true;
    } else {
        gTvinConfig.userpet = false;
    }

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_USERPET_TIMEROUT, "null" );
    gTvinConfig.userpet_timeout = ( unsigned int ) strtol ( config_value, NULL, 10 );

    if ( gTvinConfig.userpet_timeout <= 0 || gTvinConfig.userpet_timeout > 100 ) {
        gTvinConfig.userpet_timeout = 10;
    }

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_USERPET_RESET, "null" );
    if ( strcmp ( config_value, "disable" ) == 0 ) {
        gTvinConfig.userpet_reset = 0;
    } else {
        gTvinConfig.userpet_reset = 1;
    }
    return 0;
}

TvRunStatus_t CTv::GetTvStatus()
{
    AutoMutex _l( mLock );
    return mTvStatus;
}

int CTv::GetTvAction() {
    return mTvAction;
}

int CTv::OpenTv ( void )
{
    LOGD("%s: start at %fs\n", __FUNCTION__, getUptimeSeconds());
    const char * value;
    value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_ATV_DISPLAY_SNOW, "null" );
    if (strcmp(value, "enable") == 0 ) {
        mATVDisplaySnow = true;
    } else {
        mATVDisplaySnow = false;
    }
    mpTvin->AFE_EnableSnowByConfig(mATVDisplaySnow);

    value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( value, "black" ) == 0 ) {
        iSBlackPattern = true;
    } else {
        iSBlackPattern = false;
    }

    value = config_get_str ( CFG_SECTION_TV, CGF_TV_AUTOSWITCH_MONITOR_EN, "disable" );
    if (strcmp(value, "enable") == 0 ) {
        mAutoSwitchMonitorCfg = true;
    } else {
        mAutoSwitchMonitorCfg = false;
    }

    /*value = config_get_str ( CFG_SECTION_TV, CFG_FBC_PANEL_INFO, "null" );
    LOGD("open tv, get fbc panel info:%s\n", value);
    if (strcmp(value, "edid") == 0 ) {
        rebootSystemByEdidInfo();
    } else if (strcmp(value, "uart") == 0 ) {
        rebootSystemByUartPanelInfo(fbcIns);
    }*/

    //tv ssm check
    SSMHandlePreCopying();

    mpTvin->OpenTvin();

    CVpp::getInstance()->Vpp_Init();

    SSMSetHDCPKey();
    system ( "/vendor/bin/dec" );

    value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_DISPLAY_FREQ_AUTO, "null" );
    if ( strcmp ( value, "enable" ) == 0 ) {
        mAutoSetDisplayFreq = true;
    } else {
        mAutoSetDisplayFreq = false;
    }

    mHDMIRxManager.SetHdmiPortCecPhysicAddr();
    Tv_HandeHDMIEDIDFilePathConfig();
    SSMSetHDMIEdid(1);
    mHDMIRxManager.HdmiRxEdidUpdate();

    Tvin_GetTvinConfig();
    m_last_source_input = SOURCE_INVALID;
    m_source_input = SOURCE_INVALID;

    mBlackoutEnable = ((getBlackoutEnable() == 1)?true:false);
    mAv.Open();
    //mDevicesPollStatusDetectThread.startDetect();
    //ClearAnalogFrontEnd();
    InitCurrenSignalInfo();

    m_first_enter_tvinput = true;
    mTvStatus = TV_OPEN_ED;
    return 0;
}

int CTv::CloseTv ( void )
{
    mpTvin->Tv_uninit_afe();
    mpTvin->uninit_vdin();
    TvMisc_DisableWDT ( gTvinConfig.userpet );
    mTvStatus = TV_CLOSE_ED;
    return 0;
}

int CTv::StartTvLock ()
{
    LOGD("StartTvLock status = %d", mTvStatus);
    if (mTvStatus == TV_START_ED)
        return 0;

    AutoMutex _l( mLock );
    //tvWriteSysfs("/sys/power/wake_lock", "tvserver.run");

    if ( m_first_enter_tvinput ) {
        if (mpTvin->Tvin_RemovePath (TV_PATH_TYPE_TVIN) > 0) {
            mpTvin->VDIN_AddVideoPath(TV_PATH_VDIN_AMLVIDEO2_PPMGR_DEINTERLACE_AMVIDEO);
        }
        if (mpTvin->Tvin_RemovePath (TV_PATH_TYPE_DEFAULT) > 0) {
            mpTvin->VDIN_AddVideoPath(TV_PATH_DECODER_AMLVIDEO2_PPMGR_DEINTERLACE_AMVIDEO);
        }
        m_first_enter_tvinput = false;
    }

    setDvbLogLevel();
    mAv.SetVideoLayerStatus(ENABLE_AND_CLEAR_VIDEO_LAYER);
    TvMisc_EnableWDT ( gTvinConfig.kernelpet_disable, gTvinConfig.userpet, gTvinConfig.kernelpet_timeout, gTvinConfig.userpet_timeout, gTvinConfig.userpet_reset );
    mpTvin->TvinApi_SetCompPhaseEnable ( 1 );
    mpTvin->VDIN_EnableRDMA ( 1 );

    mTvStatus = TV_START_ED;
    MnoNeedAutoSwitchToMonitorMode = false;
    LOGD("StartTvLock end");
    return 0;
}

int CTv::startTvDetect()
{
    mDevicesPollStatusDetectThread.startDetect();
    return 0;
}

int CTv::DoSuspend(int type __unused)
{
    return 0;
}

int CTv::DoResume(int type __unused)
{
    return 0;
}

int CTv::StopTvLock ( void )
{
    LOGD("%s: mTvStatus = %d, mBlackoutEnable = %d, SameSourceEnableStatus = %d\n", __FUNCTION__, mTvStatus, mBlackoutEnable, mbSameSourceEnableStatus);
    if (mbSameSourceEnableStatus) {
        mbSameSourceEnableStatus = false;
        return 0;
    } else {
        LOGD("%s StopTvLock Start SwitchSourceTime Time = %fs\n", __FUNCTION__,getUptimeSeconds());
        AutoMutex _l( mLock );
        mTvAction |= TV_ACTION_STOPING;
        mTvAction &= ~TV_ACTION_IN_VDIN;

        /* release ATV/DTV early */
        mFrontDev->setMode(TV_FE_AUTO);

        mAv.DisableVideoWithBlackColor();
        tryReleasePlayer(false, m_source_input);
        stopPlaying(false);
        if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
            SetSnowShowEnable( false );
        }

        //stop scan  if scanning
        stopScan();

        mpTvin->Tvin_StopDecoder();
        mpTvin->VDIN_ClosePort();

        mFrontDev->SetAnalogFrontEndTimerSwitch(0);

        setAudioChannel(TV_AOUT_OUTPUT_STEREO);
        mpTvin->setMpeg2Vdin(0);
        mAv.setLookupPtsForDtmb(0);
        m_last_source_input = SOURCE_INVALID;
        m_source_input = SOURCE_INVALID;
        m_source_input_virtual = SOURCE_INVALID;
        int ret = tvSetCurrentSourceInfo(SOURCE_MPEG, TVIN_SIG_FMT_NULL, TVIN_TFMT_2D);
        if (ret < 0) {
            LOGE("%s Set CurrentSourceInfo error!\n", __FUNCTION__);
        }

        mFrontDev->Close();
        mTvAction &= ~TV_ACTION_STOPING;
        mTvStatus = TV_STOP_ED;
        MnoNeedAutoSwitchToMonitorMode = false;
        if (mBlackoutEnable) {
            mAv.EnableVideoBlackout();
        }
        mAv.ClearVideoBuffer();
        LOGD("%s StopTvLock End SwitchSourceTime Time = %fs\n", __FUNCTION__,getUptimeSeconds());
        return 0;
    }
}

int CTv::Tv_MiscSetBySource ( tv_source_input_t source_input )
{
    int ret = -1;

    switch ( source_input ) {
    case SOURCE_TV:
        CVpp::getInstance()->VPP_SetScalerPathSel(1);
        ret = tvWriteSysfs ( SYS_VECM_DNLP_ADJ_LEVEL, "4" );
        break;

    case SOURCE_HDMI1:
    case SOURCE_HDMI2:
    case SOURCE_HDMI3:
    case SOURCE_HDMI4:
        CVpp::getInstance()->VPP_SetScalerPathSel(0);
        //ret = CVpp::getInstance()->Tv_SavePanoramaMode ( VPP_PANORAMA_MODE_FULL, SOURCE_TYPE_HDMI );
        ret |= tvWriteSysfs ( SYS_VECM_DNLP_ADJ_LEVEL, "5" );
        break;

    case SOURCE_DTV:
        CVpp::getInstance()->VPP_SetScalerPathSel(0);
        break;

    case SOURCE_AV1:
    case SOURCE_AV2:
    case SOURCE_VGA:
        CVpp::getInstance()->VPP_SetScalerPathSel(1);
        ret |= tvWriteSysfs ( SYS_VECM_DNLP_ADJ_LEVEL, "5" );
        break;

    case SOURCE_SVIDEO:
    case SOURCE_IPTV:
        CVpp::getInstance()->VPP_SetScalerPathSel(1);
        break;
    default:
        CVpp::getInstance()->VPP_SetScalerPathSel(0);
        ret |= tvWriteSysfs ( SYS_VECM_DNLP_ADJ_LEVEL, "5" );
        break;
    }
    return ret;
}

bool CTv::isVirtualSourceInput(tv_source_input_t source_input) {
    switch (source_input) {
        case SOURCE_ADTV:
            return true;
        default:
            return false;
    }
    return false;
}

int CTv::tryReleasePlayer(bool isEnter, tv_source_input_t si)
{
    if ((isEnter && si != SOURCE_ADTV && si != SOURCE_TV && si != SOURCE_DTV)
        ||(!isEnter && (si == SOURCE_ADTV || si == SOURCE_TV || si == SOURCE_DTV))){
        LOGD("remove all Player & Recorder");
        PlayerManager::getInstance().releaseAll();
        RecorderManager::getInstance().releaseAll();
    }
    return 0;
}

int CTv::SetSourceSwitchInput(tv_source_input_t source_input)
{
   AutoMutex _l( mLock );

    LOGD ( "%s, source input = %d SwitchSourceTime = %fs", __FUNCTION__, source_input,getUptimeSeconds());

    tryReleasePlayer(true, source_input);

    m_source_input_virtual = source_input;

    if (isVirtualSourceInput(source_input))
        return 0;//defer real source setting.

    return SetSourceSwitchInputLocked(m_source_input_virtual, source_input);
}

int CTv::SetSourceSwitchInput(tv_source_input_t virtual_input, tv_source_input_t source_input) {
    AutoMutex _l( mLock );
    return SetSourceSwitchInputLocked(virtual_input, source_input);
}

int CTv::SetSourceSwitchInputLocked(tv_source_input_t virtual_input, tv_source_input_t source_input)
{
    LOGD ( "SwitchSourceTime Time = %fs, %s, virtual source input = %d source input = %d m_source_input = %d",
           getUptimeSeconds(), __FUNCTION__, virtual_input, source_input, m_source_input );

    tvin_port_t cur_port;
    m_source_input_virtual = virtual_input;

    Tv_SetDDDRCMode(source_input);
    if (source_input != SOURCE_DTV) {
        tvWriteSysfs(VIDEO_SYNC_ENABLE, "0");
        // set sync mode to vmaster. 0: vmaster; 1:amaster
        tvWriteSysfs(VIDEO_SYNC_MODE, "0");
    }
    if (source_input == m_source_input ) {
        LOGW("%s,same source input, return", __FUNCTION__ );
        return 0;
    }
    stopPlaying(false, !(source_input == SOURCE_DTV && m_source_input == SOURCE_TV));
    KillMediaServerClient();
    //if HDMI, need to set EDID of each port
    if (mSetHdmiEdid) {
        int tmp_ret = 0;
        switch ( source_input ) {
        case SOURCE_HDMI1:
            tmp_ret = SSMSetHDMIEdid(1);
            break;
        case SOURCE_HDMI2:
            tmp_ret = SSMSetHDMIEdid(2);
            break;
        case SOURCE_HDMI3:
            tmp_ret = SSMSetHDMIEdid(3);
            break;
        case SOURCE_HDMI4:
            tmp_ret = SSMSetHDMIEdid(4);
            break;
        default:
            tmp_ret = -1;
            break;
        }
        if (tmp_ret < 0)
            LOGE ( "%s, do not set hdmi port%d edid.ret=%d", __FUNCTION__, source_input - 4, tmp_ret );
    }
    mTvAction |= TV_ACTION_SOURCE_SWITCHING;

    //set front dev mode
    if ( source_input == SOURCE_TV ) {
        mFrontDev->Open(TV_FE_AUTO);
        mFrontDev->SetAnalogFrontEndTimerSwitch(1);
    } else if ( source_input == SOURCE_DTV ) {
        mFrontDev->Open(TV_FE_AUTO);
        mFrontDev->SetAnalogFrontEndTimerSwitch(0);
    } else {
        mFrontDev->Close();
        mFrontDev->SetAnalogFrontEndTimerSwitch(0);
    }

    //ok
    m_last_source_input = m_source_input;
    m_source_input = source_input;
    SSMSaveSourceInput ( source_input );

    if ( source_input == SOURCE_DTV ) {
        resetDmxAndAvSource();

        //we should stop audio first for audio mute.
        mpTvin->Tvin_StopDecoder();
        mpTvin->VDIN_ClosePort();
        //mpTvin->Tvin_WaitPathInactive ( TV_PATH_TYPE_DEFAULT );
        if (mpTvin->Tvin_CheckVideoPathComplete(TV_PATH_TYPE_DEFAULT) != 0) {
            if (mpTvin->Tvin_RemovePath (TV_PATH_TYPE_DEFAULT) > 0) {
                mpTvin->VDIN_AddVideoPath(TV_PATH_DECODER_AMLVIDEO2_PPMGR_DEINTERLACE_AMVIDEO);
            }
        }

        //double confirm we set the main volume lut buffer to mpeg
        mpTvin->setMpeg2Vdin(1);
        mAv.setLookupPtsForDtmb(1);
        int ret = tvSetCurrentSourceInfo(m_source_input, TVIN_SIG_FMT_NULL, TVIN_TFMT_2D);
        if (ret < 0) {
            LOGE("%s Set CurrentSourceInfo error!\n", __FUNCTION__);
        }
    } else {
        mpTvin->setMpeg2Vdin(0);
        mAv.setLookupPtsForDtmb(0);
    }

    cur_port = mpTvin->Tvin_GetSourcePortBySourceInput ( source_input );
    Tv_MiscSetBySource ( source_input );

    if (source_input != SOURCE_DTV) {
        if (mpTvin->SwitchPort ( cur_port ) == 0) { //ok
            mTvAction |= TV_ACTION_IN_VDIN;
            if (source_input != SOURCE_SPDIF)
            {
                unsigned char std;
                SSMReadCVBSStd(&std);
                int fmt = CFrontEnd::stdEnumToCvbsFmt (std, std);
                mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

                //for HDMI source connect detect
                mpTvin->VDIN_OpenHDMIPinMuxOn(true);
            }
            m_sig_spdif_nums = 0;
        }
    }

    if (source_input == SOURCE_SPDIF) { //audio only in spdif source
        TvEvent::SignalInfoEvent ev;
        ev.mTrans_fmt = TVIN_TFMT_2D;
        ev.mFmt = TVIN_SIG_FMT_NULL;
        ev.mStatus = TVIN_SIG_STATUS_NOSIG;
        ev.mReserved = 0;
        sendTvEvent ( ev );
    }

    mTvAction &= ~ TV_ACTION_SOURCE_SWITCHING;
    LOGD ( "%s, SwitchInputLocked End SwitchSourceTime Time = %fs", __FUNCTION__,getUptimeSeconds());
    return 0;
}

void CTv::onSigToStable()
{
    LOGD ( "SwitchSourceTime Time = %fs, onSigToStable start", getUptimeSeconds());

    tv_source_input_type_t source_type = CTvin::Tvin_SourceInputToSourceInputType(m_source_input);
    if (source_type == SOURCE_TYPE_HDMI) {
        tvSetCurrentHdrInfo(m_cur_sig_info.hdr_info);
    }

    int ret = tvSetCurrentSourceInfo(m_source_input, m_cur_sig_info.fmt, m_cur_sig_info.trans_fmt);
    if (ret < 0) {
        LOGE("%s Set CurrentSourceInfo error!\n", __FUNCTION__);
    }

    if ((source_type == SOURCE_TYPE_HDMI) && mAutoSwitchMonitorCfg) {//autoswitch for hdmi source
            autoSwitchToMonitorMode();
    }

    if (mAutoSetDisplayFreq && !mPreviewEnabled) {
        int freq = 60;
        if (source_type == SOURCE_TYPE_HDMI ) {//HDMI source
            int fps = getHDMIFrameRate();
            LOGD("%s: HDMI source frame rate is %d\n", __FUNCTION__, fps);
            freq = fps;
        }else {//ATV or AV source
            if (CTvin::Tvin_is50HzFrameRateFmt(m_cur_sig_info.fmt)) {
                freq = 50;
            } else {
                freq = 60;
            }
        }

        mpTvin->VDIN_SetDisplayVFreq(freq);
    }
    //showbo mark  hdmi auto 3d, tran fmt  is 3d, so switch to 3d

    if ( m_win_mode == PREVIEW_WONDOW ) {
        mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
        mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
    }
    LOGD ( "onSigToStable end");
}

void CTv::onSigStillStable()
{
    LOGD ( "%s, signal Still Stable!\n", __FUNCTION__);

    if ((SOURCE_TV == m_source_input) && mATVDisplaySnow && mpTvin->getSnowStatus()) {
        SetSnowShowEnable( false );
        mpTvin->Tvin_StopDecoder();
    }
    LOGD ( "%s, startDecoder SwitchSourceTime Time = %fs\n", __FUNCTION__,getUptimeSeconds());
    int startdec_status = mpTvin->Tvin_StartDecoder ( m_cur_sig_info );
    LOGD ( "%s, startDecoder End SwitchSourceTime = %fs\n", __FUNCTION__,getUptimeSeconds());
    //showboz  codes from  start decode fun
    if ((startdec_status == 0) &&
        ((SOURCE_TV == m_source_input) || (SOURCE_AV1 == m_source_input) || (SOURCE_AV2 == m_source_input))) {
        const char *value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_DB_REG, "null" );
        if (strcmp(value, "enable") == 0) {
            CVpp::getInstance()->VPP_SetCVD2Values();
        }
    }
    mAv.SetVideoLayerStatus(ENABLE_VIDEO_LAYER);
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_ENABLE_VIDEO_LATER;
    msg.mpPara[0] = 2;
    mTvMsgQueue.sendMsg ( msg );

    TvEvent::SignalInfoEvent ev;
    ev.mTrans_fmt = m_cur_sig_info.trans_fmt;
    ev.mFmt = m_cur_sig_info.fmt;
    ev.mStatus = m_cur_sig_info.status;
    ev.mReserved = getHDMIFrameRate();
    sendTvEvent ( ev );
}

bool CTv::isTvViewBlocked() {
    char prop_value[PROPERTY_VALUE_MAX];
    memset(prop_value, '\0', PROPERTY_VALUE_MAX);
    property_get("persist.sys.tvview.blocked", prop_value, "false");
    LOGD("%s, persist.sys.tvview.blocked = %s", __FUNCTION__, prop_value);
    return (strcmp(prop_value, "true") == 0) ? true : false;
}
void CTv::isVideoFrameAvailable(unsigned int u32NewFrameCount)
{
    unsigned int u32TimeOutCount = 0;
    while(1) {
        if ((unsigned int)mAv.getVideoFrameCount() >= u32NewFrameCount) {
            LOGD("%s video available SwitchSourceTime = %f", __FUNCTION__,getUptimeSeconds());
            break;
        } else {
            if (u32TimeOutCount >= NEW_FRAME_TIME_OUT_COUNT) {
            LOGD("%s Not available frame consume time = %f", __FUNCTION__,getUptimeSeconds());
            break;
            }
        }
        LOGD("%s new frame count = %d",__FUNCTION__,mAv.getVideoFrameCount());
        usleep(10*1000);
        u32TimeOutCount++;
    }
}
void CTv::onEnableVideoLater(int framecount)
{
    mAv.EnableVideoWhenVideoPlaying(framecount);
}

void CTv::onVideoAvailableLater(int framecount)
{
    LOGD("[ctv]onVideoAvailableLater framecount = %d", framecount);
    int ret = mAv.WaittingVideoPlaying(framecount);
    if (ret == 0) { //video available
        TvEvent::SignalInfoEvent ev;
        ev.mStatus = TVIN_SIG_STATUS_STABLE;
        ev.mTrans_fmt = TVIN_TFMT_2D;
        ev.mFmt = TVIN_SIG_FMT_NULL;
        ev.mReserved = 1;/*<< VideoAvailable flag here*/
        sendTvEvent ( ev );
    }
}

void CTv::onSigToUnstable()
{
    LOGD ( "%s, signal to Unstable SwitchSourceTime = %fs\n", __FUNCTION__,getUptimeSeconds());
    if (SOURCE_TV != m_source_input) {
        mAv.DisableVideoWithBlackColor();
        mpTvin->Tvin_StopDecoder();
    }
}

void CTv::onSigToUnSupport()
{
    LOGD ( "%s, signal to UnSupport\n", __FUNCTION__);
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        SetSnowShowEnable(true);
        mpTvin->Tvin_StartDecoder(m_cur_sig_info);
        mAv.EnableVideoNow(false);
    } else {
        if (iSBlackPattern) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        mpTvin->Tvin_StopDecoder();
    }

    if (!(mTvAction & TV_ACTION_SCANNING)) {
        //tvin_info_t info = m_cur_sig_info;
        TvEvent::SignalInfoEvent ev;
        ev.mTrans_fmt = m_cur_sig_info.trans_fmt;
        ev.mFmt = m_cur_sig_info.fmt;
        ev.mStatus = m_cur_sig_info.status;
        ev.mReserved = m_cur_sig_info.is_dvi;
        sendTvEvent ( ev );
    }
}

void CTv::onSigToNoSig()
{
    LOGD ( "%s, signal to NoSignal\n", __FUNCTION__);
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        SetSnowShowEnable(true);
        mpTvin->Tvin_StartDecoder(m_cur_sig_info);
        mAv.EnableVideoNow(false);
    } else {
        if (iSBlackPattern) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        mpTvin->Tvin_StopDecoder();
    }

    if (!(mTvAction & TV_ACTION_SCANNING)) {
        //tvin_info_t info = m_cur_sig_info;
        TvEvent::SignalInfoEvent ev;
        ev.mTrans_fmt = m_cur_sig_info.trans_fmt;
        ev.mFmt = m_cur_sig_info.fmt;
        ev.mStatus = m_cur_sig_info.status;
        ev.mReserved = m_cur_sig_info.is_dvi;
        sendTvEvent ( ev );
    }
}

void CTv::onBootvideoRunning() {
    //LOGD("%s,boot video is running", __FUNCTION__);
}

void CTv::onBootvideoStopped() {
    LOGD("%s,boot video has stopped", __FUNCTION__);
    if (mpTvin->Tvin_RemovePath (TV_PATH_TYPE_TVIN) > 0) {
        mpTvin->VDIN_AddVideoPath(TV_PATH_VDIN_AMLVIDEO2_PPMGR_DEINTERLACE_AMVIDEO);
    }
}

void CTv::onSourceConnect(int source_type, int connect_status)
{
    TvEvent::SourceConnectEvent ev;
    ev.mSourceInput = source_type;
    ev.connectionState = connect_status;
    sendTvEvent(ev);
}

int CTv::GetSourceConnectStatus(tv_source_input_t source_input)
{
    return mDevicesPollStatusDetectThread.GetSourceConnectStatus((tv_source_input_t)source_input);
}

tv_source_input_t CTv::GetCurrentSourceInputLock ( void )
{
    AutoMutex _l( mLock );
    return m_source_input;
}

tv_source_input_t CTv::GetCurrentSourceInputVirtualLock ( void )
{
    AutoMutex _l( mLock );
    return m_source_input_virtual;
}

//dtv and tvin
void CTv::InitCurrenSignalInfo ( void )
{
    m_cur_sig_info.trans_fmt = TVIN_TFMT_2D;
    m_cur_sig_info.fmt = TVIN_SIG_FMT_NULL;
    m_cur_sig_info.status = TVIN_SIG_STATUS_NULL;
    m_cur_sig_info.is_dvi = 0;
}

tvin_info_t CTv::GetCurrentSignalInfo ( void )
{
    tvin_trans_fmt det_fmt = TVIN_TFMT_2D;
    //tvin_sig_status_t signalState = TVIN_SIG_STATUS_NULL;
    //tvin_info_t signal_info = m_cur_sig_info;

    int feState = mFrontDev->getStatus();
    if ( (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_DTV ) ) {
        det_fmt = mpTvin->TvinApi_Get3DDectMode();
        if ((feState & TV_FE_HAS_LOCK) == TV_FE_HAS_LOCK) {
            m_cur_sig_info.status = TVIN_SIG_STATUS_STABLE;
        } else if ((feState & TV_FE_TIMEDOUT) == TV_FE_TIMEDOUT) {
            m_cur_sig_info.status = TVIN_SIG_STATUS_NOSIG;
        }
        if ( det_fmt != TVIN_TFMT_2D ) {
            m_cur_sig_info.trans_fmt = det_fmt;
        }
        m_cur_sig_info.fmt = mAv.getVideoResolutionToFmt();
    }
    LOGD("%s, m_source_input = %d, trans_fmt is %d,fmt is %d, status is %d", __FUNCTION__,
            m_source_input, m_cur_sig_info.trans_fmt, m_cur_sig_info.fmt, m_cur_sig_info.status);
    return m_cur_sig_info;
}

int CTv::setPreviewWindowMode(bool mode)
{
    mPreviewEnabled = mode;
    return 0;
}

int CTv::SetPreviewWindow ( tvin_window_pos_t pos )
{
    m_win_pos.x1 = pos.x1;
    m_win_pos.y1 = pos.y1;
    m_win_pos.x2 = pos.x2;
    m_win_pos.y2 = pos.y2;
    LOGD ( "%s, SetPreviewWindow x = %d y=%d", __FUNCTION__, pos.x2, pos.y2 );

    tvin_window_pos_t def_pos;
    Vpp_GetDisplayResolutionInfo(&def_pos);

    if (pos.x1 != 0 || pos.y1 != 0 || pos.x2 != def_pos.x2 || pos.y2 != def_pos.y2) {
        m_win_mode = PREVIEW_WONDOW;
    } else {
        m_win_mode = NORMAL_WONDOW;
    }

    return mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
}

unsigned int CTv::Vpp_GetDisplayResolutionInfo(tvin_window_pos_t *win_pos)
{
    int display_resolution = mAv.getVideoDisplayResolution();
    int width = 0, height = 0;
    switch (display_resolution) {
    case VPP_DISPLAY_RESOLUTION_1366X768:
        width = CC_RESOLUTION_1366X768_W;
        height = CC_RESOLUTION_1366X768_H;
        break;
    case VPP_DISPLAY_RESOLUTION_1920X1080:
        width = CC_RESOLUTION_1920X1080_W;
        height = CC_RESOLUTION_1920X1080_H;
        break;
    case VPP_DISPLAY_RESOLUTION_3840X2160:
        width = CC_RESOLUTION_3840X2160_W;
        height = CC_RESOLUTION_3840X2160_H;
        break;
    default:
        width = CC_RESOLUTION_3840X2160_W;
        height = CC_RESOLUTION_3840X2160_H;
        break;
    }

    if (win_pos != NULL) {
        win_pos->x1 = 0;
        win_pos->y1 = 0;
        win_pos->x2 = width - 1;
        win_pos->y2 = height - 1;
    }
    return 0;
}

int CTv::setBlackoutEnable(int enable, int isSave)
{
    LOGD("%s: setValue = %d, isSave = %d\n", __FUNCTION__, enable, isSave);
    mBlackoutEnable = ((enable==1)?true:false);
    if (isSave == 1) {
        SSMSaveBlackoutEnable(enable);
    }

    return 0;
}

int CTv::getBlackoutEnable()
{
    int8_t enable;
    SSMReadBlackoutEnable(&enable);
    LOGD("%s: getValue = %d\n", __FUNCTION__, enable);
    return enable;
}

int CTv::getAverageLuma()
{
    return mpTvin->VDIN_Get_avg_luma();
}

int CTv::setAutobacklightData(const char *value)
{
    const char *KEY = "haier.autobacklight.mp.data";

    const char *keyValue = config_get_str(CFG_SECTION_TV, KEY, "null");
    if (strcmp(keyValue, value) != 0) {
        config_set_str(CFG_SECTION_TV, KEY, value);
        LOGD("%s, config string now set as: %s \n", __FUNCTION__, keyValue);
    } else {
        LOGD("%s, config string has been set as: %s \n", __FUNCTION__, keyValue);
    }
    return 0;
}

int CTv::getAutoBacklightData(int *data)
{
    char datas[512] = {0};
    char delims[] = ",";
    char *result = NULL;
    const char *KEY = "haier.autobacklight.mp.data";
    int i = 0;

    if (NULL == data) {
        LOGE("[ctv]%s, data is null", __FUNCTION__);
        return -1;
    }

    const char *keyValue = config_get_str(CFG_SECTION_TV, KEY, (char *) "null");
    if (strcmp(keyValue, "null") == 0) {
        LOGE("[ctv]%s, value is NULL\n", __FUNCTION__);
        return -1;
    }

    strncpy(datas, keyValue, sizeof(datas) - 1);
    result = strtok( datas, delims );
    while ( result != NULL ) {
        char *pd = strstr(result, ":");
        if (pd != NULL) {
            data[i] = strtol(pd + 1, NULL, 10 );
            i++;
        }
        result = strtok( NULL, delims );
    }
    return i;
}

int CTv::setLcdEnable(bool enable)
{
    return tvWriteSysfs(LCD_ENABLE, enable ? 1 : 0);
}

int CTv::Tv_GetIwattRegs()
{
    int ret = -1;
    time_t timer;
    struct tm *tp;
    char path[100] = {0};

    time(&timer);
    tp = localtime(&timer);

    sprintf(path, "%s /data/log/%s-%d-%d-%d-%d%d%d",
            IWATT_SHELL_PATH,
            "iwatt-7019", tp->tm_year + 1900, tp->tm_mon+1,
            tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
    LOGD("save iwatt regs to %s", path);

    ret = system(path);
    if (ret < 0) {
        LOGE("%s: system commond error\n", __FUNCTION__);
    }

    return ret;
}

/*********************** SSM  start **********************/
int CTv::Tv_SSMRestoreDefaultSetting()
{
    SSMRestoreDeviceMarkValues();
    MiscSSMRestoreDefault();
    ReservedSSMRestoreDefault();
    SSMSaveCVBSStd ( 0 );
    SSMSaveLastSelectSourceInput ( SOURCE_TV );
    SSMSavePanelType ( 0 );
    //tvconfig default
    saveDTVProgramID ( -1 );
    saveATVProgramID ( -1 );
    SSMSaveStandbyMode( 0 );
    //restore EDID
    const char * value;
    value = config_get_str ( CFG_SECTION_TV, CFG_SSM_HDMI_EDID_VERSION, "null" );
    if (strcmp(value, "edid_20") == 0 ) {
        SSMHDMIEdidRestoreDefault(HDMI_EDID_VER_20);
    } else {
        SSMHDMIEdidRestoreDefault(HDMI_EDID_VER_14);
    }
    SSMSaveHDMIHdcpSwitcher(0);
    SSMSaveHDMIColorRangeMode(TVIN_COLOR_RANGE_AUTO);
    return 0;
}

int CTv::clearDbAllProgramInfoTable()
{
    return CTvDatabase::GetTvDb()->clearDbAllProgramInfoTable();
}

int CTv::Tv_SSMFacRestoreDefaultSetting()
{
    MiscSSMFacRestoreDefault();
    return 0;
}
/*********************** SSM  End **********************/

//not in CTv, not use lock
void CTv::setSourceSwitchAndPlay()
{
    int progID = 0;
    CTvProgram prog;
    LOGD ( "%s\n", __FUNCTION__ );
    static const int POWERON_SOURCE_TYPE_NONE = 0;//not play source
    static const int POWERON_SOURCE_TYPE_LAST = 1;//play last save source
    static const int POWERON_SOURCE_TYPE_SETTING = 2;//play ui set source
    int to_play_source = -1;
    int powerup_type = SSMReadPowerOnOffChannel();
    LOGD("read power on source type = %d", powerup_type);
    if (powerup_type == POWERON_SOURCE_TYPE_NONE) {
        return ;
    } else if (powerup_type == POWERON_SOURCE_TYPE_LAST) {
        to_play_source = SSMReadSourceInput();
    } else if (powerup_type == POWERON_SOURCE_TYPE_SETTING) {
        to_play_source = SSMReadLastSelectSourceInput();
    }
    SetSourceSwitchInput (( tv_source_input_t ) to_play_source );
    if ( to_play_source == SOURCE_TV ) {
        progID = getATVProgramID();
    } else if ( to_play_source == SOURCE_DTV ) {
        progID = getDTVProgramID();
    }
}

//==============vchip end================================

int CTv::GetHdmiHdcpKeyKsvInfo(int data_buf[])
{
    int ret = -1;
    if (m_source_input != SOURCE_HDMI1 && m_source_input != SOURCE_HDMI2 && m_source_input != SOURCE_HDMI3
       &&m_source_input != SOURCE_HDMI4) {
        return -1;
    }

    struct _hdcp_ksv msg;
    ret = mHDMIRxManager.GetHdmiHdcpKeyKsvInfo(&msg);
    memset((void *)data_buf, 0, 2);
    data_buf[0] = msg.bksv0;
    data_buf[1] = msg.bksv1;
    return ret;
}

bool CTv::hdmiOutWithFbc()
{
    const char *value = config_get_str(CFG_SECTION_TV, CFG_FBC_USED, "true");
    if (strcmp(value, "true") == 0) {
        return true;
    }

    return false;
}

int CTv::StartHeadSetDetect()
{
    mHeadSet.startDetect();
    return 0;
}

void CTv::onHeadSetDetect(int state, int para)
{
    TvEvent::HeadSetOf2d4GEvent ev;
    if (state == 1)
        property_set("audio.headset_plug.enable", "1");
    else
        property_set("audio.headset_plug.enable", "0");
    ev.state = state;
    ev.para = para;
    sendTvEvent(ev);
}

void CTv::onThermalDetect(int state)
{
    const char *value;
    int val = 0;
    static int preValue = -1;

    value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_THERMAL_THRESHOLD_ENABLE, "null" );
    if ( strcmp ( value, "enable" ) == 0 ) {
        value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_THERMAL_THRESHOLD_VALUE, "null" );
        int threshold = atoi(value);
        LOGD ( "%s, threshold value: %d\n", __FUNCTION__, threshold);

        if (state > threshold) {
            const char *valueNormal = config_get_str ( CFG_SECTION_TV, CFG_TVIN_THERMAL_FBC_NORMAL_VALUE, "null" );
            val = atoi(valueNormal);
            if (val == 0) {
                val = 0x4210000;    //normal default
            }
            LOGD ( "%s, current temp: %d set 1\n", __FUNCTION__, state);
        } else {
            const char *valueCold = config_get_str ( CFG_SECTION_TV, CFG_TVIN_THERMAL_FBC_COLD_VALUE, "null" );
            val = atoi(valueCold);
            if (val == 0) {
                val = 0x8210000;    //cold default
            }
            LOGD ( "%s, current temp: 0x%x set 0\n", __FUNCTION__, state);
        }

        if (preValue != val) {
            LOGD ( "%s, pre value :0x%x, new value :0x%x, bypass\n", __FUNCTION__, preValue, val);
            preValue = val;
            mFactoryMode.fbcSetThermalState(val);
        }
    } else {
        LOGD ( "%s, tvin thermal threshold disable\n", __FUNCTION__);
    }
}

int CTv::Tv_HDMIEDIDFileSelect(tv_hdmi_port_id_t port, tv_hdmi_edid_version_t version)
{
    char edid_path[100] = {0};
    char edid_path_cfg[100] = {0};

    config_set_str(CFG_SECTION_TV, CS_HDMI_EDID_USE_CFG, "customer_edid");
    if ( HDMI_EDID_VER_14== version ) {
        sprintf(edid_path, "%s", TV_CONFIG_EDID14_FILE_PATH);
    } else if (HDMI_EDID_VER_20== version) {
        sprintf(edid_path, "%s", TV_CONFIG_EDID20_FILE_PATH);
    } else {
        LOGE("%s HDMI EDID VERSION error!\n", __FUNCTION__);
        sprintf(edid_path, "%s", TV_CONFIG_EDID14_FILE_PATH);
    }
    sprintf(edid_path_cfg, "ssm.handle.hdmi.port%d.edid.file.path", port);
    if (access(edid_path, 0) < 0) {
        config_set_str(CFG_SECTION_TV, CS_HDMI_EDID_USE_CFG, "hdmi_edid");
    }
    config_set_str(CFG_SECTION_TV, edid_path_cfg, edid_path);

    int EdidVersion = 0;
    char tmp[32] = {0};
    for (int i=HDMI_PORT_1;i<HDMI_PORT_MAX;i++) {
        EdidVersion |= (SSMReadHDMIEdidVersion((tv_hdmi_port_id_t)i)<<(4*i-4));
        sprintf(tmp, "%x", EdidVersion);
    }
    setBootEnv(UBOOTENV_EDID_VERSION, tmp);
    return 0;
}

int CTv::Tv_HandeHDMIEDIDFilePathConfig()
{
    if (1 == GetSSMHandleHDMIEdidByCustomerEnableCFG()) {
        //set file's path for hdmi edid of each port
        for (int i = HDMI_PORT_1; i < HDMI_PORT_MAX; i++) {
            Tv_HDMIEDIDFileSelect((tv_hdmi_port_id_t)i, (tv_hdmi_edid_version_t)GetHdmiEdidVersion((tv_hdmi_port_id_t)i));
        }
    }
    mSetHdmiEdid = true;
    return 0;
}

int CTv::Tv_SetDDDRCMode(tv_source_input_t source_input)
{
    if (source_input == SOURCE_DTV) {
        if (GetPlatformHaveDDFlag() == 1) {
            tvWriteSysfs(SYS_AUIDO_DSP_AC3_DRC, (char *)"drcmode 3");
        }
    } else {
        if (GetPlatformHaveDDFlag() == 1) {
            tvWriteSysfs(SYS_AUIDO_DSP_AC3_DRC, (char *)"drcmode 2");
        }
    }
    return 0;
}

int CTv::Tv_SetVdinForPQ (int gameStatus, int pcStatus, int autoSwitchFlag)
{
    LOGD("%s: game mode: %d, pc mode: %d, autoSwitchFlag: %d\n", __FUNCTION__, gameStatus, pcStatus, autoSwitchFlag);

    int ret = -1;
    if (pcStatus == MODE_ON) {
        CVpp::getInstance()->enableMonitorMode(true);
    } else if (pcStatus == MODE_OFF){
        CVpp::getInstance()->enableMonitorMode(false);
    } else {
        LOGD("%s: not about pc mode!\n", __FUNCTION__);
    }

    if (gameStatus != MODE_STABLE) {
        ret = mpTvin->VDIN_SetGameMode((pq_status_update_e)gameStatus);
    }

    if (autoSwitchFlag == PQ_MODE_SWITCH_TYPE_AUTO) {
        MnoNeedAutoSwitchToMonitorMode = false;
    } else if (autoSwitchFlag == PQ_MODE_SWITCH_TYPE_MANUAL) {
        MnoNeedAutoSwitchToMonitorMode = true;
        tvin_port_t cur_port = mpTvin->Tvin_GetSourcePortBySourceInput(m_source_input);
        mpTvin->SwitchPort (cur_port);
    }

    return 0;
}

int CTv::Tv_SetWssStatus (int status)
{
    return mpTvin->VDIN_SetWssStatus(status);
}

int CTv::Tv_Easupdate()
{
    int ret = 0;
    if (mTvEas->mEasScanHandle != NULL) {
        ret = mTvEas->StopEasUpdate();
        if (ret < 0) {
            LOGD("StopEasUpdate error!\n");
            return ret;
        }
    }

    return mTvEas->StartEasUpdate();
}

int CTv::Tv_SetDeviceIdForCec(int deviceId)
{
    //send source switch event for cec while HDMI source
    TvEvent::SourceSwitchEvent ev;
    ev.DestSourceInput = deviceId;
    sendTvEvent(ev);
    return 0;
}
//audio

void CTv::updateSubtitle(int pic_width, int pic_height)
{
    TvEvent::SubtitleEvent ev;
    ev.pic_width = pic_width;
    ev.pic_height = pic_height;
    sendTvEvent(ev);
}

int CTv::SendCmdToOffBoardFBCExternalDac(int cmd __unused, int para __unused)
{
    /*int set_val = 0;
    CFbcCommunication *pFBC = GetSingletonFBC();
    if (pFBC != NULL) {
        if (cmd == AUDIO_CMD_SET_MUTE) {
            if (para == CC_AUDIO_MUTE) {
                set_val = CC_MUTE_ON;
            } else if (para == CC_AUDIO_UNMUTE) {
                set_val = CC_MUTE_OFF;
            } else {
                return -1;
            }

            return pFBC->cfbc_Set_Mute(COMM_DEV_SERIAL, set_val);
        } else if (cmd == AUDIO_CMD_SET_VOLUME_BAR) {
            LOGD("%s, send AUDIO_CMD_SET_VOLUME_BAR (para = %d) to fbc.\n", __FUNCTION__, para);
            return pFBC->cfbc_Set_Volume_Bar(COMM_DEV_SERIAL, para);
        } else if (cmd == AUDIO_CMD_SET_BALANCE) {
            LOGD("%s, send AUDIO_CMD_SET_BALANCE (para = %d) to fbc.\n", __FUNCTION__, para);
            return pFBC->cfbc_Set_Balance(COMM_DEV_SERIAL, para);
        } else if (cmd == AUDIO_CMD_SET_SOURCE) {
            LOGD("%s, send AUDIO_CMD_SET_SOURCE (para = %d) to fbc.\n", __FUNCTION__, para);
            return pFBC->cfbc_Set_FBC_Audio_Source(COMM_DEV_SERIAL, para);
        }
    }*/
    return 0;
}

int CTv::GetHdmiAvHotplugDetectOnoff()
{
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_SSM_HDMI_AV_DETECT, "null" );
    if ( strtoul(value, NULL, 10) == 1 ) {
        return 1;
    }

    return 0;
}

int CTv::SetHdmiEdidVersion(tv_hdmi_port_id_t port, tv_hdmi_edid_version_t version)
{
    LOGD("%s: port: %d, curent source: %d\n", __FUNCTION__, port, m_source_input);
    if (m_source_input == (port + 4) ) {
        mAv.DisableVideoWithBlackColor();
        mpTvin->Tvin_StopDecoder();
        Tv_HDMIEDIDFileSelect(port, version);
        SSMSetHDMIEdid(port);
        mHDMIRxManager.HdmiRxEdidUpdate();
    }
    return 0;
}

int CTv::GetHdmiEdidVersion(tv_hdmi_port_id_t port)
{
    return SSMReadHDMIEdidVersion(port);
}

int CTv::SaveHdmiEdidVersion(tv_hdmi_port_id_t port, tv_hdmi_edid_version_t version)
{
    return SSMSaveHDMIEdidVersion(port, version);
}

void CTv::SetHdmiEdidForUboot(void)
{
    char tmp[32] = {0};
    //set edid file path
    setBootEnv(UBOOTENV_EDID14_PATH, TV_CONFIG_EDID14_FILE_PATH);
    setBootEnv(UBOOTENV_EDID20_PATH, TV_CONFIG_EDID20_FILE_PATH);
    //set hdmi port map
    int portmap = mHDMIRxManager.CalHdmiPortCecPhysicAddr();
    sprintf(tmp, "%x", portmap);
    setBootEnv(UBOOTENV_PORT_MAP, tmp);
    //set edid version
    memset(tmp, 0x0, sizeof(tmp));
    int EdidVersion = 0;
    for (int i=HDMI_PORT_1;i<HDMI_PORT_MAX;i++) {
        EdidVersion |= (SSMReadHDMIEdidVersion((tv_hdmi_port_id_t)i)<<(4*i-4));
        sprintf(tmp, "%x", EdidVersion);
    }
    setBootEnv(UBOOTENV_EDID_VERSION, tmp);
}

int CTv::SetHdmiHDCPSwitcher(tv_hdmi_hdcpkey_enable_t enable)
{
    mHDMIRxManager.HdmiRxHdcpOnOff(enable);
    return 0;
}

int CTv::SetHdmiColorRangeMode(tvin_color_range_t range_mode)
{
    LOGD("%s, mode: %d!\n", __FUNCTION__, range_mode);
    int ret = mpTvin->VDIN_SetColorRangeMode(range_mode);
    if (ret >= 0) {
        SSMSaveHDMIColorRangeMode(range_mode);
    }

    return ret;
}

int CTv::GetHdmiColorRangeMode()
{
    int mode = 0;
    SSMReadHDMIColorRangeMode(&mode);
    if ((mode > TVIN_COLOR_RANGE_LIMIT) || (mode < TVIN_COLOR_RANGE_AUTO)) {
        LOGE("%s error!\n", __FUNCTION__);
        mode = TVIN_COLOR_RANGE_AUTO;
    }
    LOGD("%s: mode = %d!\n", __FUNCTION__, mode);
    return mode;
}

tvin_port_t CTv::Tv_GetHdmiPortBySourceInput(tv_source_input_t source_input)
{
    tv_source_input_type_t source_type = mpTvin->Tvin_SourceInputToSourceInputType(source_input);
    if (source_type == SOURCE_TYPE_HDMI) {
        return mpTvin->Tvin_GetSourcePortBySourceInput(source_input);
    } else {
        LOGE("Not HDMI source!\n");
        return TVIN_PORT_NULL;
    }
}

int CTv::SetVideoAxis(int x, int y, int width, int heigth)
{
    return mAv.setVideoAxis(x, y, width, heigth);
}

int CTv::handleGPIO(const char *port_name, bool is_out, int edge)
{
    return pGpio->processCommand(port_name, is_out, edge);
}

int CTv::KillMediaServerClient()
{
    char buf[PROPERTY_VALUE_MAX] = { 0 };
    int len = property_get("media.player.pid", buf, "");
    if (len > 0) {
        char* end;
        int pid = strtol(buf, &end, 0);
        if (end != buf) {
            LOGD("[ctv] %s, remove video path, but video decorder has used, kill it:%d\n",
                __FUNCTION__, pid);
            kill(pid, SIGKILL);
            property_set("media.player.pid", "");
        }
    }
    return 0;
}

int CTv::autoSwitchToMonitorMode()
{
    int ret = -1;
    if (!MnoNeedAutoSwitchToMonitorMode) {
        LOGD("%s, PQ mode set by signal!\n", __FUNCTION__);
        tvin_latency_t allmInfo;
        memset(&allmInfo, 0x0, sizeof(tvin_latency_t));
        mpTvin->VDIN_GetAllmInfo(&allmInfo);
        LOGD("%s: allm_mode is %d, it_content is %d, cn_type is %d!\n", __FUNCTION__, allmInfo.allm_mode,
                                   allmInfo.it_content, allmInfo.cn_type);

        int CurrentMode = CVpp::getInstance()->GetPQMode(m_source_input);
        LOGD("%s, CurrentMode is %d!\n", __FUNCTION__, CurrentMode);
        vpp_picture_mode_t NewMode = VPP_PICTURE_MODE_STANDARD;

        if (allmInfo.allm_mode) {
            LOGD("%s autoswitch to game mode!\n", __FUNCTION__);
            NewMode = VPP_PICTURE_MODE_GAME;
        } else if (allmInfo.it_content){
            if (allmInfo.cn_type == GAME) {//GAME
                LOGD("%s autoswitch to game mode!\n", __FUNCTION__);
                NewMode = VPP_PICTURE_MODE_GAME;
            } else {//Graphics/photo/cinema
                LOGD("%s:vesa fmt, autoswitch to monitor mode!\n", __FUNCTION__);
                NewMode = VPP_PICTURE_MODE_MONITOR;
            }
        } else if (m_cur_sig_info.is_dvi) {//dvi fmt
            LOGD("%s: DVI fmt, autoswitch to monitor mode!\n", __FUNCTION__);
            NewMode = VPP_PICTURE_MODE_MONITOR;
        } else {
            if ((CurrentMode == VPP_PICTURE_MODE_GAME) || (CurrentMode == VPP_PICTURE_MODE_MONITOR)) {
                LOGD("%s, auto switch to standard mode!\n", __FUNCTION__);
                NewMode = VPP_PICTURE_MODE_STANDARD;
            } else {
                LOGD("%s, Signal don't match autoswitch condition!\n", __FUNCTION__);
                NewMode = (vpp_picture_mode_t)CurrentMode;
            }
        }

        ret = CVpp::getInstance()->SetPQMode(NewMode, m_source_input, m_cur_sig_info.fmt, m_cur_sig_info.trans_fmt, INDEX_2D, 1, 1);
    }else {
        LOGD("%s, PQ mode set by user!\n", __FUNCTION__);
    }

    return ret;
}

void CTv::onVdinSignalChange()
{
    AutoMutex _l( mLock );
    LOGD("%s: mTvAction = 0x%x, m_source_input = %d\n", __FUNCTION__, mTvAction, m_source_input);
    if (!((mTvAction & TV_ACTION_IN_VDIN) || (mTvAction & TV_ACTION_SCANNING)) || (SOURCE_SPDIF == m_source_input)) {
        return;
    }

    int ret = mpTvin->VDIN_GetSignalInfo ( &m_cur_sig_info );
    if (ret < 0) {
        LOGD("Get Signal Info error!\n");
        m_cur_sig_info.status = TVIN_SIG_STATUS_NULL;
    }

    LOGD("%s: trans_fmt is %d, sig_fmt is %d, status is %d, isDVI is %d, hdr_info is 0x%x\n", __FUNCTION__,
     m_cur_sig_info.trans_fmt, m_cur_sig_info.fmt, m_cur_sig_info.status, m_cur_sig_info.is_dvi, m_cur_sig_info.hdr_info);
    if ( m_cur_sig_info.status == TVIN_SIG_STATUS_STABLE ) {
        onSigToStable();
        onSigStillStable();
    } else if ( m_cur_sig_info.status == TVIN_SIG_STATUS_UNSTABLE ) {
        onSigToUnstable();
    } else if ( m_cur_sig_info.status == TVIN_SIG_STATUS_NOTSUP ) {
        onSigToUnSupport();
    } else if ( m_cur_sig_info.status == TVIN_SIG_STATUS_NOSIG ) {
        onSigToNoSig();
    } else {
        InitCurrenSignalInfo();
    }
}

CTvRecord *CTv::getRecorder(const char *id, const char *param) {
    CTvRecord *recorder = RecorderManager::getInstance().getDev(id);
    if (recorder) {
        recorder->stop(NULL);
        recorder->setupDefault(param);
        return recorder;
    }
    recorder = new CTvRecord();
    recorder->setupDefault(param);
    recorder->setId(id);
    int err = RecorderManager::getInstance().addDev(*recorder);
    if (err) {
        LOGE("create Recorder(%s) fail(%d)", toReadable(id), err);
        delete recorder;
        recorder = NULL;
    }
    return recorder;
}

int CTv::prepareRecording(const char *id, const char *param)
{
    //validate param
    if (!getRecorder(id, param))
        return -1;
    return 0;
}

int CTv::startRecording(const char *id, const char *param, CTvRecord::IObserver *observer)
{
    CTvRecord *recorder = NULL;
    int ret = -1;
    if (!(recorder = getRecorder(id, param))) {
        LOGD("recorder(%s) not found", toReadable(id));
        return -1;
    }
    recorder->setObserver(observer);
    ret = recorder->start(param);
    if (ret != 0)
        RecorderManager::getInstance().removeDev(id);
    return ret;
}

int CTv::stopRecording(const char *id, const char *param)
{
    CTvRecord *recorder = NULL;
    if (!(recorder = RecorderManager::getInstance().getDev(id))) {
        LOGD("recorder(%s) not found", toReadable(id));
        return -1;
    }
    recorder->stop(param);
    recorder->setObserver(NULL);
    RecorderManager::getInstance().removeDev(id);
    return 0;
}

int CTv::doRecordingCommand(int cmd, const char *id, const char *param)
{
    LOGD("doRec(cmd:%d, id:%s, para:%s)", cmd, id, param);
    AutoMutex _l(mLock);
    int ret = -10;
    setDvbLogLevel();
    switch (cmd) {
        case RECORDING_CMD_PREPARE:
        ret = prepareRecording(id, param);
        break;
        case RECORDING_CMD_START:
        ret = startRecording(id, param, &mTvMsgQueue);
        break;
        case RECORDING_CMD_STOP:
        ret = stopRecording(id, param);
        break;
    }
    return ret;
}

CTvPlayer *CTv::getPlayer(const char *id, const char *param) {
    CTvPlayer *player = PlayerManager::getInstance().getDev(id);
    if (player) {
        player->setupDefault(param);
        player->stop(param);
        return player;
    }

    if (PlayerManager::singleMode)
        PlayerManager::getInstance().releaseAll();

    std::string type = paramGetString(param, NULL, "type", "dtv");
    if (type.compare("dtv") == 0)
        player = new CDTVTvPlayer(this);
    else if (type.compare("atv") == 0)
        player = new CATVTvPlayer(this);

    if (player) {
        player->setupDefault(param);
        player->setId(id);
        int err = PlayerManager::getInstance().addDev(*player);
        if (err) {
            LOGE("create Player(%s) fail(%d)", toReadable(id), err);
            delete player;
            player = NULL;
        }
    }
    return player;
}

int CTv::startPlay(const char *id, const char *param)
{
    LOGD("%s\n", __FUNCTION__);
    int ret = -1;
    CTvPlayer *player = NULL;
    if (!(player = getPlayer(id, param))) {
        LOGD("player(%s) not found", toReadable(id));
        return -1;
    }
    ret =  player->start(param);
    if (ret != 0)
        RecorderManager::getInstance().removeDev(id);
    return ret;
}

int CTv::stopPlay(const char *id, const char *param)
{
    LOGD("%s\n", __FUNCTION__);
    CTvPlayer *player = NULL;
    if (!(player = PlayerManager::getInstance().getDev(id))) {
        LOGD("player(%s) not found", toReadable(id));
        return -1;
    }
    player->stop(param);
    PlayerManager::getInstance().removeDev(id);
    return 0;
}

int CTv::pausePlay(const char *id, const char *param)
{
    CTvPlayer *player = NULL;
    if (!(player = PlayerManager::getInstance().getDev(id))) {
        LOGD("player(%s) not found", toReadable(id));
        return -1;
    }
    return player->pause(param);
}

int CTv::resumePlay(const char *id, const char *param)
{
    CTvPlayer *player = NULL;
    if (!(player = PlayerManager::getInstance().getDev(id))) {
        LOGD("player(%s) not found", toReadable(id));
        return -1;
    }
    return player->resume(param);
}

int CTv::seekPlay(const char *id, const char *param)
{
    CTvPlayer *player = NULL;
    if (!(player = PlayerManager::getInstance().getDev(id))) {
        LOGD("player(%s) not found", toReadable(id));
        return -1;
    }
    return player->seek(param);
}

int CTv::setPlayParam(const char *id, const char *param)
{
    CTvPlayer *player = NULL;
    if (!(player = PlayerManager::getInstance().getDev(id))) {
        LOGD("player(%s) not found", toReadable(id));
        return -1;
    }
    return player->set(param);
}

int CTv::doPlayCommand(int cmd, const char *id, const char *param)
{
    LOGD("%s: cmd:%d, id:%s, para:%s\n", __FUNCTION__, cmd, id, param);
    AutoMutex _l(mLock);
    int ret = -10;
    setDvbLogLevel();
    switch (cmd) {
        case PLAY_CMD_START:
        ret = startPlay(id, param);
        break;
        case PLAY_CMD_STOP:
        ret = stopPlay(id, param);
        break;
        case PLAY_CMD_PAUSE:
        ret = pausePlay(id, param);
        break;
        case PLAY_CMD_RESUME:
        ret = resumePlay(id, param);
        break;
        case PLAY_CMD_SEEK:
        ret = seekPlay(id, param);
        break;
        case PLAY_CMD_SETPARAM:
        ret = setPlayParam(id, param);
        break;
    }
    return ret;
}

ANDROID_SINGLETON_STATIC_INSTANCE(RecorderManager);
ANDROID_SINGLETON_STATIC_INSTANCE(PlayerManager);

int CTv::Tv_RrtUpdate(int freq, int modulation, int mode)
{
    int ret = 0;
#ifdef SUPPORT_ADTV
    if (mTvRrt->mRrtScanHandle != NULL) {
        ret = mTvRrt->StopRrtUpdate();
        if (ret < 0) {
            LOGD("Tv_RrtUpdate error!\n");
            return 0;
        }
    }

    if (mode == RRT_MANU_SEARCH) {//manual
        fe_status_t status = (fe_status_t)mFrontDev->getStatus();
        if (status & TV_FE_HAS_LOCK) {
            return mTvRrt->StartRrtUpdate(RRT_MANU_SEARCH);
        }else {
            mFrontDev->setPara(TV_FE_ATSC, freq, modulation, 0);
            return mTvRrt->StartRrtUpdate(RRT_MANU_SEARCH);
        }
    } else {//auto
        return mTvRrt->StartRrtUpdate(RRT_AUTO_SEARCH);
    }
#endif
    return ret;
}

rrt_select_info_t CTv::Tv_RrtSearch(int rating_region_id, int dimension_id, int value_id, int program_id)
{
    rrt_select_info_t tmp;
    memset(&tmp, 0, sizeof(rrt_select_info_t));
#ifdef SUPPORT_ADTV

    int ret = mTvRrt->GetRRTRating(rating_region_id, dimension_id, value_id, program_id, &tmp);
    if (ret < 0) {
        LOGD("Tv_RrtSearch error!\n");
    }

#endif
    return tmp;
}

void CTv::setDvbLogLevel() {
#ifdef SUPPORT_ADTV
    AM_DebugSetLogLevel(property_get_int32("tv.dvb.loglevel", 1));
#endif
}

void CTv::dump(String8 &result)
{
    result.appendFormat("\naction = %x\n", mTvAction);
    result.appendFormat("status = %d\n", mTvStatus);
    result.appendFormat("current source input = %d\n", m_source_input);
    result.appendFormat("last source input = %d\n\n", m_last_source_input);

    result.appendFormat("tvserver git branch:%s\n", tvservice_get_git_branch_info());
    result.appendFormat("tvserver git version:%s\n", tvservice_get_git_version_info());
    result.appendFormat("tvserver Last Changed:%s\n", tvservice_get_last_chaned_time_info());
    result.appendFormat("tvserver Last Build:%s\n", tvservice_get_build_time_info());
    result.appendFormat("tvserver Builer Name:%s\n", tvservice_get_build_name_info());
    result.appendFormat("tvserver board version:%s\n\n", tvservice_get_board_version_info());

#ifdef SUPPORT_ADTV
    result.appendFormat("libdvb git branch:%s\n", dvb_get_git_branch_info());
    result.appendFormat("libdvb git version:%s\n", dvb_get_git_version_info());
    result.appendFormat("libdvb Last Changed:%s\n", dvb_get_last_chaned_time_info());
    result.appendFormat("libdvb Last Build:%s\n", dvb_get_build_time_info());
    result.appendFormat("libdvb Builer Name:%s\n\n", dvb_get_build_name_info());
#endif
}

int CTv::SetAtvAudioOutmode(int mode)
{
#ifdef SUPPORT_ADTV
    return mFrontDev->setVLPropLocked(V4L2_SOUND_SYS, mode);
#else
    return -1;
#endif
}

int CTv::GetAtvAudioOutmode()
{
    int mode = 0;
#ifdef SUPPORT_ADTV
    mFrontDev->getVLPropLocked(V4L2_SOUND_SYS, &mode);
#endif
    return (mode & 0xFF);
}

int CTv::GetAtvAudioInputmode()
{
    int mode = 0;
#ifdef SUPPORT_ADTV
    mFrontDev->getVLPropLocked(V4L2_SOUND_SYS, &mode);
#endif
    /*
     * bit24-16: audio std.
     * bit15-8: audio input type.
     * bit7-0: audio output mode.
     */
    return (mode >> 8 & 0xFFFF);
}

int CTv::GetAtvAutoScanMode()
{
    int mode = -1;
    const char *config_value  = config_get_str(CFG_SECTION_ATV, CFG_ATV_AUTO_SCAN_MODE, "0");
    mode = strtoul(config_value, NULL, 10);

    LOGD("%s: %d", "atv.scan.auto.mode", mode);

    /* @mode
     * 0: use freq table list mode to scan.
     * 1: use step mode to auto scan.
     */
    return mode;
}

int CTv::SetSnowShowEnable(bool enable)
{
    if (enable) {
        mLastScreenMode = mAv.getVideoScreenMode();
        LOGD("%s: Get LastScreenMode = %d", __FUNCTION__, mLastScreenMode);
        mAv.setVideoScreenMode(1);//while show snow,need show full screen
    } else {
        LOGD("%s: Set LastScreenMode = %d", __FUNCTION__, mLastScreenMode);
        mAv.setVideoScreenMode(mLastScreenMode);
    }

    return mpTvin->SwitchSnow(enable);
}

int CTv::TV_SetSameSourceEnable(bool enable)
{
    mbSameSourceEnableStatus = enable;
    return 0;
}
