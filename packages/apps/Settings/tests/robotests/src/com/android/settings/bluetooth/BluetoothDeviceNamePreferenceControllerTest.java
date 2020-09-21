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

package com.android.settings.bluetooth;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.bluetooth.LocalBluetoothAdapter;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class BluetoothDeviceNamePreferenceControllerTest {

    private static final String DEVICE_NAME = "Nightshade";
    private static final int ORDER = 1;
    private static final String KEY_DEVICE_NAME = "test_key_name";

    private Context mContext;
    @Mock
    private LocalBluetoothAdapter mLocalAdapter;
    @Mock
    private PreferenceScreen mPreferenceScreen;
    private Preference mPreference;

    private BluetoothDeviceNamePreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = spy(RuntimeEnvironment.application);

        when(mPreferenceScreen.getContext()).thenReturn(mContext);
        mPreference = new Preference(mContext);
        mPreference.setKey(KEY_DEVICE_NAME);
        mController = spy(new BluetoothDeviceNamePreferenceController(mContext, mLocalAdapter,
                KEY_DEVICE_NAME));
        doReturn(DEVICE_NAME).when(mController).getDeviceName();
    }

    @Test
    public void testUpdateDeviceName_showSummaryWithDeviceName() {
        mController.updatePreferenceState(mPreference);

        final CharSequence summary = mPreference.getSummary();

        assertThat(summary.toString())
                .isEqualTo("Visible as \u201CNightshade\u201D to other devices");
        assertThat(mPreference.isSelectable()).isFalse();
    }

    @Test
    public void testCreateBluetoothDeviceNamePreference() {
        Preference preference =
            mController.createBluetoothDeviceNamePreference(mPreferenceScreen, ORDER);

        assertThat(preference.getKey()).isEqualTo(mController.getPreferenceKey());
        assertThat(preference.getOrder()).isEqualTo(ORDER);
        verify(mPreferenceScreen).addPreference(preference);
    }

    @Test
    public void testOnStart_receiverRegistered() {
        mController.onStart();
        verify(mContext).registerReceiver(eq(mController.mReceiver), any());
    }

    @Test
    public void testOnStop_receiverUnregistered() {
        // register it first
        mContext.registerReceiver(mController.mReceiver, null);

        mController.onStop();
        verify(mContext).unregisterReceiver(mController.mReceiver);
    }
}
