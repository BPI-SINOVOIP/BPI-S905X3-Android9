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

package com.android.settings.network;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.arch.lifecycle.LifecycleOwner;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.provider.SettingsSlicesContract;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.core.BasePreferenceController;
import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.RestrictedSwitchPreference;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(SettingsRobolectricTestRunner.class)
public class AirplaneModePreferenceControllerTest {

    private static final int ON = 1;
    private static final int OFF = 0;

    @Mock
    private PackageManager mPackageManager;

    private Context mContext;
    private ContentResolver mResolver;
    private PreferenceManager mPreferenceManager;
    private PreferenceScreen mScreen;
    private RestrictedSwitchPreference mPreference;
    private AirplaneModePreferenceController mController;
    private LifecycleOwner mLifecycleOwner;
    private Lifecycle mLifecycle;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        FakeFeatureFactory.setupForTest();
        mContext = spy(RuntimeEnvironment.application);
        mResolver = RuntimeEnvironment.application.getContentResolver();
        doReturn(mPackageManager).when(mContext).getPackageManager();
        mController = new AirplaneModePreferenceController(mContext,
                SettingsSlicesContract.KEY_AIRPLANE_MODE);

        mPreferenceManager = new PreferenceManager(mContext);
        mScreen = mPreferenceManager.createPreferenceScreen(mContext);
        mPreference = new RestrictedSwitchPreference(mContext);
        mPreference.setKey(SettingsSlicesContract.KEY_AIRPLANE_MODE);
        mScreen.addPreference(mPreference);
        mController.setFragment(null);
        mLifecycleOwner = () -> mLifecycle;
        mLifecycle = new Lifecycle(mLifecycleOwner);
        mLifecycle.addObserver(mController);
    }

    @Test
    @Config(qualifiers = "mcc999")
    public void airplaneModePreference_shouldNotBeAvailable_ifSetToNotVisible() {
        assertThat(mController.getAvailabilityStatus())
                .isNotEqualTo(BasePreferenceController.AVAILABLE);

        mController.displayPreference(mScreen);

        // This should not crash
        mController.onResume();
        mController.onPause();
    }

    @Test
    public void airplaneModePreference_shouldNotBeAvailable_ifHasLeanbackFeature() {
        when(mPackageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)).thenReturn(true);
        assertThat(mController.getAvailabilityStatus())
                .isNotEqualTo(BasePreferenceController.AVAILABLE);

        mController.displayPreference(mScreen);

        // This should not crash
        mController.onResume();
        mController.onPause();
    }

    @Test
    public void airplaneModePreference_shouldBeAvailable_ifNoLeanbackFeature() {
        when(mPackageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)).thenReturn(false);
        assertThat(mController.getAvailabilityStatus())
                .isEqualTo(BasePreferenceController.AVAILABLE);
    }

    @Test
    public void airplaneModePreference_testSetValue_updatesCorrectly() {
        // Airplane mode default off
        Settings.Global.putInt(mResolver, Settings.Global.AIRPLANE_MODE_ON, OFF);

        mController.displayPreference(mScreen);
        mController.onResume();

        assertThat(mPreference.isChecked()).isFalse();

        assertThat(mController.isChecked()).isFalse();

        // Set airplane mode ON by setChecked
        boolean updated = mController.setChecked(true);
        assertThat(updated).isTrue();

        // Check return value if set same status.
        updated = mController.setChecked(true);
        assertThat(updated).isFalse();

        // UI is updated
        assertThat(mPreference.isChecked()).isTrue();

        // Settings status changed.
        int updatedValue = Settings.Global.getInt(mResolver, Settings.Global.AIRPLANE_MODE_ON, OFF);
        assertThat(updatedValue).isEqualTo(ON);

        // Set to OFF
        assertThat(mController.setChecked(false)).isTrue();
        assertThat(mPreference.isChecked()).isFalse();
        updatedValue = Settings.Global.getInt(mResolver, Settings.Global.AIRPLANE_MODE_ON, OFF);
        assertThat(updatedValue).isEqualTo(OFF);
    }

    @Test
    public void airplaneModePreference_testGetValue_correctValueReturned() {
        // Set airplane mode ON
        Settings.Global.putInt(mResolver, Settings.Global.AIRPLANE_MODE_ON, ON);

        mController.displayPreference(mScreen);
        mController.onResume();

        assertThat(mController.isChecked()).isTrue();

        Settings.Global.putInt(mResolver, Settings.Global.AIRPLANE_MODE_ON, OFF);
        assertThat(mController.isChecked()).isFalse();
    }

    @Test
    public void airplaneModePreference_testPreferenceUI_updatesCorrectly() {
        // Airplane mode default off
        Settings.Global.putInt(mResolver, Settings.Global.AIRPLANE_MODE_ON, OFF);

        mController.displayPreference(mScreen);
        mController.onResume();

        assertThat(mPreference.isChecked()).isFalse();

        mController.onAirplaneModeChanged(true);

        assertThat(mPreference.isChecked()).isTrue();
    }

    @Test
    public void isSliceableCorrectKey_returnsTrue() {
        final AirplaneModePreferenceController controller =
                new AirplaneModePreferenceController(mContext,"toggle_airplane");
        assertThat(controller.isSliceable()).isTrue();
    }

    @Test
    public void isSliceableIncorrectKey_returnsFalse() {
        final AirplaneModePreferenceController controller =
                new AirplaneModePreferenceController(mContext, "bad_key");
        assertThat(controller.isSliceable()).isFalse();
    }
}
