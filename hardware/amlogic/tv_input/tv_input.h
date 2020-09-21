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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2018/1/12
 *  @par function description:
 *  - 1 tv input hal
 */

#ifndef _ANDROID_TV_INPUT_HAL_H_
#define _ANDROID_TV_INPUT_HAL_H_

#ifdef __cplusplus
//extern "C" {
#endif

#include "TvInputIntf.h"
#include "aml_screen.h"

#define LOGD(...) \
{ \
__android_log_print(ANDROID_LOG_DEBUG, "tv_input", __VA_ARGS__); }

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((char*)(ptr) - offsetof(type, member))
#endif

class EventCallback : public TvPlayObserver {
public:
    EventCallback(void *data) {
        mPri = data;
    }

    ~EventCallback() {}

    void onTvEvent (const source_connect_t &scrConnect);
private:
    void *mPri;
};

struct sideband_handle_t {
    native_handle_t nativeHandle;
    int identflag;
    int usage;
};

typedef struct tv_input_private {
    tv_input_device_t device;
    const tv_input_callback_ops_t *callback;
    void *callback_data;
    aml_screen_device_t *mDev;
    TvInputIntf *mpTv;
    EventCallback *eventCallback;
} tv_input_private_t;

enum {
    STREAM_ID_NORMAL            = 1,
    STREAM_ID_FRAME_CAPTURE = 2,
};

void channelControl(tv_input_private_t *priv, bool opsStart, int device_id);
int notifyDeviceStatus(tv_input_private_t *priv, tv_source_input_t inputSrc, int type);
void initTvDevices(tv_input_private_t *priv);

#ifdef __cplusplus
//}
#endif

#endif /*_ANDROID_TV_INPUT_HAL_H_*/

