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

#include "DroidVold.h"

#define LOG_TAG "DroidVold"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <cutils/fs.h>
#include <cutils/log.h>

#include "VolumeManager.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace droidvold {
namespace V1_0 {
namespace implementation {

DroidVold *sInstance;

DroidVold *DroidVold::Instance() {
    if (!sInstance)
        sInstance = new DroidVold();
    return sInstance;
}

DroidVold::DroidVold() {
    mCallback = NULL;
}

DroidVold::~DroidVold() {
}

Return<void> DroidVold::setCallback(const sp<IDroidVoldCallback>& callback) {
    if (VolumeManager::Instance()->getDebug())
        LOG(DEBUG) << "setCallback =" << callback.get();

    /*
    if (callback != NULL) {
        mClients.push_back(callback);
    }
    */

    mCallback = callback;
    return Void();
}

Return<Result> DroidVold::reset() {
    VolumeManager *vm = VolumeManager::Instance();
    vm->reset();
    return Result::OK;
}

Return<Result> DroidVold::shutdown() {
    VolumeManager *vm = VolumeManager::Instance();
    vm->shutdown();
    return Result::OK;
}

Return<Result> DroidVold::mount(const hidl_string& id, uint32_t flag, uint32_t uid) {
    // mount [volId] [flags]
    if (VolumeManager::Instance()->getDebug())
        LOG(DEBUG) << "mount id=" <<  id << " flag=" << flag;

    VolumeManager *vm = VolumeManager::Instance();
    std::string vid = id;

#ifdef HAS_VIRTUAL_CDROM
    // is loop volume
    if (flag == 0xF) {
        if (vm->mountloop(vid.c_str())) {
            LOG(ERROR) << " mount loop error!";
            return Result::FAIL;
        }
        return Result::OK;
    }
#endif

    auto vol = vm->findVolume(vid);
    if (vol == nullptr) {
        LOG(ERROR) << "mount ,count not find volume id=" <<  vid;
        return Result::FAIL;
    }

    vol->setMountFlags(flag);
    vol->setMountUserId(uid);

    int res = vol->mount();
    if (res != 0) {
        LOG(ERROR) << "failed to mount volume=" << vol;
        return Result::FAIL;
    }

    return Result::OK;
}

Return<Result> DroidVold::unmount(const hidl_string& id) {
    // unmount [volId]
    VolumeManager *vm = VolumeManager::Instance();
    std::string vid = id;

#ifdef HAS_VIRTUAL_CDROM
    if (vid.find("/mnt/loop") != std::string::npos) {
        if (vm->unmountloop(true)) {
            LOG(ERROR) << "unmount loop error!";
            return Result::FAIL;
        }
        return Result::OK;
    }
#endif

    auto vol = vm->findVolume(vid);
    if (vol == nullptr) {
        LOG(ERROR) << "unmount, count not find volume id=" <<  vid;
        return Result::FAIL;
    }

    int res = vol->unmount();

    if (res != 0) {
        LOG(ERROR) << "failed to unmount volume=" << vol;
        return Result::FAIL;
    }

    return Result::OK;
}

Return<Result> DroidVold::format(const hidl_string& id, const hidl_string& type) {
    // format [volId] [fsType|auto]
    VolumeManager *vm = VolumeManager::Instance();
    std::string vid = id;
    std::string fsType= type;
    auto vol = vm->findVolume(id);

    if (vol == nullptr) {
        LOG(ERROR) << "format, count not find volume id=" <<  vid;
        return Result::FAIL;
    }

    int res = vol->format(fsType);

    if (res != 0) {
        LOG(ERROR) << "failed to unmount volume=" << vol;
        return Result::FAIL;
    }

    return Result::OK;
}

void DroidVold::sendBroadcast(int event, const std::string& message) {
    if (VolumeManager::Instance()->getDebug())
        LOG(DEBUG) << "event=" << event << " message=" << message;

    hidl_string mss = message;
    if (mCallback != NULL)
        mCallback->onEvent(event, mss);

    /*
    for (int i = 0; i < clientSize; i++) {
        mClients[i]->onEvent(event, mss);
    }
    for (auto client : mClients) {
        client->onEvent(event, mss);
    }
    */
}

Return<int32_t> DroidVold::getDiskFlag(const hidl_string &path) {
    int32_t flag = 0;
    std::string mPath = path;

    flag = VolumeManager::Instance()->getDiskFlag(mPath);

    return flag;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace droidvold
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
