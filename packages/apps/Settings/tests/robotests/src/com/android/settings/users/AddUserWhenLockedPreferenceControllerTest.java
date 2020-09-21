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
package com.android.settings.users;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Answers.RETURNS_DEEP_STUBS;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.arch.lifecycle.LifecycleOwner;
import android.content.Context;
import android.content.pm.UserInfo;
import android.os.UserManager;
import android.provider.Settings.Global;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.RestrictedSwitchPreference;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.shadows.ShadowApplication;

@RunWith(SettingsRobolectricTestRunner.class)
public class AddUserWhenLockedPreferenceControllerTest {

    @Mock(answer = RETURNS_DEEP_STUBS)
    private PreferenceScreen mScreen;
    @Mock(answer = RETURNS_DEEP_STUBS)
    private UserInfo mUserInfo;
    @Mock(answer = RETURNS_DEEP_STUBS)
    private UserManager mUserManager;

    private LifecycleOwner mLifecycleOwner;
    private Lifecycle mLifecycle;
    private Context mContext;
    private AddUserWhenLockedPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        ShadowApplication shadowContext = ShadowApplication.getInstance();
        shadowContext.setSystemService(Context.USER_SERVICE, mUserManager);
        mContext = shadowContext.getApplicationContext();
        mLifecycleOwner = () -> mLifecycle;
        mLifecycle = new Lifecycle(mLifecycleOwner);
        mController = new AddUserWhenLockedPreferenceController(mContext, "fake_key", mLifecycle);
    }

    @Test
    public void displayPref_NotAdmin_shouldNotDisplay() {
        when(mUserManager.getUserInfo(anyInt())).thenReturn(mUserInfo);
        when(mUserInfo.isAdmin()).thenReturn(false);
        final RestrictedSwitchPreference preference = mock(RestrictedSwitchPreference.class);
        when(preference.getKey()).thenReturn(mController.getPreferenceKey());
        when(mScreen.findPreference(preference.getKey())).thenReturn(preference);

        mController.displayPreference(mScreen);

        verify(preference).setVisible(false);
    }

    @Test
    public void updateState_preferenceSetCheckedWhenSettingIsOn() {
        final RestrictedSwitchPreference preference = mock(RestrictedSwitchPreference.class);
        Global.putInt(mContext.getContentResolver(), Global.ADD_USERS_WHEN_LOCKED, 1);

        mController.updateState(preference);

        verify(preference).setChecked(true);
    }

    @Test
    public void updateState_preferenceSetUncheckedWhenSettingIsOff() {
        final RestrictedSwitchPreference preference = mock(RestrictedSwitchPreference.class);
        Global.putInt(mContext.getContentResolver(), Global.ADD_USERS_WHEN_LOCKED, 0);

        mController.updateState(preference);

        verify(preference).setChecked(false);
    }

    @Test
    public void onPreferenceChange_SettingIsOnWhenPreferenceChecked() {
        final RestrictedSwitchPreference preference = mock(RestrictedSwitchPreference.class);
        preference.setChecked(true);

        mController.onPreferenceChange(preference, Boolean.TRUE);

        assertThat(Global.getInt(mContext.getContentResolver(), Global.ADD_USERS_WHEN_LOCKED, 0))
                .isEqualTo(1);
    }

    @Test
    public void onPreferenceChange_SettingIsOffWhenPreferenceNotChecked() {
        final RestrictedSwitchPreference preference = mock(RestrictedSwitchPreference.class);
        preference.setChecked(false);

        mController.onPreferenceChange(preference, Boolean.FALSE);

        assertThat(Global.getInt(mContext.getContentResolver(), Global.ADD_USERS_WHEN_LOCKED, 0))
                .isEqualTo(0);
    }
}
