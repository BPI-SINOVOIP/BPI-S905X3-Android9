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

package com.android.car.testutils.shadow;

import android.content.Context;
import android.content.pm.UserInfo;
import android.os.UserHandle;
import android.os.UserManager;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.HiddenApi;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadow.api.Shadow;

@Implements(UserManager.class)
public class ShadowUserManager extends org.robolectric.shadows.ShadowUserManager {
    private final Map<Integer, UserInfo> mUserInfos = new HashMap<>();
    private int mCurrentUser = UserHandle.USER_SYSTEM;
    public boolean isSystemUser = true;
    public boolean isPrimaryUser = true;
    public boolean isLinkedUser = false;
    public boolean isDemoUser = false;
    public boolean isManagedProfile = false;
    public boolean isGuestUser = false;
    public boolean canSwitchUser = true;

    {
        addUserInfo(new UserInfo(UserHandle.USER_SYSTEM, "system_user", 0));
    }

    /**
     * Used by BaseActivity when creating intents.
     */
    @Implementation
    @HiddenApi
    public List<UserInfo> getUsers() {
        return new ArrayList<>(mUserInfos.values());
    }

    @Implementation
    public boolean isSystemUser() {
        return isSystemUser;
    }

    @Implementation
    public boolean isPrimaryUser() {
        return isPrimaryUser;
    }

    @Implementation
    public boolean isLinkedUser() {
        return isLinkedUser;
    }

    @Implementation
    public boolean isDemoUser() {
        return isDemoUser;
    }

    @Implementation
    public boolean isGuestUser() {
        return isGuestUser;
    }

    @Implementation
    public boolean isManagedProfile(int userId) {
        return isManagedProfile;
    }

    @Implementation
    public static boolean isSplitSystemUser() {
        return false;
    }

    @Implementation
    public UserInfo getUserInfo(int id) {
        return mUserInfos.get(id);
    }

    @Implementation
    public boolean isUserUnlockingOrUnlocked(int userId) {
        return isUserUnlocked();
    }

    @Implementation
    public boolean canSwitchUsers() {
        return canSwitchUser;
    }

    @Implementation
    public boolean removeUser(int userId) {
        mUserInfos.remove(userId);
        return true;
    }

    @Implementation
    public UserInfo createGuest(Context context, String name) {
        UserInfo guest = new UserInfo(12, name, UserInfo.FLAG_GUEST);

        addUserInfo(guest);
        return guest;
    }

    public void switchUser(int userId) {
        if (!mUserInfos.containsKey(userId)) {
            throw new UnsupportedOperationException("Must add user before switching to it");
        }
        mCurrentUser = userId;
    }

    public int getCurrentUser() {
        return mCurrentUser;
    }

    public void setCurrentUser(int userId) {
        mCurrentUser = userId;
    }

    public void addUserInfo(UserInfo userInfo) {
        mUserInfos.put(userInfo.id, userInfo);
    }

    public static ShadowUserManager getShadow() {
        return (ShadowUserManager) Shadow.extract(
                RuntimeEnvironment.application.getSystemService(UserManager.class));
    }
}
