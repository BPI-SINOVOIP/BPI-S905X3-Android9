#ifndef __SUBTITLE_ANDROID_DISPLAY_H__
#define __SUBTITLE_ANDROID_DISPLAY_H__

//#include <gui/ISurfaceComposer.h>
//#include <gui/Surface.h>
//#include <gui/SurfaceComposerClient.h>

#include "Display.h"
#include <binder/IServiceManager.h>

class AndroidSurfaceHandle : public SurfaceHandle {
public:
    //AndroidSurfaceHandle(sp<SurfaceControl> ctl) { mControl=ctl;}
    AndroidSurfaceHandle() {};

    virtual ~AndroidSurfaceHandle() {}

private:
        //sp<SurfaceControl> mControl;


};

class AndroidDisplay : public Display, public android::IBinder::DeathRecipient {
public:
    AndroidDisplay();
    virtual ~AndroidDisplay() {};

    virtual DisplayInfo getDisplayInfo();
    virtual std::shared_ptr<SurfaceHandle> createLayer(int width, int height, int format);
    virtual int destroySurface(std::shared_ptr<SurfaceHandle> handle);

    virtual void *lockDisplayBuffer(int *width, int *height, int *stride, int *bpp) {return nullptr;}
    virtual bool unlockAndPostDisplayBuffer(void *width) {return true; }

private:
    void binderDied(const android::wp<android::IBinder>&);

    /* These sp is android specific still use it */
    //sp<SurfaceComposerClient>  mSession;

};


#endif
