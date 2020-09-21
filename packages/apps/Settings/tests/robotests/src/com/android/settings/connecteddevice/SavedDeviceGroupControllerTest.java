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
 * limitations under the License
 */
package com.android.settings.connecteddevice;

import static com.android.settings.core.BasePreferenceController.AVAILABLE;
import static com.android.settings.core.BasePreferenceController.UNSUPPORTED_ON_DEVICE;
import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import android.arch.lifecycle.LifecycleOwner;
import android.content.Context;
import android.content.pm.PackageManager;
import android.support.v7.preference.PreferenceManager;
import com.android.settings.bluetooth.BluetoothDeviceUpdater;
import com.android.settings.connecteddevice.dock.DockUpdater;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class SavedDeviceGroupControllerTest {
    private static final String PREFERENCE_KEY_1 = "pref_key_1";

    @Mock
    private DashboardFragment mDashboardFragment;
    @Mock
    private BluetoothDeviceUpdater mBluetoothDeviceUpdater;
    @Mock
    private DockUpdater mSavedDockUpdater;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private PreferenceManager mPreferenceManager;
    @Mock
    private PackageManager mPackageManager;

    private Context mContext;
    private SavedDeviceGroupController mSavedDeviceGroupController;
    private LifecycleOwner mLifecycleOwner;
    private Lifecycle mLifecycle;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application);
        mLifecycleOwner = () -> mLifecycle;
        mLifecycle = new Lifecycle(mLifecycleOwner);
        doReturn(mContext).when(mDashboardFragment).getContext();
        doReturn(mPackageManager).when(mContext).getPackageManager();
        mSavedDeviceGroupController = new SavedDeviceGroupController(mContext);
        mSavedDeviceGroupController.setBluetoothDeviceUpdater(mBluetoothDeviceUpdater);
        mSavedDeviceGroupController.setSavedDockUpdater(mSavedDockUpdater);
    }

    @Test
    public void testRegister() {
        // register the callback in onStart()
        mSavedDeviceGroupController.onStart();
        verify(mBluetoothDeviceUpdater).registerCallback();
        verify(mSavedDockUpdater).registerCallback();
    }
    @Test
    public void testUnregister() {
        // unregister the callback in onStop()
        mSavedDeviceGroupController.onStop();
        verify(mBluetoothDeviceUpdater).unregisterCallback();
        verify(mSavedDockUpdater).unregisterCallback();
    }
    @Test
    public void testGetAvailabilityStatus_noBluetoothFeature_returnUnSupported() {
        doReturn(false).when(mPackageManager).hasSystemFeature(PackageManager.FEATURE_BLUETOOTH);
        assertThat(mSavedDeviceGroupController.getAvailabilityStatus()).isEqualTo(
                UNSUPPORTED_ON_DEVICE);
    }
    @Test
    public void testGetAvailabilityStatus_BluetoothFeature_returnSupported() {
        doReturn(true).when(mPackageManager).hasSystemFeature(PackageManager.FEATURE_BLUETOOTH);
        assertThat(mSavedDeviceGroupController.getAvailabilityStatus()).isEqualTo(
                AVAILABLE);
    }
}
