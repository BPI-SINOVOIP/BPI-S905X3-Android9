/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev2.h>
#include <sys/time.h>

#include <utils/KeyedVector.h>
#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utils/threads.h>
#include <android/native_window.h>
#include <system/window.h>
#include <gralloc_priv.h>

#include <aml_screen.h>

namespace android {

#define NB_BUFFER 6

struct VideoInfo {
    struct v4l2_capability cap;
    struct v4l2_format format;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers rb;
    long *mem[NB_BUFFER];
    unsigned canvas[NB_BUFFER];
    unsigned refcount[NB_BUFFER];
    bool isStreaming;
    int width;
    int height;
    int formatIn;
    int framesizeIn;
    int displaymode;
    int dimming_flag;
};
enum State{
    START,
    PAUSE,
    STOPING,
    STOP,
};

enum FrameType{
    NATIVE_WINDOW_DATA = 0x1,
    CALL_BACK_DATA = 0x2,
};

typedef void (*olStateCB)(int state);

typedef void (*app_data_callback)(void *user, aml_screen_buffer_info_t *buff_info);

#define SCREENSOURCE_GRALLOC_USAGE  GRALLOC_USAGE_HW_TEXTURE | \
                                    GRALLOC_USAGE_HW_RENDER | \
                                    GRALLOC_USAGE_SW_READ_RARELY | \
                                    GRALLOC_USAGE_SW_WRITE_NEVER
/**
 * set_port_type() parameter description:
 portType is consisted by 32-bit binary.
 bit 28 : start tvin service flag, 1 : enable,	0 : disable.
 bit 24 : vdin device num : 0 or 1, which means use vdin0 or vdin1.
 bit 15~0 : tvin port type --TVIN_PORT_VIU,TVIN_PORT_HDMI0...
				(port type define in tvin.h)
 */
class vdin_screen_source {
    public:
        vdin_screen_source();
        ~vdin_screen_source();
        int init(int id);
        int start();
        int stop();
        int pause();
        int get_format();
        int set_format(int width = 640, int height = 480, int color_format = V4L2_PIX_FMT_NV21);
        int set_rotation(int degree);
        int set_crop(int x, int y, int width, int height);
        int get_amlvideo2_crop(int *x, int *y, int *width, int *height);
        int set_amlvideo2_crop(int x, int y, int width, int height);
        int aquire_buffer(aml_screen_buffer_info_t *buff_info);
        // int inc_buffer_refcount(int* ptr);
        int release_buffer(long* ptr);
        int set_state_callback(olStateCB callback);
        int set_data_callback(app_data_callback callback, void* user);
        int set_preview_window(ANativeWindow* window);
        int set_frame_rate(int frameRate);
        int get_current_sourcesize(int * width,int * height);
        int set_screen_mode(int mode);
        int start_v4l2_device();
        int stop_v4l2_device();
        int get_port_type();
        int set_port_type(unsigned int portType);
        int set_mode(int display_mode);
        int microdimming(long* src, unsigned char *dest);
    private:
        int init_native_window();
        int workThread();
    private:
        class WorkThread : public Thread {
            vdin_screen_source* mSource;
            public:
                WorkThread(vdin_screen_source* source) :
                    Thread(false), mSource(source) { }
                virtual void onFirstRef() {
                    run("vdin screen source work thread", PRIORITY_URGENT_DISPLAY);
                }
                virtual bool threadLoop() {
                    mSource->workThread();
                    // loop until we need to quit
                    return true;
                }
        };
    private:
        //int mCurrentIndex;
        KeyedVector<long *, long> mBufs;
        KeyedVector<long *, long> mTemp_Bufs;
        int mBufferCount;
        int mFrameWidth;
        int mFrameHeight;
        int mBufferSize;
        unsigned int flex_ratio;
        unsigned int flex_original;
        int mFramecount = 0;
        int m_FrameHeight = 0;
        int m_FrameWidth = 0;
        int m_rest = 0;
        int m_displaymode;
        volatile int mState;
        olStateCB mSetStateCB;
        int mPixelFormat;
        int mNativeWindowPixelFormat;
        sp<ANativeWindow> mANativeWindow;
        sp<WorkThread>   mWorkThread;
        mutable Mutex mLock;
        int mCameraHandle;
        struct VideoInfo *mVideoInfo;
        int mFrameType;
        app_data_callback mDataCB;
        bool mOpen;
        void *mUser;
        long * src_temp[4];
};

}