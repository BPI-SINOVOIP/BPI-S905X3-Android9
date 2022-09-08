#define LOG_TAG "SubtitleDisplay"

#include "BitmapDisplay.h"

#include <utils/Log.h>

#include <core/SkBitmap.h>
#include <core/SkStream.h>
#include <core/SkCanvas.h>

#include <core/SkImageEncoder.h>

#include <utils/Timers.h>
#include <time.h>

BitmapDisplay::BitmapDisplay() {

}

void *BitmapDisplay::lockDisplayBuffer(int *width, int *height, int *stride, int *bpp) {
    std::shared_ptr<SkBitmap> dst = std::shared_ptr<SkBitmap>(new SkBitmap());
    dst->allocPixels(SkImageInfo::Make(1920, 1080, kN32_SkColorType, kPremul_SkAlphaType));
    mCurrentBitmap = dst;
    *width = 1920;
    *height = 1080;
    *stride = dst->rowBytes()/dst->bytesPerPixel();
    *bpp = dst->bytesPerPixel();
    ALOGD("lockDisplayBuffer called here");
    return dst->getPixels();
}

bool BitmapDisplay::unlockAndPostDisplayBuffer(void *buffer) {
    char name[1024] = { 0 };
    std::shared_ptr<SkBitmap> dst = mCurrentBitmap;

    sprintf(name, "/data/media/0/Pictures/%lld.png", ns2ms(systemTime(SYSTEM_TIME_MONOTONIC)));

    SkFILEWStream stream(name);
    if (!stream.isValid()) {
        ALOGE("Can't write %s.", name);
        return false;
    }
    if (!SkEncodeImage(&stream, *dst, SkEncodedImageFormat::kPNG, 100)) {
        ALOGE("Can't encode a PNG.");
        return false;
    }

    return true;
}

