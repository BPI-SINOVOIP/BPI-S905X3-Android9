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

package com.android.settings.applications.appinfo;

import static android.arch.lifecycle.Lifecycle.Event.ON_START;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.Mockito.when;

import android.app.ActionBar;
import android.app.Activity;
import android.arch.lifecycle.LifecycleOwner;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.graphics.drawable.Drawable;
import android.support.v7.preference.PreferenceScreen;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import com.android.settings.R;
import com.android.settings.applications.LayoutPreference;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.applications.ApplicationsState;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class AppHeaderViewPreferenceControllerTest {

    @Mock
    private AppInfoDashboardFragment mFragment;

    @Mock
    private PreferenceScreen mScreen;
    @Mock
    private LayoutPreference mPreference;

    private Context mContext;
    private Activity mActivity;
    private LifecycleOwner mLifecycleOwner;
    private Lifecycle mLifecycle;
    private View mHeader;
    private AppHeaderViewPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application;
        mActivity = spy(Robolectric.buildActivity(Activity.class).get());
        mLifecycleOwner = () -> mLifecycle;
        mLifecycle = new Lifecycle(mLifecycleOwner);
        mHeader = LayoutInflater.from(mContext).inflate(R.layout.settings_entity_header, null);

        when(mFragment.getActivity()).thenReturn(mActivity);
        when(mScreen.findPreference(anyString())).thenReturn(mPreference);
        when(mPreference.findViewById(R.id.entity_header)).thenReturn(mHeader);

        mController =
            new AppHeaderViewPreferenceController(mContext, mFragment, "Package1", mLifecycle);
    }

    @Test
    public void refreshUi_shouldRefreshButton() {
        final PackageInfo packageInfo = mock(PackageInfo.class);
        final ApplicationsState.AppEntry appEntry = mock(ApplicationsState.AppEntry.class);
        final String appLabel = "App1";
        appEntry.label = appLabel;
        final ApplicationInfo info = new ApplicationInfo();
        info.flags = ApplicationInfo.FLAG_INSTALLED;
        info.enabled = true;
        packageInfo.applicationInfo = info;
        appEntry.info = info;
        when(mFragment.getAppEntry()).thenReturn(appEntry);
        when(mFragment.getPackageInfo()).thenReturn(packageInfo);


        final TextView title = mHeader.findViewById(R.id.entity_header_title);
        final TextView summary = mHeader.findViewById(R.id.entity_header_summary);

        mController.displayPreference(mScreen);
        mController.refreshUi();

        assertThat(title).isNotNull();
        assertThat(title.getText()).isEqualTo(appLabel);
        assertThat(summary).isNotNull();
        assertThat(summary.getText()).isEqualTo(mContext.getString(R.string.installed));
    }

    @Test
    public void onStart_shouldStyleActionBar() {
        final ActionBar actionBar = mock(ActionBar.class);
        when(mActivity.getActionBar()).thenReturn(actionBar);

        mController.displayPreference(mScreen);

        verifyZeroInteractions(actionBar);

        mLifecycle.handleLifecycleEvent(ON_START);

        verify(actionBar).setBackgroundDrawable(any(Drawable.class));
    }
}
