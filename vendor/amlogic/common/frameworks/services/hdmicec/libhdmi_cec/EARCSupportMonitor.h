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
 *  @author   Jinping Wang
 *  @version  1.0
 *  @date     2019/08/16
 *  @par function description:
 *  - 1 process HDCP RX authenticate
 */

#ifndef _HDMI_EARC_SUPPORT_MONITOR_H
#define _HDMI_EARC_SUPPORT_MONITOR_H
#include <utils/Mutex.h>
#define HDMI_EARC_RX_UEVENT     "earc/extcon/earcrx"
#define HDMI_EARC_RX_NAME       "earcrx"
#define HDMI_EARC_TX_UEVENT     "earc/extcon/earctx"
#define HDMI_EARC_TX_NAME       "earctx"
#define UNSUPPORT               "0"
#define SUPPORT                 "1"
class EARCSupportMonitor {
public:
    EARCSupportMonitor();
    ~EARCSupportMonitor();
    bool getEARCSupport();
    void setEARCSupport(bool status);
    static void* MonitorUenventThreadLoop(void* data);
private:
    bool mEARC;
    mutable android::Mutex mLock;
};

#endif // _HDMI_EARC_SUPPORT_MONITOR_H
