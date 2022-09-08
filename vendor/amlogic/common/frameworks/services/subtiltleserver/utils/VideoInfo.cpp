#define LOG_TAG "SysfsVideoInfo"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "VideoInfo.h"
#include <utils/Log.h>
#include "ParserFactory.h"

#define VIDEO_VDEC_CORE "/sys/class/vdec/core"
#define DEFAULT_VIDEO_WIDTH 720;
#define DEFAULT_VIDEO_HEIGHT 480;


static inline unsigned long sysfsReadInt(const char *path, int base) {
    int fd;
    unsigned long val = 0;
    char bcmd[32];
    ::memset(bcmd, 0, 32);
    fd = ::open(path, O_RDONLY);
    if (fd >= 0) {
        int c = ::read(fd, bcmd, sizeof(bcmd));
        if (c > 0) val = strtoul(bcmd, NULL, base);
        ::close(fd);
    }
    return val;
}


class SysfsVideoInfo : public VideoInfo {
public:
    SysfsVideoInfo() {}
    virtual ~SysfsVideoInfo() {}
    virtual int getVideoWidth() {
        int ret = 0;
        ret = sysfsReadInt("/sys/class/video/frame_width", 10);
        if (ret == 0) {
            ret = DEFAULT_VIDEO_WIDTH;
        }
        return ret;
    }

    virtual int getVideoHeight() {
        int ret = 0;
        ret = sysfsReadInt("/sys/class/video/frame_height", 10);
        if (ret == 0) {
            ret = DEFAULT_VIDEO_HEIGHT;
        }
        return ret;
    }

    /**
     *   helper function to check is h264 or mpeg codec.
     *   CC decoder process deferently in these 2 codecs
    */
    virtual int getVideoFormat() {
        int retry = 100; // check 200ms for parser ready
        while (retry-- >= 0) {
            int fd = ::open(VIDEO_VDEC_CORE, O_RDONLY);
            int bytes = 0;
            if (fd >= 0) {
                uint8_t ubuf8[1025];
                memset(ubuf8, 0, 1025);
                bytes = read(fd, ubuf8, 1024);
                ALOGI("getVideoFormat ubuf8:%s", ubuf8);
                if (strstr((const char*)ubuf8, "amvdec_h264"/*"vdec.h264.00"*/)) {
                    ALOGI("H264_CC_TYPE");
                    close(fd);
                    return H264_CC_TYPE;
                } else if (strstr((const char*)ubuf8, "amvdec_mpeg12"/*"vdec.mpeg12.00"*/)) {
                    ALOGI("MPEG_CC_TYPE");
                    close(fd);
                    return MPEG_CC_TYPE;
                } else if (strstr((const char*)ubuf8, "VDEC_STATUS_CONNECTED")) {
                    ALOGI("vdec has connect, no mpeg or h264 type, return!");
                    close(fd);
                    return INVALID_CC_TYPE;
                }

                close(fd);
            } else {
                ALOGE("open error:%d,%s!!", errno,strerror(errno));
            }
            usleep(20*1000);
        }

        return INVALID_CC_TYPE;
    }


};


VideoInfo *VideoInfo::mInstance = nullptr;

VideoInfo *VideoInfo::Instance() {
    if (mInstance == nullptr) {
        mInstance = new SysfsVideoInfo();
    }
    return mInstance;
}

