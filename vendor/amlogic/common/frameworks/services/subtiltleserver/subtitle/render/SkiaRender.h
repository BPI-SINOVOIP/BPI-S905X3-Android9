#ifndef __SUBTITLE_SKIA_RENDER_H__
#define __SUBTITLE_SKIA_RENDER_H__

#include <memory>
#include <list>
#include <mutex>
#include "Parser.h"

#include "Render.h"
#include "Display.h"

class SkiaRender : public Render {
public:
    // TODO: we ask display from render, or upper layer?
    SkiaRender(std::shared_ptr<Display> display);
    //SkiaRender() = delete;
    virtual ~SkiaRender() {};

    // TODO: the subtitle may has some params, config how to render
    //       Need impl later.
    virtual bool showSubtitleItem(std::shared_ptr<AML_SPUVAR> spu,int type);
    virtual bool hideSubtitleItem(std::shared_ptr<AML_SPUVAR> spu);
    virtual void resetSubtitleItem();
private:
    bool drawAndPost();

    std::shared_ptr<Display> mDisplay;

    std::list<std::shared_ptr<AML_SPUVAR>> mShowingSubs;

    // currently, all the render is in present thread. no need lock
    // however, later may changed, add one here.
    std::mutex mLock;
};

#endif

