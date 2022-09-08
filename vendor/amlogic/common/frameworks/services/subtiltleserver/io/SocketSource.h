#ifndef __SUBTITLE_SOCKET_SOURCE_H__
#define __SUBTITLE_SOCKET_SOURCE_H__

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <mutex>
#include "DataSource.h"
#include "InfoChangeListener.h"
#include "Segment.h"

// re-design later
// still don't know how to impl this
// currently, very simple wrap of previous impl



class SocketSource : public DataSource {

public:
    SocketSource();
    SocketSource(const std::string url);
    virtual ~SocketSource();

    virtual bool start();
    virtual bool stop();

    virtual size_t availableDataSize() {
        int size = 0;
        if (mSegment != nullptr) size += mSegment->availableDataSize();
        if (mCurrentItem != nullptr) size += mCurrentItem->getRemainSize();
        return size;
    }
    virtual size_t lseek(int offSet, int whence) {return 0;}
    virtual size_t read(void *buffer, size_t size);
    virtual int onData(const char*buffer, int len);
    virtual void dump(int fd, const char *prefix);

private:
    bool notifyInfoChange_l(int type);

    // subtitle infos, here
    int mTotalSubtitle;
    int mSubtitleType;

    uint64_t mRenderTimeUs;
    uint64_t mStartPts;
    std::string mSubInfo;

    std::shared_ptr<BufferSegment> mSegment;
    // for poping and parsing
    std::shared_ptr<BufferItem> mCurrentItem;
    SubtitleIOState mState;
};


#endif

