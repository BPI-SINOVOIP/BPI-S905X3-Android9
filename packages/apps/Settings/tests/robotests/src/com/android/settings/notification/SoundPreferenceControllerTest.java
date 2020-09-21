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
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;
import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.UserManager;
import android.provider.Settings;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.util.AttributeSet;

import com.android.settings.SettingsPreferenceFragment;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.SettingsShadowResources;
import com.android.settingslib.RestrictedLockUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplication;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = SettingsShadowResources.class)
public class SoundPreferenceControllerTest {

    private Context mContext;
    @Mock
    private NotificationBackend mBackend;
    @Mock
    private NotificationManager mNm;
    @Mock
    private UserManager mUm;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private PreferenceScreen mScreen;
    @Mock
    private SettingsPreferenceFragment mFragment;
    @Mock
    private NotificationSettingsBase.ImportanceListener mImportanceListener;

    private SoundPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        ShadowApplication shadowApplication = ShadowApplication.getInstance();
        shadowApplication.setSystemService(Context.NOTIFICATION_SERVICE, mNm);
        shadowApplication.setSystemService(Context.USER_SERVICE, mUm);
        SettingsShadowResources.overrideResource(com.android.internal.R.string.ringtone_silent,
                "silent");
        mContext = shadowApplication.getApplicationContext();
        mController = spy(new SoundPreferenceController(
                mContext, mFragment, mImportanceListener, mBackend));
    }

    @After
    public void tearDown() {
        SettingsShadowResources.reset();
    }

    @Test
    public void testNoCrashIfNoOnResume() throws Exception {
        mController.isAvailable();
        mController.updateState(mock(NotificationSoundPreference.class));
        mController.onPreferenceChange(mock(NotificationSoundPreference.class), Uri.EMPTY);
        mController.handlePreferenceTreeClick(mock(NotificationSoundPreference.class));
        mController.onActivityResult(1, 1, null);
        SoundPreferenceController.hasValidSound(null);
    }

    @Test
    public void testIsAvailable_notIfChannelNull() throws Exception {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        mController.onResume(appRow, null, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable_notIfNotImportant() throws Exception {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = new NotificationChannel("", "", IMPORTANCE_LOW);
        mController.onResume(appRow, channel, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable_notIfDefaultChannel() throws Exception {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel =
                new NotificationChannel(DEFAULT_CHANNEL_ID, "", IMPORTANCE_DEFAULT);
        mController.onResume(appRow, channel, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable() throws Exception {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = new NotificationChannel("", "", IMPORTANCE_DEFAULT);
        mController.onResume(appRow, channel, null, null);
        assertTrue(mController.isAvailable());
    }

    @Test
    public void testDisplayPreference_savesPreference() throws Exception {
        NotificationSoundPreference pref = mock(NotificationSoundPreference.class);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(pref);
        mController.displayPreference(mScreen);

        mController.onActivityResult(SoundPreferenceController.CODE, 1, new Intent());
        verify(pref, times(1)).onActivityResult(anyInt(), anyInt(), any());
    }

    @Test
    public void testUpdateState_disabledByAdmin() throws Exception {
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getId()).thenReturn("something");
        mController.onResume(new NotificationBackend.AppRow(), channel, null, mock(
                RestrictedLockUtils.EnforcedAdmin.class));

        Preference pref = new NotificationSoundPreference(mContext, mock(AttributeSet.class));
        mController.updateState(pref);

        assertFalse(pref.isEnabled());
    }

    @Test
    public void testUpdateState_notConfigurable() throws Exception {
        String lockedId = "locked";
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        appRow.lockedChannelId = lockedId;
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getId()).thenReturn(lockedId);
        mController.onResume(appRow, channel, null, null);

        Preference pref = new NotificationSoundPreference(mContext, mock(AttributeSet.class));
        mController.updateState(pref);

        assertFalse(pref.isEnabled());
    }

    @Test
    public void testUpdateState_configurable() throws Exception {
        Uri sound = Settings.System.DEFAULT_ALARM_ALERT_URI;
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getId()).thenReturn("something");
        when(channel.getSound()).thenReturn(sound);
        mController.onResume(appRow, channel, null, null);

        NotificationSoundPreference pref =
                new NotificationSoundPreference(mContext, mock(AttributeSet.class));
        mController.updateState(pref);

        assertEquals(sound, pref.onRestoreRingtone());
        assertTrue(pref.isEnabled());
    }

    @Test
    public void testOnPreferenceChange() throws Exception {
        Uri sound = Settings.System.DEFAULT_ALARM_ALERT_URI;
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = new NotificationChannel("", "", IMPORTANCE_HIGH);
        channel.setSound(sound, Notification.AUDIO_ATTRIBUTES_DEFAULT);
        mController.onResume(appRow, channel, null, null);

        NotificationSoundPreference pref =
                new NotificationSoundPreference(mContext, mock(AttributeSet.class));
        mController.updateState(pref);

        mController.onPreferenceChange(pref, Uri.EMPTY);
        assertEquals(Uri.EMPTY, channel.getSound());
        assertEquals(Notification.AUDIO_ATTRIBUTES_DEFAULT, channel.getAudioAttributes());
        verify(mBackend, times(1)).updateChannel(any(), anyInt(), any());
    }

    @Test
    public void testOnPreferenceTreeClick_incorrectPref() throws Exception {
        NotificationSoundPreference pref = mock(NotificationSoundPreference.class);
        mController.handlePreferenceTreeClick(pref);

        verify(pref, never()).onPrepareRingtonePickerIntent(any());
        verify(mFragment, never()).startActivityForResult(any(), anyInt());
    }

    @Test
    public void testOnPreferenceTreeClick_correctPref() throws Exception {
        NotificationSoundPreference pref =
                spy(new NotificationSoundPreference(mContext, mock(AttributeSet.class)));
        pref.setKey(mController.getPreferenceKey());
        mController.handlePreferenceTreeClick(pref);

        verify(pref, times(1)).onPrepareRingtonePickerIntent(any());
        verify(mFragment, times(1)).startActivityForResult(any(), anyInt());
    }

    @Test
    public void testOnActivityResult() {
        NotificationSoundPreference pref = mock(NotificationSoundPreference.class);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(pref);
        mController.displayPreference(mScreen);

        mController.onActivityResult(SoundPreferenceController.CODE, 1, new Intent("hi"));
        verify(pref, times(1)).onActivityResult(anyInt(), anyInt(), any());
        verify(mImportanceListener, times(1)).onImportanceChanged();
    }

    @Test
    public void testHasValidSound() {
        NotificationChannel channel =
                new NotificationChannel(DEFAULT_CHANNEL_ID, "a", IMPORTANCE_HIGH);
        assertTrue(SoundPreferenceController.hasValidSound(channel));

        channel.setSound(Uri.EMPTY, Notification.AUDIO_ATTRIBUTES_DEFAULT);
        assertFalse(SoundPreferenceController.hasValidSound(channel));

        channel.setSound(null, Notification.AUDIO_ATTRIBUTES_DEFAULT);
        assertFalse(SoundPreferenceController.hasValidSound(channel));
    }
}
