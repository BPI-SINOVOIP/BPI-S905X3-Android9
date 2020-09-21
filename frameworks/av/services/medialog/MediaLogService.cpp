/*
 * Copyright (C) 2013 The Android Open Source Project
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

#define LOG_TAG "MediaLog"
//#define LOG_NDEBUG 0

#include <sys/mman.h>
#include <utils/Log.h>
#include <binder/PermissionCache.h>
#include <media/nblog/NBLog.h>
#include <private/android_filesystem_config.h>
#include "MediaLogService.h"

namespace android {

static const char kDeadlockedString[] = "MediaLogService may be deadlocked\n";

// mMerger, mMergeReader, and mMergeThread all point to the same location in memory
// mMergerShared. This is the local memory FIFO containing data merged from all
// individual thread FIFOs in shared memory. mMergeThread is used to periodically
// call NBLog::Merger::merge() to collect the data and write it to the FIFO, and call
// NBLog::MergeReader::getAndProcessSnapshot to process the merged data.
MediaLogService::MediaLogService() :
    BnMediaLogService(),
    mMergerShared((NBLog::Shared*) malloc(NBLog::Timeline::sharedSize(kMergeBufferSize))),
    mMerger(mMergerShared, kMergeBufferSize),
    mMergeReader(mMergerShared, kMergeBufferSize, mMerger),
    mMergeThread(new NBLog::MergeThread(mMerger, mMergeReader))
{
    mMergeThread->run("MergeThread");
}

MediaLogService::~MediaLogService()
{
    mMergeThread->requestExit();
    mMergeThread->setTimeoutUs(0);
    mMergeThread->join();
    free(mMergerShared);
}

void MediaLogService::registerWriter(const sp<IMemory>& shared, size_t size, const char *name)
{
    if (IPCThreadState::self()->getCallingUid() != AID_AUDIOSERVER || shared == 0 ||
            size < kMinSize || size > kMaxSize || name == NULL ||
            shared->size() < NBLog::Timeline::sharedSize(size)) {
        return;
    }
    sp<NBLog::Reader> reader(new NBLog::Reader(shared, size));
    NBLog::NamedReader namedReader(reader, name);
    Mutex::Autolock _l(mLock);
    mNamedReaders.add(namedReader);
    mMerger.addReader(namedReader);
}

void MediaLogService::unregisterWriter(const sp<IMemory>& shared)
{
    if (IPCThreadState::self()->getCallingUid() != AID_AUDIOSERVER || shared == 0) {
        return;
    }
    Mutex::Autolock _l(mLock);
    for (size_t i = 0; i < mNamedReaders.size(); ) {
        if (mNamedReaders[i].reader()->isIMemory(shared)) {
            mNamedReaders.removeAt(i);
        } else {
            i++;
        }
    }
}

bool MediaLogService::dumpTryLock(Mutex& mutex)
{
    bool locked = false;
    for (int i = 0; i < kDumpLockRetries; ++i) {
        if (mutex.tryLock() == NO_ERROR) {
            locked = true;
            break;
        }
        usleep(kDumpLockSleepUs);
    }
    return locked;
}

status_t MediaLogService::dump(int fd, const Vector<String16>& args __unused)
{
    // FIXME merge with similar but not identical code at services/audioflinger/ServiceUtilities.cpp
    static const String16 sDump("android.permission.DUMP");
    if (!(IPCThreadState::self()->getCallingUid() == AID_AUDIOSERVER ||
            PermissionCache::checkCallingPermission(sDump))) {
        dprintf(fd, "Permission Denial: can't dump media.log from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        return NO_ERROR;
    }

    if (args.size() > 0) {
        const String8 arg0(args[0]);
        if (!strcmp(arg0.string(), "-r")) {
            // needed because mNamedReaders is protected by mLock
            bool locked = dumpTryLock(mLock);

            // failed to lock - MediaLogService is probably deadlocked
            if (!locked) {
                String8 result(kDeadlockedString);
                if (fd >= 0) {
                    write(fd, result.string(), result.size());
                } else {
                    ALOGW("%s:", result.string());
                }
                // TODO should we instead proceed to mMergeReader.dump? does it need lock?
                return NO_ERROR;
            }

            for (const auto& namedReader : mNamedReaders) {
                if (fd >= 0) {
                    dprintf(fd, "\n%s:\n", namedReader.name());
                } else {
                    ALOGI("%s:", namedReader.name());
                }
            }
            mLock.unlock();
        }
    }
    mMergeReader.dump(fd);
    return NO_ERROR;
}

status_t MediaLogService::onTransact(uint32_t code, const Parcel& data, Parcel* reply,
        uint32_t flags)
{
    return BnMediaLogService::onTransact(code, data, reply, flags);
}

void MediaLogService::requestMergeWakeup() {
    mMergeThread->wakeup();
}

}   // namespace android
