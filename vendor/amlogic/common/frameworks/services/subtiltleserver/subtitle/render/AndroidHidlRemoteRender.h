#ifndef __SUBTITLE_ANDROID_HIDL_REMOTE_RENDER_H__
#define __SUBTITLE_ANDROID_HIDL_REMOTE_RENDER_H__

#include <memory>
#include <list>
#include <mutex>
#include "Parser.h"

#include "Render.h"
#include "Display.h"

class AndroidHidlRemoteRender : public Render {
public:
    // AndroidHidlRemoteRender: we ask display from render, or upper layer?
    AndroidHidlRemoteRender(std::shared_ptr<Display> display) {
        mDisplay = display;
        mParseType = -1;
    }
    AndroidHidlRemoteRender() {}
    //SkiaRender() = delete;
    virtual ~AndroidHidlRemoteRender() {};

    // TODO: the subtitle may has some params, config how to render
    //       Need impl later.
    virtual bool showSubtitleItem(std::shared_ptr<AML_SPUVAR> spu,int type);
    virtual bool hideSubtitleItem(std::shared_ptr<AML_SPUVAR> spu);
    virtual void resetSubtitleItem();
    virtual void removeSubtitleItem(std::shared_ptr<AML_SPUVAR> spu);

private:
    bool postSubtitleData();

    std::shared_ptr<Display> mDisplay;

    std::list<std::shared_ptr<AML_SPUVAR>> mShowingSubs;

    // currently, all the render is in present thread. no need lock
    // however, later may changed, add one here.
    std::mutex mLock;
    int mParseType;
};

#endif

