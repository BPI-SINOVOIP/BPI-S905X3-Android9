/*
 * Copyright (C) 2016 The Android Open Source Project
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

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.NotificationManager;
import android.app.NotificationManager.Policy;
import android.content.Context;
import android.support.v7.preference.Preference;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.shadows.ShadowApplication;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
public class ZenModePreferenceControllerTest {

    @Mock
    private Preference mPreference;
    @Mock
    private NotificationManager mNotificationManager;
    @Mock
    private Policy mPolicy;

    private Context mContext;
    private ZenModePreferenceController mController;
    private ZenModeSettings.SummaryBuilder mSummaryBuilder;
    private static final String KEY_ZEN_MODE = "zen_mode";

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        ShadowApplication shadowApplication = ShadowApplication.getInstance();
        shadowApplication.setSystemService(Context.NOTIFICATION_SERVICE, mNotificationManager);
        mContext = shadowApplication.getApplicationContext();
        mController = new ZenModePreferenceController(mContext, null, KEY_ZEN_MODE);
        when(mNotificationManager.getNotificationPolicy()).thenReturn(mPolicy);
        mSummaryBuilder = spy(new ZenModeSettings.SummaryBuilder(mContext));
        ReflectionHelpers.setField(mController, "mSummaryBuilder", mSummaryBuilder);
        doReturn(0).when(mSummaryBuilder).getEnabledAutomaticRulesCount();
    }

    @Test
    public void isAlwaysAvailable() {
        assertThat(mController.isAvailable()).isTrue();
    }

    @Test
    public void updateState_automaticRuleEnabled_shouldSetSummary() {
        when(mPreference.isEnabled()).thenReturn(true);

        mController.updateState(mPreference);
        verify(mPreference).setSummary(mContext.getString(R.string.zen_mode_sound_summary_off));

        doReturn(1).when(mSummaryBuilder).getEnabledAutomaticRulesCount();
        mController.updateState(mPreference);
        verify(mPreference).setSummary(mSummaryBuilder.getSoundSummary());
    }

    @Test
    public void updateState_preferenceDisabled_shouldNotSetSummary() {
        when(mPreference.isEnabled()).thenReturn(false);

        mController.updateState(mPreference);

        verify(mPreference, never()).setSummary(anyString());
    }
}
