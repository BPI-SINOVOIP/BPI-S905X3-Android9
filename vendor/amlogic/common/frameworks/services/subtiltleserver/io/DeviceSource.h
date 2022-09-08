#ifndef __SUBTITLE_DEVICE_SOURCE_H__
#define __SUBTITLE_DEVICE_SOURCE_H__


#include <string>
#include <memory>
#include <mutex>
#include <utils/Thread.h>
#include <thread>
#include "Segment.h"


#include "DataSource.h"

class DeviceSource : public DataSource {

public:
    DeviceSource();
    virtual ~DeviceSource();
    size_t totalSize();

    bool start();
    bool stop();

    bool isFileAvailble();
    virtual size_t availableDataSize();
    virtual size_t read(void *buffer, size_t size);
    virtual void dump(int fd, const char *prefix);

    virtual int onData(const char*buffer, int len) {
        return -1;
    }
    virtual size_t lseek(int offSet, int whence) {return 0;}

private:
    void loopRenderTime();
    void loopDriverData();
    size_t readDriverData(void *buffer, size_t size);

    bool notifyInfoChange();
    int mRdFd;
    std::shared_ptr<std::thread> mRenderTimeThread;

    std::shared_ptr<std::thread> mReadThread;
    std::shared_ptr<BufferSegment> mSegment;
    // for poping and parsing
    std::shared_ptr<BufferItem> mCurrentItem;

    SubtitleIOState mState;
    bool mExitRequested;

};

#endif

