/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.settings.development;

import android.content.Context;
import android.os.SystemProperties;
import android.support.annotation.VisibleForTesting;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.development.DeveloperOptionsPreferenceController;

public class BluetoothA2dpHwOffloadPreferenceController extends DeveloperOptionsPreferenceController
        implements Preference.OnPreferenceChangeListener, PreferenceControllerMixin {

    private static final String PREFERENCE_KEY = "bluetooth_disable_a2dp_hw_offload";
    private final DevelopmentSettingsDashboardFragment mFragment;

    static final String A2DP_OFFLOAD_DISABLED_PROPERTY = "persist.bluetooth.a2dp_offload.disabled";
    static final String A2DP_OFFLOAD_SUPPORTED_PROPERTY = "ro.bluetooth.a2dp_offload.supported";

    public BluetoothA2dpHwOffloadPreferenceController(Context context,
            DevelopmentSettingsDashboardFragment fragment) {
        super(context);
        mFragment = fragment;
    }

    @Override
    public String getPreferenceKey() {
        return PREFERENCE_KEY;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        BluetoothA2dpHwOffloadRebootDialog.show(mFragment, this);
        return false;
    }

    @Override
    public void updateState(Preference preference) {
        super.updateState(preference);
        final boolean offloadSupported =
                SystemProperties.getBoolean(A2DP_OFFLOAD_SUPPORTED_PROPERTY, false);
        if (offloadSupported) {
            final boolean offloadDisabled =
                    SystemProperties.getBoolean(A2DP_OFFLOAD_DISABLED_PROPERTY, false);
            ((SwitchPreference) mPreference).setChecked(offloadDisabled);
        } else {
            mPreference.setEnabled(false);
            ((SwitchPreference) mPreference).setChecked(true);
        }
    }

    @Override
    protected void onDeveloperOptionsSwitchDisabled() {
        super.onDeveloperOptionsSwitchDisabled();
        final boolean offloadSupported =
                SystemProperties.getBoolean(A2DP_OFFLOAD_SUPPORTED_PROPERTY, false);
        if (offloadSupported) {
            ((SwitchPreference) mPreference).setChecked(false);
            SystemProperties.set(A2DP_OFFLOAD_DISABLED_PROPERTY, "false");
        } else {
            ((SwitchPreference) mPreference).setChecked(true);
            SystemProperties.set(A2DP_OFFLOAD_DISABLED_PROPERTY, "true");
        }
    }

    public void onA2dpHwDialogConfirmed() {
        final boolean offloadDisabled =
                SystemProperties.getBoolean(A2DP_OFFLOAD_DISABLED_PROPERTY, false);
        SystemProperties.set(A2DP_OFFLOAD_DISABLED_PROPERTY, Boolean.toString(!offloadDisabled));
    }

}
