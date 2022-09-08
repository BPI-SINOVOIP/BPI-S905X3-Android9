#define LOG_TAG "DeviceSource"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>

#include <string>
#include <utils/Log.h>
#include <utils/CallStack.h>
#include "DeviceSource.h"

#include <pthread.h>


static const std::string SYSFS_VIDEO_PTS = "/sys/class/tsync/pts_video";
static const std::string SYSFS_SUBTITLE_TYPE = "/sys/class/subtitle/subtype";
static const std::string SYSFS_SUBTITLE_SIZE = "/sys/class/subtitle/size";
static const std::string SYSFS_SUBTITLE_TOTAL = "/sys/class/subtitle/total";
static const std::string SUBTITLE_READ_DEVICE = "/dev/amstream_sub_read";

#define AMSTREAM_IOC_MAGIC  'S'
#define AMSTREAM_IOC_SUB_LENGTH  _IOR(AMSTREAM_IOC_MAGIC, 0x1b, int)
#define AMSTREAM_IOC_SUB_RESET   _IOW(AMSTREAM_IOC_MAGIC, 0x1a, int)


static inline bool pollFd(int fd, int timeout) {
    struct pollfd pollFd[1];
    if (fd <= 0)    {
        return false;
    }
    pollFd[0].fd = fd;
    pollFd[0].events = POLLOUT;
    return (::poll(pollFd, 1, timeout) > 0);
}

