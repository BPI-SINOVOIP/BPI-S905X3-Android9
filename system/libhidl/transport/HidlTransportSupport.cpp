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
#include <hidl/HidlTransportSupport.h>
#include <hidl/HidlBinderSupport.h>

#include <android/hidl/manager/1.0/IServiceManager.h>

namespace android {
namespace hardware {

void configureRpcThreadpool(size_t maxThreads, bool callerWillJoin) {
    // TODO(b/32756130) this should be transport-dependent
    configureBinderRpcThreadpool(maxThreads, callerWillJoin);
}
void joinRpcThreadpool() {
    // TODO(b/32756130) this should be transport-dependent
    joinBinderRpcThreadpool();
}

int setupTransportPolling() {
    return setupBinderPolling();
}

status_t handleTransportPoll(int /*fd*/) {
    return handleBinderPoll();
}

bool setMinSchedulerPolicy(const sp<::android::hidl::base::V1_0::IBase>& service,
                           int policy, int priority) {
    if (service->isRemote()) {
        ALOGE("Can't set scheduler policy on remote service.");
        return false;
    }

    if (policy != SCHED_NORMAL && policy != SCHED_FIFO && policy != SCHED_RR) {
        ALOGE("Invalid scheduler policy %d", policy);
        return false;
    }

    if (policy == SCHED_NORMAL && (priority < -20 || priority > 19)) {
        ALOGE("Invalid priority for SCHED_NORMAL: %d", priority);
        return false;
    } else if (priority < 1 || priority > 99) {
        ALOGE("Invalid priority for real-time policy: %d", priority);
        return false;
    }

    details::gServicePrioMap.set(service, { policy, priority });

    return true;
}

namespace details {
int32_t getPidIfSharable() {
#if LIBHIDL_TARGET_DEBUGGABLE
    return getpid();
#else
    using android::hidl::manager::V1_0::IServiceManager;
    return static_cast<int32_t>(IServiceManager::PidConstant::NO_PID);
#endif
}
}  // namespace details

}  // namespace hardware
}  // namespace android
