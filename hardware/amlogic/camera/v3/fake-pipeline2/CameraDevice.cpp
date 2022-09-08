//#define LOG_NDEBUG  0
//#define LOG_NNDEBUG 0

#define LOG_TAG "VirtualCameraDevice"

#if defined(LOG_NNDEBUG) && LOG_NNDEBUG == 0
#define ALOGVV ALOGV
#else
#define ALOGVV(...) ((void)0)
#endif

#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL | ATRACE_TAG_ALWAYS)
#include <utils/Log.h>
#include <utils/Trace.h>
#include <cutils/properties.h>
#include <android/log.h>
#include "CameraDevice.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define ARRAY_SIZE(x) (sizeof((x))/sizeof(((x)[0])))

CameraVirtualDevice* CameraVirtualDevice::mInstance = nullptr;

struct VirtualDevice CameraVirtualDevice::videoDevices[] = {
        {"/dev/video0",1,{FREED_DEVICE,NONE_DEVICE,NONE_DEVICE},{-1,-1,-1}},
        {"/dev/video1",1,{FREED_DEVICE,NONE_DEVICE,NONE_DEVICE},{-1,-1,-1}},
        {"/dev/video2",1,{FREED_DEVICE,NONE_DEVICE,NONE_DEVICE},{-1,-1,-1}},
        {"/dev/video3",1,{FREED_DEVICE,NONE_DEVICE,NONE_DEVICE},{-1,-1,-1}},
        {"/dev/video4",1,{FREED_DEVICE,NONE_DEVICE,NONE_DEVICE},{-1,-1,-1}},
        {"/dev/video5",1,{FREED_DEVICE,NONE_DEVICE,NONE_DEVICE},{-1,-1,-1}},
        {"/dev/video50",3,{FREED_DEVICE,NONE_DEVICE,FREED_DEVICE},{-1,-1,-1}},
        {"/dev/video51",3,{FREED_DEVICE,NONE_DEVICE,FREED_DEVICE},{-1,-1,-1}}
};

int CameraVirtualDevice::openVirtualDevice(int id) {
    ALOGD("%s: id = %d E", __FUNCTION__,id);
    int count = 0;
    /*scan the device name*/
    for (size_t i = 0; i < ARRAY_SIZE(videoDevices); i++) {
        struct VirtualDevice* pDev = &videoDevices[i];
        if (0 != access(pDev->name, F_OK | R_OK | W_OK)) {
            ALOGD("%s: device %s is invaild", __FUNCTION__,pDev->name);
               continue;
        }
        /*scan free device*/
        ALOGD("%s: scan device %s ", __FUNCTION__,pDev->name);
        for (int j = 0; j < pDev->streamNum; j++) {
            if (pDev->status[j] == FREED_DEVICE) {
                if (count != id) {
                    count++;
                    ALOGD("%s: device number is %d ",__FUNCTION__,count);
                    continue;
                }
                ALOGD("%s:open device %s, device number=%d ",__FUNCTION__,pDev->name,count);
                int fd = open(pDev->name,O_RDWR | O_NONBLOCK);
                if (fd < 0) {
                    ALOGE("open device %s , the %dth stream fail!",pDev->name,j);
                    ALOGE("the reason is %s",strerror(errno));
                    continue;
                }
                else {
                    pDev->status[j] = USED_DEVICE;
                    pDev->cameraId[j] = id;
                    return fd;
                }
            } else {
                count++;
                ALOGI("the device %s is not free",pDev->name);
            }
        }
    }
    return -1;
}

int CameraVirtualDevice::releaseVirtualDevice(int id,int fd) {
    ALOGD("%s: E", __FUNCTION__);
    for (size_t i = 0; i < ARRAY_SIZE(videoDevices); i++) {
        struct VirtualDevice* pDev = &videoDevices[i];
        for (int j = 0; j < pDev->streamNum; j++) {
            ALOGD("%s: id = %d", __FUNCTION__,pDev->cameraId[j]);
            if (pDev->status[j] == USED_DEVICE && pDev->cameraId[j] == id) {
                ALOGD("%s: name = %s", __FUNCTION__,pDev->name);
                pDev->status[j] = FREED_DEVICE;
                pDev->cameraId[j] = -1;
                close(fd);
                return 0;
            }
        }
    }
    return -1;
}

CameraVirtualDevice* CameraVirtualDevice::getInstance() {
    if (mInstance != nullptr) {
        return mInstance;
    } else {
        mInstance = new CameraVirtualDevice();
        return mInstance;
    }
}

int CameraVirtualDevice::getCameraNum() {
    int iCamerasNum = 0;
    ATRACE_CALL();
    for (int i = 0; i < (int)ARRAY_SIZE(videoDevices); i++ ) {
        struct VirtualDevice* pDev = &videoDevices[i];
        if (0 == access(pDev->name, F_OK | R_OK | W_OK)) {
            ALOGD("access %s success\n", pDev->name);
            iCamerasNum++;
        }
    }
    return iCamerasNum;
}
