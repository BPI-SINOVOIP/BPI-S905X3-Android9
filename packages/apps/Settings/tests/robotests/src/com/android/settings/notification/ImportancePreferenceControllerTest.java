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
import static android.app.NotificationManager.IMPORTANCE_HIGH;
import static android.app.NotificationManager.IMPORTANCE_LOW;
import static android.app.NotificationManager.IMPORTANCE_NONE;
import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertNull;
import static junit.framework.Assert.assertTrue;
import static org.junit.Assert.assertNotNull;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.UserManager;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;

import com.android.settings.RestrictedListPreference;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.RestrictedLockUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.shadows.ShadowApplication;

@RunWith(SettingsRobolectricTestRunner.class)
public class ImportancePreferenceControllerTest {

    private Context mContext;
    @Mock
    private NotificationManager mNm;
    @Mock
    private NotificationBackend mBackend;
    @Mock
    private NotificationSettingsBase.ImportanceListener mImportanceListener;
    @Mock
    private UserManager mUm;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private PreferenceScreen mScreen;

    private ImportancePreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        ShadowApplication shadowApplication = ShadowApplication.getInstance();
        shadowApplication.setSystemService(Context.NOTIFICATION_SERVICE, mNm);
        shadowApplication.setSystemService(Context.USER_SERVICE, mUm);
        mContext = RuntimeEnvironment.application;
        mController = spy(new ImportancePreferenceController(
                mContext, mImportanceListener, mBackend));
    }

    @Test
    public void testNoCrashIfNoOnResume() {
        mController.isAvailable();
        mController.updateState(mock(Preference.class));
    }

    @Test
    public void testIsAvailable_notIfNull() {
        mController.onResume(null, null, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable_notIfAppBlocked() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        appRow.banned = true;
        mController.onResume(appRow, mock(NotificationChannel.class), null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable_notIfChannelBlocked() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getImportance()).thenReturn(IMPORTANCE_NONE);
        mController.onResume(appRow, channel, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable_notForDefaultChannel() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getImportance()).thenReturn(IMPORTANCE_LOW);
        when(channel.getId()).thenReturn(DEFAULT_CHANNEL_ID);
        mController.onResume(appRow, channel, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getImportance()).thenReturn(IMPORTANCE_LOW);
        mController.onResume(appRow, channel, null, null);
        assertTrue(mController.isAvailable());
    }

    @Test
    public void testUpdateState_disabledByAdmin() {
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getImportance()).thenReturn(IMPORTANCE_HIGH);
        mController.onResume(new NotificationBackend.AppRow(), channel, null, mock(
                RestrictedLockUtils.EnforcedAdmin.class));

        Preference pref = new RestrictedListPreference(mContext, null);
        mController.updateState(pref);

        assertFalse(pref.isEnabled());
        assertFalse(TextUtils.isEmpty(pref.getSummary()));
    }

    @Test
    public void testUpdateState_notConfigurable() {
        String lockedId = "locked";
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        appRow.lockedChannelId = lockedId;
        NotificationChannel channel = mock(NotificationChannel.class);
        when(channel.getId()).thenReturn(lockedId);
        when(channel.getImportance()).thenReturn(IMPORTANCE_HIGH);
        mController.onResume(appRow, channel, null, null);

        Preference pref = new RestrictedListPreference(mContext, null);
        mController.updateState(pref);

        assertFalse(pref.isEnabled());
        assertFalse(TextUtils.isEmpty(pref.getSummary()));
    }

    @Test
    public void testUpdateState() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        NotificationChannel channel = new NotificationChannel("", "", IMPORTANCE_HIGH);
        mController.onResume(appRow, channel, null, null);

        Preference pref = new RestrictedListPreference(mContext, null);
        mController.updateState(pref);

        assertTrue(pref.isEnabled());
        assertFalse(TextUtils.isEmpty(pref.getSummary()));
    }
    
    @Test
    public void testImportanceLowToHigh() {
        NotificationChannel channel =
                new NotificationChannel(DEFAULT_CHANNEL_ID, "a", IMPORTANCE_LOW);
        channel.setSound(null, Notification.AUDIO_ATTRIBUTES_DEFAULT);
        mController.onResume(new NotificationBackend.AppRow(), channel, null, null);

        RestrictedListPreference pref = new RestrictedListPreference(mContext, null);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(pref);
        mController.displayPreference(mScreen);
        mController.updateState(pref);

        pref.setValue(String.valueOf(IMPORTANCE_HIGH));
        mController.onPreferenceChange(pref, pref.getValue());

        assertEquals(IMPORTANCE_HIGH, channel.getImportance());
        assertNotNull(channel.getSound());
    }

    @Test
    public void testImportanceHightToLow() {
        NotificationChannel channel =
                new NotificationChannel(DEFAULT_CHANNEL_ID, "a", IMPORTANCE_HIGH);
        channel.setSound(null, Notification.AUDIO_ATTRIBUTES_DEFAULT);
        mController.onResume(new NotificationBackend.AppRow(), channel, null, null);

        RestrictedListPreference pref = new RestrictedListPreference(mContext, null);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(pref);
        mController.displayPreference(mScreen);
        mController.updateState(pref);

        pref.setValue(String.valueOf(IMPORTANCE_LOW));
        mController.onPreferenceChange(pref, pref.getValue());

        assertEquals(IMPORTANCE_LOW, channel.getImportance());
        assertNull(channel.getSound());
    }
}
