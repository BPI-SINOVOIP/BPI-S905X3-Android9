/*
 * Copyright (C) 2009 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "FileSource"
#include <utils/Log.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/FileSource.h>
#include <media/stagefright/Utils.h>
#include <private/android_filesystem_config.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace android {

FileSource::FileSource(const char *filename)
    : mFd(-1),
      mOffset(0),
      mLength(-1),
      mName("<null>"),
      mDecryptHandle(NULL),
      mDrmManagerClient(NULL),
      mDrmBufOffset(0),
      mDrmBufSize(0),
      mDrmBuf(NULL){

    if (filename) {
        mName = String8::format("FileSource(%s)", filename);
    }
    ALOGV("%s", filename);
    mFd = open(filename, O_LARGEFILE | O_RDONLY);

    if (mFd >= 0) {
        mLength = lseek64(mFd, 0, SEEK_END);
    } else {
        ALOGE("Failed to open file '%s'. (%s)", filename, strerror(errno));
    }
}

FileSource::FileSource(int fd, int64_t offset, int64_t length)
    : mFd(fd),
      mOffset(offset),
      mLength(length),
      mName("<null>"),
      mDecryptHandle(NULL),
      mDrmManagerClient(NULL),
      mDrmBufOffset(0),
      mDrmBufSize(0),
      mDrmBuf(NULL) {
    ALOGV("fd=%d (%s), offset=%lld, length=%lld",
            fd, nameForFd(fd).c_str(), (long long) offset, (long long) length);

    if (mOffset < 0) {
        mOffset = 0;
    }
    if (mLength < 0) {
        mLength = 0;
    }
    if (mLength > INT64_MAX - mOffset) {
        mLength = INT64_MAX - mOffset;
    }
    struct stat s;
    if (fstat(fd, &s) == 0) {
        if (mOffset > s.st_size) {
            mOffset = s.st_size;
            mLength = 0;
        }
        if (mOffset + mLength > s.st_size) {
            mLength = s.st_size - mOffset;
        }
    }
    if (mOffset != offset || mLength != length) {
        ALOGW("offset/length adjusted from %lld/%lld to %lld/%lld",
                (long long) offset, (long long) length,
                (long long) mOffset, (long long) mLength);
    }

    mName = String8::format(
            "FileSource(fd(%s), %lld, %lld)",
            nameForFd(fd).c_str(),
            (long long) mOffset,
            (long long) mLength);

}

FileSource::~FileSource() {
    if (mFd >= 0) {
        ::close(mFd);
        mFd = -1;
    }

    if (mDrmBuf != NULL) {
        delete[] mDrmBuf;
        mDrmBuf = NULL;
    }

    if (mDecryptHandle != NULL) {
        // To release mDecryptHandle
        CHECK(mDrmManagerClient);
        mDrmManagerClient->closeDecryptSession(mDecryptHandle);
        mDecryptHandle = NULL;
    }

    if (mDrmManagerClient != NULL) {
        delete mDrmManagerClient;
        mDrmManagerClient = NULL;
    }
}

status_t FileSource::initCheck() const {
    return mFd >= 0 ? OK : NO_INIT;
}

ssize_t FileSource::readAt(off64_t offset, void *data, size_t size) {
    if (mFd < 0) {
        return NO_INIT;
    }

    Mutex::Autolock autoLock(mLock);

    if (mLength >= 0) {
        if (offset >= mLength) {
            return 0;  // read beyond EOF.
        }
        uint64_t numAvailable = mLength - offset;
        if ((uint64_t)size > numAvailable) {
            size = numAvailable;
        }
    }

    if (mDecryptHandle != NULL && DecryptApiType::CONTAINER_BASED
            == mDecryptHandle->decryptApiType) {
        return readAtDRM(offset, data, size);
   } else {
        off64_t result = lseek64(mFd, offset + mOffset, SEEK_SET);
        if (result == -1) {
            ALOGE("seek to %lld failed", (long long)(offset + mOffset));
            return UNKNOWN_ERROR;
        }

        return ::read(mFd, data, size);
    }
}

status_t FileSource::getSize(off64_t *size) {
    Mutex::Autolock autoLock(mLock);

    if (mFd < 0) {
        return NO_INIT;
    }

    *size = mLength;

    return OK;
}

sp<DecryptHandle> FileSource::DrmInitialization(const char *mime) {
    if (getuid() == AID_MEDIA_EX) return nullptr; // no DRM in media extractor
    if (mDrmManagerClient == NULL) {
        mDrmManagerClient = new DrmManagerClient();
    }

    if (mDrmManagerClient == NULL) {
        return NULL;
    }

    if (mDecryptHandle == NULL) {
        mDecryptHandle = mDrmManagerClient->openDecryptSession(
                mFd, mOffset, mLength, mime);
    }

    if (mDecryptHandle == NULL) {
        delete mDrmManagerClient;
        mDrmManagerClient = NULL;
    }

    return mDecryptHandle;
}

ssize_t FileSource::readAtDRM(off64_t offset, void *data, size_t size) {
    size_t DRM_CACHE_SIZE = 1024;
    if (mDrmBuf == NULL) {
        mDrmBuf = new unsigned char[DRM_CACHE_SIZE];
    }

    if (mDrmBuf != NULL && mDrmBufSize > 0 && (offset + mOffset) >= mDrmBufOffset
            && (offset + mOffset + size) <= static_cast<size_t>(mDrmBufOffset + mDrmBufSize)) {
        /* Use buffered data */
        memcpy(data, (void*)(mDrmBuf+(offset+mOffset-mDrmBufOffset)), size);
        return size;
    } else if (size <= DRM_CACHE_SIZE) {
        /* Buffer new data */
        mDrmBufOffset =  offset + mOffset;
        mDrmBufSize = mDrmManagerClient->pread(mDecryptHandle, mDrmBuf,
                DRM_CACHE_SIZE, offset + mOffset);
        if (mDrmBufSize > 0) {
            int64_t dataRead = 0;
            dataRead = size > static_cast<size_t>(mDrmBufSize) ? mDrmBufSize : size;
            memcpy(data, (void*)mDrmBuf, dataRead);
            return dataRead;
        } else {
            return mDrmBufSize;
        }
    } else {
        /* Too big chunk to cache. Call DRM directly */
        return mDrmManagerClient->pread(mDecryptHandle, data, size, offset + mOffset);
    }
}

/* static */
bool FileSource::requiresDrm(int fd, int64_t offset, int64_t length, const char *mime) {
    std::unique_ptr<DrmManagerClient> drmClient(new DrmManagerClient());
    sp<DecryptHandle> decryptHandle =
            drmClient->openDecryptSession(fd, offset, length, mime);
    bool requiresDrm = false;
    if (decryptHandle != nullptr) {
        requiresDrm = decryptHandle->decryptApiType == DecryptApiType::CONTAINER_BASED;
        drmClient->closeDecryptSession(decryptHandle);
    }
    return requiresDrm;
}

}  // namespace android
