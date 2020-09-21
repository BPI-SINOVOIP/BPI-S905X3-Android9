/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.settings.connecteddevice;

import android.content.Context;
import android.content.pm.PackageManager;
import android.provider.SearchIndexableResource;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.R;
import com.android.settings.bluetooth.BluetoothFilesPreferenceController;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.nfc.AndroidBeamPreferenceController;
import com.android.settings.print.PrintSettingPreferenceController;
import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.core.lifecycle.Lifecycle;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * This fragment contains all the advanced connection preferences(i.e, Bluetooth, NFC, USB..)
 */
public class AdvancedConnectedDeviceDashboardFragment extends DashboardFragment {

    private static final String TAG = "AdvancedConnectedDeviceFrag";

    static final String KEY_BLUETOOTH = "bluetooth_settings";

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.CONNECTION_DEVICE_ADVANCED;
    }

    @Override
    protected String getLogTag() {
        return TAG;
    }

    @Override
    public int getHelpResource() {
        return R.string.help_url_connected_devices;
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.connected_devices_advanced;
    }

    @Override
    protected List<AbstractPreferenceController> createPreferenceControllers(Context context) {
        return buildControllers(context, getLifecycle());
    }

    private static List<AbstractPreferenceController> buildControllers(Context context,
            Lifecycle lifecycle) {
        final List<AbstractPreferenceController> controllers = new ArrayList<>();

        controllers.add(new BluetoothFilesPreferenceController(context));
        controllers.add(new BluetoothOnWhileDrivingPreferenceController(context));

        final PrintSettingPreferenceController printerController =
                new PrintSettingPreferenceController(context);

        if (lifecycle != null) {
            lifecycle.addObserver(printerController);
        }
        controllers.add(printerController);

        return controllers;
    }

    /**
     * For Search.
     */
    public static final SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider() {
                @Override
                public List<SearchIndexableResource> getXmlResourcesToIndex(
                        Context context, boolean enabled) {
                    final SearchIndexableResource sir = new SearchIndexableResource(context);
                    sir.xmlResId = R.xml.connected_devices_advanced;
                    return Arrays.asList(sir);
                }

                @Override
                public List<String> getNonIndexableKeys(Context context) {
                    final List<String> keys = super.getNonIndexableKeys(context);
                    PackageManager pm = context.getPackageManager();
                    if (!pm.hasSystemFeature(PackageManager.FEATURE_NFC)) {
                        keys.add(AndroidBeamPreferenceController.KEY_ANDROID_BEAM_SETTINGS);
                    }

                    // Parent duplicate
                    keys.add(KEY_BLUETOOTH);

                    return keys;
                }

                @Override
                public List<AbstractPreferenceController> createPreferenceControllers(
                        Context context) {
                    return buildControllers(context, null /* lifecycle */);
                }
            };
}
