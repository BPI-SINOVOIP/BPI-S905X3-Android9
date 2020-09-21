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

package com.android.car.users;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.when;

import android.app.ActivityManager;
import android.car.user.CarUserManagerHelper;
import android.content.Context;
import android.content.pm.UserInfo;
import android.os.UserManager;

import com.android.car.CarServiceRobolectricTestRunner;
import com.android.car.testutils.shadow.ShadowActivityManager;
import com.android.car.testutils.shadow.ShadowUserHandle;
import com.android.car.testutils.shadow.ShadowUserManager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(CarServiceRobolectricTestRunner.class)
@Config(shadows = { ShadowActivityManager.class,
        ShadowUserHandle.class, ShadowUserManager.class})
public class CarUserManagerHelperRoboTest {
    @Mock
    private Context mContext;

    private CarUserManagerHelper mHelper;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        when(mContext.getSystemService(Context.USER_SERVICE)).thenReturn(
                RuntimeEnvironment.application.getSystemService(UserManager.class));
        when(mContext.getSystemService(Context.ACTIVITY_SERVICE)).thenReturn(
                RuntimeEnvironment.application.getSystemService(ActivityManager.class));
        when(mContext.getApplicationContext()).thenReturn(mContext);
        mHelper = new CarUserManagerHelper(mContext);
    }

    @After
    public void tearDown() {
        ShadowActivityManager.getShadow().reset();
    }

    @Test
    public void testGetForegroundUserId() {
        ShadowActivityManager.setCurrentUser(15);
        assertThat(mHelper.getCurrentForegroundUserId()).isEqualTo(15);
    }

    @Test
    public void testGetForegroundUserInfo() {
        int currentForegroundUserId = 17;
        ShadowActivityManager.setCurrentUser(currentForegroundUserId);

        assertThat(mHelper.getCurrentForegroundUserInfo().id).isEqualTo(currentForegroundUserId);
    }

    @Test
    public void testGetCurrentProcessUserId() {
        int currentProcessUserId = 11;
        ShadowUserManager.getShadow().setCurrentUser(currentProcessUserId);

        assertThat(mHelper.getCurrentProcessUserId()).isEqualTo(currentProcessUserId);
    }

    @Test
    public void testGetCurrentProcessUserInfo() {
        int currentProcessUserId = 12;
        ShadowUserManager.getShadow().setCurrentUser(currentProcessUserId);
        assertThat(mHelper.getCurrentProcessUserInfo().id).isEqualTo(currentProcessUserId);
    }

    @Test
    public void testGetAllUsers() {
        int currentProcessUserId = 12;
        ShadowUserManager userManager = ShadowUserManager.getShadow();
        userManager.setCurrentUser(currentProcessUserId);

        UserInfo currentProcessUser = createUserInfoForId(currentProcessUserId);
        UserInfo systemUser = createUserInfoForId(0);

        UserInfo otherUser1 = createUserInfoForId(13);
        UserInfo otherUser2 = createUserInfoForId(14);

        userManager.addUserInfo(systemUser);
        userManager.addUserInfo(currentProcessUser);
        userManager.addUserInfo(otherUser1);
        userManager.addUserInfo(otherUser2);

        if (mHelper.isHeadlessSystemUser()) {
            // Should return 3 users that don't have system user id.
            assertThat(mHelper.getAllUsers())
                .containsExactly(currentProcessUser, otherUser1, otherUser2);
        } else {
            assertThat(mHelper.getAllUsers())
                .containsExactly(systemUser, currentProcessUser, otherUser1, otherUser2);
        }
    }

    @Test
    public void testGetAllUsersExcludesForegroundUser() {
        ShadowActivityManager.setCurrentUser(11);
        ShadowUserManager userManager = ShadowUserManager.getShadow();

        UserInfo foregroundUser = createUserInfoForId(11);
        UserInfo otherUser1 = createUserInfoForId(12);
        UserInfo otherUser2 = createUserInfoForId(13);
        UserInfo otherUser3 = createUserInfoForId(14);

        userManager.addUserInfo(foregroundUser);
        userManager.addUserInfo(otherUser1);
        userManager.addUserInfo(otherUser2);
        userManager.addUserInfo(otherUser3);

        // Should return 3 users that don't have foregroundUser id.
        assertThat(mHelper.getAllSwitchableUsers()).hasSize(3);
        assertThat(mHelper.getAllSwitchableUsers())
            .containsExactly(otherUser1, otherUser2, otherUser3);
    }

    @Test
    public void testCheckForegroundUser() {
        ShadowActivityManager.setCurrentUser(10);
        assertThat(mHelper.isForegroundUser(createUserInfoForId(10))).isTrue();
        assertThat(mHelper.isForegroundUser(createUserInfoForId(11))).isFalse();

        ShadowActivityManager.setCurrentUser(11);
        assertThat(mHelper.isForegroundUser(createUserInfoForId(11))).isTrue();
    }

    @Test
    public void testIsUserRunningCurrentProcess() {
        ShadowUserManager shadowUserManager = ShadowUserManager.getShadow();
        UserInfo user1 = createUserInfoForId(10);
        UserInfo user2 = createUserInfoForId(11);
        shadowUserManager.addUserInfo(user1);
        shadowUserManager.addUserInfo(user2);
        shadowUserManager.setCurrentUser(10);

        assertThat(mHelper.isCurrentProcessUser(user1)).isTrue();
        assertThat(mHelper.isCurrentProcessUser(user2)).isFalse();

        shadowUserManager.setCurrentUser(11);
        assertThat(mHelper.isCurrentProcessUser(user2)).isTrue();
        assertThat(mHelper.isCurrentProcessUser(user1)).isFalse();
    }

    @Test
    public void testRemoveCurrentProcessUserSwitchesToGuestUser() {
        // Set currentProcess user to be user 10.
        ShadowUserManager shadowUserManager = ShadowUserManager.getShadow();
        UserInfo user1 = createUserInfoForId(10);
        UserInfo user2 = createUserInfoForId(11);
        shadowUserManager.addUserInfo(user1);
        shadowUserManager.addUserInfo(user2);
        shadowUserManager.setCurrentUser(10);

        // Removing a currentProcess user, calls "switch" to guest user
        mHelper.removeUser(user1, "testGuest");
        assertThat(ShadowActivityManager.getShadow().getSwitchUserCalled()).isTrue();
        assertThat(ShadowActivityManager.getShadow().getUserSwitchedTo()).isEqualTo(0);

        assertThat(shadowUserManager.removeUser(10)).isTrue();
    }

    @Test
    public void testSwitchToUser() {
        ShadowActivityManager.setCurrentUser(20);

        // Switching to foreground user doesn't do anything.
        mHelper.switchToUser(createUserInfoForId(20));
        assertThat(ShadowActivityManager.getShadow().getSwitchUserCalled()).isFalse();

        // Switching to non-current, non-guest user, simply calls switchUser.
        UserInfo userToSwitchTo = new UserInfo(22, "Test User", 0);
        mHelper.switchToUser(userToSwitchTo);
        assertThat(ShadowActivityManager.getShadow().getSwitchUserCalled()).isTrue();
        assertThat(ShadowActivityManager.getShadow().getUserSwitchedTo()).isEqualTo(22);
    }

    private UserInfo createUserInfoForId(int id) {
        UserInfo userInfo = new UserInfo();
        userInfo.id = id;
        return userInfo;
    }
}
