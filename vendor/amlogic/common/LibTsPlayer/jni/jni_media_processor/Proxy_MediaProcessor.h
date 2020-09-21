/**
 * @file 				Proxy_MediaProcessor.h
 * @author    	zhouyj
 * @date      	2012/9/12
 * @version   	ver 1.0
 * @brief     	����CTC_MediaControl���з����Ĵ���ӿڡ�
 * @attention
*/
#ifndef _PROXY_MEDIA_PROCESSOR_H_
#define _PROXY_MEDIA_PROCESSOR_H_


#include <CTC_MediaControl.h>
#include <amports/vformat.h>
#include <amports/aformat.h>
//#include "AudioBufferProvider.h"
//#include "Common.h"
#include <CTC_MediaProcessor.h>
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/android_view_Surface.h"
#if ANDROID_PLATFORM_SDK_VERSION <= 27
#include <CTsOmxPlayer.h>
#include <gui/Surface.h>
#include <gui/ISurfaceTexture.h>
#else
//#include "player_type.h"
#endif
using namespace android;

#define USE_NEW_CTC_INTERFACE

class Proxy_MediaProcessor
{
	protected:
#ifdef USE_NEW_CTC_INTERFACE
        aml::CTC_MediaProcessor* ctc_MediaControl; 
#else
		ITsPlayer* ctc_MediaControl;
#endif
	public:
		Proxy_MediaProcessor(int use_omx_decoder = 0);
		~Proxy_MediaProcessor();
	public:
	//	CTC_MediaControl* Proxy_GetMediaControl();
		int  Proxy_GetMediaControlVersion();//��ȡ�汾
		int  Proxy_GetPlayMode();//ȡ�ò���ģʽ
		int  Proxy_SetVideoWindow(int x,int y,int width,int height);//������Ƶ��ʾ��λ�ã��Լ���Ƶ��ʾ�Ŀ��
		int  Proxy_VideoShow(void);//��ʾ��Ƶͼ��
		int  Proxy_VideoHide(void);//������Ƶͼ��
		void Proxy_InitVideo(PVIDEO_PARA_T pVideoPara);//��ʼ����Ƶ����
		void Proxy_InitAudio(PAUDIO_PARA_T pAudioPara);//��ʼ����Ƶ����
		bool Proxy_StartPlay();//��ʼ����
		int  Proxy_WriteData(unsigned char* pBuffer, unsigned int nSize);//��ts��д�뻺����
		bool Proxy_Pause();//��ͣ
		bool Proxy_Resume();//��ͣ��Ļָ�
		bool Proxy_Fast();//������߿���
		bool Proxy_StopFast();//ֹͣ������߿���
		bool Proxy_Stop();//ֹͣ
		bool Proxy_Seek();//��λ
		bool Proxy_SetVolume(int volume);//�趨����
		int  Proxy_GetVolume();//��ȡ����
		bool Proxy_SetRatio(int nRatio);//�趨��Ƶ��ʾ����
		int  Proxy_GetAudioBalance();//��ȡ��ǰ����
		bool Proxy_SetAudioBalance(int nAudioBalance);//��������
		void Proxy_GetVideoPixels(int& width, int& height);//��ȡ��Ƶ�ֱ���
		bool Proxy_IsSoftFit();//�ж��Ƿ����������
		void Proxy_SetEPGSize(int w, int h);//����EPG��С
		void Proxy_SetSurface(Surface* pSurface);//������ʾ�õ�surface

		int Proxy_GetCurrentPlayTime();
		void Proxy_SwitchAudioTrack(int pid);
		void Proxy_InitSubtitle(PSUBTITLE_PARA_T sParam);
		void Proxy_SwitchSubtitle(int pid);//������ʾ�õ�surface
		void Proxy_playerback_register_evt_cb(IPTV_PLAYER_EVT_CB pfunc, void *hander);
};

#endif
