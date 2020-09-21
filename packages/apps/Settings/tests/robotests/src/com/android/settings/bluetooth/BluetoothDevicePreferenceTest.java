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
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.os.UserManager;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.core.instrumentation.MetricsFeatureProvider;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
public class BluetoothDevicePreferenceTest {
    private static final boolean SHOW_DEVICES_WITHOUT_NAMES = true;

    private Context mContext;
    @Mock
    private CachedBluetoothDevice mCachedBluetoothDevice;

    private FakeFeatureFactory mFakeFeatureFactory;
    private MetricsFeatureProvider mMetricsFeatureProvider;
    private BluetoothDevicePreference mPreference;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application.getApplicationContext());
        mFakeFeatureFactory = FakeFeatureFactory.setupForTest();
        mMetricsFeatureProvider = mFakeFeatureFactory.getMetricsFeatureProvider();
        mPreference = new BluetoothDevicePreference(mContext, mCachedBluetoothDevice,
                SHOW_DEVICES_WITHOUT_NAMES);
    }

    @Test
    public void onClicked_deviceConnected_shouldLogBluetoothDisconnectEvent() {
        when(mCachedBluetoothDevice.isConnected()).thenReturn(true);

        mPreference.onClicked();

        verify(mMetricsFeatureProvider)
            .action(mContext, MetricsEvent.ACTION_SETTINGS_BLUETOOTH_DISCONNECT);
    }

    @Test
    public void onClicked_deviceBonded_shouldLogBluetoothConnectEvent() {
        when(mCachedBluetoothDevice.isConnected()).thenReturn(false);
        when(mCachedBluetoothDevice.getBondState()).thenReturn(BluetoothDevice.BOND_BONDED);

        mPreference.onClicked();

        verify(mMetricsFeatureProvider)
            .action(mContext, MetricsEvent.ACTION_SETTINGS_BLUETOOTH_CONNECT);
    }

    @Test
    public void onClicked_deviceNotBonded_shouldLogBluetoothPairEvent() {
        when(mCachedBluetoothDevice.isConnected()).thenReturn(false);
        when(mCachedBluetoothDevice.getBondState()).thenReturn(BluetoothDevice.BOND_NONE);
        when(mCachedBluetoothDevice.startPairing()).thenReturn(true);
        when(mCachedBluetoothDevice.hasHumanReadableName()).thenReturn(true);

        mPreference.onClicked();

        verify(mMetricsFeatureProvider)
            .action(mContext, MetricsEvent.ACTION_SETTINGS_BLUETOOTH_PAIR);
        verify(mMetricsFeatureProvider, never())
            .action(mContext, MetricsEvent.ACTION_SETTINGS_BLUETOOTH_PAIR_DEVICES_WITHOUT_NAMES);
    }

    @Test
    public void onClicked_deviceNotBonded_shouldLogBluetoothPairEventAndPairWithoutNameEvent() {
        when(mCachedBluetoothDevice.isConnected()).thenReturn(false);
        when(mCachedBluetoothDevice.getBondState()).thenReturn(BluetoothDevice.BOND_NONE);
        when(mCachedBluetoothDevice.startPairing()).thenReturn(true);
        when(mCachedBluetoothDevice.hasHumanReadableName()).thenReturn(false);

        mPreference.onClicked();

        verify(mMetricsFeatureProvider)
            .action(mContext, MetricsEvent.ACTION_SETTINGS_BLUETOOTH_PAIR);
        verify(mMetricsFeatureProvider)
            .action(mContext, MetricsEvent.ACTION_SETTINGS_BLUETOOTH_PAIR_DEVICES_WITHOUT_NAMES);
    }

    @Test
    public void getSecondTargetResource_shouldBeGearIconLayout() {
        assertThat(mPreference.getSecondTargetResId()).isEqualTo(R.layout.preference_widget_gear);
    }

    @Test
    public void shouldHideSecondTarget_noDevice_shouldReturnTrue() {
        ReflectionHelpers.setField(mPreference, "mCachedDevice", null);

        assertThat(mPreference.shouldHideSecondTarget()).isTrue();
    }

    @Test
    public void shouldHideSecondTarget_notBond_shouldReturnTrue() {
        when(mCachedBluetoothDevice.getBondState()).thenReturn(BluetoothDevice.BOND_NONE);

        assertThat(mPreference.shouldHideSecondTarget()).isTrue();
    }

    @Test
    public void shouldHideSecondTarget_hasUserRestriction_shouldReturnTrue() {
        final UserManager um = mock(UserManager.class);
        ReflectionHelpers.setField(mPreference, "mUserManager", um);
        when(um.hasUserRestriction(UserManager.DISALLOW_CONFIG_BLUETOOTH)).thenReturn(true);

        assertThat(mPreference.shouldHideSecondTarget()).isTrue();
    }

    @Test
    public void shouldHideSecondTarget_hasBoundDeviceAndNoRestriction_shouldReturnFalse() {
        when(mCachedBluetoothDevice.getBondState()).thenReturn(BluetoothDevice.BOND_BONDED);
        final UserManager um = mock(UserManager.class);
        ReflectionHelpers.setField(mPreference, "mUserManager", um);
        when(um.hasUserRestriction(UserManager.DISALLOW_CONFIG_BLUETOOTH)).thenReturn(false);

        assertThat(mPreference.shouldHideSecondTarget()).isFalse();
    }

    @Test
    public void imagingDeviceIcon_isICSettingsPrint() {
        when(mCachedBluetoothDevice.getBatteryLevel())
            .thenReturn(BluetoothDevice.BATTERY_LEVEL_UNKNOWN);
        when(mCachedBluetoothDevice.getBtClass())
            .thenReturn(new BluetoothClass(BluetoothClass.Device.Major.IMAGING));

        mPreference.onDeviceAttributesChanged();
        assertThat(mPreference.getIcon()).isEqualTo(
                mContext.getDrawable(R.drawable.ic_settings_print));
    }

    @Test
    public void testVisible_showDeviceWithoutNames_visible() {
        doReturn(false).when(mCachedBluetoothDevice).hasHumanReadableName();
        BluetoothDevicePreference preference =
                new BluetoothDevicePreference(mContext, mCachedBluetoothDevice,
                        SHOW_DEVICES_WITHOUT_NAMES);

        assertThat(preference.isVisible()).isTrue();
    }

    @Test
    public void testVisible_hideDeviceWithoutNames_invisible() {
        doReturn(false).when(mCachedBluetoothDevice).hasHumanReadableName();
        BluetoothDevicePreference preference =
                new BluetoothDevicePreference(mContext, mCachedBluetoothDevice, false);

        assertThat(preference.isVisible()).isFalse();
    }
}
