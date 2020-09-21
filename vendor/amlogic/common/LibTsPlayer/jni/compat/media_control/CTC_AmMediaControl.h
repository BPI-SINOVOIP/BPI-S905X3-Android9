/*
 * author: wei.liu@amlogic.com
 * date: 2012-07-12
 * wrap original source code for CTC usage
 */

#ifndef _CTC_AMMEDIACONTROL_H_
#define _CTC_AMMEDIACONTROL_H_
//#include "CTsPlayer.h"
#include "CTC_AmMediaProcessor.h"
#if ANDROID_PLATFORM_SDK_VERSION <= 27
#include <media/CTC_MediaControl.h>
#else
class ICTC_MCNotify {
public:
    ICTC_MCNotify() {}
    virtual ~ICTC_MCNotify() {}
    public:
        virtual void OnNotify(int event, const char* msg) = 0;
};
class ICTC_MediaControl {
public:
    ICTC_MediaControl(){}
    virtual ~ICTC_MediaControl(){}
public:
    virtual void  PlayFromStart(char* url) = 0;
    virtual void  PlayFromStart(int fd) = 0;
    virtual void  Pause() = 0;
    virtual void  Resume() = 0;
    virtual void  PlayByTime(int time) = 0;
    virtual void  Fast(int speed) = 0;
    virtual void Stop() = 0;
    virtual void SetSurface(Surface* pSurface) = 0;
    virtual int GetCurrentPlayTime() = 0;
    virtual int GetDuration() = 0;
    virtual void SetListenNotify(ICTC_MCNotify* notify) = 0;
    virtual void SetVolume(float leftVolume, float rightVolume) = 0;
};
typedef ICTC_MediaControl* (*FGetMediaControl)();
typedef void (*FFreeMediaControl)(ICTC_MediaControl* pMediaContrl);
#endif

class CTC_AmMediaControl :public ICTC_MediaControl {
public:
    CTC_AmMediaControl();
    virtual ~CTC_AmMediaControl();
public:
    virtual void  PlayFromStart(char* url);
    virtual void  PlayFromStart(int fd);
    virtual void  Pause();
    virtual void  Resume();
    virtual void  PlayByTime(int time);
    virtual void  Fast(int speed);
    virtual void Stop();
    virtual void SetSurface(Surface* pSurface);
    virtual int GetCurrentPlayTime();
    virtual int GetDuration();
    virtual void SetListenNotify(ICTC_MCNotify* notify);
    virtual void SetVolume(float leftVolume, float rightVolume);
private:
    CTC_AmMediaProcessor *pAmMediaProce;
    ICTC_MCNotify* mNotify;
};
//typedef CTC_MediaControl* (*FGetMediaControl)();
//typedef void (*FFreeMediaControl)(CTC_MediaControl* pMediaContrl);


#endif  // _CTC_AMMEDIACONTROL_H_
