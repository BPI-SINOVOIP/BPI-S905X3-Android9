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

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.NotificationManager;
import android.content.Context;
import android.support.v7.preference.Preference;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.shadows.ShadowApplication;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
public final class ZenModeBehaviorSoundPreferenceControllerTest {

    private ZenModeBehaviorSoundPreferenceController mController;
    @Mock
    private NotificationManager mNotificationManager;
    @Mock
    private NotificationManager.Policy mPolicy;

    private Context mContext;
    @Mock
    private ZenModeBackend mBackend;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        ShadowApplication shadowApplication = ShadowApplication.getInstance();
        shadowApplication.setSystemService(Context.NOTIFICATION_SERVICE, mNotificationManager);

        mContext = RuntimeEnvironment.application;
        when(mNotificationManager.getNotificationPolicy()).thenReturn(mPolicy);

        mController = new ZenModeBehaviorSoundPreferenceController(
                mContext, mock(Lifecycle.class));
        ReflectionHelpers.setField(mController, "mBackend", mBackend);
    }

    @Test
    public void testIsAvailable() {
        assertTrue(mController.isAvailable());
    }

    @Test
    public void testHasSummary() {
        Preference pref = mock(Preference.class);
        mController.updateState(pref);
        verify(pref).setSummary(any());
    }
}
