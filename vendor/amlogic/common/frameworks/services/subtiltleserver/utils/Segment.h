
// TODO: implement

#ifndef __SUBTITLE_SEGMENT_H__
#define __SUBTITLE_SEGMENT_H__
#include <list>
#include <memory>
#include <mutex>

class BufferSegment;

class BufferItem {
public:
    BufferItem() {mSize = 0;}
    BufferItem(std::shared_ptr<char> buf, size_t size) {
        mBuffer = buf;
        mSize = size;
        mPtr = 0;
        mRemainSize = mSize;
    }

    int read_l(char *buf, int size) {
        int needRead = 0;
        if (size == 0 || mRemainSize == 0) {
            return 0;
        }

        char *ptr = mBuffer.get();
        if (size > mRemainSize){
            needRead = mRemainSize;
        } else {
            needRead = size;
        }

        memcpy(buf, ptr+mPtr, needRead);
        mPtr += needRead;
        mRemainSize -= needRead;
        return needRead;
    }

    bool isEmpty() {
        return mRemainSize == 0;
    }
    size_t getRemainSize() {return mRemainSize;}
    size_t getSize() {return mSize;}

private:
    friend class BufferSegment;


    size_t mSize;
    // TODO: unique_ptr?
    std::shared_ptr<char> mBuffer;
    int mPtr;
    int mRemainSize;
};


class BufferSegment {
public:
    BufferSegment() {
        mEnabled = true;
        mAvailableDataSize = 0;
    }
    ~BufferSegment() {
        notifyExit();
    }

    std::shared_ptr<BufferItem> pop();
    bool push(std::shared_ptr<char> buf, size_t size);

    void notifyExit() {
        mEnabled = false;
        std::unique_lock<std::mutex> autolock(mMutex);
        mAvailableDataSize = 0;
        mCv.notify_all();
    }

    size_t availableDataSize() {return mAvailableDataSize;}

private:
    std::list< std::shared_ptr<BufferItem>> mSegments;
    int mAvailableDataSize;
    std::mutex mMutex;
    std::condition_variable mCv;
    bool mEnabled;
};

#endif

