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

#ifndef CAR_PROCFS_PROCESS
#define CAR_PROCFS_PROCESS

#include <sys/types.h>

#include <binder/Parcelable.h>

using namespace android;

namespace procfsinspector {
    class ProcessInfo : public Parcelable {
    public:
        pid_t getPid() { return mPid; }
        uid_t getUid() { return mUid; }

        // default initialize to invalid values
        ProcessInfo(pid_t pid = -1, uid_t uid = -1) : mPid(pid), mUid(uid) {}

        virtual status_t writeToParcel(Parcel* parcel) const override;
        virtual status_t readFromParcel(const Parcel* parcel) override;

    private:
        pid_t mPid;
        uid_t mUid;
    };
}

#endif // CAR_PROCFS_PROCESS
