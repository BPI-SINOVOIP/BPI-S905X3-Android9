/* 
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#define LOG_NDEBUG 0
#define LOG_TAG "CTC_MediaProcessor"
#include "CTC_Log.h"
#include <cutils/properties.h>

#include <CTC_MediaProcessor.h>
#if ANDROID_PLATFORM_SDK_VERSION <= 27
#include "CTsOmxPlayer.h"
#endif
//#include "Amsysfsutils.h"
#include "CTC_Utils.h"
#include "ITsPlayer.h"
#include <gui/Surface.h>

namespace aml {


CTC_MediaProcessor* GetMediaProcessor(int channelNo,  CTC_InitialParameter* initialParam)
{
    uint32_t interfaceExtension = 0;

    if (initialParam != nullptr) {
        interfaceExtension = initialParam->interfaceExtension;
    }

    return new CTC_MediaProcessor(channelNo, initialParam);
}

int GetMediaProcessorVersion()
{
    return 0;
}

CTC_MediaProcessor::CTC_MediaProcessor(int channelNo, CTC_InitialParameter* initialParam)
{
    mTsPlayer = ITsPlayerFactory::instance().createITsPlayer(channelNo, initialParam);
    
    MLOG("mTsPlayer:%p", mTsPlayer);
}

CTC_MediaProcessor::~CTC_MediaProcessor()
{
    MLOG("mTsPlayer:%p, ref count:%d", mTsPlayer, mTsPlayer->getStrongCount());
    mTsPlayer->decStrong(this);
}

int CTC_MediaProcessor::GetPlayMode()
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->GetPlayMode();

    return ret;
}

int CTC_MediaProcessor::SetVideoWindow(int x, int y, int width, int height)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->SetVideoWindow(x, y, width, height);

    return ret;
}

int CTC_MediaProcessor::VideoShow()
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->VideoShow();

    return ret;
}

int CTC_MediaProcessor::VideoHide()
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->VideoHide();

    return ret;
}


void CTC_MediaProcessor::InitVideo(PVIDEO_PARA_T pVideoPara)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->InitVideo(pVideoPara);
}

void CTC_MediaProcessor::InitAudio(PAUDIO_PARA_T pAudioPara)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->InitAudio(pAudioPara);
}

void CTC_MediaProcessor::InitSubtitle(PSUBTITLE_PARA_T pSubtitlePara)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->InitSubtitle(pSubtitlePara);
}

bool CTC_MediaProcessor::StartPlay()
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->StartPlay();

    return ret;
}

int CTC_MediaProcessor::WriteData(unsigned char* pBuffer, unsigned int nSize)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->WriteData(pBuffer, nSize);

    return ret;
}

int CTC_MediaProcessor::SoftWriteData(PLAYER_STREAMTYPE_E type, uint8_t *pBuffer, uint32_t nSize, uint64_t timestamp)
{

    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->SoftWriteData(type, pBuffer, nSize, timestamp);

    return ret;
}

bool CTC_MediaProcessor::Pause()
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->Pause();

    return ret;
}

bool CTC_MediaProcessor::Resume()
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->Resume();

    return ret;
}

bool CTC_MediaProcessor::Fast()
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->Fast();

    return ret;
}

bool CTC_MediaProcessor::StopFast()
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->StopFast();

    return ret;
}

bool CTC_MediaProcessor::Stop()
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->Stop();

    return ret;
}

bool CTC_MediaProcessor::Seek()
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->Seek();

    return ret;
}

int CTC_MediaProcessor::SetVolume(float volume)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->SetVolume(volume);

    return ret;
}

float CTC_MediaProcessor::GetVolume()
{
    RETURN_IF(0.0f, mTsPlayer == nullptr);

    float volume = 1.0f;

    volume = mTsPlayer->GetVolume();

    return volume;
}

int CTC_MediaProcessor::SetRatio(CONTENTMODE_E contentMode)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->SetRatio(contentMode);

    return ret;
}

int CTC_MediaProcessor::GetAudioBalance()
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->GetAudioBalance();

    return ret;
}

bool CTC_MediaProcessor::SetAudioBalance(ABALANCE_E nAudioBalance)
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->SetAudioBalance(nAudioBalance);


    return ret;
}

void CTC_MediaProcessor::GetVideoPixels(int& width, int& height)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->GetVideoPixels(width, height);
}

bool CTC_MediaProcessor::IsSoftFit()
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = false;

    ret = mTsPlayer->IsSoftFit();

    return ret;
}

void CTC_MediaProcessor::SetEPGSize(int w, int h)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->SetEPGSize(w, h);
}

void CTC_MediaProcessor::SetSurface(ANativeWindow* pSurface)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->SetSurface(static_cast<Surface*>(pSurface));
}

void CTC_MediaProcessor::SetSurface(android::Surface* pSurface)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->SetSurface(pSurface);
}

void CTC_MediaProcessor::SwitchAudioTrack(uint16_t pid, PAUDIO_PARA_T pAudioPara)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->SwitchAudioTrack(pid);
}

void CTC_MediaProcessor::SwitchSubtitle(int pid)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->SwitchSubtitle(pid);
}

void CTC_MediaProcessor::SetProperty(int nType, int nSub, int nValue)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->SetProperty(nType, nSub, nValue);
}

int64_t CTC_MediaProcessor::GetCurrentPlayTime()
{
    RETURN_IF(kUnknownPTS, mTsPlayer == nullptr);

    int64_t playTime = kUnknownPTS;

    playTime = mTsPlayer->GetCurrentPlayTime();

    return playTime;
}

void CTC_MediaProcessor::leaveChannel()
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->leaveChannel();
}

void CTC_MediaProcessor::playerback_register_evt_cb(IPTV_PLAYER_EVT_CB pfunc, void* handler)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->playerback_register_evt_cb(pfunc, handler);
}

int CTC_MediaProcessor::playerback_getStatusInfo(IPTV_ATTR_TYPE_e enAttrType, int* value)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    mTsPlayer->playerback_getStatusInfo(enAttrType, value);

    return ret;
}

void CTC_MediaProcessor::ClearLastFrame()
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->ClearLastFrame();
}

void CTC_MediaProcessor::BlackOut(int EarseLastFrame)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->BlackOut(EarseLastFrame);
}

bool CTC_MediaProcessor::SetErrorRecovery(int mode)
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->SetErrorRecovery(mode);

    return ret;
}

int CTC_MediaProcessor::GetAvBufStatus(PAVBUF_STATUS pstatus)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    mTsPlayer->GetAvbufStatus(pstatus);

    return ret;
}

int CTC_MediaProcessor::GetRealTimeFrameRate()
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->GetRealTimeFrameRate();

    return ret;
}

int CTC_MediaProcessor::GetVideoFrameRate()
{
    RETURN_IF(0, mTsPlayer == nullptr);

    int ret = 0;

    ret = mTsPlayer->GetVideoFrameRate();

    return ret;
}

int CTC_MediaProcessor::GetVideoDropNumber()
{
    RETURN_IF(0, mTsPlayer == nullptr);

    int dropNumber = 0;

    dropNumber = mTsPlayer->GetVideoDropNumber();

    return dropNumber;
}

int CTC_MediaProcessor::GetVideoTotalNumber()
{
    RETURN_IF(0, mTsPlayer == nullptr);

    int totalNumber = 0;

    totalNumber = mTsPlayer->GetVideoTotalNumber();

    return totalNumber;
}

void CTC_MediaProcessor::GetVideoResolution()
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->GetVideoResolution();
}

bool CTC_MediaProcessor::SubtitleShowHide(bool bShow)
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    ret = mTsPlayer->SubtitleShowHide(bShow);

    return ret;
}

void CTC_MediaProcessor::SetVideoHole(int x, int y, int w, int h)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->SetVideoHole(x, y, w, h);
}

void CTC_MediaProcessor::writeScaleValue()
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->writeScaleValue();
}

int CTC_MediaProcessor::GetCurrentVidPTS(unsigned long long* pPTS)
{
    RETURN_IF(0, mTsPlayer == nullptr);

    int videoPts = 0;

    videoPts = mTsPlayer->GetCurrentVidPTS(pPTS);

    return videoPts;
}

int CTC_MediaProcessor::GetVideoInfo(int* width, int *height, int* ratio)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    mTsPlayer->GetVideoInfo(width, height, ratio);

    return ret;
}

int CTC_MediaProcessor::GetPlayerInstanceNo()
{
    RETURN_IF(0, mTsPlayer == nullptr);

    int instanceNo = 0;

    instanceNo = mTsPlayer->GetPlayerInstanceNo();

    return instanceNo;
}

void CTC_MediaProcessor::ExecuteCmd(const char* cmd_str)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->ExecuteCmd(cmd_str);
}

int CTC_MediaProcessor::RegisterCallBack(void *hander, IPTV_PLAYER_PARAM_Evt_e enEvt, IPTV_PLAYER_PARAM_EVENT_CB  pfunc)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    ret = mTsPlayer->RegisterCallBack(hander, enEvt, pfunc);

    return ret;
}

void CTC_MediaProcessor::SetParameter(int32_t key, void* request)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->SetParameter(nullptr, key, request);
}

void CTC_MediaProcessor::GetParameter(int32_t key, void* reply)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    mTsPlayer->GetParameter(nullptr, key, reply);
}

int CTC_MediaProcessor::Invoke(void *hander, int type, void * inptr, void * outptr)
{
    return 0;
}

bool CTC_MediaProcessor::GetIsEos()
{
    RETURN_IF(true, mTsPlayer == nullptr);

    bool isEos = false;

    //isEos = mTsPlayer->GetIsEos();

    return isEos;
}

int64_t CTC_MediaProcessor::GetCurrentPts()
{
    RETURN_IF(kUnknownPTS, mTsPlayer == nullptr);

    int64_t currentPts = kUnknownPTS;

    //currentPts = mTsPlayer->GetCurrentPts();

    return currentPts;
}

void CTC_MediaProcessor::SetStopMode(bool bHoldLastPic)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    //mTsPlayer->SetStopMode(bHoldLastPic);
}

int CTC_MediaProcessor::GetBufferStatus(int *totalsiz, int* datasize)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    //ret = mTsPlayer->GetBufferStatus(totalsiz, datasize);

    return ret;
}

int CTC_MediaProcessor::GetStreamInfo(PVIDEO_INFO_T pVideoInfo, PAUDIO_INFO_T pAudioInfo)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    //ret = mTsPlayer->GetStreamInfo(pVideoInfo, pAudioInfo);

    return ret;
}

bool CTC_MediaProcessor::SetDrmInfo(PDRM_PARA_T pDrmPara)
{
    RETURN_IF(false, mTsPlayer == nullptr);

    bool ret = true;

    //ret = mTsPlayer->SetDrmInfo(pDrmPara);

    return ret;
}

int CTC_MediaProcessor::Set3DMode(DISPLAY_3DMODE_E mode)
{
    RETURN_IF(-1, mTsPlayer == nullptr);

    int ret = AML_SUCCESS;

    //ret = mTsPlayer->Set3DMode(mode);

    return ret;
}

void CTC_MediaProcessor::SetSurfaceOrder(SURFACE_ORDER_E pOrder)
{
    RETURN_VOID_IF(mTsPlayer == nullptr);

    //mTsPlayer->SetSurfaceOrder(pOrder);

}


}
