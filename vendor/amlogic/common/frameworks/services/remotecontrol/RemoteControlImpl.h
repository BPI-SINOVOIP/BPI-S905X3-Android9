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
 *  @author   yongguang.hong
 *  @version  1.0
 *  @date     2018/03/19
 *  @par function description:
 *  - 1 control bluetooth RC
 */

#ifndef ANDROID_REMOTE_CONTROL_IMPL_H
#define ANDROID_REMOTE_CONTROL_IMPL_H

#include <utils/Errors.h>
//#include <binder/IInterface.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <string>
#include <vector>
#include <HidlTransportSupport.h>
#include "RemoteControl.h"

using namespace android;
using ::android::hardware::configureRpcThreadpool;
using ::vendor::amlogic::hardware::remotecontrol::V1_0::implementation::RemoteControl;
using ::vendor::amlogic::hardware::remotecontrol::V1_0::IRemoteControl;

class RemoteControlImpl : public  RemoteControl::Callback {
public:
    RemoteControlImpl();
    virtual ~RemoteControlImpl();

    virtual void onSetMicEnable(int flag);
    virtual void onDeviceStatusChanged(int flag);

    void setCallback(rc_callbacks_t *rcCallback);

private:
    rc_callbacks_t *mCallbacks;
    sp<IRemoteControl> mRemoteControl;

};


#endif  // ANDROID_REMOTE_CONTROL_IMPL_H

