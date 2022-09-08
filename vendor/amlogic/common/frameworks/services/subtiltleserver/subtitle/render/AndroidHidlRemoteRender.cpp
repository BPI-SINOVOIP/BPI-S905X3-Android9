#define LOG_TAG "SubtitleRender"

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <utils/Log.h>

#include "AndroidHidlRemoteRender.h"
#include "AndroidCallbackMessageQueue.h"
#include "ParserFactory.h"

//using ::vendor::amlogic::hardware::subtitleserver::V1_0::implementation::SubtitleServerHal;
using amlogic::AndroidCallbackMessageQueue;

static const int FADDING_SUB = 0;
static const int SHOWING_SUB = 1;

bool AndroidHidlRemoteRender::postSubtitleData() {
    // TODO: share buffers. if need performance
    //std:: string out;
    int width=0, height=0, size=0,x =0, y=0, videoWidth = 0, videoHeight = 0;

    sp<AndroidCallbackMessageQueue> queue = AndroidCallbackMessageQueue::Instance();

    if (mShowingSubs.size() <= 0) {
        if (queue != nullptr) {
            queue->postDisplayData(nullptr, mParseType, 0, 0, 0, 0, 0, 0, 0, FADDING_SUB);
            return true;
        } else {
            ALOGE("Error! should not null here!");
            return false;
        }
    }

    // only show the newest, since only 1 line for subtitle.
    for (auto it = mShowingSubs.rbegin(); it != mShowingSubs.rend(); it++) {
        width = (*it)->spu_width;
        height = (*it)->spu_height;
        if (mParseType == TYPE_SUBTITLE_DTVKIT_TELETEXT || mParseType == TYPE_SUBTITLE_DVB_TELETEXT) {
            x = (*it)->disPlayBackground;
        } else {
            x = (*it)->spu_start_x;
        }
        y = (*it)->spu_start_y;
        videoWidth = (*it)->spu_origin_display_w;
        videoHeight = (*it)->spu_origin_display_h;
        size = (*it)->buffer_size;

        if (((*it)->spu_data) == nullptr) {
            ALOGE("Error! why not decoded spu_data, but push to show???");
            continue;
        }

        ALOGD(" in AndroidHidlRemoteRender:%s width=%d, height=%d data=%p size=%d",
            __func__, width, height, (*it)->spu_data, (*it)->buffer_size);
        DisplayType  displayType = ParserFactory::getDisplayType(mParseType);
        if ((SUBTITLE_IMAGE_DISPLAY == displayType) && ((0 == width) || (0 == height))) {
           continue;
        }

        if (SUBTITLE_TEXT_DISPLAY == displayType) {
            width = height = 0; // no use for text!
        }

        /* The same as vlc player. showing and fading policy */
        if (queue != nullptr) {
            queue->postDisplayData((const char *)((*it)->spu_data), mParseType, x, y, width, height, videoWidth, videoHeight, size, SHOWING_SUB);
            return true;
        } else {
            return false;
        }
    }

    return false;
}



// TODO: the subtitle may has some params, config how to render
//       Need impl later.
bool AndroidHidlRemoteRender::showSubtitleItem(std::shared_ptr<AML_SPUVAR> spu, int type) {
    ALOGD("showSubtitleItem");
    mShowingSubs.push_back(spu);
    mParseType = type;

    return postSubtitleData();
}

void AndroidHidlRemoteRender::resetSubtitleItem() {
    mShowingSubs.clear();
}

bool AndroidHidlRemoteRender::hideSubtitleItem(std::shared_ptr<AML_SPUVAR> spu) {
    ALOGD("hideSubtitleItem");
    //some stream is special.some subtitles have pts, but some subtitles don't have pts.
    //In this situation if use the remove() function,it may cause the subtitle contains
    //pts don't hide untile the subtitle without pts disappear.
    //mShowingSubs.remove(spu);
    mShowingSubs.clear();
    return postSubtitleData();
}

void AndroidHidlRemoteRender::removeSubtitleItem(std::shared_ptr<AML_SPUVAR> spu)  {
    ALOGD("removeSubtitleItem");
    mShowingSubs.remove(spu);
}

