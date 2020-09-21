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
 *  @version  2.0
 *  @date     2017/01/24
 *  @par function description:
 *  - 1 write property or sysfs in daemon
 */



#ifndef FRAMERATE_ADAPTER_H
#define FRAMERATE_ADAPTER_H

#include "SysWrite.h"
#include "common.h"
#include "UEventObserver.h"

/*
Duration time base is 1/96000 second.
So FRAME_RATE_DURATION is a frame display
duration time use this time base as unit.
For example frame rate is 23.976 fps
FRAME_RATE_DURATION_23.97 = 96000/23.976 = 4004
*/
#define HDMI_TX_FRAMRATE_POLICY                 "/sys/class/amhdmitx/amhdmitx0/frac_rate_policy"

//Frame rate switch
#define HDMI_TVOUT_FRAME_RATE_UEVENT            "DEVPATH=/devices/virtual/tv/tv"
#define HDMI_FRAME_RATE_AUTO                    "/sys/class/tv/policy_fr_auto"
#define FRAME_RATE_DURATION_2397                4004
#define FRAME_RATE_DURATION_2398                4003
#define FRAME_RATE_DURATION_24                  4000
#define FRAME_RATE_DURATION_25                  3840
#define FRAME_RATE_DURATION_2997                3203
#define FRAME_RATE_DURATION_30                  3200
#define FRAME_RATE_DURATION_50                  1920
#define FRAME_RATE_DURATION_5994                1601
#define FRAME_RATE_DURATION_5992                1602
#define FRAME_RATE_DURATION_60                  1600
#define FRAME_RATE_HDMI_OFF                     "0"
#define FRAME_RATE_HDMI_CLK_PULLDOWN            "1"
#define FRAME_RATE_HDMI_SWITCH_FORCE            "2"

/*
#define MODE_ADAPTER_1080P24HZ          "1080p24adahz"    //1080p23.976hz
#define MODE_ADAPTER_1080P30HZ          "1080p30adahz"    //1080p29.97hz
#define MODE_ADAPTER_1080P              "1080p60adahz"    //1080p59.94hz
#define MODE_ADAPTER_4K2K24HZ           "2160p24adahz"    //2160p23.976hz
#define MODE_ADAPTER_4K2K30HZ           "2160p30adahz"    //2160p29.97hz
#define MODE_ADAPTER_4K2K60HZ           "2160p60adahz"    //2160p59.94hz
#define MODE_ADAPTER                    "adahz"           //23.976hz or 29.97hz or 59.94hz

enum {
    DISPLAY_MODE_ORIGINAL_1080P         = 0,
    DISPLAY_MODE_ORIGINAL_1080P24HZ     = 1,
    DISPLAY_MODE_ORIGINAL_1080P50HZ     = 2,
    DISPLAY_MODE_ORIGINAL_NUM           = 3,
    DISPLAY_MODE_ADAPTER_1080P24HZ      = 3,
    DISPLAY_MODE_ADAPTER_1080P30HZ      = 4,
    DISPLAY_MODE_ADAPTER_1080P          = 5,
    DISPLAY_MODE_ADAPTER_1080_NUM       = 6
};
*/

class FrameRateAutoAdaption
{
public:
    class Callbak {
    public:
        Callbak() {};
        virtual ~Callbak() {};
        virtual void onDispModeSyncEvent (const char* outputmode, int state) = 0;
    };

    FrameRateAutoAdaption(Callbak *cb);
    ~FrameRateAutoAdaption();

    void onTxUeventReceived(uevent_data_t* ueventData);
    bool autoSwitchFlag = false;

private:
    void readSinkEdid(char *edid);
    void getMatchDurOutputMode (int dur, char *curMode, char *frameRateMode, char *newMode, bool *pulldown);

    Callbak *mCallback;
    SysWrite mSysWrite;
    char mLastVideoMode[MODE_LEN] = {0};
};
#endif // FRAMERATE_ADAPTER_H