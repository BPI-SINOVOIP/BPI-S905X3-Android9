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

import static com.android.settings.development.ClearAdbKeysPreferenceController.RO_ADB_SECURE_PROPERTY_KEY;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Fragment;
import android.content.Context;
import android.hardware.usb.IUsbManager;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = ShadowUtils.class)
public class ClearAdbKeysPreferenceControllerTest {

    @Mock
    private PreferenceScreen mScreen;
    @Mock
    private SwitchPreference mPreference;
    @Mock
    private IUsbManager mUsbManager;
    @Mock
    private DevelopmentSettingsDashboardFragment mFragment;

    private ClearAdbKeysPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        final Context context = RuntimeEnvironment.application;
        mController = spy(new ClearAdbKeysPreferenceController(context, mFragment));
        ReflectionHelpers.setField(mController, "mUsbManager", mUsbManager);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(mPreference);
    }

    @After
    public void tearDown() {
        ShadowClearAdbKeysWarningDialog.resetDialog();
        ShadowUtils.reset();
    }

    @Test
    public void isAvailable_roAdbSecureEnabled_shouldBeTrue() {
        SystemProperties.set(RO_ADB_SECURE_PROPERTY_KEY, Boolean.toString(true));

        assertThat(mController.isAvailable()).isTrue();
    }

    @Test
    public void isAvailable_roAdbSecureDisabled_shouldBeFalse() {
        SystemProperties.set(RO_ADB_SECURE_PROPERTY_KEY, Boolean.toString(false));

        assertThat(mController.isAvailable()).isFalse();
    }

    @Test
    public void displayPreference_isNotAdminUser_preferenceShouldBeDisabled() {
        SystemProperties.set(RO_ADB_SECURE_PROPERTY_KEY, Boolean.toString(true));
        doReturn(false).when(mController).isAdminUser();

        mController.displayPreference(mScreen);

        verify(mPreference).setEnabled(false);
    }

    @Test
    @Config(shadows = ShadowClearAdbKeysWarningDialog.class)
    public void handlePreferenceTreeClick_clearAdbKeysPreference_shouldShowWarningDialog() {
        SystemProperties.set(RO_ADB_SECURE_PROPERTY_KEY, Boolean.toString(true));
        doReturn(true).when(mController).isAdminUser();
        mController.displayPreference(mScreen);
        final String preferenceKey = mController.getPreferenceKey();
        when(mPreference.getKey()).thenReturn(preferenceKey);
        final boolean isHandled = mController.handlePreferenceTreeClick(mPreference);

        assertThat(ShadowClearAdbKeysWarningDialog.sIsShowing).isTrue();
        assertThat(isHandled).isTrue();
    }

    @Test
    public void handlePreferenceTreeClick_notClearAdbKeysPreference_shouldReturnFalse() {
        SystemProperties.set(RO_ADB_SECURE_PROPERTY_KEY, Boolean.toString(true));
        doReturn(true).when(mController).isAdminUser();
        mController.displayPreference(mScreen);
        when(mPreference.getKey()).thenReturn("Some random key!!!");
        final boolean isHandled = mController.handlePreferenceTreeClick(mPreference);

        assertThat(isHandled).isFalse();
    }

    @Test
    public void handlePreferenceTreeClick_monkeyUser_shouldReturnFalse() {
        SystemProperties.set(RO_ADB_SECURE_PROPERTY_KEY, Boolean.toString(true));
        doReturn(true).when(mController).isAdminUser();
        ShadowUtils.setIsUserAMonkey(true);
        mController.displayPreference(mScreen);
        final String preferenceKey = mController.getPreferenceKey();
        when(mPreference.getKey()).thenReturn(preferenceKey);

        final boolean isHandled = mController.handlePreferenceTreeClick(mPreference);

        assertThat(isHandled).isFalse();
    }

    @Test
    public void onDeveloperOptionsSwitchEnabled_isAdminUser_shouldEnablePreference() {
        SystemProperties.set(RO_ADB_SECURE_PROPERTY_KEY, Boolean.toString(true));
        doReturn(true).when(mController).isAdminUser();
        mController.displayPreference(mScreen);
        mController.onDeveloperOptionsSwitchEnabled();

        verify(mPreference).setEnabled(true);
    }

    @Test
    public void onDeveloperOptionsSwitchEnabled_isNotAdminUser_shouldNotEnablePreference() {
        SystemProperties.set(RO_ADB_SECURE_PROPERTY_KEY, Boolean.toString(true));
        doReturn(false).when(mController).isAdminUser();
        mController.displayPreference(mScreen);
        mController.onDeveloperOptionsSwitchEnabled();

        verify(mPreference, never()).setEnabled(true);
    }

    @Test
    public void onClearAdbKeysConfirmed_shouldClearKeys() throws RemoteException {
        mController.onClearAdbKeysConfirmed();

        verify(mUsbManager).clearUsbDebuggingKeys();
    }

    @Implements(ClearAdbKeysWarningDialog.class)
    public static class ShadowClearAdbKeysWarningDialog {

        private static boolean sIsShowing;

        @Implementation
        public static void show(Fragment host) {
            sIsShowing = true;
        }

        private static void resetDialog() {
            sIsShowing = false;
        }
    }
}
