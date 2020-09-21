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
package com.android.settings.accounts;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Answers.RETURNS_DEEP_STUBS;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.content.pm.UserInfo;
import android.os.UserManager;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.R;
import com.android.settings.search.SearchIndexableRaw;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowAccountManager;
import com.android.settings.testutils.shadow.ShadowContentResolver;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplication;

import java.util.ArrayList;
import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
public class EmergencyInfoPreferenceControllerTest {

    @Mock(answer = RETURNS_DEEP_STUBS)
    private Context mContext;
    @Mock(answer = RETURNS_DEEP_STUBS)
    private PreferenceScreen mScreen;
    @Mock(answer = RETURNS_DEEP_STUBS)
    private UserManager mUserManager;

    private EmergencyInfoPreferenceController mController;
    private Preference mPreference;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mController = new EmergencyInfoPreferenceController(mContext);
        mPreference = new Preference(RuntimeEnvironment.application);
        mPreference.setKey(mController.getPreferenceKey());
        when(mScreen.findPreference(mPreference.getKey())).thenReturn(mPreference);
    }

    @Test
    public void updateRawDataToIndex_prefUnavailable_shouldNotUpdate() {
        final List<SearchIndexableRaw> data = new ArrayList<>();
        when(mContext.getPackageManager().queryIntentActivities(
                any(Intent.class), anyInt()))
                .thenReturn(null);

        mController.updateRawDataToIndex(data);

        assertThat(data).isEmpty();
    }

    @Test
    public void updateRawDataToIndex_prefAvailable_shouldUpdate() {
        final List<SearchIndexableRaw> data = new ArrayList<>();
        final List<ResolveInfo> infos = new ArrayList<>();
        infos.add(new ResolveInfo());
        when(mContext.getPackageManager().queryIntentActivities(
                any(Intent.class), anyInt()))
                .thenReturn(infos);

        mController.updateRawDataToIndex(data);

        assertThat(data).isNotEmpty();
    }

    @Test
    public void displayPref_prefUnAvailable_shouldNotDisplay() {
        when(mContext.getPackageManager().queryIntentActivities(
                any(Intent.class), anyInt()))
                .thenReturn(null);

        mController.displayPreference(mScreen);

        assertThat(mPreference.isVisible()).isFalse();
    }

    @Test
    public void displayPref_prefAvailable_shouldDisplay() {
        final List<ResolveInfo> infos = new ArrayList<>();
        infos.add(new ResolveInfo());
        when(mContext.getPackageManager().queryIntentActivities(
                any(Intent.class), anyInt()))
                .thenReturn(infos);

        mController.displayPreference(mScreen);

        verify(mScreen, never()).removePreference(any(Preference.class));
    }

    @Test
    @Config(shadows = {ShadowAccountManager.class, ShadowContentResolver.class})
    public void updateState_shouldSetSummary() {
        final List<UserInfo> infos = new ArrayList<>();
        infos.add(new UserInfo(1, "user 1", UserInfo.FLAG_MANAGED_PROFILE));
        when((Object) mContext.getSystemService(UserManager.class)).thenReturn(mUserManager);
        when(mUserManager.getProfiles(anyInt())).thenReturn(infos);
        final Preference preference = mock(Preference.class);

        mController.updateState(preference);

        verify(preference).setSummary(
            mContext.getString(R.string.emergency_info_summary, "user 1"));
    }

    @Test
    public void handlePreferenceTreeClick_shouldStartActivity() {
        final ShadowApplication application = ShadowApplication.getInstance();
        final Context context = RuntimeEnvironment.application;
        final Preference preference = new Preference(context);
        preference.setKey("emergency_info");
        mController = new EmergencyInfoPreferenceController(context);

        mController.handlePreferenceTreeClick(preference);

        assertThat(application.getNextStartedActivity().getAction())
                .isEqualTo("android.settings.EDIT_EMERGENCY_INFO");
    }
}
