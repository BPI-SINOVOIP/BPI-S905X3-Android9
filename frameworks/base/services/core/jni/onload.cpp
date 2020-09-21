/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include <nativehelper/JNIHelp.h>
#include "jni.h"
#include "utils/Log.h"
#include "utils/misc.h"

#include "BroadcastRadio/BroadcastRadioService.h"
#include "BroadcastRadio/Tuner.h"

namespace android {
int register_android_server_AlarmManagerService(JNIEnv* env);
int register_android_server_BatteryStatsService(JNIEnv* env);
int register_android_server_ConsumerIrService(JNIEnv *env);
int register_android_server_InputApplicationHandle(JNIEnv* env);
int register_android_server_InputWindowHandle(JNIEnv* env);
int register_android_server_InputManager(JNIEnv* env);
int register_android_server_LightsService(JNIEnv* env);
int register_android_server_PowerManagerService(JNIEnv* env);
int register_android_server_storage_AppFuse(JNIEnv* env);
int register_android_server_SerialService(JNIEnv* env);
int register_android_server_SystemServer(JNIEnv* env);
int register_android_server_UsbAlsaJackDetector(JNIEnv* env);
int register_android_server_UsbDeviceManager(JNIEnv* env);
int register_android_server_UsbMidiDevice(JNIEnv* env);
int register_android_server_UsbHostManager(JNIEnv* env);
int register_android_server_vr_VrManagerService(JNIEnv* env);
int register_android_server_VibratorService(JNIEnv* env);
int register_android_server_location_GnssLocationProvider(JNIEnv* env);
int register_android_server_connectivity_Vpn(JNIEnv* env);
int register_android_server_connectivity_tethering_OffloadHardwareInterface(JNIEnv*);
int register_android_server_devicepolicy_CryptoTestHelper(JNIEnv*);
int register_android_server_hdmi_HdmiCecController(JNIEnv* env);
int register_android_server_tv_TvUinputBridge(JNIEnv* env);
int register_android_server_tv_TvInputHal(JNIEnv* env);
int register_android_server_PersistentDataBlockService(JNIEnv* env);
int register_android_server_Watchdog(JNIEnv* env);
int register_android_server_HardwarePropertiesManagerService(JNIEnv* env);
int register_android_server_SyntheticPasswordManager(JNIEnv* env);
int register_android_server_GraphicsStatsService(JNIEnv* env);
int register_android_hardware_display_DisplayViewport(JNIEnv* env);
int register_android_server_net_NetworkStatsService(JNIEnv* env);
#ifdef USE_ARC
int register_android_server_ArcVideoService();
#endif
};

using namespace android;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* /* reserved */)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_android_server_broadcastradio_BroadcastRadioService(env);
    register_android_server_broadcastradio_Tuner(vm, env);
    register_android_server_PowerManagerService(env);
    register_android_server_SerialService(env);
    register_android_server_InputApplicationHandle(env);
    register_android_server_InputWindowHandle(env);
    register_android_server_InputManager(env);
    register_android_server_LightsService(env);
    register_android_server_AlarmManagerService(env);
    register_android_server_UsbDeviceManager(env);
    register_android_server_UsbMidiDevice(env);
    register_android_server_UsbAlsaJackDetector(env);
    register_android_server_UsbHostManager(env);
    register_android_server_vr_VrManagerService(env);
    register_android_server_VibratorService(env);
    register_android_server_SystemServer(env);
    register_android_server_location_GnssLocationProvider(env);
    register_android_server_connectivity_Vpn(env);
    register_android_server_connectivity_tethering_OffloadHardwareInterface(env);
    register_android_server_devicepolicy_CryptoTestHelper(env);
    register_android_server_ConsumerIrService(env);
    register_android_server_BatteryStatsService(env);
    register_android_server_hdmi_HdmiCecController(env);
    register_android_server_tv_TvUinputBridge(env);
    register_android_server_tv_TvInputHal(env);
    register_android_server_PersistentDataBlockService(env);
    register_android_server_HardwarePropertiesManagerService(env);
    register_android_server_storage_AppFuse(env);
    register_android_server_SyntheticPasswordManager(env);
    register_android_server_GraphicsStatsService(env);
    register_android_hardware_display_DisplayViewport(env);
    register_android_server_net_NetworkStatsService(env);
#ifdef USE_ARC
    register_android_server_ArcVideoService();
#endif
    return JNI_VERSION_1_4;
}
