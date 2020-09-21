/*
 * Copyright (C) 2018 The Android Open Source Project
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

import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_CALLS;
import static android.app.NotificationManager.Policy.PRIORITY_CATEGORY_MESSAGES;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.app.NotificationManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.shadows.ShadowApplication;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
public class ZenModeStarredContactsPreferenceControllerTest {

    private ZenModeStarredContactsPreferenceController mCallsController;
    private ZenModeStarredContactsPreferenceController mMessagesController;

    @Mock
    private ZenModeBackend mBackend;
    @Mock
    private NotificationManager mNotificationManager;
    @Mock
    private ListPreference mockPref;
    @Mock
    private NotificationManager.Policy mPolicy;
    @Mock
    private PreferenceScreen mPreferenceScreen;
    @Mock
    private Intent testIntent;
    @Mock
    private ComponentName mComponentName;
    private Context mContext;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        ShadowApplication shadowApplication = ShadowApplication.getInstance();
        shadowApplication.setSystemService(Context.NOTIFICATION_SERVICE, mNotificationManager);

        mContext = shadowApplication.getApplicationContext();
        when(mNotificationManager.getNotificationPolicy()).thenReturn(mPolicy);
        when(testIntent.resolveActivity(any())).thenReturn(mComponentName);

        mCallsController = new ZenModeStarredContactsPreferenceController(
                mContext, mock(Lifecycle.class), PRIORITY_CATEGORY_CALLS);
        ReflectionHelpers.setField(mCallsController, "mBackend", mBackend);
        ReflectionHelpers.setField(mCallsController, "mStarredContactsIntent", testIntent);
        when(mPreferenceScreen.findPreference(mCallsController.getPreferenceKey()))
                .thenReturn(mockPref);
        mCallsController.displayPreference(mPreferenceScreen);

        mMessagesController = new ZenModeStarredContactsPreferenceController(
                mContext, mock(Lifecycle.class), PRIORITY_CATEGORY_MESSAGES);
        ReflectionHelpers.setField(mMessagesController, "mBackend", mBackend);
        ReflectionHelpers.setField(mMessagesController, "mStarredContactsIntent", testIntent);
        when(mPreferenceScreen.findPreference(mMessagesController.getPreferenceKey()))
                .thenReturn(mockPref);
        mMessagesController.displayPreference(mPreferenceScreen);
    }

    @Test
    public void isAvailable_noCallers() {
        when(mBackend.isPriorityCategoryEnabled(NotificationManager.Policy.PRIORITY_CATEGORY_CALLS))
                .thenReturn(false);
        assertThat(mCallsController.isAvailable()).isFalse();
    }

    @Test
    public void isAvailable_anyCallers() {
        when(mBackend.isPriorityCategoryEnabled(NotificationManager.Policy.PRIORITY_CATEGORY_CALLS))
                .thenReturn(true);
        when(mBackend.getPriorityCallSenders())
                .thenReturn(NotificationManager.Policy.PRIORITY_SENDERS_ANY);


        assertThat(mCallsController.isAvailable()).isFalse();
    }

    @Test
    public void isAvailable_starredCallers() {
        when(mBackend.isPriorityCategoryEnabled(NotificationManager.Policy.PRIORITY_CATEGORY_CALLS))
                .thenReturn(true);
        when(mBackend.getPriorityCallSenders())
                .thenReturn(NotificationManager.Policy.PRIORITY_SENDERS_STARRED);

        assertThat(mCallsController.isAvailable()).isTrue();
    }

    @Test
    public void isAvailable_noMessages() {
        when(mBackend.isPriorityCategoryEnabled(
                NotificationManager.Policy.PRIORITY_CATEGORY_MESSAGES)).thenReturn(false);
        assertThat(mMessagesController.isAvailable()).isFalse();
    }

    @Test
    public void isAvailable_anyMessages() {
        when(mBackend.isPriorityCategoryEnabled(
                NotificationManager.Policy.PRIORITY_CATEGORY_MESSAGES)).thenReturn(true);
        when(mBackend.getPriorityMessageSenders())
                .thenReturn(NotificationManager.Policy.PRIORITY_SENDERS_ANY);

        assertThat(mMessagesController.isAvailable()).isFalse();
    }

    @Test
    public void isAvailable_starredMessageContacts() {
        when(mBackend.isPriorityCategoryEnabled(
                NotificationManager.Policy.PRIORITY_CATEGORY_MESSAGES)).thenReturn(true);
        when(mBackend.getPriorityMessageSenders())
                .thenReturn(NotificationManager.Policy.PRIORITY_SENDERS_STARRED);

        assertThat(mMessagesController.isAvailable()).isTrue();
    }
}
