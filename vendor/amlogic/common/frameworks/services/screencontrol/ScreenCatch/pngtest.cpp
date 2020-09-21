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
#define LOG_TAG "png_test"

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

#include <ui/PixelFormat.h>
#include <ui/DisplayInfo.h>

#include <system/graphics.h>

//#include <SkBitmap.h>
//#include <SkDocument.h>
//#include <SkStream.h>

// TODO: Fix Skia.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <SkImageEncoder.h>
#include <SkData.h>
#include <SkColorSpace.h>
#pragma GCC diagnostic pop

#include "ScreenCatch.h"
#include "../ScreenManager.h"

using namespace android;

/*****************************************************************************/

static SkColorType flinger2skia(PixelFormat f) {
    switch (f) {
        case PIXEL_FORMAT_RGB_565:
            return kRGB_565_SkColorType;
        default:
            return kN32_SkColorType;
    }
}

int main(int argc, char **argv) {

    using namespace android;

    status_t ret = NO_ERROR;
    int status;
    int dumpfd;
    uint32_t type;
    const char* sourcetype;
    int framecount = 0;
    int width = 1280;
    int height = 720;
    uint32_t f = PIXEL_FORMAT_RGBA_8888;
    const size_t size = width * height * 4;

    ScreenCatch* mScreenCatch;

    if (argc < 2) {
        printf("usage:%s [type]\n", argv[0]);
	printf(" type:0  video only\n");
	printf(" type:1  osd + video\n");
	return 0;
    } else {
        sourcetype = argv[1];
	type = atoi(sourcetype);
    }

    ALOGE("[%s %d] type is %d", __FUNCTION__, __LINE__, type);

    sp<MemoryHeapBase> memoryBase(new MemoryHeapBase(size, 0, "screen-capture"));
    void* const base = memoryBase->getBase();

    if (base != MAP_FAILED) {
        fprintf(stderr, "start screencap\n");
        mScreenCatch = new ScreenCatch(width, height, 32, type);
        mScreenCatch->setVideoCrop(0, 0, 1280, 720);

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
                usleep(100);
                continue;
            }

            framecount++;
            sprintf(dump_path, "%s/%d.jpeg", dump_dir, framecount);
            ALOGE("[%s %d] dump:%s size:%d", __FUNCTION__, __LINE__, dump_path, buffer->size());

            dumpfd = open(dump_path, O_CREAT | O_RDWR | O_TRUNC, 0644);
            memcpy(base, buffer->data(), buffer->size());

            const SkImageInfo info = SkImageInfo::Make(width, height, flinger2skia(f), kPremul_SkAlphaType, nullptr);
            SkPixmap pixmap(info, base, width * bytesPerPixel(f));
            struct FDWStream final : public SkWStream {
                size_t fBytesWritten = 0;
                int fFd;
                FDWStream(int f) : fFd(f) {}
                size_t bytesWritten() const override {
                    return fBytesWritten;
                }
                bool write(const void* buffer, size_t size) override {
                    fBytesWritten += size;
                    return size == 0 || ::write(fFd, buffer, size) > 0;
                }
            } fdStream(dumpfd);

            (void)SkEncodeImage(&fdStream, pixmap, SkEncodedImageFormat::kJPEG, 100);
            fprintf(stderr, "Png transform finish!\n");

            close(dumpfd);
            buffer->release();
            buffer = NULL;
        }

	memoryBase.clear();
        mScreenCatch->stop();
        pMeta->clear();
        delete mScreenCatch;
    } else {
	ret = UNKNOWN_ERROR;
    }
    ALOGE("[%s %d] screencap finish", __FUNCTION__, __LINE__);
    return ret;
}
/*****************************************************************************/
