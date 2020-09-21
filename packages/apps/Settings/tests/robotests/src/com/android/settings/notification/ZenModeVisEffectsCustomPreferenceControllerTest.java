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

import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_AMBIENT;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_BADGE;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_FULL_SCREEN_INTENT;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_LIGHTS;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_NOTIFICATION_LIST;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_PEEK;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_SCREEN_OFF;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_SCREEN_ON;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_STATUS_BAR;

import static com.android.internal.logging.nano.MetricsProto.MetricsEvent.ACTION_ZEN_CUSTOM;
import static com.android.internal.logging.nano.MetricsProto.MetricsEvent
        .ACTION_ZEN_SOUND_AND_VIS_EFFECTS;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.NotificationManager;
import android.content.Context;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.testutils.FakeFeatureFactory;
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
public class ZenModeVisEffectsCustomPreferenceControllerTest {
    private ZenModeVisEffectsCustomPreferenceController mController;

    @Mock
    private ZenModeBackend mBackend;
    @Mock
    private ZenCustomRadioButtonPreference mockPref;
    private Context mContext;
    private FakeFeatureFactory mFeatureFactory;
    @Mock
    private PreferenceScreen mScreen;
    @Mock NotificationManager mNotificationManager;

    private static final String PREF_KEY = "main_pref";

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        ShadowApplication shadowApplication = ShadowApplication.getInstance();
        mContext = shadowApplication.getApplicationContext();
        mFeatureFactory = FakeFeatureFactory.setupForTest();
        shadowApplication.setSystemService(Context.NOTIFICATION_SERVICE, mNotificationManager);
        when(mNotificationManager.getNotificationPolicy()).thenReturn(
                mock(NotificationManager.Policy.class));
        mController = new ZenModeVisEffectsCustomPreferenceController(
                mContext, mock(Lifecycle.class), PREF_KEY);
        ReflectionHelpers.setField(mController, "mBackend", mBackend);

        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(mockPref);
        mController.displayPreference(mScreen);
    }

    @Test
    public void isAvailable_noVisEffects() {
        mBackend.mPolicy = new NotificationManager.Policy(0, 0, 0, 0);
        assertThat(mController.isAvailable()).isTrue();
    }

    @Test
    public void isAvailable_visEffects() {
        mBackend.mPolicy = new NotificationManager.Policy(0, 0, 0, 1);
        assertThat(mController.isAvailable()).isTrue();
    }


    @Test
    public void updateState_notChecked_noVisEffects() {
        mBackend.mPolicy = new NotificationManager.Policy(0, 0, 0, 0);
        mController.updateState(mockPref);

        verify(mockPref).setChecked(false);
    }

    @Test
    public void updateState_notChecked_allVisEffects() {
        int allSuppressed = SUPPRESSED_EFFECT_SCREEN_OFF
                | SUPPRESSED_EFFECT_SCREEN_ON
                | SUPPRESSED_EFFECT_FULL_SCREEN_INTENT
                | SUPPRESSED_EFFECT_AMBIENT
                | SUPPRESSED_EFFECT_STATUS_BAR
                | SUPPRESSED_EFFECT_BADGE
                | SUPPRESSED_EFFECT_LIGHTS
                | SUPPRESSED_EFFECT_PEEK
                | SUPPRESSED_EFFECT_NOTIFICATION_LIST;
        mBackend.mPolicy = new NotificationManager.Policy(0, 0, 0, allSuppressed);
        mController.updateState(mockPref);

        verify(mockPref).setChecked(false);
    }

    @Test
    public void updateState_checked() {
        mBackend.mPolicy = new NotificationManager.Policy(0, 0, 0, 2);
        mController.updateState(mockPref);

        verify(mockPref).setChecked(true);
    }

    @Test
    public void updateState_listeners() {
        mBackend.mPolicy = new NotificationManager.Policy(0, 0, 0, 2);
        mController.updateState(mockPref);

        verify(mockPref).setOnGearClickListener(any());
        verify(mockPref).setOnRadioButtonClickListener(any());
    }
}
