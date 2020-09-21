/*
 * author: wei.liu@amlogic.com
 * date: 2012-07-12
 * wrap original source code for CTC usage
 */

//#include "CTC_MediaControl.h"
//#include "CTsOmxPlayer.h"
#include <cutils/properties.h>
#include "CTC_AmMediaControl.h"


#define LOG_TAG "CTC_AmMediaControl"


using namespace android;


#if 1
extern "C" ICTC_MediaControl* GetMediaControl()
{
    ALOGD("GetMediaControl\n");
    return new CTC_AmMediaControl();
}

extern "C" void FreeMediaControl(ICTC_MediaControl *p)
{
    ALOGD("FreeMediaControl\n");
    delete p;
}


#endif


CTC_AmMediaControl::CTC_AmMediaControl()
{
    ALOGD("CTC_MediaControl create\n");
    pAmMediaProce = new CTC_AmMediaProcessor();
    mNotify = NULL;
}

CTC_AmMediaControl::~CTC_AmMediaControl()
{
    ALOGD("CTC_MediaControl destruct\n");
    if (pAmMediaProce != NULL) {
        delete pAmMediaProce;
        pAmMediaProce = NULL;
    }
}
//static int mplayer_count = 0;
void  CTC_AmMediaControl::PlayFromStart(char* url)
{
    //check url
    pAmMediaProce->CTC_MediaProcessorInit(url);

    //set stream type
    STREAM_MEDIA_TYPE_e eStreamType;
    eStreamType = STREAM_MEDIA_TYPE_VOD;
    pAmMediaProce->CTC_SetNetStreamType(eStreamType);

   // ALOGD("--PlayFromStart,mplayer_count=%d---\n", mplayer_count);

   // pAmMediaProce->CTC_SetVideoWindow((mplayer_count%3)*630, (mplayer_count/3)*430, 620, 420);
   // mplayer_count++;

#if 0
    if (mplayer_count == 0) {
        pAmMediaProce->CTC_SetVideoWindow(10, 10, 700, 300);
        mplayer_count++;
    } else if (mplayer_count == 1) {
        pAmMediaProce->CTC_SetVideoWindow(10, 400, 700, 300);
    } else if (mplayer_count == 2) {

    }
#endif
    //start play
    pAmMediaProce->CTC_StartPlay();

}

void  CTC_AmMediaControl::PlayFromStart(int fd)
{
    //check url
    pAmMediaProce->CTC_MediaProcessorInit(fd);
#if 0
    //set stream type
    STREAM_MEDIA_TYPE_e eStreamType;
    eStreamType = STREAM_MEDIA_TYPE_VOD;
    pAmMediaProce->CTC_SetNetStreamType(eStreamType);

    //start play
    pAmMediaProce->CTC_StartPlay();
#endif
}


void  CTC_AmMediaControl::Pause()
{
    pAmMediaProce->CTC_Pause();
}

void  CTC_AmMediaControl::Resume()
{
    pAmMediaProce->CTC_Resume();

}

void  CTC_AmMediaControl::PlayByTime(int mtime)
{
    pAmMediaProce->CTC_PlayByTime(mtime);
}

void  CTC_AmMediaControl::Fast(int speed)
{
    pAmMediaProce->CTC_Fast(speed);
}

void CTC_AmMediaControl::Stop()
{
    pAmMediaProce->CTC_Stop();
}

void CTC_AmMediaControl::SetSurface(Surface* pSurface)
{
    pAmMediaProce->CTC_SetSurface(pSurface);
}

int CTC_AmMediaControl::GetCurrentPlayTime()
{
    int time = 0;
    time = pAmMediaProce->CTC_GetCurrentPlayTime();
    return time;
}

int CTC_AmMediaControl::GetDuration()
{
    int time = 0;
    time = pAmMediaProce->CTC_GetDuraion();
    return time;

}

void CTC_AmMediaControl::SetListenNotify(ICTC_MCNotify* notify)
{
    mNotify = notify;
}

void CTC_AmMediaControl::SetVolume(float leftVolume, float rightVolume)
{
    int volume = (leftVolume + rightVolume) * 100 / 2;
    pAmMediaProce->CTC_SetVolume(volume);
}
