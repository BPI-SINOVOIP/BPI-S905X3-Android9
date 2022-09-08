#define LOG_TAG "SubtitleRender"

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <utils/Log.h>

#include "SkiaRender.h"


#include <core/SkBitmap.h>
#include <core/SkStream.h>
#include <core/SkCanvas.h>

// color, bpp, text size, font ...
SkPaint collectPaintInfo() {
    SkPaint paint;
    //paint.setXfermodeMode(SkXfermode::kSrc_Mode);

    paint.setARGB(255, 198, 0, 255);
    paint.setTextSize(32);
    paint.setAntiAlias(true);
    paint.setSubpixelText(true);
    paint.setLCDRenderText(true);
    return paint;
}

// TODO: auto wrap to next line, if text too long
bool drawTextAt(int x, int y, const char *text, SkCanvas &canvas, SkPaint &paint) {

    SkRect textRect;
    int textlen = paint.measureText(text, strlen(text), &textRect);

    canvas.drawText(text, strlen(text), x, y, paint);

    return true;
}

SkiaRender::SkiaRender(std::shared_ptr<Display> display) : mDisplay(display) {

}

bool SkiaRender::drawAndPost() {
    int width =1920, height = 1080, stride=0;
    int bytesPerPixel;

    void *buf = mDisplay->lockDisplayBuffer(&width, &height, &stride, &bytesPerPixel);
    //ssize_t bpr = stride * bytesPerPixel;

    SkBitmap surfaceBitmap;
    surfaceBitmap.setInfo(SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
    surfaceBitmap.setPixels(buf);

    SkPaint paint = collectPaintInfo();
    SkCanvas surfaceCanvas(surfaceBitmap);

    int delta = 50;
    for (auto it = mShowingSubs.begin(); it != mShowingSubs.end(); it++) {
        drawTextAt(50, height-delta -50, (const char *)((*it)->spu_data), surfaceCanvas, paint);
        delta += 50;
    }

    mDisplay->unlockAndPostDisplayBuffer(nullptr);
    return true;

}

// TODO: the subtitle may has some params, config how to render
//       Need impl later.
bool SkiaRender::showSubtitleItem(std::shared_ptr<AML_SPUVAR> spu,int type) {
    ALOGD("showSubtitleItem");
    mShowingSubs.push_back(spu);
    return drawAndPost();
}

void AndroidHidlRemoteRender::resetSubtitleItem() {
    mShowingSubs.clear();
}

bool SkiaRender::hideSubtitleItem(std::shared_ptr<AML_SPUVAR> spu) {
    ALOGD("hideSubtitleItem");
    mShowingSubs.remove(spu);
    return drawAndPost();
}

