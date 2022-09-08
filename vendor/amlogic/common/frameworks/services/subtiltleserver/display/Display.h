#ifndef __SUBTITLE_DISPLAY_H__
#define __SUBTITLE_DISPLAY_H__
#include <memory>

struct DisplayInfo {
    int width;
    int height;
};

class SurfaceHandle {
public:
    SurfaceHandle() {mFd = 0;}
private:
    int mFd;
};

class Display {
public:
    Display() {}
    virtual ~Display() {};

    virtual DisplayInfo getDisplayInfo() = 0;
    virtual std::shared_ptr<SurfaceHandle> createLayer(int width, int height, int format) = 0;
    virtual int destroySurface(std::shared_ptr<SurfaceHandle> handle) = 0;

    virtual void *lockDisplayBuffer(int *width, int *height, int *stride, int *bpp);
    virtual bool unlockAndPostDisplayBuffer(void *width);

};
#endif

