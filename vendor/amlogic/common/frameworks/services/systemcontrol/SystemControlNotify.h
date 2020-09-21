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
 *  @author   tellen
 *  @version  1.0
 *  @date     2017/10/18
 *  @par function description:
 *  - 1 control system sysfs proc env & property
 */

#ifndef ANDROID_SYSTEM_CONTROL_NOTIFY_H
#define ANDROID_SYSTEM_CONTROL_NOTIFY_H

#include <utils/RefBase.h>

namespace android {

class SystemControlNotify : virtual public RefBase
{
public:
    SystemControlNotify() {}
    virtual ~SystemControlNotify(){}
    virtual void onEvent(int event) = 0;
    virtual void onFBCUpgradeEvent(int32_t state, int32_t param) = 0;
    virtual void onSetDisplayMode(int mode) = 0;
};

}; // namespace android

#endif // ANDROID_SYSTEM_CONTROL_NOTIFY_H
