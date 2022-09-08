#ifndef SUBTITLE_SERVICE_UTILS_H
#define SUBTITLE_SERVICE_UTILS_H
#if ANDROID_PLATFORM_SDK_VERSION > 27
#include "subtitleServerHidlClient/SubtitleServerHidlClient.h"
#endif

#if ANDROID_PLATFORM_SDK_VERSION > 27
    typedef enum
    {
        SUBTITLE_UNAVAIABLE,
        SUBTITLE_AVAIABLE
    }SUBTITLE_STATE;

    typedef enum
    {
         SUBTITLE_EVENT_DATA,
         SUBTITLE_EVENT_NONE
    }SUBTITLE_EVENT;

    typedef void (*AM_SUBTITLEEVT)(SUBTITLE_EVENT evt, int index);
    typedef void (*AM_SUBTITLELIS)(SUBTITLE_STATE state, int val);

    typedef struct
    {
        AM_SUBTITLEEVT sub_evt;
        AM_SUBTITLELIS  available;
    }AM_SUBTITLE_Para_t;

    typedef void (*notifyAvailable) (int available);
    void subtitleCreat();
    void subtitleDestory();
    void subtitleOpen(char* path, void *pthis, android::SubtitleServerHidlClient::SUB_Para_t * para);
    void subtitleSetSubPid(int pid);
    int subtitleGetSubHeight();
    const char* subtitleGetLanguage();
    int subtitleGetSubWidth();
    void subtitleSetSubType(int type);
    void registerSubtitleMiddleListener();
    void subtitle_register_available(AM_SUBTITLELIS sub);
    void subtitle_register_event(AM_SUBTITLEEVT evt);
    void switchSubtitle(android::SubtitleServerHidlClient::SUB_Para_t * para);
#else
    void subtitleOpen(char* path, void *pthis);
    char* subtitleGetLanguage(int idx);
#endif

    void subtitleShow();

    void subtitleOpenIdx(int idx);
    void subtitleClose();
    int subtitleGetTotal();
    void subtitleNext();
    void subtitlePrevious();
    void subtitleShowSub(int pos);
    void subtitleOption();
    int subtitleGetType();
    char* subtitleGetTypeStr();
    int subtitleGetTypeDetial();
    void subtitleSetTextColor(int color);
    void subtitleSetTextSize(int size);
    void subtitleSetGravity(int gravity);
    void subtitleSetTextStyle(int style);
    void subtitleSetPosHeight(int height);
    void subtitleSetImgRatio(float ratioW, float ratioH, int maxW, int maxH);
    void subtitleClear();
    void subtitleResetForSeek();
    void subtitleHide();
    void subtitleDisplay();
    char* subtitleGetCurName();
    char* subtitleGetName(int idx);
    void subtitleLoad(char* path);
    void subtitleSetSurfaceViewParam(int x, int y, int w, int h);

#endif
