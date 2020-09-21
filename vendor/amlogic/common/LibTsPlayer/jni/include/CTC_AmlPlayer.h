/*
 * author: bo.cao@amlogic.com
 * date: 2012-07-20
 * wrap original source code for CTC usage
 */

#ifndef _CTC_AMLPLAYER_H_
#define _CTC_AMLPLAYER_H_
//#include "CTsPlayer.h"
#include "CTC_MediaProcessor.h"
using namespace android;

class CTC_AmlPlayer
{
    public:
        CTC_AmlPlayer();
        CTC_AmlPlayer(int count);
        ~CTC_AmlPlayer();
        int  CTC_GetAmlPlayerVersion();//��ȡ�汾
        int  CTC_GetPlayMode();//ȡ�ò���ģʽ
        int  CTC_SetVideoWindow(int x,int y,int width,int height);//������Ƶ��ʾ��λ�ã��Լ���Ƶ��ʾ�Ŀ��
        void CTC_SetSurface(Surface* pSurface);//������ʾ�õ�surface
        void CTC_GetVideoPixels(int& width, int& height);//��ȡ��Ƶ�ֱ���
        bool CTC_IsSoftFit();//�ж��Ƿ����������
        int  CTC_SetEPGSize(int w, int h);//����EPG��С
        int  CTC_VideoShow();//��ʾ��Ƶͼ��
        int  CTC_VideoHide();//������Ƶͼ��
        int  CTC_StartPlay();//begin play after init
        int  CTC_Pause();//��ͣ
        int  CTC_Resume();//��ͣ��Ļָ�
        int  CTC_Fast();//������߿���,
        int  CTC_StopFast();//ֹͣ������߿���
        int  CTC_Stop();//ֹͣ
        int  CTC_Seek();//��λ //���ֱ��cpu clock;  �㲥:npt����
        int  CTC_WriteData(unsigned char* pBuffer, unsigned int nSize);
        int  SoftWriteData(int type, uint8_t *pBuffer, uint32_t nSize, uint64_t timestamp);
        int  CTC_SetVolume(int volume);//�趨����
        int  CTC_GetVolume();//��ȡ����
        int  CTC_SetRatio(int nRatio);//�趨��Ƶ��ʾ����
        int  CTC_GetAudioBalance();//��ȡ��ǰ����
        int  CTC_SetAudioBalance(int nAudioBalance);//��������
        int  CTC_GetCurrentPlayTime();  //get current play time
        int  CTC_SwitchSubtitle(int pid);//switch subtitle
        int  CTC_SwitchAudio(int pid);// switch audio
        int  CTC_InitVideo(void *para);
        int  CTC_InitAudio(void *para);
        int  CTC_InitSubtitle(void *para);
        int CTC_GetAVStatus(float *abuf, float *vbuf);
        int CTC_GetAvBufStatus(AVBUF_STATUS *avstatus);
        int CTC_ClearLastFrame();
        int CTC_GetVideoInfo(int *width, int *height, int *ratio);
        int RegisterCallBack(void *hander, IPTV_PLAYER_PARAM_Evt_e enEvt, IPTV_PLAYER_PARAM_EVENT_CB  pfunc);
        int SetParameter(void *hander, int type, void * ptr);
        int GetParameter(void *hander, int type, void * ptr);
        int Invoke(void *hander, int type, void * inptr, void * outptr);
        void BlackOut(int EarseLastFrame);

    private:
        ITsPlayer  *m_pTsPlayer;

};

#endif  // _CTC_AMLPLAYER_H_
