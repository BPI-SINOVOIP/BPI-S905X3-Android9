/*
 * Copyright (C) 2006 The Android Open Source Project
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
#define LOG_TAG "ScreenControlHal"
#include <log/log.h>
#include <string>
#include <inttypes.h>
#include <utils/String8.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include "ScreenControlHal.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace screencontrol {
namespace V1_0 {
namespace implementation {
//    using ::android::hidl::memory::V1_0::IMapper;
	using ::android::hidl::allocator::V1_0::IAllocator;
	using ::android::hardware::hidl_memory;
	using ::android::hidl::memory::V1_0::IMemory;
	using ::android::hardware::mapMemory;

    ScreenControlHal::ScreenControlHal() {
        mScreenControl = new ScreenControlService();
    }

    ScreenControlHal::~ScreenControlHal() {
        delete mScreenControl;
    }

    Return<Result> ScreenControlHal::startScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const hidl_string& filename) {
        if ( NULL != mScreenControl) {
	    std::string filenamestr = filename;
            mScreenControl->startScreenRecord(width, height, frameRate, bitRate, limitTimeSec, sourceType, filenamestr.c_str());
            return Result::OK;
        }
        return Result::FAIL;
    }

    Return<Result>  ScreenControlHal::startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, const hidl_string& filename) {
        if ( NULL != mScreenControl) {
            std::string filenamestr = filename;
            mScreenControl->startScreenCap(left, top, right, bottom, width, height, sourceType, filenamestr.c_str());
            return Result::OK;
        }
        return Result::FAIL;
    }

	Return<void> ScreenControlHal::startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, startScreenCapBuffer_cb _cb)
	{
		sp<IAllocator> allocator = IAllocator::getService("ashmem");
		allocator->allocate(width*height*4, [&](bool success, const hidl_memory& mem) {
			int bufSize = 0;
			if (success) {
				sp<IMemory> memory = mapMemory(mem);
				void* data = memory->getPointer();
				memory->update();
				mScreenControl->startScreenCapBuffer(left, top, right, bottom,
						width, height, sourceType, data, &bufSize);
				memory->commit();
				_cb(Result::OK, mem);
			} else {
				ALOGI("alloc memory Fail");
				_cb(Result::FAIL, mem);
			}
		});
		return Void();
	}

    void ScreenControlHal::handleServiceDeath(uint32_t cookie) {

    }

    ScreenControlHal::DeathRecipient::DeathRecipient(sp<ScreenControlHal> sch):mScreenControlHal(sch) {}
    void ScreenControlHal::DeathRecipient::serviceDied(uint64_t cookie,
                    const ::android::wp<::android::hidl::base::V1_0::IBase>& who) {
        ALOGE("screencontrolservice daemon client died cookie:%d",(int)cookie);
        uint32_t type = static_cast<uint32_t>(cookie);
        mScreenControlHal->handleServiceDeath(type);
    }

} //namespace implementation
}//namespace V1_0
} //namespace screencontrol
}//namespace hardware
} //namespace android
} //namespace vendor
