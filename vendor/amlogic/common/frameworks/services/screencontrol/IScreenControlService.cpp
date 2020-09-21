/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "IScreenControlService"

#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <IScreenControlService.h>
#include <utils/Log.h>

namespace android {

class BpScreenControlService : public BpInterface<IScreenControlService>
{
public:
    BpScreenControlService(const sp<IBinder>& impl)
        : BpInterface<IScreenControlService>(impl) {
    }

    virtual int startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, const char* fileName) {
        ALOGI("BpScreenControlService startScreenCap left:%d, top:%d, right:%d, bottom:%d, width:%d, height:%d, sourceType:%d, fileName:%s\n", left, top, right, bottom, width, height, sourceType, fileName);
        Parcel data, reply;
        data.writeInterfaceToken(IScreenControlService::getInterfaceDescriptor());
        data.writeInt32(left);
        data.writeInt32(top);
        data.writeInt32(right);
        data.writeInt32(bottom);
        data.writeInt32(width);
        data.writeInt32(height);
        data.writeInt32(sourceType);
        data.writeCString(fileName);
        remote()->transact(BnScreenControlService::SCREEN_CAP, data, &reply);
        return NO_ERROR;
    }

    virtual int startScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* fileName) {
        ALOGI("BpScreenControlService startScreenRecord width:%d, height:%d, frameRate:%d, bitRate:%d, limitTimeSec:%d, sourceType:%d, fileName\n", width, height, frameRate, bitRate, limitTimeSec, sourceType, fileName);
        Parcel data, reply;
        data.writeInterfaceToken(IScreenControlService::getInterfaceDescriptor());
	data.writeInt32(width);
	data.writeInt32(height);
	data.writeInt32(frameRate);
	data.writeInt32(bitRate);
	data.writeInt32(limitTimeSec);
	data.writeInt32(sourceType);
	data.writeCString(fileName);
	remote()->transact(BnScreenControlService::SCREEN_REC, data, &reply);
	return NO_ERROR;
    }

};

IMPLEMENT_META_INTERFACE(ScreenControlService, "droidlogic.IScreenControlService");

status_t BnScreenControlService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case SCREEN_CAP: {
            CHECK_INTERFACE(IScreenControlService, data, reply);
            int32_t left = data.readInt32();
            int32_t top = data.readInt32();
            int32_t right = data.readInt32();
            int32_t bottom = data.readInt32();
            int32_t width = data.readInt32();
            int32_t height = data.readInt32();
            int32_t sourceType = data.readInt32();
            const char* fileName = data.readCString();
            ALOGI("BnScreenControlService onTransact left:%d, top:%d, right:%d, bottom:%d, width:%d, height:%d, sourceType:%d, fileName:%s\n", left, top, right, bottom, width, height, sourceType, fileName);
            int err = startScreenCap(left, top, right, bottom, width, height, sourceType, fileName);
            reply->writeInt32(err);
            return NO_ERROR;
        }

        case SCREEN_REC: {
            CHECK_INTERFACE(IScreenControlService, data, reply);
            int32_t width = data.readInt32();
            int32_t height = data.readInt32();
            int32_t frameRate = data.readInt32();
            int32_t bitRate = data.readInt32();
            int32_t limitTimeSec = data.readInt32();
            int32_t sourceType = data.readInt32();
            const char* fileName = data.readCString();
            ALOGI("BnScreenControlService onTransact width:%d, height:%d, frameRate:%d, bitRate:%d, limitTimeSec:%d, sourceType:%d, fileName:%s\n", width, height, frameRate, bitRate, limitTimeSec, sourceType, fileName);
            int err = startScreenRecord(width, height, frameRate, bitRate, limitTimeSec, sourceType, fileName);
            reply->writeInt32(err);
            return NO_ERROR;
        }

        default: {
            return BBinder::onTransact(code, data, reply, flags);
        }
    }
}

};
