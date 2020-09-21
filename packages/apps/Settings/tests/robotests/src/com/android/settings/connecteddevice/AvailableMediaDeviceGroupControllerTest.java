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
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.R;
import com.android.settings.bluetooth.AvailableMediaBluetoothDeviceUpdater;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowAudioManager;
import com.android.settings.testutils.shadow.ShadowBluetoothUtils;
import com.android.settingslib.bluetooth.BluetoothCallback;
import com.android.settingslib.bluetooth.BluetoothEventManager;
import com.android.settingslib.bluetooth.LocalBluetoothManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = {
        ShadowAudioManager.class,
        ShadowBluetoothUtils.class}
)
public class AvailableMediaDeviceGroupControllerTest {

    private static final String PREFERENCE_KEY_1 = "pref_key_1";

    @Mock
    private DashboardFragment mDashboardFragment;
    @Mock
    private AvailableMediaBluetoothDeviceUpdater mAvailableMediaBluetoothDeviceUpdater;
    @Mock
    private PreferenceScreen mPreferenceScreen;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private PreferenceManager mPreferenceManager;
    @Mock
    private PackageManager mPackageManager;
    @Mock
    private BluetoothEventManager mEventManager;
    @Mock
    private LocalBluetoothManager mLocalManager;

    private PreferenceGroup mPreferenceGroup;
    private Context mContext;
    private Preference mPreference;
    private AvailableMediaDeviceGroupController mAvailableMediaDeviceGroupController;
    private LocalBluetoothManager mLocalBluetoothManager;
    private ShadowAudioManager mShadowAudioManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = spy(RuntimeEnvironment.application);
        mPreference = new Preference(mContext);
        mPreference.setKey(PREFERENCE_KEY_1);
        mPreferenceGroup = spy(new PreferenceScreen(mContext, null));
        when(mPreferenceGroup.getPreferenceManager()).thenReturn(mPreferenceManager);
        doReturn(mContext).when(mDashboardFragment).getContext();
        doReturn(mPackageManager).when(mContext).getPackageManager();
        doReturn(true).when(mPackageManager).hasSystemFeature(PackageManager.FEATURE_BLUETOOTH);

        ShadowBluetoothUtils.sLocalBluetoothManager = mLocalManager;
        mLocalBluetoothManager = ShadowBluetoothUtils.getLocalBtManager(mContext);
        mShadowAudioManager = ShadowAudioManager.getShadow();
        doReturn(mEventManager).when(mLocalBluetoothManager).getEventManager();

        mAvailableMediaDeviceGroupController = new AvailableMediaDeviceGroupController(mContext);
        mAvailableMediaDeviceGroupController.
                setBluetoothDeviceUpdater(mAvailableMediaBluetoothDeviceUpdater);
        mAvailableMediaDeviceGroupController.mPreferenceGroup = mPreferenceGroup;
    }

    @Test
    public void onDeviceAdded_firstAdd_becomeVisibleAndPreferenceAdded() {
        mAvailableMediaDeviceGroupController.onDeviceAdded(mPreference);

        assertThat(mPreferenceGroup.isVisible()).isTrue();
        assertThat(mPreferenceGroup.findPreference(PREFERENCE_KEY_1)).isEqualTo(mPreference);
    }

    @Test
    public void onDeviceRemoved_lastRemove_becomeInvisibleAndPreferenceRemoved() {
        mPreferenceGroup.addPreference(mPreference);

        mAvailableMediaDeviceGroupController.onDeviceRemoved(mPreference);

        assertThat(mPreferenceGroup.isVisible()).isFalse();
        assertThat(mPreferenceGroup.getPreferenceCount()).isEqualTo(0);
    }

    @Test
    public void onDeviceRemoved_notLastRemove_stillVisible() {
        mPreferenceGroup.setVisible(true);
        mPreferenceGroup.addPreference(mPreference);
        mPreferenceGroup.addPreference(new Preference(mContext));

        mAvailableMediaDeviceGroupController.onDeviceRemoved(mPreference);

        assertThat(mPreferenceGroup.isVisible()).isTrue();
    }

    @Test
    public void displayPreference_becomeInvisible() {
        doReturn(mPreferenceGroup).when(mPreferenceScreen).findPreference(anyString());

        mAvailableMediaDeviceGroupController.displayPreference(mPreferenceScreen);

        assertThat(mPreferenceGroup.isVisible()).isFalse();
    }

    @Test
    public void testRegister() {
        // register the callback in onStart()
        mAvailableMediaDeviceGroupController.onStart();
        verify(mAvailableMediaBluetoothDeviceUpdater).registerCallback();
        verify(mLocalBluetoothManager.getEventManager()).registerCallback(
                any(BluetoothCallback.class));
    }

    @Test
    public void testUnregister() {
        // unregister the callback in onStop()
        mAvailableMediaDeviceGroupController.onStop();
        verify(mAvailableMediaBluetoothDeviceUpdater).unregisterCallback();
        verify(mLocalBluetoothManager.getEventManager()).unregisterCallback(
                any(BluetoothCallback.class));
    }

    @Test
    public void testGetAvailabilityStatus_noBluetoothFeature_returnUnSupported() {
        doReturn(false).when(mPackageManager).hasSystemFeature(PackageManager.FEATURE_BLUETOOTH);

        assertThat(mAvailableMediaDeviceGroupController.getAvailabilityStatus()).isEqualTo(
                UNSUPPORTED_ON_DEVICE);
    }

    @Test
    public void testGetAvailabilityStatus_BluetoothFeature_returnSupported() {
        doReturn(true).when(mPackageManager).hasSystemFeature(PackageManager.FEATURE_BLUETOOTH);

        assertThat(mAvailableMediaDeviceGroupController.getAvailabilityStatus()).isEqualTo(
                AVAILABLE);
    }

    @Test
    public void setTitle_inCallState_showCallStateTitle() {
        mShadowAudioManager.setMode(AudioManager.MODE_IN_CALL);
        mAvailableMediaDeviceGroupController.onAudioModeChanged();

        assertThat(mPreferenceGroup.getTitle()).isEqualTo(
                mContext.getText(R.string.connected_device_available_call_title));
    }

    @Test
    public void setTitle_notInCallState_showMediaStateTitle() {
        mShadowAudioManager.setMode(AudioManager.MODE_NORMAL);
        mAvailableMediaDeviceGroupController.onAudioModeChanged();

        assertThat(mPreferenceGroup.getTitle()).isEqualTo(
                mContext.getText(R.string.connected_device_available_media_title));
    }
}
