#define LOG_TAG "FmqReceiver"
#include "FmqReceiver.h"
#include "ringbuffer.h"
#include <utils/CallStack.h>


using ::android::CallStack;

static inline void dumpBuffer(const char *buf, int size) {
    char str[64] = {0};

    for (int i=0; i<size; i++) {
        char chars[6] = {0};
        sprintf(chars, "%02x ", buf[i]);
        strcat(str, chars);
        if (i%8 == 7) {
            ALOGD("%s", str);
            str[0] = str[1] = 0;
        }
    }
    ALOGD("%s", str);
}


FmqReceiver::FmqReceiver(std::unique_ptr<FmqReader> reader) {
    mStop = false;
    mReader = std::move(reader);
    ALOGD("%s mReader=%p", __func__, mReader.get());
    mReaderThread = std::thread(&FmqReceiver::readLoop, this);

}

FmqReceiver::~FmqReceiver() {
    ALOGD("%s", __func__);
    mStop = true;
    mReaderThread.join();
}


bool FmqReceiver::readLoop() {
    IpcPackageHeader curHeader;
    rbuf_handle_t bufferHandle = ringbuffer_create(1024*1024, "fmqbuffer");

    ALOGD("%s", __func__);
    while (!mStop) {

        if (mClients.size() <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                // no consumer attached, avoid lost message, wait here
            continue;
        }

        if (mReader != nullptr) {
            size_t available = mReader->availableSize();
            if (available > 0) {
                char *recvBuf = (char *)malloc(available);
                size_t readed = mReader->read((uint8_t*)recvBuf, available);
                if (readed <= 0) {
                    usleep(1000);
                    continue;
                }

                //ALOGD("readed: available %d size: %d", available, readed);

                ringbuffer_write(bufferHandle, recvBuf, readed, RBUF_MODE_BLOCK);
                free(recvBuf);
            } else {
                usleep(1000);
            }

            // consume all data.
            while (!mStop && (ringbuffer_read_avail(bufferHandle) > sizeof(IpcPackageHeader))) {
                char buffer[sizeof(IpcPackageHeader)];
                uint32_t size = ringbuffer_peek(bufferHandle, buffer, sizeof(IpcPackageHeader), RBUF_MODE_BLOCK);
                if (size < sizeof(IpcPackageHeader)) {
                    ALOGE("Error! read size: %d request %d", size, sizeof(IpcPackageHeader));
                }
                if ((peekAsSocketWord(buffer) != START_FLAG) && (peekAsSocketWord(buffer+8) != MAGIC_FLAG)) {
                    ALOGD("!!!Wrong Sync header found! %x %x", peekAsSocketWord(buffer), peekAsSocketWord(buffer+8));
                    ringbuffer_read(bufferHandle, buffer, 4, RBUF_MODE_BLOCK);
                    continue; // ignore and try next.
                }
                curHeader.syncWord  = peekAsSocketWord(buffer);
                curHeader.sessionId = peekAsSocketWord(buffer+4);
                curHeader.magicWord = peekAsSocketWord(buffer+8); // need check magic or not??
                curHeader.dataSize  = peekAsSocketWord(buffer+12);
                curHeader.pkgType   = peekAsSocketWord(buffer+16);
                //ALOGD("data: syncWord:%x session:%x magic:%x subType:%x size:%x",
                //    curHeader.syncWord, curHeader.sessionId, curHeader.magicWord,
                //    curHeader.pkgType, curHeader.dataSize);
                if (ringbuffer_read_avail(bufferHandle) < (sizeof(IpcPackageHeader)+curHeader.dataSize)) {
                    ALOGW("not enough data, try next...");
                    break; // not enough data. try next
                }

                // eat the header
                ringbuffer_read(bufferHandle, buffer, sizeof(IpcPackageHeader), RBUF_MODE_BLOCK);

                char *payloads = (char *) malloc(curHeader.dataSize +4);
                memcpy(payloads, &curHeader.pkgType, 4); // fill package type
                ringbuffer_read(bufferHandle, payloads+4, curHeader.dataSize, RBUF_MODE_BLOCK);
                {  // notify listener
                    std::lock_guard<std::mutex> guard(mLock);
                    std::shared_ptr<DataListener> listener = mClients.front();
                    //ALOGD("payload listener=%p type=%x, %d", listener.get(), peekAsSocketWord(payloads), curHeader.dataSize);
                    if (listener != nullptr) {
                        if (listener->onData(payloads, curHeader.dataSize+4) < 0) {
                            free(payloads);
                            ringbuffer_free(bufferHandle);
                            return -1;
                        }
                    }
                }
                free(payloads);
            }
        }
    }
    ALOGD("exit %s", __func__);

    ringbuffer_free(bufferHandle);
    return true;
}

void FmqReceiver::dump(int fd, const char *prefix) {
    dprintf(fd, "%s FastMessageQueue Receiver:\n", prefix);
    {
        std::unique_lock<std::mutex> autolock(mLock);
        for (auto it = mClients.begin(); it != mClients.end(); it++) {
            auto lstn = (*it);
            if (lstn != nullptr)
                dprintf(fd, "%s   InforListener: %p\n", prefix, lstn.get());
        }
    }
    dprintf(fd, "%s   mStop: %d\n", prefix, mStop);
    dprintf(fd, "%s   mReader: %p\n", prefix, mReader.get());
}

