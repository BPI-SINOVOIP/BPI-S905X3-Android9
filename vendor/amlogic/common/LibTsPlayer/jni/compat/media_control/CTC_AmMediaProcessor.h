#ifndef _CTC_MEDIA_PROCESSOR_H_
#define _CTC_MEDIA_PROCESSOR_H_


#include <gui/Surface.h>
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/android_view_Surface.h"
//#include "CTsPlayer.h"
using namespace android;

enum STREAM_MEDIA_TYPE_e
{
    STREAM_MEDIA_TYPE_UNKNOW = 0,
    STREAM_MEDIA_TYPE_CHANNEL = 1, //live
    STREAM_MEDIA_TYPE_VOD,         //vod
    STREAM_MEDIA_TYPE_TVOD,         //timeshift
    STREAM_MEDIA_TYPE_LOCAL
};

enum EVENT_MEDIA_TYPE_e
{
    EVENT_MEDIA_END = 2101,
    EVENT_MEDIA_BEGINING = 2102,
    EVENT_MEDIA_PLAYMODE_CHANGE,
    EVENT_MEDIA_ERROR,
    EVENT_MEDIA_IGMP_ERROR,

    // add for hebei mobile
    EVENT_MEDIA_PLAYER_START = 3101,
    EVENT_MEDIA_PLAYER_END = 3102,
    EVENT_MEDIA_PLAYER_BUFFER_BEGIN = 3103,
    EVENT_MEDIA_PLAYER_BUFFER_END = 3104,
};

int CTC_SupportProtocol(const char* url);

typedef void (*IPTV_PLAYER_EVT_CallBack)(EVENT_MEDIA_TYPE_e evt, void *handler, void* pvParam);

class CTC_AmMediaProcessor
{
    public:
        CTC_AmMediaProcessor();
        ~CTC_AmMediaProcessor();
        int  CTC_GetMediaControlVersion();//获取版本
        int  CTC_MediaProcessorInit(char* url); //player init, parser stream info
        int  CTC_MediaProcessorInit(int fd);
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
        int  CTC_Fast(int scale);//快进或者快退,
        int  CTC_StopFast();//停止快进或者快退
        int  CTC_Stop();//停止
        int  CTC_Seek(int64_t seek_timestamp);//定位 //如果直播cpu clock;  点播:npt秒数
        int  CTC_SetVolume(int volume);//设定音量
        int  CTC_GetVolume();//获取音量
        int  CTC_SetRatio(int nRatio);//设定视频显示比例
        int  CTC_GetAudioBalance();//获取当前声道
        int  CTC_SetAudioBalance(int nAudioBalance);//设置声道
        int  CTC_GetCurrentPlayTime();  //get current play time
        int  CTC_SwitchSubtitle(int pid);//switch subtitle
        int  CTC_SwitchAudio(int pid);// switch audio
        void CTC_playerback_register_evt_cb(IPTV_PLAYER_EVT_CallBack pfunc, void *hander); // play info callback
        int  CTC_Release();//release all resource[similar to deconstruct]
        int  CTC_SetNetStreamType(STREAM_MEDIA_TYPE_e eStreamType);
        int  CTC_SetScale(int scale); //set play scale
        int  CTC_PlayByTime(int64_t seektimeMsec);//microsecond
        int  CTC_GetDuraion();
    private:
        void* m_pHandler;
        int mInstanceNo;
};
#endif
