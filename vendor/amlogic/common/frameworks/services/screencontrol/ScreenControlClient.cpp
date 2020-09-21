/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   huijie huang
 *  @version  1.0
 *  @date     2019/05/06
 *  @par function description:
 *  - screen capture
 *  - screen record
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */
#define LOG_TAG "ScreenControlClient"

#include <utils/Log.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <android-base/logging.h>
#include <ScreenControlClient.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::hidl_memory;
using ::android::hardware::mapMemory;

namespace android {

ScreenControlClient *ScreenControlClient::mInstance = NULL;

ScreenControlClient::ScreenControlClient()
{
	sp<IScreenControl> ctrl = IScreenControl::tryGetService();
	while (ctrl == nullptr) {
         usleep(200*1000);//sleep 200ms
         ctrl = IScreenControl::tryGetService();
         ALOGE("tryGet screen control daemon Service");
    };

	mScreenCtrl = ctrl;
}

ScreenControlClient::~ScreenControlClient()
{
	if (mInstance != NULL)
        delete mInstance;
}

ScreenControlClient *ScreenControlClient::getInstance()
{
	if (NULL == mInstance)
         mInstance = new ScreenControlClient();
    return mInstance;
}

int ScreenControlClient::startScreenRecord(int32_t width, int32_t height, int32_t frameRate,
	int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename)
{
	int result = -1;
	ALOGI("enter %s,width=%d,height=%d,rate=%d,bitrate=%d,timesec=%d,srctype=%d,filename=%s",
		__func__, width, height, frameRate, bitRate, limitTimeSec, sourceType, filename);
	if (Result::OK == mScreenCtrl->startScreenRecord(width, height, frameRate, 
		bitRate, limitTimeSec, sourceType, filename))
		result = 0;
	return result;
}

int ScreenControlClient::startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom,
	int32_t width, int32_t height, int32_t sourceType, const char* filename)
{
	int result = -1;
	ALOGI("enter %s,left=%d,top=%d,right=%d,bottom=%d,width=%d,height=%d,srctype=%d,filename=%s",
		__func__, left, top, right, bottom, width, height, sourceType, filename);

	if (Result::OK == mScreenCtrl->startScreenCap(left, top, right, bottom, width,
		height, sourceType, filename))
		result = 0;
	return result;
}

int ScreenControlClient::startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom,
		int32_t width, int32_t height, int32_t sourceType, void **buffer, int *bufSize)
{
	int result = -1;
	ALOGI("enter %s,left=%d,top=%d,right=%d,bottom=%d,width=%d,height=%d,srctype=%d",
			__func__, left, top, right, bottom, width, height, sourceType);
	mScreenCtrl->startScreenCapBuffer(left, top, right, bottom, width, height, sourceType, 
		[&](const Result &ret, const hidl_memory &mem){
			if(Result::OK == ret) {
				sp<IMemory> memory = mapMemory(mem);
				*bufSize = memory->getSize();
				*buffer = new uint8_t[*bufSize];
				memcpy(*buffer, memory->getPointer(), *bufSize);
				ALOGI("get memory, size=%d", *bufSize);
				result = 0;
			}
		});

	return result;
}

}

