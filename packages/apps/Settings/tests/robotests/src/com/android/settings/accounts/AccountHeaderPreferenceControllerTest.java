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

package com.android.settings.accounts;

import static android.arch.lifecycle.Lifecycle.Event.ON_RESUME;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;

import android.accounts.Account;
import android.app.Activity;
import android.arch.lifecycle.LifecycleOwner;
import android.os.Bundle;
import android.os.UserHandle;
import android.support.v14.preference.PreferenceFragment;
import android.support.v7.preference.PreferenceScreen;
import android.widget.TextView;

import com.android.settings.R;
import com.android.settings.applications.LayoutPreference;
import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.accounts.AuthenticatorHelper;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = AccountHeaderPreferenceControllerTest.ShadowAuthenticatorHelper.class)
public class AccountHeaderPreferenceControllerTest {

    @Mock
    private Activity mActivity;
    @Mock
    private PreferenceFragment mFragment;
    @Mock
    private PreferenceScreen mScreen;

    private LayoutPreference mHeaderPreference;

    private AccountHeaderPreferenceController mController;

    private LifecycleOwner mLifecycleOwner;
    private Lifecycle mLifecycle;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        FakeFeatureFactory.setupForTest();
        mHeaderPreference = new LayoutPreference(
                RuntimeEnvironment.application, R.layout.settings_entity_header);
        doReturn(RuntimeEnvironment.application).when(mActivity).getApplicationContext();
        mLifecycleOwner = () -> mLifecycle;
        mLifecycle = new Lifecycle(mLifecycleOwner);
    }

    @Test
    public void isAvailable_noArgs_shouldReturnNull() {
        mController = new AccountHeaderPreferenceController(RuntimeEnvironment.application,
                mLifecycle, mActivity, mFragment, null /* args */);

        assertThat(mController.isAvailable()).isFalse();
    }

    @Test
    public void onResume_shouldDisplayAccountInEntityHeader() {
        final Account account = new Account("name1@abc.com", "com.abc");
        Bundle args = new Bundle();
        args.putParcelable(AccountDetailDashboardFragment.KEY_ACCOUNT, account);
        args.putParcelable(AccountDetailDashboardFragment.KEY_USER_HANDLE, UserHandle.CURRENT);
        mController = new AccountHeaderPreferenceController(RuntimeEnvironment.application,
                mLifecycle, mActivity, mFragment, args);

        assertThat(mController.isAvailable()).isTrue();

        when(mScreen.findPreference(anyString())).thenReturn(mHeaderPreference);

        mController.displayPreference(mScreen);
        mLifecycle.handleLifecycleEvent(ON_RESUME);

        final CharSequence label =
                ((TextView) mHeaderPreference.findViewById(R.id.entity_header_title)).getText();

        assertThat(label).isEqualTo(account.name);
    }

    @Implements(AuthenticatorHelper.class)
    public static class ShadowAuthenticatorHelper {
        @Implementation
        public void onAccountsUpdated(Account[] accounts) {

        }
    }
}
