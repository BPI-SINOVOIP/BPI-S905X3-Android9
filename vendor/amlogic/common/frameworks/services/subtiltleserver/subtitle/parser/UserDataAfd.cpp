#define LOG_TAG "UserDataAfd"
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <utils/Log.h>

#include <UserDataAfd.h>
#include "VideoInfo.h"




#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  TRACE()    LOGI("[%s::%d]\n",__FUNCTION__,__LINE__)

UserDataAfd *UserDataAfd::sInstance = nullptr;

void UserDataAfd::notifyCallerAfdChange(int afd) {
    //LOGI("afd_evt_callback, afdValue:%x", afd);
    if (mNotifier != nullptr) {
        mNotifier->onVideoAfdChange(afd);
    }
}


UserDataAfd *UserDataAfd::getCurrentInstance() {
    return UserDataAfd::sInstance;
}


//afd callback
void afd_evt_callback(long devno, int eventType, void *param, void *userdata) {
    (void)devno;
    (void)eventType;
    (void)userdata;
    int afdValue;
    AM_USERDATA_AFD_t *afd = (AM_USERDATA_AFD_t *)param;
    afdValue = afd->af;
    UserDataAfd *instance = UserDataAfd::getCurrentInstance();
    if (instance != nullptr && afdValue != instance->mNewAfdValue) {
        instance->notifyCallerAfdChange(afdValue);
        instance->mNewAfdValue = afdValue;
        LOGI("AFD callback, value:0x%x", instance->mNewAfdValue);
    }
}

UserDataAfd::UserDataAfd() {
    mNewAfdValue = 0;
    mNotifier = nullptr;

    LOGI("creat UserDataAfd");
    sInstance = this;

}

UserDataAfd::~UserDataAfd() {
    LOGI("~UserDataAfd");
    sInstance = nullptr;
    mNewAfdValue = -1;
    if (mThread != nullptr) {
        mThread->join();
    }

}
int UserDataAfd::start(ParserEventNotifier *notify)
{
    mNotifier = notify;
    //get Video Format need some time, will block main thread, so start a thread to open userdata.
    mThread = std::shared_ptr<std::thread>(new std::thread(&UserDataAfd::run, this));
    return 1;
}

void UserDataAfd::run() {
    int mode;
    AM_USERDATA_OpenPara_t para;
    memset(&para, 0, sizeof(para));
    para.vfmt = VideoInfo::Instance()->getVideoFormat();

    if (AM_USERDATA_Open(USERDATA_DEVICE_NUM, &para) != AM_SUCCESS) {
         LOGI("Cannot open userdata device %d", USERDATA_DEVICE_NUM);
         goto error;
    }

    //add notify afd change
    LOGI("start afd notify change!");
    AM_USERDATA_GetMode(USERDATA_DEVICE_NUM, &mode);
    AM_USERDATA_SetMode(USERDATA_DEVICE_NUM, mode | AM_USERDATA_MODE_AFD);
    AM_EVT_Subscribe(USERDATA_DEVICE_NUM, AM_USERDATA_EVT_AFD, afd_evt_callback, NULL);
error:
    LOGE("open userdata device failed!");
}


int UserDataAfd::stop() {
    //LOGI("stopUserData");
    AM_USERDATA_Close(USERDATA_DEVICE_NUM);
    return 0;
}

/*void UserDataAfd::dump(int fd, const char *prefix) {
    dprintf(fd, "%s Closed Caption Parser\n", prefix);
    dumpCommon(fd, prefix);
    dprintf(fd, "\n");
    dprintf(fd, "%s   Device  No: %d\n", prefix, mDevNo);
    dprintf(fd, "\n");
}*/
