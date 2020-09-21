/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef VDIN_POSTPROCESSOR_H
#define VDIN_POSTPROCESSOR_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <pthread.h>

#include <HwDisplayCrtc.h>
#include <HwDisplayPlane.h>
#include <HwcPostProcessor.h>
#include <FbProcessor.h>
#include <BasicTypes.h>

/*
read back data from vdin, do processor, and repost to another
vout.
*/
class VdinPostProcessor
    :   public HwcPostProcessor {
public:
    VdinPostProcessor();
    ~VdinPostProcessor();

    int32_t setVout(std::shared_ptr<HwDisplayCrtc> & crtc,
        std::vector<std::shared_ptr<HwDisplayPlane>> & planes,
        int w, int h);
    int32_t setFbProcessor(std::shared_ptr<FbProcessor> & processor);

    int32_t start();
    int32_t stop();
    bool running();

    int32_t present(int flags, int32_t fence);

protected:
    static void * threadMain(void * data);

    int32_t process();

    int32_t startVdin();
    int32_t stopVdin();
    /*blocked, push current fb to display.*/
    int32_t postVout(std::shared_ptr<DrmFramebuffer> fb);

protected:
    enum {
        PROCESSOR_START = 0,
        PROCESSOR_STOP,
    };

    enum {
        PROCESS_ONCE = 0,
        PROCESS_ALWAYS,
        PROCESS_BLANK,
        PROCESS_IDLE,/*nothing to do, util new cmd coming.*/
    };

    enum {
        PRESENT_UPDATE_PROCESSOR = PRESENT_MAX << 1,
    };

    std::shared_ptr<HwDisplayCrtc> mVout;
    std::shared_ptr<HwDisplayPlane> mDisplayPlane;
    std::vector<std::shared_ptr<HwDisplayPlane>> mPlanes;

    int mVoutW;
    int mVoutH;
    /*problems in alloc&mmaper api, must keep it when using here.*/
    std::vector<buffer_handle_t> mVoutHnds;
    std::queue<std::shared_ptr<DrmFramebuffer>> mVoutQueue;

    std::vector<buffer_handle_t> mVdinHnds;
    std::vector<std::shared_ptr<DrmFramebuffer>> mVdinFbs;
    std::queue<int> mVdinQueue;
    int mVdinBufOnScreen;

    std::queue<int> mCmdQ;
    int mProcessMode;

    std::shared_ptr<FbProcessor> mFbProcessor;
    std::queue<std::shared_ptr<FbProcessor>> mReqFbProcessor;

    int mStat;
    pthread_t mThread;
    bool mExitThread;

    std::mutex mMutex;
    std::condition_variable mCmdCond;

};

#endif
