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

package com.android.settings.development;

import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothCodecConfig;
import android.bluetooth.BluetoothCodecStatus;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.R;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.core.lifecycle.LifecycleObserver;
import com.android.settingslib.core.lifecycle.events.OnDestroy;
import com.android.settingslib.development.DeveloperOptionsPreferenceController;

public abstract class AbstractBluetoothA2dpPreferenceController extends
        DeveloperOptionsPreferenceController implements Preference.OnPreferenceChangeListener,
        PreferenceControllerMixin, BluetoothServiceConnectionListener, LifecycleObserver,
        OnDestroy {

    @VisibleForTesting
    static final int STREAMING_LABEL_ID = R.string.bluetooth_select_a2dp_codec_streaming_label;

    protected final BluetoothA2dpConfigStore mBluetoothA2dpConfigStore;
    protected BluetoothA2dp mBluetoothA2dp;
    protected ListPreference mPreference;
    private final String[] mListValues;
    private final String[] mListSummaries;

    public AbstractBluetoothA2dpPreferenceController(Context context, Lifecycle lifecycle,
            BluetoothA2dpConfigStore store) {
        super(context);

        mBluetoothA2dpConfigStore = store;
        mListValues = getListValues();
        mListSummaries = getListSummaries();

        if (lifecycle != null) {
            lifecycle.addObserver(this);
        }
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);

        mPreference = (ListPreference) screen.findPreference(getPreferenceKey());

        // Set a default value because BluetoothCodecConfig is null initially.
        mPreference.setValue(mListValues[getDefaultIndex()]);
        mPreference.setSummary(mListSummaries[getDefaultIndex()]);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (mBluetoothA2dp == null) {
            return false;
        }

        writeConfigurationValues(newValue);

        final BluetoothCodecConfig codecConfig = mBluetoothA2dpConfigStore.createCodecConfig();
        synchronized (mBluetoothA2dpConfigStore) {
            if (mBluetoothA2dp != null) {
                setCodecConfigPreference(null, codecConfig);    // Use current active device
            }
        }
        // Because the setting is not persisted into permanent storage, we cannot call update state
        // here to update the preference.
        // Instead, we just assume it was set and update the preference here.
        final int index = mPreference.findIndexOfValue(newValue.toString());
        // We only want to append "Streaming" if not using default
        if (index == getDefaultIndex()) {
            mPreference.setSummary(mListSummaries[index]);
        } else {
            mPreference.setSummary(
                    mContext.getResources().getString(STREAMING_LABEL_ID, mListSummaries[index]));
        }
        return true;
    }

    @Override
    public void updateState(Preference preference) {
        if (getCodecConfig(null) == null || mPreference == null) { // Use current active device
            return;
        }

        BluetoothCodecConfig codecConfig;
        synchronized (mBluetoothA2dpConfigStore) {
            codecConfig = getCodecConfig(null);         // Use current active device
        }

        final int index = getCurrentA2dpSettingIndex(codecConfig);
        mPreference.setValue(mListValues[index]);

        // We only want to append "Streaming" if not using default
        if (index == getDefaultIndex()) {
            mPreference.setSummary(mListSummaries[index]);
        } else {
            mPreference.setSummary(
                    mContext.getResources().getString(STREAMING_LABEL_ID, mListSummaries[index]));
        }

        writeConfigurationValues(mListValues[index]);
    }

    @Override
    public void onBluetoothServiceConnected(BluetoothA2dp bluetoothA2dp) {
        mBluetoothA2dp = bluetoothA2dp;
        updateState(mPreference);
    }

    @Override
    public void onBluetoothCodecUpdated() {
        // intentional no-op
        // We do not want to call update state here because the setting is not persisted in
        // permanent storage.
    }

    @Override
    public void onBluetoothServiceDisconnected() {
        mBluetoothA2dp = null;
    }

    @Override
    public void onDestroy() {
        mBluetoothA2dp = null;
    }

    /**
     * @return an array of string values that correspond to the current {@link ListPreference}.
     */
    protected abstract String[] getListValues();

    /**
     * @return an array of string summaries that correspond to the current {@link ListPreference}.
     */
    protected abstract String[] getListSummaries();

    /**
     * Updates the new value to the {@link BluetoothA2dpConfigStore} and the {@link BluetoothA2dp}.
     *
     * @param newValue the new setting value
     */
    protected abstract void writeConfigurationValues(Object newValue);

    /**
     * @return the current selected index for the {@link ListPreference}.
     */
    protected abstract int getCurrentA2dpSettingIndex(BluetoothCodecConfig config);

    /**
     * @return default setting index for the {@link ListPreference}.
     */
    protected abstract int getDefaultIndex();

    @VisibleForTesting
    void setCodecConfigPreference(BluetoothDevice device,
                                  BluetoothCodecConfig config) {
        mBluetoothA2dp.setCodecConfigPreference(device, config);
    }

    @VisibleForTesting
    BluetoothCodecConfig getCodecConfig(BluetoothDevice device) {
        if (mBluetoothA2dp != null) {
            BluetoothCodecStatus codecStatus = mBluetoothA2dp.getCodecStatus(device);
            if (codecStatus != null) {
                return codecStatus.getCodecConfig();
            }
        }
        return null;
    }
}
