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

import static android.app.NotificationManager.IMPORTANCE_NONE;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;
import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.app.NotificationChannel;
import android.app.NotificationChannelGroup;
import android.app.NotificationManager;
import android.content.Context;
import android.os.UserManager;
import android.support.v14.preference.PreferenceFragment;
import android.view.View;

import com.android.settings.applications.LayoutPreference;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.shadows.ShadowApplication;

@RunWith(SettingsRobolectricTestRunner.class)
public class HeaderPreferenceControllerTest {

    private Context mContext;
    @Mock
    private NotificationManager mNm;
    @Mock
    private UserManager mUm;

    private HeaderPreferenceController mController;
    @Mock
    private LayoutPreference mPreference;
    @Mock
    private View mView;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        ShadowApplication shadowApplication = ShadowApplication.getInstance();
        shadowApplication.setSystemService(Context.NOTIFICATION_SERVICE, mNm);
        shadowApplication.setSystemService(Context.USER_SERVICE, mUm);
        mContext = shadowApplication.getApplicationContext();
        PreferenceFragment fragment = mock(PreferenceFragment.class);
        when(fragment.getContext()).thenReturn(mContext);
        Activity activity = mock(Activity.class);
        when(activity.getApplicationContext()).thenReturn(mContext);
        when(fragment.getActivity()).thenReturn(activity);
        mController = spy(new HeaderPreferenceController(mContext, fragment));
        when(mPreference.findViewById(anyInt())).thenReturn(mView);
    }

    @Test
    public void testNoCrashIfNoOnResume() {
        mController.isAvailable();
        mController.updateState(mock(LayoutPreference.class));
    }

    @Test
    public void testIsAvailable_notIfNull() {
        mController.onResume(null, null, null, null);
        assertFalse(mController.isAvailable());
    }

    @Test
    public void testIsAvailable() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        appRow.banned = true;
        mController.onResume(appRow, null, null, null);
        assertTrue(mController.isAvailable());
    }

    @Test
    public void testGetLabel() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        appRow.label = "bananas";
        mController.onResume(appRow, null, null, null);
        assertEquals(appRow.label, mController.getLabel());

        NotificationChannelGroup group = new NotificationChannelGroup("id", "name");
        mController.onResume(appRow, null, group, null);
        assertEquals(group.getName(), mController.getLabel());

        NotificationChannel channel = new NotificationChannel("cid", "cname", IMPORTANCE_NONE);
        mController.onResume(appRow, channel, group, null);
        assertEquals(channel.getName(), mController.getLabel());

        NotificationChannel defaultChannel = new NotificationChannel(
                NotificationChannel.DEFAULT_CHANNEL_ID, "", IMPORTANCE_NONE);
        mController.onResume(appRow, defaultChannel, null, null);
        assertEquals(appRow.label, mController.getLabel());
    }

    @Test
    public void testGetSummary() {
        NotificationBackend.AppRow appRow = new NotificationBackend.AppRow();
        appRow.label = "bananas";
        mController.onResume(appRow, null, null, null);
        assertEquals("", mController.getSummary());

        NotificationChannelGroup group = new NotificationChannelGroup("id", "name");
        mController.onResume(appRow, null, group, null);
        assertEquals(appRow.label, mController.getSummary());

        NotificationChannel channel = new NotificationChannel("cid", "cname", IMPORTANCE_NONE);
        mController.onResume(appRow, channel, group, null);
        assertTrue(mController.getSummary().toString().contains(group.getName()));
        assertTrue(mController.getSummary().toString().contains(appRow.label));

        mController.onResume(appRow, channel, null, null);
        assertFalse(mController.getSummary().toString().contains(group.getName()));
        assertTrue(mController.getSummary().toString().contains(appRow.label));

        NotificationChannel defaultChannel = new NotificationChannel(
                NotificationChannel.DEFAULT_CHANNEL_ID, "", IMPORTANCE_NONE);
        mController.onResume(appRow, defaultChannel, null, null);
        assertEquals("", mController.getSummary());
    }
}
