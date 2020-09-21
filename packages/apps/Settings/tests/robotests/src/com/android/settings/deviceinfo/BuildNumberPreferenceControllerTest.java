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

package com.android.settings.deviceinfo;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Answers.RETURNS_DEEP_STUBS;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.app.Fragment;
import android.arch.lifecycle.LifecycleOwner;
import android.content.Context;
import android.os.Build;
import android.os.Process;
import android.os.UserManager;
import android.provider.Settings;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.text.BidiFormatter;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.search.DatabaseIndexingManager;
import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowUtils;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.development.DevelopmentSettingsEnabler;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowUserManager;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = ShadowUtils.class)
public class BuildNumberPreferenceControllerTest {

    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private Activity mActivity;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private Fragment mFragment;
    @Mock(answer = RETURNS_DEEP_STUBS)
    private PreferenceScreen mScreen;

    private ShadowUserManager mShadowUserManager;

    private Context mContext;
    private LifecycleOwner mLifecycleOwner;
    private Lifecycle mLifecycle;
    private FakeFeatureFactory mFactory;
    private Preference mPreference;
    private BuildNumberPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application;
        final UserManager userManager =
                (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mShadowUserManager = Shadows.shadowOf(userManager);
        mShadowUserManager.setIsAdminUser(true);
        mFactory = FakeFeatureFactory.setupForTest();
        mLifecycleOwner = () -> mLifecycle;
        mLifecycle = new Lifecycle(mLifecycleOwner);
        mController =
                new BuildNumberPreferenceController(mContext, mActivity, mFragment, mLifecycle);

        mPreference = new Preference(mContext);
        mPreference.setKey(mController.getPreferenceKey());
        DevelopmentSettingsEnabler.setDevelopmentSettingsEnabled(mContext, false);
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.DEVICE_PROVISIONED, 1);
    }

    @After
    public void tearDown() {
        ShadowUtils.reset();
        mShadowUserManager.setIsAdminUser(false);
        mShadowUserManager.setIsDemoUser(false);
    }

    @Test
    public void displayPref_shouldAlwaysDisplay() {
        mController.displayPreference(mScreen);

        verify(mScreen.findPreference(mController.getPreferenceKey()))
                .setSummary(BidiFormatter.getInstance().unicodeWrap(Build.DISPLAY));
        verify(mScreen, never()).removePreference(any(Preference.class));
    }

    @Test
    public void handlePrefTreeClick_onlyHandleBuildNumberPref() {
        assertThat(mController.handlePreferenceTreeClick(mock(Preference.class))).isFalse();
    }

    @Test
    public void handlePrefTreeClick_notAdminUser_notDemoUser_doNothing() {
        mShadowUserManager.setIsAdminUser(false);
        mShadowUserManager.setIsDemoUser(false);

        assertThat(mController.handlePreferenceTreeClick(mPreference)).isFalse();
    }

    @Test
    public void handlePrefTreeClick_isAdminUser_notDemoUser_handleBuildNumberPref() {
        mShadowUserManager.setIsAdminUser(true);
        mShadowUserManager.setIsDemoUser(false);

        assertThat(mController.handlePreferenceTreeClick(mPreference)).isTrue();
    }

    @Test
    public void handlePrefTreeClick_notAdminUser_isDemoUser_handleBuildNumberPref() {
        mShadowUserManager.setIsAdminUser(false);
        mShadowUserManager.setIsDemoUser(true);

        assertThat(mController.handlePreferenceTreeClick(mPreference)).isTrue();
    }

    @Test
    public void handlePrefTreeClick_deviceNotProvisioned_doNothing() {
        mShadowUserManager.setIsAdminUser(true);
        mShadowUserManager.setIsDemoUser(false);

        Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.DEVICE_PROVISIONED,
                0);

        assertThat(mController.handlePreferenceTreeClick(mPreference)).isFalse();
        verify(mFactory.metricsFeatureProvider).action(
                any(Context.class),
                eq(MetricsProto.MetricsEvent.ACTION_SETTINGS_BUILD_NUMBER_PREF));
    }

    @Test
    public void handlePrefTreeClick_isMonkeyRun_doNothing() {
        ShadowUtils.setIsUserAMonkey(true);
        assertThat(mController.handlePreferenceTreeClick(mPreference)).isFalse();
    }

    @Test
    public void handlePrefTreeClick_userHasRestriction_doNothing() {
        mShadowUserManager.setIsAdminUser(true);
        mShadowUserManager.setIsDemoUser(false);

        mShadowUserManager.setUserRestriction(Process.myUserHandle(),
                UserManager.DISALLOW_DEBUGGING_FEATURES, true);

        assertThat(mController.handlePreferenceTreeClick(mPreference)).isFalse();
        verify(mFactory.metricsFeatureProvider).action(
                any(Context.class),
                eq(MetricsProto.MetricsEvent.ACTION_SETTINGS_BUILD_NUMBER_PREF));
    }

    @Test
    public void onActivityResult_notConfirmPasswordRequest_doNothing() {
        final boolean activityResultHandled = mController.onActivityResult(
                BuildNumberPreferenceController.REQUEST_CONFIRM_PASSWORD_FOR_DEV_PREF + 1,
                Activity.RESULT_OK,
                null);

        assertThat(activityResultHandled).isFalse();
        assertThat(DevelopmentSettingsEnabler.isDevelopmentSettingsEnabled(mContext)).isFalse();
    }

    @Test
    public void onActivityResult_confirmPasswordRequestFailed_doNotEnableDevPref() {
        final boolean activityResultHandled = mController.onActivityResult(
                BuildNumberPreferenceController.REQUEST_CONFIRM_PASSWORD_FOR_DEV_PREF,
                Activity.RESULT_CANCELED,
                null);

        assertThat(activityResultHandled).isTrue();
        assertThat(DevelopmentSettingsEnabler.isDevelopmentSettingsEnabled(mContext)).isFalse();
    }

    @Test
    public void onActivityResult_confirmPasswordRequestCompleted_enableDevPref() {
        when(mFactory.searchFeatureProvider.getIndexingManager(any(Context.class)))
                .thenReturn(mock(DatabaseIndexingManager.class));

        mController =
                new BuildNumberPreferenceController(mContext, mActivity, mFragment, mLifecycle);

        final boolean activityResultHandled = mController.onActivityResult(
                BuildNumberPreferenceController.REQUEST_CONFIRM_PASSWORD_FOR_DEV_PREF,
                Activity.RESULT_OK,
                null);

        assertThat(activityResultHandled).isTrue();
        assertThat(DevelopmentSettingsEnabler.isDevelopmentSettingsEnabled(mContext)).isTrue();
    }
}
