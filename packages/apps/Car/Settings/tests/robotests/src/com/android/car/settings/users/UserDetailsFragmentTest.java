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

package com.android.car.settings.users;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;

import android.car.user.CarUserManagerHelper;
import android.content.pm.UserInfo;
import android.view.View;
import android.widget.Button;

import com.android.car.settings.CarSettingsRobolectricTestRunner;
import com.android.car.settings.R;
import com.android.car.settings.testutils.BaseTestActivity;
import com.android.car.settings.testutils.ShadowUserIconProvider;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

/**
 * Tests for UserDetailsFragment.
 */
@RunWith(CarSettingsRobolectricTestRunner.class)
@Config(shadows = { ShadowUserIconProvider.class })
public class UserDetailsFragmentTest {
    private BaseTestActivity mTestActivity;
    private UserDetailsFragment mUserDetailsFragment;

    @Mock
    private CarUserManagerHelper mCarUserManagerHelper;
    @Mock
    private UserIconProvider mUserIconProvider;

    private Button mRemoveUserButton;
    private Button mSwitchUserButton;

    @Before
    public void setUpTestActivity() {
        MockitoAnnotations.initMocks(this);

        when(mUserIconProvider.getUserIcon(any(), any())).thenReturn(null);

        mTestActivity = Robolectric.buildActivity(BaseTestActivity.class)
                .setup()
                .get();
    }

    /**
     * Tests that if the current user cannot remove other users, the removeUserButton is hidden.
     */
    @Test
    public void test_canCurrentProcessRemoveUsers_isFalse_hidesRemoveUserButton() {
        doReturn(true).when(mCarUserManagerHelper).canCurrentProcessRemoveUsers();
        doReturn(true).when(mCarUserManagerHelper).canUserBeRemoved(any());
        doReturn(false).when(mCarUserManagerHelper).isCurrentProcessDemoUser();
        createUserDetailsFragment();

        assertThat(mRemoveUserButton.getVisibility()).isEqualTo(View.VISIBLE);

        doReturn(false).when(mCarUserManagerHelper).canCurrentProcessRemoveUsers();
        refreshFragment();

        assertThat(mRemoveUserButton.getVisibility()).isEqualTo(View.GONE);
    }

    /**
     * Tests that if the user cannot be removed, the remove button is hidden.
     */
    @Test
    public void test_canUserBeRemoved_isFalse_hidesRemoveUserButton() {
        doReturn(true).when(mCarUserManagerHelper).canCurrentProcessRemoveUsers();
        doReturn(true).when(mCarUserManagerHelper).canUserBeRemoved(any());
        doReturn(false).when(mCarUserManagerHelper).isCurrentProcessDemoUser();
        createUserDetailsFragment();

        assertThat(mRemoveUserButton.getVisibility()).isEqualTo(View.VISIBLE);

        doReturn(false).when(mCarUserManagerHelper).canUserBeRemoved(any());
        refreshFragment();

        assertThat(mRemoveUserButton.getVisibility()).isEqualTo(View.GONE);
    }

    /**
     * Tests that demo user cannot remove other users.
     */
    @Test
    public void test_isCurrentProcessDemoUser_isTrue_hidesRemoveUserButton() {
        doReturn(true).when(mCarUserManagerHelper).canCurrentProcessRemoveUsers();
        doReturn(true).when(mCarUserManagerHelper).canUserBeRemoved(any());
        doReturn(false).when(mCarUserManagerHelper).isCurrentProcessDemoUser();
        createUserDetailsFragment();

        assertThat(mRemoveUserButton.getVisibility()).isEqualTo(View.VISIBLE);

        doReturn(true).when(mCarUserManagerHelper).isCurrentProcessDemoUser();
        refreshFragment();

        assertThat(mRemoveUserButton.getVisibility()).isEqualTo(View.GONE);
    }

    /**
     * Tests that if the current user cannot switch to other users, the switchUserButton is hidden.
     */
    @Test
    public void test_canCurrentProcessSwitchUsers_isFalse_hidesSwitchUserButton() {
        doReturn(true).when(mCarUserManagerHelper).canCurrentProcessSwitchUsers();
        doReturn(false).when(mCarUserManagerHelper).isForegroundUser(any());
        createUserDetailsFragment();

        assertThat(mSwitchUserButton.getVisibility()).isEqualTo(View.VISIBLE);

        doReturn(false).when(mCarUserManagerHelper).canCurrentProcessSwitchUsers();
        refreshFragment();

        assertThat(mSwitchUserButton.getVisibility()).isEqualTo(View.GONE);
    }

    /**
     * Tests that if UserDetailsFragment is displaying foreground user already, switch button is
     * hidden.
     */
    @Test
    public void test_isForegroundUser_isTrue_hidesSwitchUserButton() {
        doReturn(true).when(mCarUserManagerHelper).canCurrentProcessSwitchUsers();
        doReturn(false).when(mCarUserManagerHelper).isForegroundUser(any());
        createUserDetailsFragment();

        assertThat(mSwitchUserButton.getVisibility()).isEqualTo(View.VISIBLE);

        doReturn(false).when(mCarUserManagerHelper).canCurrentProcessSwitchUsers();
        refreshFragment();

        assertThat(mSwitchUserButton.getVisibility()).isEqualTo(View.GONE);
    }

    private void createUserDetailsFragment() {
        UserInfo testUser = new UserInfo();

        mUserDetailsFragment = UserDetailsFragment.newInstance(testUser);
        mUserDetailsFragment.mCarUserManagerHelper = mCarUserManagerHelper;
        mTestActivity.launchFragment(mUserDetailsFragment);

        refreshButtons();
    }

    private void refreshFragment() {
        mTestActivity.reattachFragment(mUserDetailsFragment);

        refreshButtons();
    }

    private void refreshButtons() {
        // Get buttons;
        mRemoveUserButton = (Button) mTestActivity.findViewById(R.id.action_button1);
        mSwitchUserButton = (Button) mTestActivity.findViewById(R.id.action_button2);
    }
}