static inline unsigned long sysfsReadInt(const char *path, int base) {
    int fd;
    unsigned long val = 0;
    char bcmd[32];
    memset(bcmd, 0, 32);
    fd = ::open(path, O_RDONLY);
    if (fd >= 0) {
        int c = read(fd, bcmd, sizeof(bcmd));
        if (c > 0) val = strtoul(bcmd, NULL, base);
        ::close(fd);
    } else {
        ALOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return val;
}

DeviceSource::DeviceSource() {
    mState = E_SOURCE_INV;
    mExitRequested = false;
    mNeedDumpSource = false;
    mDumpFd = -1;
    mRdFd = ::open(SUBTITLE_READ_DEVICE.c_str(), O_RDONLY);
    ALOGD("DeviceSource: %s=%d", SUBTITLE_READ_DEVICE.c_str(), mRdFd);
}

DeviceSource::~DeviceSource() {
    ALOGD("~DeviceSource: %s=%d", SUBTITLE_READ_DEVICE.c_str(), mRdFd);

    mExitRequested = true;
    if (mRenderTimeThread != nullptr) {
        mRenderTimeThread->join();
        mRenderTimeThread = nullptr;
    }

    ::close(mRdFd);
    if (mReadThread != nullptr) {
        mReadThread->join();
        mReadThread = nullptr;
    }

    if (mDumpFd > 0) ::close(mDumpFd);
}

size_t DeviceSource::totalSize() {
    return 0;
}


bool DeviceSource::notifyInfoChange() {
    std::unique_lock<std::mutex> autolock(mLock);
    for (auto it = mInfoListeners.begin(); it != mInfoListeners.end(); it++) {
        auto wk_lstner = (*it);
        int value = -1;


        if (auto lstn = wk_lstner.lock()) {
            value = sysfsReadInt(SYSFS_SUBTITLE_TYPE.c_str(), 10);
            if (value > 0) {  //0:no sub
                lstn->onTypeChanged(value);
            }

            value = sysfsReadInt(SYSFS_SUBTITLE_TOTAL.c_str(), 10);
            if (value >= 0) {
                lstn->onSubtitleChanged(value);
            }
            return true;
        }
    }

    return false;
}

void DeviceSource::loopRenderTime() {
    while (!mExitRequested) {
        mLock.lock();
        for (auto it = mInfoListeners.begin(); it != mInfoListeners.end(); it++) {
            auto wk_lstner = (*it);

            if (wk_lstner.expired()) {
                ALOGV("[threadLoop] lstn null.\n");
                continue;
            }

            unsigned long value = sysfsReadInt(SYSFS_VIDEO_PTS.c_str(), 16);

            static int i = 0;
            if (i++%300 == 0) {
                ALOGD("read pts: %ld %lu", value, value);
            }
            if (!mExitRequested) {
                if (auto lstn = wk_lstner.lock()) {
                    lstn->onRenderTimeChanged(value);
                }
            }
        }
        mLock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void DeviceSource::loopDriverData() {
    while (!mExitRequested && mRdFd > 0) {
        if (pollFd(mRdFd, 10)) {
            unsigned long size = 0;
            int r = ::ioctl(mRdFd, AMSTREAM_IOC_SUB_LENGTH, &size);
            if (r < 0) continue;
            char *rdBuffer = new char[size]();
            int readed = readDriverData(rdBuffer,  size);
            std::shared_ptr<char> spBuf = std::shared_ptr<char>(rdBuffer, [](char *buf) { delete [] buf; });
            mSegment->push(spBuf, readed);
        } else {
            if (mExitRequested || mRdFd < 0) return;
            usleep(1);   //in case of little page subtile loss. improve read freq
        }
    }
}

bool DeviceSource::start() {
    if (E_SOURCE_STARTED == mState) {
        ALOGE("already stated");
        return false;
    }
    mState = E_SOURCE_STARTED;
    int total = 0;
    int subtype = 0;

    total = sysfsReadInt(SYSFS_SUBTITLE_TOTAL.c_str(), 10);
    subtype = sysfsReadInt(SYSFS_SUBTITLE_TYPE.c_str(), 10); //scte27 subtype:0,not start thread

    mSegment = std::shared_ptr<BufferSegment>(new BufferSegment());

    if (total > 0 && subtype > 0) {
        mRenderTimeThread = std::shared_ptr<std::thread>(new std::thread(&DeviceSource::loopRenderTime, this));
    }

    mReadThread = std::shared_ptr<std::thread>(new std::thread(&DeviceSource::loopDriverData, this));

    // first check types
    notifyInfoChange();

    return false;
}



bool DeviceSource::stop() {
    mState = E_SOURCE_STOPED;
    ::ioctl(mRdFd, AMSTREAM_IOC_SUB_RESET);

    // If parser has problem, it read more data than provided
    // then stop cannot stopped. need notify exit here.
    mSegment->notifyExit();

    return false;
}

bool DeviceSource::isFileAvailble() {
    return false;
}

size_t DeviceSource::availableDataSize() {
    if (mSegment == nullptr) return 0;

    return mSegment->availableDataSize();
}

size_t DeviceSource::readDriverData(void *buffer, size_t size) {
    int data_size = size, r, read_done = 0;
    char *buf = (char *)buffer;

    while (data_size && mState == E_SOURCE_STARTED) {
        r = ::read(mRdFd, buf + read_done, data_size);
        if (r < 0) {
            return 0;
        } else {
            data_size -= r;
            read_done += r;
        }
    }

    if (mNeedDumpSource) {
        if (mDumpFd == -1) {
            mDumpFd = ::open("/data/local/traces/cur_sub.dump", O_RDWR | O_CREAT, 0666);
            ALOGD("need dump Source: mDumpFd=%d %d", mDumpFd, errno);
        }

        if (mDumpFd > 0) {
            write(mDumpFd, buffer, size);
        }
    }

    return read_done;
}

size_t DeviceSource::read(void *buffer, size_t size) {
    int readed = 0;

    //Current design of Parser Read, do not need add lock protection.
    // because all the read, is in Parser's parser thread.
    // We only need add lock here, is for protect access the mCurrentItem's
    // member function multi-thread.
    // Current Impl do not need lock, if required, then redesign the SegmentBuffer.

    //in case of applied size more than 1024*2, such as dvb subtitle,
    //and data process error for two reads.
    //so add until read applied data then exit.
    while (readed != size && mState == E_SOURCE_STARTED) {
        if (mCurrentItem != nullptr && !mCurrentItem->isEmpty()) {
            readed += mCurrentItem->read_l(((char *)buffer+readed), size-readed);
            //ALOGD("readed:%d,size:%d", readed, size);
            if (readed == size) return readed;
        } else {
            //ALOGD("mCurrentItem null, pop next buffer item");
            mCurrentItem = mSegment->pop();
        }
    }
    //readed += mCurrentItem->read(((char *)buffer+readed), size-readed);
    return readed;

}


void DeviceSource::dump(int fd, const char *prefix) {
    dprintf(fd, "%s nDeviceSource:\n", prefix);
    {
        std::unique_lock<std::mutex> autolock(mLock);
        for (auto it = mInfoListeners.begin(); it != mInfoListeners.end(); it++) {
            auto wk_lstner = (*it);
            if (auto lstn = wk_lstner.lock())
                dprintf(fd, "%s   InforListener: %p\n", prefix, lstn.get());
        }
    }
    dprintf(fd, "%s   state:%d\n\n", prefix, mState);

    dprintf(fd, "%s   Total Subtitle Tracks: (%s)%ld\n", prefix,
            SYSFS_SUBTITLE_TOTAL.c_str(), sysfsReadInt(SYSFS_SUBTITLE_TOTAL.c_str(), 10));
    dprintf(fd, "%s   Current Subtitle type: (%s)%ld\n", prefix,
            SYSFS_SUBTITLE_TYPE.c_str(), sysfsReadInt(SYSFS_SUBTITLE_TYPE.c_str(), 10));

    dprintf(fd, "\n%s   Current Unconsumed Data Size: %d\n", prefix, availableDataSize());
}

