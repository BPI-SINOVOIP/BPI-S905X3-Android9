/*
 * Copyright 2017 The Android Open Source Project
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
#undef LOG_TAG
#define LOG_TAG "SurfaceTracing"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "SurfaceTracing.h"

#include <android-base/file.h>
#include <log/log.h>
#include <utils/SystemClock.h>
#include <utils/Trace.h>

namespace android {

void SurfaceTracing::enable() {
    if (mEnabled) {
        return;
    }
    ATRACE_CALL();
    mEnabled = true;
    std::lock_guard<std::mutex> protoGuard(mTraceMutex);

    mTrace.set_magic_number(uint64_t(LayersTraceFileProto_MagicNumber_MAGIC_NUMBER_H) << 32 |
                            LayersTraceFileProto_MagicNumber_MAGIC_NUMBER_L);
}

status_t SurfaceTracing::disable() {
    if (!mEnabled) {
        return NO_ERROR;
    }
    ATRACE_CALL();
    std::lock_guard<std::mutex> protoGuard(mTraceMutex);
    mEnabled = false;
    status_t err(writeProtoFileLocked());
    ALOGE_IF(err == PERMISSION_DENIED, "Could not save the proto file! Permission denied");
    ALOGE_IF(err == NOT_ENOUGH_DATA, "Could not save the proto file! There are missing fields");
    mTrace.Clear();
    return err;
}

bool SurfaceTracing::isEnabled() {
    return mEnabled;
}

void SurfaceTracing::traceLayers(const char* where, LayersProto layers) {
    std::lock_guard<std::mutex> protoGuard(mTraceMutex);

    LayersTraceProto* entry = mTrace.add_entry();
    entry->set_elapsed_realtime_nanos(elapsedRealtimeNano());
    entry->set_where(where);
    entry->mutable_layers()->Swap(&layers);
}

status_t SurfaceTracing::writeProtoFileLocked() {
    ATRACE_CALL();

    if (!mTrace.IsInitialized()) {
        return NOT_ENOUGH_DATA;
    }
    std::string output;
    if (!mTrace.SerializeToString(&output)) {
        return PERMISSION_DENIED;
    }
    if (!android::base::WriteStringToFile(output, mOutputFileName, true)) {
        return PERMISSION_DENIED;
    }

    return NO_ERROR;
}

} // namespace android
