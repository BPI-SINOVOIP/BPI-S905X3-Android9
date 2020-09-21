/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "lowpan-hdlc-adapter"

#include "hdlc_lite.h"

#include <unistd.h>

#include <mutex>
#include <condition_variable>

#include <hidl/HidlTransportSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidl/Status.h>
#include <hardware/hardware.h>
#include <utils/Thread.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <log/log.h>
#include <android/hardware/lowpan/1.0/ILowpanDevice.h>
#include <android/hidl/manager/1.0/IServiceManager.h>

#define LOWPAN_HDLC_ADAPTER_MAX_FRAME_SIZE 2048

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::joinRpcThreadpool;
using ::android::sp;
using namespace ::android::hardware::lowpan::V1_0;
using namespace ::android;

struct LowpanDeathRecipient : hidl_death_recipient {
    LowpanDeathRecipient() = default;
    virtual void serviceDied(uint64_t /*cookie*/, const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
        ALOGE("LowpanDevice died");
        exit(EXIT_FAILURE);
    }
};

struct LowpanDeviceCallback : public ILowpanDeviceCallback {
    int mFd;
    std::mutex mMutex;
    std::condition_variable mConditionVariable;
    int mOpenError;
    static const uint32_t kMaxFrameSize = LOWPAN_HDLC_ADAPTER_MAX_FRAME_SIZE;
public:
    LowpanDeviceCallback(int fd): mFd(fd), mOpenError(-1) {}
    virtual ~LowpanDeviceCallback() = default;

    int waitForOpenStatus() {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mOpenError == -1) {
            mConditionVariable.wait(lock);
        }
        return mOpenError;
    }

    Return<void> onReceiveFrame(const hidl_vec<uint8_t>& data)  override {
        if (data.size() > kMaxFrameSize) {
            ALOGE("TOOBIG: Frame received from device is too big");
            return Return<void>();
        }

        int bufferIndex = 0;
        uint16_t fcs = kHdlcCrcResetValue;
        uint8_t buffer[kMaxFrameSize*2 + 5]; // every character escaped, escaped crc, and frame marker
        uint8_t c;

        for (size_t i = 0; i < data.size(); i++)
        {
            c = data[i];
            fcs = hdlc_crc16(fcs, c);
            bufferIndex += hdlc_write_byte(buffer + bufferIndex, c);
        }

        fcs = hdlc_crc16_finalize(fcs);

        bufferIndex += hdlc_write_byte(buffer + bufferIndex, uint8_t(fcs & 0xFF));
        bufferIndex += hdlc_write_byte(buffer + bufferIndex, uint8_t((fcs >> 8) & 0xFF));

        buffer[bufferIndex++] = HDLC_BYTE_FLAG;

        std::unique_lock<std::mutex> lock(mMutex);

        if (write(mFd, buffer, bufferIndex) != bufferIndex) {
            ALOGE("IOFAIL: write: %s (%d)", strerror(errno), errno);
            exit(EXIT_FAILURE);
        }

        return Return<void>();
    }

    Return<void> onEvent(LowpanEvent event, LowpanStatus status)  override {
        std::unique_lock<std::mutex> lock(mMutex);

        switch (event) {
        case LowpanEvent::OPENED:
            if (mOpenError == -1) {
                mOpenError = 0;
                mConditionVariable.notify_all();
            }
            ALOGI("Device opened");
            break;

        case LowpanEvent::CLOSED:
            ALOGI("Device closed");
            exit(EXIT_SUCCESS);
            break;

        case LowpanEvent::RESET:
            ALOGI("Device reset");
            break;

        case LowpanEvent::ERROR:
            if (mOpenError == -1) {
                mOpenError = int(status);
                mConditionVariable.notify_all();
            }
            switch (status) {
            case LowpanStatus::IOFAIL:
                ALOGE("IOFAIL: Input/Output error from device. Terminating.");
                exit(EXIT_FAILURE);
                break;
            case LowpanStatus::GARBAGE:
                ALOGW("GARBAGE: Bad frame from device.");
                break;
            case LowpanStatus::TOOBIG:
                ALOGW("TOOBIG: Device sending frames that are too large.");
                break;
            default:
                ALOGW("Unknown error %d", status);
                break;
            }
            break;
        }
        return Return<void>();
    }
};

class ReadThread : public Thread {
    int kReadThreadBufferSize;

