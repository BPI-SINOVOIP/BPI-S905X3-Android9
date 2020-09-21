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

package com.android.settings.notification;

import static android.app.NotificationChannel.DEFAULT_CHANNEL_ID;
import static android.app.NotificationManager.IMPORTANCE_DEFAULT;
import static android.app.NotificationManager.IMPORTANCE_HIGH;
import static android.app.NotificationManager.IMPORTANCE_LOW;
import static android.provider.Settings.System.NOTIFICATION_LIGHT_PULSE;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.UserManager;
import android.provider.Settings;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.SettingsShadowResources;
import com.android.settingslib.RestrictedLockUtils;
import com.android.settingslib.RestrictedSwitchPreference;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplication;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = SettingsShadowResources.class)
public class LightsPreferenceControllerTest {

    private Context mContext;
    @Mock
    private NotificationBackend mBackend;
    @Mock
    private NotificationManager mNm;
    @Mock
    private UserManager mUm;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private PreferenceScreen mScreen;

    private LightsPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        ShadowApplication shadowApplication = ShadowApplication.getInstance();
        shadowApplication.setSystemService(Context.NOTIFICATION_SERVICE, mNm);
        shadowApplication.setSystemService(Context.USER_SERVICE, mUm);
        mContext = RuntimeEnvironment.application;
        mController = spy(new LightsPreferenceController(mContext, mBackend));

        // By default allow lights
        SettingsShadowResources.overrideResource(
                com.android.internal.R.bool.config_intrusiveNotificationLed, true);
        Settings.Secure.putInt(mContext.getContentResolver(), NOTIFICATION_LIGHT_PULSE, 1);
    }

    @Test
    public void testNoCrashIfNoOnResume() {
        mController.isAvailable();
        mController.updateState(mock(RestrictedSwitchPreference.class));
        mController.onPreferenceChange(mock(RestrictedSwitchPreference.class), true);
    }

    @Test
    public void testIsAvailable_notIfConfigNotAllowed() {
        SettingsShadowResources.overrideResource(
                com.android.internal.R.bool.config_intrusiveNotificationLed, false);
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = new NotificationChannel("", "", IMPORTANCE_DEFAULT);
        mController.onResume(appRow, channel, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable_notIfSettingNotAllowed() {
        Settings.Secure.putInt(mContext.getContentResolver(), NOTIFICATION_LIGHT_PULSE, 0);
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = new NotificationChannel("", "", IMPORTANCE_DEFAULT);
        mController.onResume(appRow, channel, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable_notIfNotImportant() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = new NotificationChannel("", "", IMPORTANCE_LOW);
        mController.onResume(appRow, channel, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable_notIfDefaultChannel() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel =
                new NotificationChannel(DEFAULT_CHANNEL_ID, "", IMPORTANCE_DEFAULT);
        mController.onResume(appRow, channel, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = new NotificationChannel("", "", IMPORTANCE_DEFAULT);
        mController.onResume(appRow, channel, null, null);
        assertTrue(mController.isAvailable());
    }

    @Test
    public void testUpdateState_disabledByAdmin() {
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getId()).thenReturn("something");
        mController.onResume(new NotificationBackend.AppRow(), channel, null,
            mock(RestrictedLockUtils.EnforcedAdmin.class));

        Preference pref = new RestrictedSwitchPreference(mContext);
        mController.updateState(pref);

        assertFalse(pref.isEnabled());
    }

    @Test
    public void testUpdateState_notConfigurable() {
        String lockedId = "locked";
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        appRow.lockedChannelId = lockedId;
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getId()).thenReturn(lockedId);
        mController.onResume(appRow, channel, null, null);

        Preference pref = new RestrictedSwitchPreference(mContext);
        mController.updateState(pref);

        assertFalse(pref.isEnabled());
    }

    @Test
    public void testUpdateState_configurable() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getId()).thenReturn("something");
        mController.onResume(appRow, channel, null, null);

        Preference pref = new RestrictedSwitchPreference(mContext);
        mController.updateState(pref);

        assertTrue(pref.isEnabled());
    }

    @Test
    public void testUpdateState_lightsOn() {
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.shouldShowLights()).thenReturn(true);
        mController.onResume(new NotificationBackend.AppRow(), channel, null, null);

        RestrictedSwitchPreference pref = new RestrictedSwitchPreference(mContext);
        mController.updateState(pref);
        assertTrue(pref.isChecked());
    }

    @Test
    public void testUpdateState_lightsOff() {
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.shouldShowLights()).thenReturn(false);
        mController.onResume(new NotificationBackend.AppRow(), channel, null, null);

        RestrictedSwitchPreference pref = new RestrictedSwitchPreference(mContext);
        mController.updateState(pref);
        assertFalse(pref.isChecked());
    }

    @Test
    public void testOnPreferenceChange_on() {
        NotificationChannel channel =
                new NotificationChannel(DEFAULT_CHANNEL_ID, "a", IMPORTANCE_DEFAULT);
        mController.onResume(new NotificationBackend.AppRow(), channel, null, null);

        RestrictedSwitchPreference pref = new RestrictedSwitchPreference(mContext);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(pref);
        mController.displayPreference(mScreen);
        mController.updateState(pref);

        mController.onPreferenceChange(pref, true);

        assertTrue(channel.shouldShowLights());
        verify(mBackend, times(1)).updateChannel(any(), anyInt(), any());
    }

    @Test
    public void testOnPreferenceChange_off() {
        NotificationChannel channel =
                new NotificationChannel(DEFAULT_CHANNEL_ID, "a", IMPORTANCE_HIGH);
        mController.onResume(new NotificationBackend.AppRow(), channel, null, null);

        RestrictedSwitchPreference pref =
                new RestrictedSwitchPreference(mContext);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(pref);
        mController.displayPreference(mScreen);
        mController.updateState(pref);

        mController.onPreferenceChange(pref, false);

        assertFalse(channel.shouldShowLights());
        verify(mBackend, times(1)).updateChannel(any(), anyInt(), any());
    }
}
