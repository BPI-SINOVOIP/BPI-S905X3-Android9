/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.compatibility.common.deviceinfo;

import com.android.compatibility.common.util.AppStandbyUtils;
import com.android.compatibility.common.util.DeviceInfoStore;
import com.android.compatibility.common.util.SystemUtil;

/**
 * A device info collector that collects whether app standby was turned on or off in a device
 */
public final class AppStandbyDeviceInfo extends DeviceInfo {
    private static final String KEY_APP_STANDBY_ENABLED = "app_standby_enabled";

    @Override
    protected void collectDeviceInfo(DeviceInfoStore store) throws Exception {
        store.addResult(KEY_APP_STANDBY_ENABLED, AppStandbyUtils.isAppStandbyEnabled());
    }
}
