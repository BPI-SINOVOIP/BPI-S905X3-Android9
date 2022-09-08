#ifndef __SUBTITLE_BITMAP_DISPLAY_H__
#define __SUBTITLE_BITMAP_DISPLAY_H__

#include <memory>
#include "Display.h"


#include <core/SkBitmap.h>
#include <core/SkStream.h>
#include <core/SkCanvas.h>

#include <core/SkImageEncoder.h>

class BitmapDisplay : public Display {
public:
    BitmapDisplay();
    virtual ~BitmapDisplay() {}

    virtual DisplayInfo getDisplayInfo() {
        DisplayInfo info = { 1920, 1080 };
        return info;
    }
    std::shared_ptr<SurfaceHandle> createLayer(int width, int height, int format) { return nullptr;}
    int destroySurface(std::shared_ptr<SurfaceHandle> handle) {return 0;}

    void *lockDisplayBuffer(int *width, int *height, int *stride, int *bpp);
    bool unlockAndPostDisplayBuffer(void *width);

private:
    std::shared_ptr<SkBitmap> mCurrentBitmap;
};

#endif

