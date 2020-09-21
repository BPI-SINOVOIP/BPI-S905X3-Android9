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
 *  @author   zjianqin
 *  @version  1.0
 *  @date     2016/02/17
 *  @par function description:
 *  - 1 callback OutputModeManager.java onEvent
 */

#define LOG_TAG "SystemControl"

#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <ISystemControlNotify.h>

namespace android {

class BpSystemControlNotify : public BpInterface<ISystemControlNotify>
{
public:
    BpSystemControlNotify(const sp<IBinder>& impl)
        : BpInterface<ISystemControlNotify>(impl)
    {
    }

    virtual void onEvent(int event)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemControlNotify::getInterfaceDescriptor());
        data.writeInt32(event);

        if (remote()->transact(TRANSACT_ONEVENT, data, &reply, IBinder::FLAG_ONEWAY) != NO_ERROR) {
            ALOGE("complete event could not contact remote\n");
            return;
        }
    }
};

IMPLEMENT_META_INTERFACE(SystemControlNotify, "com.droidlogic.app.ISystemControlNotify");
}; // namespace android
