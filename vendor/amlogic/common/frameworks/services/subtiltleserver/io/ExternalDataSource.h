#ifndef __SUBTITLE_EXTERNAL_SOURCE_H__
#define __SUBTITLE_EXTERNAL_SOURCE_H__


#include <string>
#include <memory>
#include <mutex>
#include <utils/Thread.h>
#include <thread>
#include "Segment.h"


#include "DataSource.h"



class ExternalDataSource : public DataSource {

public:
    ExternalDataSource();
    virtual ~ExternalDataSource();

    virtual bool start();
    virtual bool stop();

    virtual size_t availableDataSize() {
        int size = 0;
        if (mSegment != nullptr) size += mSegment->availableDataSize();
        if (mCurrentItem != nullptr) size += mCurrentItem->getRemainSize();
        return size;
    }

    virtual size_t read(void *buffer, size_t size);
    virtual int onData(const char*buffer, int len);
    virtual void dump(int fd, const char *prefix);
    virtual size_t lseek(int offSet, int whence) {return 0;}

private:
    bool notifyInfoChange_l(int type);

    // subtitle infos, here
    int mTotalSubtitle;
    int mSubtitleType;
    std::string mLanguage;

    uint64_t mRenderTimeUs;
    uint64_t mStartPts;
    std::string mSubInfo;

    std::shared_ptr<BufferSegment> mSegment;
    // for poping and parsing
    std::shared_ptr<BufferItem> mCurrentItem;
    SubtitleIOState mState;
};


#endif
