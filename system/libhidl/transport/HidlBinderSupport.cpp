/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "HidlSupport"

#include <hidl/HidlBinderSupport.h>

// C includes
#include <unistd.h>

// C++ includes
#include <fstream>
#include <sstream>

namespace android {
namespace hardware {

hidl_binder_death_recipient::hidl_binder_death_recipient(const sp<hidl_death_recipient> &recipient,
        uint64_t cookie, const sp<::android::hidl::base::V1_0::IBase> &base) :
    mRecipient(recipient), mCookie(cookie), mBase(base) {
}

void hidl_binder_death_recipient::binderDied(const wp<IBinder>& /*who*/) {
    sp<hidl_death_recipient> recipient = mRecipient.promote();
    if (recipient != nullptr && mBase != nullptr) {
        recipient->serviceDied(mCookie, mBase);
    }
    mBase = nullptr;
}

wp<hidl_death_recipient> hidl_binder_death_recipient::getRecipient() {
    return mRecipient;
}

const size_t hidl_memory::kOffsetOfHandle = offsetof(hidl_memory, mHandle);
const size_t hidl_memory::kOffsetOfName = offsetof(hidl_memory, mName);
static_assert(hidl_memory::kOffsetOfHandle == 0, "wrong offset");
static_assert(hidl_memory::kOffsetOfName == 24, "wrong offset");

status_t readEmbeddedFromParcel(const hidl_memory& memory,
        const Parcel &parcel, size_t parentHandle, size_t parentOffset) {
    const native_handle_t *handle;
    ::android::status_t _hidl_err = parcel.readNullableEmbeddedNativeHandle(
            parentHandle,
            parentOffset + hidl_memory::kOffsetOfHandle,
            &handle);

    if (_hidl_err == ::android::OK) {
        _hidl_err = readEmbeddedFromParcel(
                memory.name(),
                parcel,
                parentHandle,
                parentOffset + hidl_memory::kOffsetOfName);
    }

    return _hidl_err;
}

status_t writeEmbeddedToParcel(const hidl_memory &memory,
        Parcel *parcel, size_t parentHandle, size_t parentOffset) {
    status_t _hidl_err = parcel->writeEmbeddedNativeHandle(
            memory.handle(),
            parentHandle,
            parentOffset + hidl_memory::kOffsetOfHandle);

    if (_hidl_err == ::android::OK) {
        _hidl_err = writeEmbeddedToParcel(
            memory.name(),
            parcel,
            parentHandle,
            parentOffset + hidl_memory::kOffsetOfName);
    }

    return _hidl_err;
}
const size_t hidl_string::kOffsetOfBuffer = offsetof(hidl_string, mBuffer);
static_assert(hidl_string::kOffsetOfBuffer == 0, "wrong offset");

status_t readEmbeddedFromParcel(const hidl_string &string ,
        const Parcel &parcel, size_t parentHandle, size_t parentOffset) {
    const void *out;

    status_t status = parcel.readEmbeddedBuffer(
            string.size() + 1,
            nullptr /* buffer_handle */,
            parentHandle,
            parentOffset + hidl_string::kOffsetOfBuffer,
            &out);

    if (status != OK) {
        return status;
    }

    // Always safe to access out[string.size()] because we read size+1 bytes
    if (static_cast<const char *>(out)[string.size()] != '\0') {
        ALOGE("Received unterminated hidl_string buffer.");
        return BAD_VALUE;
    }

    return OK;
}

status_t writeEmbeddedToParcel(const hidl_string &string,
        Parcel *parcel, size_t parentHandle, size_t parentOffset) {
    return parcel->writeEmbeddedBuffer(
            string.c_str(),
            string.size() + 1,
            nullptr /* handle */,
            parentHandle,
            parentOffset + hidl_string::kOffsetOfBuffer);
}

android::status_t writeToParcel(const hidl_version &version, android::hardware::Parcel& parcel) {
    return parcel.writeUint32(static_cast<uint32_t>(version.get_major()) << 16 | version.get_minor());
}

hidl_version* readFromParcel(const android::hardware::Parcel& parcel) {
    uint32_t version;
    android::status_t status = parcel.readUint32(&version);
    if (status != OK) {
        return nullptr;
    } else {
        return new hidl_version(version >> 16, version & 0xFFFF);
    }
}

status_t readFromParcel(Status *s, const Parcel& parcel) {
    int32_t exception;
    status_t status = parcel.readInt32(&exception);
    if (status != OK) {
        s->setFromStatusT(status);
        return status;
    }

    // Skip over fat response headers.  Not used (or propagated) in native code.
    if (exception == Status::EX_HAS_REPLY_HEADER) {
        // Note that the header size includes the 4 byte size field.
        const int32_t header_start = parcel.dataPosition();
        int32_t header_size;
        status = parcel.readInt32(&header_size);
        if (status != OK) {
            s->setFromStatusT(status);
            return status;
        }
        parcel.setDataPosition(header_start + header_size);
        // And fat response headers are currently only used when there are no
        // exceptions, so act like there was no error.
        exception = Status::EX_NONE;
    }

    if (exception == Status::EX_NONE) {
        *s = Status::ok();
        return status;
    }

    // The remote threw an exception.  Get the message back.
    String16 message;
    status = parcel.readString16(&message);
    if (status != OK) {
        s->setFromStatusT(status);
        return status;
    }

    s->setException(exception, String8(message));

    return status;
}

status_t writeToParcel(const Status &s, Parcel* parcel) {
    // Something really bad has happened, and we're not going to even
    // try returning rich error data.
    if (s.exceptionCode() == Status::EX_TRANSACTION_FAILED) {
        return s.transactionError();
    }

    status_t status = parcel->writeInt32(s.exceptionCode());
    if (status != OK) { return status; }
    if (s.exceptionCode() == Status::EX_NONE) {
        // We have no more information to write.
        return status;
    }
    status = parcel->writeString16(String16(s.exceptionMessage()));
    return status;
}

void configureBinderRpcThreadpool(size_t maxThreads, bool callerWillJoin) {
    ProcessState::self()->setThreadPoolConfiguration(maxThreads, callerWillJoin /*callerJoinsPool*/);
}

void joinBinderRpcThreadpool() {
    IPCThreadState::self()->joinThreadPool();
}

int setupBinderPolling() {
    int fd;
    int err = IPCThreadState::self()->setupPolling(&fd);

    if (err != OK) {
        ALOGE("Failed to setup binder polling: %d (%s)", err, strerror(err));
    }

    return err == OK ? fd : -1;
}

status_t handleBinderPoll() {
    return IPCThreadState::self()->handlePolledCommands();
}

}  // namespace hardware
}  // namespace android
