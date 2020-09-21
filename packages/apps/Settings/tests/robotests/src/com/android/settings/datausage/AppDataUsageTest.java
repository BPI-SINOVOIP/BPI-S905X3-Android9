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

package com.android.settings.datausage;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyBoolean;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.RETURNS_DEEP_STUBS;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.pm.PackageManager;
import android.net.NetworkPolicyManager;
import android.os.Bundle;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;
import android.util.ArraySet;
import android.view.View;

import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowEntityHeaderController;
import com.android.settings.testutils.shadow.ShadowRestrictedLockUtils;
import com.android.settings.widget.EntityHeaderController;
import com.android.settingslib.AppItem;
import com.android.settingslib.RestrictedLockUtils.EnforcedAdmin;
import com.android.settingslib.RestrictedSwitchPreference;
import com.android.settingslib.wrapper.PackageManagerWrapper;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = {ShadowEntityHeaderController.class, ShadowRestrictedLockUtils.class})
public class AppDataUsageTest {

    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private EntityHeaderController mHeaderController;
    @Mock
    private PackageManagerWrapper mPackageManagerWrapper;

    private AppDataUsage mFragment;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        FakeFeatureFactory.setupForTest();
    }

    @After
    public void tearDown() {
        ShadowEntityHeaderController.reset();
    }

    @Test
    public void bindAppHeader_allWorkApps_shouldNotShowAppInfoLink() {
        ShadowEntityHeaderController.setUseMock(mHeaderController);
        when(mHeaderController.setRecyclerView(any(), any())).thenReturn(mHeaderController);
        when(mHeaderController.setUid(anyInt())).thenReturn(mHeaderController);

        mFragment = spy(new AppDataUsage());

        when(mFragment.getPreferenceManager())
            .thenReturn(mock(PreferenceManager.class, RETURNS_DEEP_STUBS));
        doReturn(mock(PreferenceScreen.class)).when(mFragment).getPreferenceScreen();
        ReflectionHelpers.setField(mFragment, "mAppItem", mock(AppItem.class));

        mFragment.onViewCreated(new View(RuntimeEnvironment.application), new Bundle());

        verify(mHeaderController).setHasAppInfoLink(false);
    }

    @Test
    public void bindAppHeader_workApp_shouldSetWorkAppUid() throws
            PackageManager.NameNotFoundException {
        final int fakeUserId = 100;

        mFragment = spy(new AppDataUsage());
        final ArraySet<String> packages = new ArraySet<>();
        packages.add("pkg");
        final AppItem appItem = new AppItem(123456789);

        ReflectionHelpers.setField(mFragment, "mPackageManagerWrapper", mPackageManagerWrapper);
        ReflectionHelpers.setField(mFragment, "mAppItem", appItem);
        ReflectionHelpers.setField(mFragment, "mPackages", packages);

        when(mPackageManagerWrapper.getPackageUidAsUser(anyString(), anyInt()))
                .thenReturn(fakeUserId);

        ShadowEntityHeaderController.setUseMock(mHeaderController);
        when(mHeaderController.setRecyclerView(any(), any())).thenReturn(mHeaderController);
        when(mHeaderController.setUid(fakeUserId)).thenReturn(mHeaderController);
        when(mHeaderController.setHasAppInfoLink(anyBoolean())).thenReturn(mHeaderController);

        when(mFragment.getPreferenceManager())
            .thenReturn(mock(PreferenceManager.class, RETURNS_DEEP_STUBS));
        doReturn(mock(PreferenceScreen.class)).when(mFragment).getPreferenceScreen();

        mFragment.onViewCreated(new View(RuntimeEnvironment.application), new Bundle());

        verify(mHeaderController).setHasAppInfoLink(true);
        verify(mHeaderController).setUid(fakeUserId);
    }

    @Test
    public void changePreference_backgroundData_shouldUpdateUI() {
        mFragment = spy(new AppDataUsage());
        final AppItem appItem = new AppItem(123456789);
        final RestrictedSwitchPreference pref = mock(RestrictedSwitchPreference.class);
        final DataSaverBackend dataSaverBackend = mock(DataSaverBackend.class);
        ReflectionHelpers.setField(mFragment, "mAppItem", appItem);
        ReflectionHelpers.setField(mFragment, "mRestrictBackground", pref);
        ReflectionHelpers.setField(mFragment, "mDataSaverBackend", dataSaverBackend);

        doNothing().when(mFragment).updatePrefs();

        mFragment.onPreferenceChange(pref, true /* value */);

        verify(mFragment).updatePrefs();
    }

    @Test
    public void updatePrefs_restrictedByAdmin_shouldDisablePreference() {
        mFragment = spy(new AppDataUsage());
        final int testUid = 123123;
        final AppItem appItem = new AppItem(testUid);
        final RestrictedSwitchPreference restrictBackgroundPref
                = mock(RestrictedSwitchPreference.class);
        final RestrictedSwitchPreference unrestrictedDataPref
                = mock(RestrictedSwitchPreference.class);
        final DataSaverBackend dataSaverBackend = mock(DataSaverBackend.class);
        final NetworkPolicyManager networkPolicyManager = mock(NetworkPolicyManager.class);
        ReflectionHelpers.setField(mFragment, "mAppItem", appItem);
        ReflectionHelpers.setField(mFragment, "mRestrictBackground", restrictBackgroundPref);
        ReflectionHelpers.setField(mFragment, "mUnrestrictedData", unrestrictedDataPref);
        ReflectionHelpers.setField(mFragment, "mDataSaverBackend", dataSaverBackend);
        ReflectionHelpers.setField(mFragment.services, "mPolicyManager", networkPolicyManager);

        ShadowRestrictedLockUtils.setRestricted(true);
        doReturn(NetworkPolicyManager.POLICY_NONE).when(networkPolicyManager)
                .getUidPolicy(testUid);

        mFragment.updatePrefs();

        verify(restrictBackgroundPref).setDisabledByAdmin(any(EnforcedAdmin.class));
        verify(unrestrictedDataPref).setDisabledByAdmin(any(EnforcedAdmin.class));
    }
}
