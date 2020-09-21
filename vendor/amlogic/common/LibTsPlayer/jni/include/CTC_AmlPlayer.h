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
        int  CTC_GetAmlPlayerVersion();//获取版本
        int  CTC_GetPlayMode();//取得播放模式
        int  CTC_SetVideoWindow(int x,int y,int width,int height);//设置视频显示的位置，以及视频显示的宽高
        void CTC_SetSurface(Surface* pSurface);//设置显示用的surface
        void CTC_GetVideoPixels(int& width, int& height);//获取视频分辩率
        bool CTC_IsSoftFit();//判断是否由软件拉伸
        int  CTC_SetEPGSize(int w, int h);//设置EPG大小
        int  CTC_VideoShow();//显示视频图像
        int  CTC_VideoHide();//隐藏视频图像
        int  CTC_StartPlay();//begin play after init
        int  CTC_Pause();//暂停
        int  CTC_Resume();//暂停后的恢复
        int  CTC_Fast();//快进或者快退,
        int  CTC_StopFast();//停止快进或者快退
        int  CTC_Stop();//停止
        int  CTC_Seek();//定位 //如果直播cpu clock;  点播:npt秒数
        int  CTC_WriteData(unsigned char* pBuffer, unsigned int nSize);
        int  SoftWriteData(int type, uint8_t *pBuffer, uint32_t nSize, uint64_t timestamp);
        int  CTC_SetVolume(int volume);//设定音量
        int  CTC_GetVolume();//获取音量
        int  CTC_SetRatio(int nRatio);//设定视频显示比例
        int  CTC_GetAudioBalance();//获取当前声道
        int  CTC_SetAudioBalance(int nAudioBalance);//设置声道
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
