#define LOG_TAG "subtitleMiddleClient"

#include <ISubTitleService.h>
#include <binder/IServiceManager.h>
#include "subtitleservice.h"
#include "CTsPlayer.h"
#include "CTsPlayerImpl.h"
#include "Amsysfsutils.h"
#include "string.h"

#include <binder/Binder.h>

#include <utils/Atomic.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/threads.h>
#include <unistd.h>


using namespace android;

#define STR_LEN 256
class DeathNotifier: public IBinder::DeathRecipient
{
    public:
        DeathNotifier() {
        }

        void binderDied(const wp<IBinder>& who) {
            ALOGW("subtitleservice died!");
        }
};

static sp<ISubTitleService> subtitleservice;
static sp<DeathNotifier> amDeathNotifier;
static Mutex amgLock;
static int RETRY_MAX = 3;
int mTotal = -1;
int mRetry = RETRY_MAX;
pthread_t mSubtitleThread;
bool mThreadStop = false;
CTsPlayer* ctc_mp = NULL;

int mpos = 0;

void *thrSubtitleShow(void *pthis)
{
    int pos = 0;
    //ALOGE("thrSubtitleShow pos:%d\n",pos);
    unsigned long firstvpts = 0;

    do {
        /*firstvpts = amsysfs_get_sysfs_ulong("/sys/class/tsync/firstvpts");
		if (firstvpts == 0) {
			pos = 0;
		} else {
			if (subtitleGetTypeDetial() == 6) {//dvb sub  return pts_video
				pos = (ctc_mp->GetCurrentPlayTime()/90);
			} else {
				pos = ((ctc_mp->GetCurrentPlayTime() - firstvpts)/90);
			}
		}
        pos = ctc_mp->GetCurrentPlayTime();*/
        firstvpts = amsysfs_get_sysfs_ulong("/sys/class/tsync/firstvpts");
        if (firstvpts == 0) {
            pos = 0;
         } else {
             if (subtitleGetTypeDetial() == 6) {//dvb sub  return pts_video
                  pos = (ctc_mp->GetCurrentPlayTime()/90);
             } else {
                 pos = ((ctc_mp->GetCurrentPlayTime() - firstvpts)/90);
             }
        }
        //ALOGE("1thrSubtitleShow pos:%d, sleep:%d\n", pos, (300 - (pos % 300)
));
        subtitleShowSub(pos);
        usleep((300 - (pos % 300)) * 1000);
    }
    while (!mThreadStop);
    return NULL;
}

void subtitleShow()
{
    int err;
    //ALOGE("subtitleShow 0 mTotal:%d, mRetry:%d\n", mTotal, mRetry);
    /*do {
        mTotal = subtitleGetTotal();
        //ALOGE("subtitleShow 1 mTotal:%d, mRetry:%d\n", mTotal, mRetry);
        mRetry--;
        usleep(500000); // 0.5 s
    }while(mTotal == -1 && mRetry > 0);*/

    //if (mTotal > 0) {
        pthread_create(&mSubtitleThread, NULL, thrSubtitleShow, NULL);
    //}
}

const sp<ISubTitleService>& getSubtitleService()
{
    Mutex::Autolock _l(amgLock);
    if (subtitleservice.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("subtitle_service"));
            if (binder != 0)
                break;
            ALOGW("subtitle_service not published, waiting...");
            usleep(500000); // 0.5 s
        } while(true);
        if (amDeathNotifier == NULL) {
            amDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(amDeathNotifier);
        subtitleservice = interface_cast<ISubTitleService>(binder);
    }
    ALOGE_IF(subtitleservice == 0, "no subtitle_service!!");
    return subtitleservice;
}

void subtitleOpen(char* path, void *pthis)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->open(String16(path));
        mRetry = RETRY_MAX;
        mThreadStop = false;
    }

    ctc_mp = static_cast<CTsPlayer *>(pthis);
    return;
}

void subtitleOpenIdx(int idx)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->openIdx(idx);
    }
    return;
}

void subtitleSetVideoFormat(int vfmt)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setVideoFormat(vfmt);
    }
    return;
}

void subtitleClose()
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->close();
    }
    mThreadStop = true;
    pthread_join(mSubtitleThread, NULL);
    return;
}

int subtitleGetTotal()
{
    int ret = -1;
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->getTotal();
    }
    return ret;
}

void subtitleNext()
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->next();
    }
    return;
}

void subtitlePrevious()
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->previous();
    }
    return;
}

void subtitleShowSub(int pos)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        //ALOGE("subtitleShowSub pos:%d\n", pos);
        subser->showSub(pos);
    }
    return;
}

void subtitleOption()
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->option();
    }
    return;
}

int subtitleGetType()
{
    int ret = 0;
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->getType();
    }
    return ret;
}

char* subtitleGetTypeStr()
{
    char* ret;
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        //@@ret = String8(subser->getTypeStr()).string();
    }
    return ret;
}

int subtitleGetTypeDetial()
{
    int ret = 0;
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->getTypeDetial();
    }
    return ret;
}

void subtitleSetTextColor(int color)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setTextColor(color);
    }
    return;
}

void subtitleSetTextSize(int size)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setTextSize(size);
    }
    return;
}

void subtitleSetGravity(int gravity)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setGravity(gravity);
    }
    return;
}

void subtitleSetTextStyle(int style)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setTextStyle(style);
    }
    return;
}

void subtitleSetPosHeight(int height)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setPosHeight(height);
    }
    return;
}

void subtitleSetImgRatio(float ratioW, float ratioH, int maxW, int maxH)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setImgRatio(ratioW, ratioH, maxW, maxH);
    }
    return;
}

void subtitleSetSurfaceViewParam(int x, int y, int w, int h)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    ALOGE("subtitleSetSurfaceViewParam 00\n");
    if (subser != 0) {
        ALOGE("subtitleSetSurfaceViewParam 01\n");
        subser->setSurfaceViewParam(x, y, w, h);
    }
    return;
}

void subtitleClear()
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->clear();
    }
    return;
}

void subtitleResetForSeek()
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->resetForSeek();
    }
    return;
}

void subtitleHide()
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->hide();
    }
    return;
}

void subtitleDisplay()
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->display();
    }
    return;
}

char* subtitleGetCurName()
{
    char ret[STR_LEN] = {0,};
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        String16 value;
        value = subser->getCurName();
        memset(ret, 0, STR_LEN);
        strcpy(ret, String8(value).string());
    }
    return ret;
}

char* subtitleGetName(int idx)
{
    char ret[STR_LEN] = {0,};
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        String16 value;
        value = subser->getName(idx);
        memset(ret, 0, STR_LEN);
        strcpy(ret, String8(value).string());
    }
    return ret;
}

char* subtitleGetLanguage(int idx)
{
    char ret[STR_LEN] = {0,};
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        String16 value;
        value = subser->getLanguage(idx);
        memset(ret, 0, STR_LEN);
        strcpy(ret, String8(value).string());
    }
    return ret;
}

void subtitleLoad(char* path)
{
    const sp<ISubTitleService>& subser = getSubtitleService();
    if (subser != 0) {
        subser->load(String16(path));
    }
    return;
}


