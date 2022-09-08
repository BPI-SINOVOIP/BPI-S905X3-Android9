#define LOG_TAG "SubtitleDisplay"

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <utils/Log.h>

#include <chrono>

#include "AndroidDisplay.h"


AndroidDisplay::AndroidDisplay() {
    //mSession = new SurfaceComposerClient();
    //mSession->linkToComposerDeath(this);
    android::sp<android::IServiceManager> sm = android::defaultServiceManager();
    android::sp<android::IBinder> binder = sm->getService(android::String16("SurfaceFlinger"));
    //android::sp<android::IBinder> binder = sm->getService(android::String16("android.ui.ISurfaceComposer"));

    ALOGD("Created ISurfaceComposer: %p", binder.get());
    if (binder == nullptr) {
        ALOGD("Error! cannot connect ISurfaceComposer!");
      return;
    }

    binder->linkToDeath(this);
}


void AndroidDisplay::binderDied(const android::wp<android::IBinder>&)
{
    // woah, surfaceflinger died!
    ALOGD("SurfaceFlinger died, we also dying...");
    // calling requestExit() is not enough here because the Surface code
    // might be blocked on a condition variable that will never be updated.
    kill( getpid(), SIGKILL );
}

std::shared_ptr<SurfaceHandle> AndroidDisplay::createLayer(int width, int height, int format) {

    /*sp<SurfaceControl> control = session()->createSurface(String8("BootAnimation"),
        dinfo.w, dinfo.h, PIXEL_FORMAT_RGB_565,ISurfaceComposerClient::eOpaque);
    sp<Surface> s = control->getSurface();

    return std::shared_ptr<SurfaceHandle>(new AndroidSurfaceHandle(sp<SurfaceControl> control));*/
    return nullptr;
}

// TODO:
DisplayInfo AndroidDisplay::getDisplayInfo() {
    DisplayInfo info;
    return info;
}

int AndroidDisplay::destroySurface(std::shared_ptr<SurfaceHandle> handle) {
    return 0;
}



