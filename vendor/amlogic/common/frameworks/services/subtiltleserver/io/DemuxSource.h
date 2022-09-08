#ifndef __SUBTITLE_DEMUXDEVICE_SOURCE_H__
#define __SUBTITLE_DEMUXDEVICE_SOURCE_H__


#include <string>
#include <memory>
#include <mutex>
#include <utils/Thread.h>
#include <thread>
#include "Segment.h"
#include "sub_types2.h"


#include "DataSource.h"
#define DEMUX_SOURCE_ID 0//0

class DemuxSource : public DataSource {

public:
    DemuxSource();
    virtual ~DemuxSource();
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
    virtual void updateParameter(int type, void *data) ;
    static inline DemuxSource *getCurrentInstance();
    std::shared_ptr<BufferSegment> mSegment;
    int subType;
    virtual size_t lseek(int offSet, int whence) {return 0;}
private:
    void loopRenderTime();
    void pes_data_cb(int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);

    bool notifyInfoChange();
    int mRdFd;
    std::shared_ptr<std::thread> mRenderTimeThread;
    std::shared_ptr<std::thread> mReadThread;
//    std::shared_ptr<BufferSegment> mSegment;
    // for poping and parsing
    std::shared_ptr<BufferItem> mCurrentItem;

    SubtitleIOState mState;
    bool mExitRequested;
    TVSubtitleData *mDemuxContext;

    int mPid = -1;
    int mDemuxId = -1;
    int mParam1;
    int mParam2; //ancillary_id
    static DemuxSource *sInstance;

};

#endif

