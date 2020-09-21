/**
 * @file 				Proxy_MediaProcessor.cpp
 * @author    	zhouyj
 * @date      	2012/9/12
 * @version   	ver 1.0
 * @brief     	����CTC_MediaControl���з����Ĵ���ӿڡ�
 * @attention
*/
#define LOG_TAG "Proxy_MediaProcessor"
#include "Proxy_MediaProcessor.h"
#include <utils/Log.h>


Proxy_MediaProcessor::Proxy_MediaProcessor(int use_omx_decoder)
{
#ifdef USE_NEW_CTC_INTERFACE
    ctc_MediaControl = aml::GetMediaProcessor(use_omx_decoder);
#else
    ctc_MediaControl = GetMediaControl(use_omx_decoder);
#endif
}

Proxy_MediaProcessor::~Proxy_MediaProcessor()
{
    ALOGI("%s", __FUNCTION__);
	//ctc_MediaControl.clear();
	delete ctc_MediaControl;
}

int Proxy_MediaProcessor::Proxy_GetMediaControlVersion()
{
	int result = 1;
	return result;
}

int Proxy_MediaProcessor::Proxy_GetPlayMode()
{
	int result = ctc_MediaControl->GetPlayMode();
	return result;
}

bool Proxy_MediaProcessor::Proxy_StartPlay()
{
	bool result = ctc_MediaControl->StartPlay();
	return result;
}

int Proxy_MediaProcessor::Proxy_WriteData(unsigned char* pBuffer, unsigned int nSize)
{
	int result = ctc_MediaControl->WriteData(pBuffer, nSize);
	return result;
}

int Proxy_MediaProcessor::Proxy_SetVideoWindow(int x ,int y, int width, int height)
{
	int result = ctc_MediaControl->SetVideoWindow(x, y, width, height);
	return result;
}

int Proxy_MediaProcessor::Proxy_VideoShow()
{
	int result = ctc_MediaControl->VideoShow();
	return result;
}

int Proxy_MediaProcessor::Proxy_VideoHide()
{
	int result = ctc_MediaControl->VideoHide();
	return result;
}

void Proxy_MediaProcessor::Proxy_InitVideo(PVIDEO_PARA_T pVideoPara)
{
	ctc_MediaControl->InitVideo(pVideoPara);
	return;
}

void Proxy_MediaProcessor::Proxy_InitAudio(PAUDIO_PARA_T pAudioPara)
{
	ctc_MediaControl->InitAudio(pAudioPara);
	return;
}

bool Proxy_MediaProcessor::Proxy_Pause()
{
	bool result = ctc_MediaControl->Pause();
	return result;
}

bool Proxy_MediaProcessor::Proxy_Resume()
{
	bool result = ctc_MediaControl->Resume();
	return result;
}

bool Proxy_MediaProcessor::Proxy_Fast()
{
	bool result = ctc_MediaControl->Fast();
	return result;
}

bool Proxy_MediaProcessor::Proxy_StopFast()
{
	bool result = ctc_MediaControl->StopFast();
	return result;
}

bool Proxy_MediaProcessor::Proxy_Stop()
{
	bool result = ctc_MediaControl->Stop();
	return result;
}

bool Proxy_MediaProcessor::Proxy_Seek()
{
	bool result = ctc_MediaControl->Seek();
	return result;
}

bool Proxy_MediaProcessor::Proxy_SetVolume(int volume)
{
	bool result = ctc_MediaControl->SetVolume(volume);
	return result;
}

int Proxy_MediaProcessor::Proxy_GetVolume()
{
	int result = ctc_MediaControl->GetVolume();
	return result;
}

bool Proxy_MediaProcessor::Proxy_SetRatio(int nRatio)
{
	bool result = true;
#ifdef USE_NEW_CTC_INTERFACE
    ctc_MediaControl->SetRatio((CONTENTMODE_E)nRatio);
#else
    ctc_MediaControl->SetRatio(nRatio);
#endif
	return result;
}

int Proxy_MediaProcessor::Proxy_GetAudioBalance()
{
	int result = ctc_MediaControl->GetAudioBalance();
	return result;
}

bool Proxy_MediaProcessor::Proxy_SetAudioBalance(int nAudioBalance)
{
	bool result = true;
#ifdef USE_NEW_CTC_INTERFACE 
    ctc_MediaControl->SetAudioBalance((ABALANCE_E)nAudioBalance);
#else
    ctc_MediaControl->SetAudioBalance(nAudioBalance);
#endif
	return result;
}

void Proxy_MediaProcessor::Proxy_GetVideoPixels(int& width, int& height)
{
	ctc_MediaControl->GetVideoPixels(width, height);
	return;
}

bool Proxy_MediaProcessor::Proxy_IsSoftFit()
{
	bool result = ctc_MediaControl->IsSoftFit();
	return result;
}

void Proxy_MediaProcessor::Proxy_SetEPGSize(int w, int h)
{
	ctc_MediaControl->SetEPGSize(w, h);
	return;
}

void Proxy_MediaProcessor::Proxy_SetSurface(Surface* pSurface)
{
    ctc_MediaControl->SetSurface(pSurface);
	return;
}

int Proxy_MediaProcessor::Proxy_GetCurrentPlayTime()
{
	long currentPlayTime=0;
	currentPlayTime = ctc_MediaControl->GetCurrentPlayTime()/90;//ms
	return currentPlayTime;
}

void Proxy_MediaProcessor::Proxy_SwitchAudioTrack(int pid)
{
#ifdef USE_NEW_CTC_INTERFACE
    ctc_MediaControl->SwitchAudioTrack(pid, nullptr);
#else
	ctc_MediaControl->SwitchAudioTrack(pid);
#endif
	return;
}

void Proxy_MediaProcessor::Proxy_InitSubtitle(PSUBTITLE_PARA_T sParam)
{
	ctc_MediaControl->InitSubtitle(sParam);
	return;
}

void Proxy_MediaProcessor::Proxy_SwitchSubtitle(int pid)
{
	ctc_MediaControl->SwitchSubtitle(pid);
	return;
}

void Proxy_MediaProcessor::Proxy_playerback_register_evt_cb(IPTV_PLAYER_EVT_CB pfunc, void *hander)
{
	ctc_MediaControl->playerback_register_evt_cb(pfunc, hander);
	return;
}