    sp<ILowpanDevice> mService;
    int mFd;
    uint8_t mBuffer[LOWPAN_HDLC_ADAPTER_MAX_FRAME_SIZE];
    int mBufferIndex;
    bool mUnescapeNextByte;
    uint16_t mFcs;
    sp<LowpanDeviceCallback> mCallback;

public:
    ReadThread(sp<ILowpanDevice> service, int fd, sp<LowpanDeviceCallback> callback):
            Thread(false /*canCallJava*/),
            kReadThreadBufferSize(service->getMaxFrameSize()),
            mService(service),
            mFd(fd),
            mBufferIndex(0),
            mUnescapeNextByte(false),
            mFcs(kHdlcCrcResetValue),
            mCallback(callback) {
        if (kReadThreadBufferSize < 16) {
            ALOGE("Device returned bad max frame size: %d bytes", kReadThreadBufferSize);
            exit(EXIT_FAILURE);
        }
        if ((size_t)kReadThreadBufferSize > sizeof(mBuffer)) {
            kReadThreadBufferSize = (int)sizeof(mBuffer);
        }
    }

    virtual ~ReadThread() {}

private:

    bool threadLoop() override {
        uint8_t buffer[LOWPAN_HDLC_ADAPTER_MAX_FRAME_SIZE];

        if (int error = mCallback->waitForOpenStatus()) {
            ALOGE("Call to `open()` failed: %d", error);
            exit(EXIT_FAILURE);
        }

        while (!exitPending()) {
            ssize_t bytesRead = read(mFd, buffer, sizeof(buffer));
            if (exitPending()) {
                break;
            }

            if (bytesRead < 0) {
                ALOGE("IOFAIL: read: %s (%d)", strerror(errno), errno);
                exit(EXIT_FAILURE);
                break;
            }
            feedBytes(buffer, bytesRead);
        }

        return false;
    }

    void feedBytes(const uint8_t* dataPtr, ssize_t dataLen) {
        while(dataLen--) {
            feedByte(*dataPtr++);
        }
    }

    void sendFrame(uint8_t* p_data, uint16_t data_len) {
        hidl_vec<uint8_t> data;
        data.setToExternal(p_data, data_len);
        mService->sendFrame(data);
    }

    void feedByte(uint8_t byte) {
        if (mBufferIndex >= kReadThreadBufferSize) {
            ALOGE("TOOBIG: HDLC frame too big (Max: %d)", kReadThreadBufferSize);
            mUnescapeNextByte = false;
            mBufferIndex = 0;
            mFcs = kHdlcCrcResetValue;

        } else if (byte == HDLC_BYTE_FLAG) {
            if (mBufferIndex <= 2) {
                // Ignore really small frames.
                // Don't remove this or we will underflow our
                // index for onReceiveFrame(), below!

            } else if (mUnescapeNextByte || (mFcs != kHdlcCrcCheckValue)) {
                ALOGE("GARBAGE: HDLC frame with bad CRC (LEN:%d, mFcs:0x%04X)", mBufferIndex, mFcs);

            } else {
                // -2 for CRC
                sendFrame(mBuffer, uint16_t(mBufferIndex - 2));
            }

            mUnescapeNextByte = false;
            mBufferIndex = 0;
            mFcs = kHdlcCrcResetValue;

        } else if (byte == HDLC_BYTE_ESC) {
            mUnescapeNextByte = true;

        } else if (hdlc_byte_needs_escape(byte)) {
            // Skip all other control codes.

        } else {
            if (mUnescapeNextByte) {
                byte = byte ^ HDLC_ESCAPE_XFORM;
                mUnescapeNextByte = false;
            }

            mFcs = hdlc_crc16(mFcs, byte);
            mBuffer[mBufferIndex++] = byte;
        }
    }
};

int main(int argc, char* argv []) {
    using ::android::hardware::defaultServiceManager;
    using Transport = ::android::hidl::manager::V1_0::IServiceManager::Transport;

    const char* serviceName = "default";

    if (argc >= 2) {
        serviceName = argv[1];
    }

    sp<ILowpanDevice> service = ILowpanDevice::getService(serviceName, false /* getStub */);

    if (service == nullptr) {
        ALOGE("Unable to find LowpanDevice named \"%s\"", serviceName);
        exit(EXIT_FAILURE);
    }

    service->linkToDeath(new LowpanDeathRecipient(), 0 /*cookie*/);

    configureRpcThreadpool(1, true /* callerWillJoin */);

    sp<LowpanDeviceCallback> callback = new LowpanDeviceCallback(STDOUT_FILENO);

    {
        auto status = service->open(callback);
        if (status.isOk()) {
            if (status == LowpanStatus::OK) {
                ALOGD("%s: open() ok.", serviceName);
            } else {
                ALOGE("%s: open() failed: (%d).", serviceName, LowpanStatus(status));
                exit(EXIT_FAILURE);
            }
        } else {
            ALOGE("%s: open() failed: transport error", serviceName);
            exit(EXIT_FAILURE);
        }
    }

    sp<Thread> readThread = new ReadThread(service, STDIN_FILENO, callback);

    readThread->run("ReadThread");

    joinRpcThreadpool();

    ALOGI("Shutting down");

    return EXIT_SUCCESS;
}
