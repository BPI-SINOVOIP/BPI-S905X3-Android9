/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.settings.about;

import android.content.Context;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.deviceinfo.AbstractSimStatusImeiInfoPreferenceController;
import com.android.tv.settings.NopePreferenceController;
import com.android.tv.settings.PreferenceControllerFragment;
import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.List;

/**
 * Fragment for showing device hardware info, such as MAC addresses and serial numbers
 */
public class StatusFragment extends PreferenceControllerFragment {

    private static final String KEY_BATTERY_STATUS = "battery_status";
    private static final String KEY_BATTERY_LEVEL = "battery_level";
    private static final String KEY_SIM_STATUS = "sim_status";
    private static final String KEY_IMEI_INFO = "imei_info";

    public static StatusFragment newInstance() {
        return new StatusFragment();
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.DEVICEINFO_STATUS;
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.device_info_status;
    }

    @Override
    protected List<AbstractPreferenceController> onCreatePreferenceControllers(Context context) {
        final List<AbstractPreferenceController> controllers = new ArrayList<>(10);
        final Lifecycle lifecycle = getLifecycle();

        // TODO: detect if we have a battery or not
        controllers.add(new NopePreferenceController(context, KEY_BATTERY_LEVEL));
        controllers.add(new NopePreferenceController(context, KEY_BATTERY_STATUS));

        controllers.add(new SerialNumberPreferenceController(context));
        controllers.add(new UptimePreferenceController(context, lifecycle));
        controllers.add(new BluetoothAddressPreferenceController(context, lifecycle));
        controllers.add(new IpAddressPreferenceController(context, lifecycle));
        controllers.add(new WifiMacAddressPreferenceController(context, lifecycle));
        controllers.add(new ImsStatusPreferenceController(context, lifecycle));

        controllers.add(new AdminUserAndPhoneOnlyPreferenceController(context, KEY_SIM_STATUS));
        controllers.add(new AdminUserAndPhoneOnlyPreferenceController(context, KEY_IMEI_INFO));

        return controllers;
    }

    private static class AdminUserAndPhoneOnlyPreferenceController
            extends AbstractSimStatusImeiInfoPreferenceController {

        private final String mKey;

        private AdminUserAndPhoneOnlyPreferenceController(Context context, String key) {
            super(context);
            mKey = key;
        }

        @Override
        public String getPreferenceKey() {
            return mKey;
        }
    }
}
