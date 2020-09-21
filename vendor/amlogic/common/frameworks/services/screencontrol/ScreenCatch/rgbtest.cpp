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

#define LOG_NDEBUG 0
#define LOG_TAG "rgb_test"

#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#include <media/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <OMX_IVCommon.h>

#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>

#include <media/stagefright/MetaData.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ScreenCatch.h"
#include "../ScreenManager.h"

/*****************************************************************************/
int main(int argc, char **argv) {

    using namespace android;

    int ret = 0;
    int status;
    int dumpfd;
    int framecount = 0;
    ScreenCatch* mScreenCatch;

    mScreenCatch = new ScreenCatch(1280, 720, 24, AML_CAPTURE_OSD_VIDEO);

    //mScreenCatch->setVideoCrop(100,100,300,400);

    MetaData* pMeta;
    pMeta = new MetaData();
    pMeta->setInt32(kKeyColorFormat, OMX_COLOR_Format32bitARGB8888);
    mScreenCatch->start(pMeta);
    char dump_path[128];
    char dump_dir[64] = "/data/temp";

    MediaBuffer *buffer;
    while (framecount < 1) {
        status = mScreenCatch->read(&buffer);
        if (status != OK) {
            usleep(1000);
            continue;
        }
        framecount++;
        sprintf(dump_path, "%s/%drgb.rgb32", dump_dir, framecount);
        ALOGE("[%s %d] dump:%s size:%d", __FUNCTION__, __LINE__, dump_path, buffer->size());
        dumpfd = open(dump_path, O_CREAT | O_RDWR | O_TRUNC, 0644);
        write(dumpfd, buffer->data(), buffer->size());
        close(dumpfd);
        buffer->release();
        buffer = NULL;
    }
    ALOGE("[%s %d]", __FUNCTION__, __LINE__);
    mScreenCatch->stop();
    pMeta->clear();
    delete mScreenCatch;
    return ret;
}
/*****************************************************************************/
