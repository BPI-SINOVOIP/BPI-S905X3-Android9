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
 * limitations under the License
 */
package com.android.settings.connecteddevice;

import static com.android.settings.core.BasePreferenceController.AVAILABLE;
import static com.android.settings.core.BasePreferenceController.UNSUPPORTED_ON_DEVICE;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.arch.lifecycle.LifecycleOwner;
import android.content.Context;
import android.content.pm.PackageManager;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.bluetooth.ConnectedBluetoothDeviceUpdater;
import com.android.settings.connecteddevice.dock.DockUpdater;
import com.android.settings.connecteddevice.usb.ConnectedUsbDeviceUpdater;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplicationPackageManager;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = ShadowApplicationPackageManager.class)
public class ConnectedDeviceGroupControllerTest {

    private static final String PREFERENCE_KEY_1 = "pref_key_1";

    @Mock
    private DashboardFragment mDashboardFragment;
    @Mock
    private ConnectedBluetoothDeviceUpdater mConnectedBluetoothDeviceUpdater;
    @Mock
    private ConnectedUsbDeviceUpdater mConnectedUsbDeviceUpdater;
    @Mock
    private DockUpdater mConnectedDockUpdater;
    @Mock
    private PreferenceScreen mPreferenceScreen;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private PreferenceManager mPreferenceManager;

    private ShadowApplicationPackageManager mPackageManager;
    private PreferenceGroup mPreferenceGroup;
    private Context mContext;
    private Preference mPreference;
    private ConnectedDeviceGroupController mConnectedDeviceGroupController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = RuntimeEnvironment.application;
        mPreference = new Preference(mContext);
        mPreference.setKey(PREFERENCE_KEY_1);
        mPackageManager = (ShadowApplicationPackageManager) Shadows.shadowOf(
                mContext.getPackageManager());
        mPreferenceGroup = spy(new PreferenceScreen(mContext, null));
        when(mPreferenceGroup.getPreferenceManager()).thenReturn(mPreferenceManager);
        doReturn(mContext).when(mDashboardFragment).getContext();

        mConnectedDeviceGroupController = new ConnectedDeviceGroupController(mContext);
        mConnectedDeviceGroupController.init(mConnectedBluetoothDeviceUpdater,
                mConnectedUsbDeviceUpdater, mConnectedDockUpdater);
        mConnectedDeviceGroupController.mPreferenceGroup = mPreferenceGroup;
    }

    @Test
    public void testOnDeviceAdded_firstAdd_becomeVisibleAndPreferenceAdded() {
        mConnectedDeviceGroupController.onDeviceAdded(mPreference);

        assertThat(mPreferenceGroup.isVisible()).isTrue();
        assertThat(mPreferenceGroup.findPreference(PREFERENCE_KEY_1)).isEqualTo(mPreference);
    }

    @Test
    public void testOnDeviceRemoved_lastRemove_becomeInvisibleAndPreferenceRemoved() {
        mPreferenceGroup.addPreference(mPreference);

        mConnectedDeviceGroupController.onDeviceRemoved(mPreference);

        assertThat(mPreferenceGroup.isVisible()).isFalse();
        assertThat(mPreferenceGroup.getPreferenceCount()).isEqualTo(0);
    }

    @Test
    public void testOnDeviceRemoved_notLastRemove_stillVisible() {
        mPreferenceGroup.setVisible(true);
        mPreferenceGroup.addPreference(mPreference);
        mPreferenceGroup.addPreference(new Preference(mContext));

        mConnectedDeviceGroupController.onDeviceRemoved(mPreference);

        assertThat(mPreferenceGroup.isVisible()).isTrue();
    }

    @Test
    public void testDisplayPreference_becomeInvisible() {
        doReturn(mPreferenceGroup).when(mPreferenceScreen).findPreference(anyString());

        mConnectedDeviceGroupController.displayPreference(mPreferenceScreen);

        assertThat(mPreferenceGroup.isVisible()).isFalse();
    }

    @Test
    public void testRegister() {
        // register the callback in onStart()
        mConnectedDeviceGroupController.onStart();
        verify(mConnectedBluetoothDeviceUpdater).registerCallback();
        verify(mConnectedUsbDeviceUpdater).registerCallback();
        verify(mConnectedDockUpdater).registerCallback();
    }

    @Test
    public void testUnregister() {
        // unregister the callback in onStop()
        mConnectedDeviceGroupController.onStop();
        verify(mConnectedBluetoothDeviceUpdater).unregisterCallback();
        verify(mConnectedUsbDeviceUpdater).unregisterCallback();
        verify(mConnectedDockUpdater).unregisterCallback();
    }

    @Test
    public void testGetAvailabilityStatus_noBluetoothFeature_returnUnSupported() {
        mPackageManager.setSystemFeature(PackageManager.FEATURE_BLUETOOTH, false);

        assertThat(mConnectedDeviceGroupController.getAvailabilityStatus()).isEqualTo(
                UNSUPPORTED_ON_DEVICE);
    }

    @Test
    public void testGetAvailabilityStatus_BluetoothFeature_returnSupported() {
        mPackageManager.setSystemFeature(PackageManager.FEATURE_BLUETOOTH, true);

        assertThat(mConnectedDeviceGroupController.getAvailabilityStatus()).isEqualTo(
                AVAILABLE);
    }
}
