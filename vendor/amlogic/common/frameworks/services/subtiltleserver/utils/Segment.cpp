
// TODO: implement
#define LOG_TAG "SubtitleService"

#include <list>
#include <memory>
#include <mutex>
#include <utils/Log.h>
#include "Segment.h"


std::shared_ptr<BufferItem> BufferSegment::pop() {
    std::shared_ptr<BufferItem> item;

    std::unique_lock<std::mutex> autolock(mMutex);
    while (mSegments.size() == 0 && mEnabled) {
        mCv.wait(autolock);
    }

    if (mSegments.size() == 0) return nullptr;

    item = mSegments.front();
    mSegments.pop_front();
    mAvailableDataSize -= item->getSize();

    return item;
}

bool BufferSegment::push(std::shared_ptr<char> buf, size_t size) {
    if (size <= 0) return false;

    std::shared_ptr<BufferItem> item = std::shared_ptr<BufferItem>(new BufferItem(buf, size));

    std::unique_lock<std::mutex> autolock(mMutex);
    ALOGD("BufferSegment:current size: %d", mSegments.size());
    mSegments.push_back(item);
    mAvailableDataSize += size;
    mCv.notify_all();

    return true;
}


