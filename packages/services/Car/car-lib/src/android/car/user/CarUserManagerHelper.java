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
package android.car.user;

import android.annotation.Nullable;
import android.app.ActivityManager;
import android.car.settings.CarSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.UserInfo;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.Log;

import com.android.internal.util.UserIcons;

import java.util.Iterator;
import java.util.List;

/**
 * Helper class for {@link UserManager}, this is meant to be used by builds that support
 * Multi-user model with headless user 0. User 0 is not associated with a real person, and
 * can not be brought to foreground.
 *
 * <p>This class provides method for user management, including creating, removing, adding
 * and switching users. Methods related to get users will exclude system user by default.
 *
 * @hide
 */
public class CarUserManagerHelper {
    private static final String TAG = "CarUserManagerHelper";
    private static final String HEADLESS_SYSTEM_USER = "android.car.systemuser.headless";
    private final Context mContext;
    private final UserManager mUserManager;
    private final ActivityManager mActivityManager;
    private Bitmap mDefaultGuestUserIcon;
    private OnUsersUpdateListener mUpdateListener;
    private final BroadcastReceiver mUserChangeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mUpdateListener.onUsersUpdate();
        }
    };

    public CarUserManagerHelper(Context context) {
        mContext = context.getApplicationContext();
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mActivityManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
    }

    /**
     * Registers a listener for updates to all users - removing, adding users or changing user info.
     *
     * <p> Best practise is to keep one listener per helper.
     *
     * @param listener Instance of {@link OnUsersUpdateListener}.
     */
    public void registerOnUsersUpdateListener(OnUsersUpdateListener listener) {
        if (mUpdateListener != null) {
            unregisterOnUsersUpdateListener();
        }

        mUpdateListener = listener;
        registerReceiver();
    }

    /**
     * Unregisters on user update listener by unregistering {@code BroadcastReceiver}.
     */
    public void unregisterOnUsersUpdateListener() {
        unregisterReceiver();
    }

    /**
     * Returns {@code true} if the system is in the headless user 0 model.
     *
     * @return {@boolean true} if headless system user.
     */
    public boolean isHeadlessSystemUser() {
        return SystemProperties.getBoolean(HEADLESS_SYSTEM_USER, false);
    }

    /**
     * Gets UserInfo for the system user.
     *
     * @return {@link UserInfo} for the system user.
     */
    public UserInfo getSystemUserInfo() {
        return mUserManager.getUserInfo(UserHandle.USER_SYSTEM);
    }

    /**
     * Gets UserInfo for the current foreground user.
     *
     * Concept of foreground user is relevant for the multi-user deployment. Foreground user
     * corresponds to the currently "logged in" user.
     *
     * @return {@link UserInfo} for the foreground user.
     */
    public UserInfo getCurrentForegroundUserInfo() {
        return mUserManager.getUserInfo(getCurrentForegroundUserId());
    }

    /**
     * @return Id of the current foreground user.
     */
    public int getCurrentForegroundUserId() {
        return mActivityManager.getCurrentUser();
    }

    /**
     * Gets UserInfo for the user running the caller process.
     *
     * <p>Differentiation between foreground user and current process user is relevant for
     * multi-user deployments.
     *
     * <p>Some multi-user aware components (like SystemUI) needs to run a singleton component
     * in system user. Current process user is always the same for that component, even when
     * the foreground user changes.
     *
     * @return {@link UserInfo} for the user running the current process.
     */
    public UserInfo getCurrentProcessUserInfo() {
        return mUserManager.getUserInfo(getCurrentProcessUserId());
    }

    /**
     * @return Id for the user running the current process.
     */
    public int getCurrentProcessUserId() {
        return UserHandle.myUserId();
    }

    /**
     * Gets all the existing foreground users on the system that are not currently running as
     * the foreground user.
     *
     * @return List of {@code UserInfo} for each user that is not the foreground user.
     */
    public List<UserInfo> getAllSwitchableUsers() {
        if (isHeadlessSystemUser()) {
            return getAllUsersExceptSystemUserAndSpecifiedUser(getCurrentForegroundUserId());
        } else {
            return getAllUsersExceptSpecifiedUser(getCurrentForegroundUserId());
        }
    }

    /**
     * Gets all the users that can be brought to the foreground on the system.
     *
     * @return List of {@code UserInfo} for users that associated with a real person.
     */
    public List<UserInfo> getAllUsers() {
        if (isHeadlessSystemUser()) {
            return getAllUsersExceptSystemUserAndSpecifiedUser(UserHandle.USER_SYSTEM);
        } else {
            return mUserManager.getUsers(/* excludeDying= */true);
        }
    }

    /**
     * Get all the users except the one with userId passed in.
     *
     * @param userId of the user not to be returned.
     * @return All users other than user with userId.
     */
    private List<UserInfo> getAllUsersExceptSpecifiedUser(int userId) {
        List<UserInfo> users = mUserManager.getUsers(/* excludeDying= */true);

        for (Iterator<UserInfo> iterator = users.iterator(); iterator.hasNext(); ) {
            UserInfo userInfo = iterator.next();
            if (userInfo.id == userId) {
                // Remove user with userId from the list.
                iterator.remove();
            }
        }
        return users;
    }

    /**
     * Get all the users except system user and the one with userId passed in.
     *
     * @param userId of the user not to be returned.
     * @return All users other than system user and user with userId.
     */
    private List<UserInfo> getAllUsersExceptSystemUserAndSpecifiedUser(int userId) {
        List<UserInfo> users = mUserManager.getUsers(/* excludeDying= */true);

        for (Iterator<UserInfo> iterator = users.iterator(); iterator.hasNext(); ) {
            UserInfo userInfo = iterator.next();
            if (userInfo.id == userId || userInfo.id == UserHandle.USER_SYSTEM) {
                // Remove user with userId from the list.
                iterator.remove();
            }
        }
        return users;
    }

    // User information accessors

    /**
     * Checks whether the user is system user.
     *
     * @param userInfo User to check against system user.
     * @return {@code true} if system user, {@code false} otherwise.
     */
    public boolean isSystemUser(UserInfo userInfo) {
        return userInfo.id == UserHandle.USER_SYSTEM;
    }

    /**
     * Checks whether the user is default user.
     *
     * @param userInfo User to check against system user.
     * @return {@code true} if is default user, {@code false} otherwise.
     */
    public boolean isDefaultUser(UserInfo userInfo) {
        return userInfo.id == CarSettings.DEFAULT_USER_ID_TO_BOOT_INTO;
    }

    /**
     * Checks whether passed in user is the foreground user.
     *
     * @param userInfo User to check.
     * @return {@code true} if foreground user, {@code false} otherwise.
     */
    public boolean isForegroundUser(UserInfo userInfo) {
        return getCurrentForegroundUserId() == userInfo.id;
    }

    /**
     * Checks whether passed in user is the user that's running the current process.
     *
     * @param userInfo User to check.
     * @return {@code true} if user running the process, {@code false} otherwise.
     */
    public boolean isCurrentProcessUser(UserInfo userInfo) {
        return getCurrentProcessUserId() == userInfo.id;
    }

    // Foreground user information accessors.

    /**
     * Checks if the foreground user is a guest user.
     */
    public boolean isForegroundUserGuest() {
        return getCurrentForegroundUserInfo().isGuest();
    }

    /**
     * Returns whether this user can be removed from the system.
     *
     * @param userInfo User to be removed
     * @return {@code true} if they can be removed, {@code false} otherwise.
     */
    public boolean canUserBeRemoved(UserInfo userInfo) {
        return !isSystemUser(userInfo);
    }

    /**
     * Return whether the foreground user has a restriction.
     *
     * @param restriction Restriction to check. Should be a UserManager.* restriction.
     * @return Whether that restriction exists for the foreground user.
     */
    public boolean foregroundUserHasUserRestriction(String restriction) {
        return mUserManager.hasUserRestriction(
            restriction, getCurrentForegroundUserInfo().getUserHandle());
    }

    /**
     * Checks if the foreground user can add new users.
     */
    public boolean canForegroundUserAddUsers() {
        return !foregroundUserHasUserRestriction(UserManager.DISALLOW_ADD_USER);
    }

    // Current process user information accessors

    /**
     * Checks whether this process is running under the system user.
     */
    public boolean isCurrentProcessSystemUser() {
        return mUserManager.isSystemUser();
    }

    /**
     * Checks if the calling app is running in a demo user.
     */
    public boolean isCurrentProcessDemoUser() {
        return mUserManager.isDemoUser();
    }

    /**
     * Checks if the calling app is running as a guest user.
     */
    public boolean isCurrentProcessGuestUser() {
        return mUserManager.isGuestUser();
    }

    /**
     * Check is the calling app is running as a restricted profile user (ie. a LinkedUser).
     * Restricted profiles are only available when {@link #isHeadlessSystemUser()} is false.
     */
    public boolean isCurrentProcessRestrictedProfileUser() {
        return mUserManager.isRestrictedProfile();
    }

    // Current process user restriction accessors

    /**
     * Return whether the user running the current process has a restriction.
     *
     * @param restriction Restriction to check. Should be a UserManager.* restriction.
     * @return Whether that restriction exists for the user running the process.
     */
    public boolean isCurrentProcessUserHasRestriction(String restriction) {
        return mUserManager.hasUserRestriction(restriction);
    }

    /**
     * Checks if the current process user can modify accounts. Demo and Guest users cannot modify
     * accounts even if the DISALLOW_MODIFY_ACCOUNTS restriction is not applied.
     */
    public boolean canCurrentProcessModifyAccounts() {
        return !isCurrentProcessUserHasRestriction(UserManager.DISALLOW_MODIFY_ACCOUNTS)
            && !isCurrentProcessDemoUser()
            && !isCurrentProcessGuestUser();
    }

    /**
     * Checks if the user running the current process can add new users.
     */
    public boolean canCurrentProcessAddUsers() {
        return !isCurrentProcessUserHasRestriction(UserManager.DISALLOW_ADD_USER);
    }

    /**
     * Checks if the user running the current process can remove users.
     */
    public boolean canCurrentProcessRemoveUsers() {
        return !isCurrentProcessUserHasRestriction(UserManager.DISALLOW_REMOVE_USER);
    }

    /**
     * Checks if the user running the current process is allowed to switch to another user.
     */
    public boolean canCurrentProcessSwitchUsers() {
        return !isCurrentProcessUserHasRestriction(UserManager.DISALLOW_USER_SWITCH);
    }

    /**
     * Creates a new user on the system, the created user would be granted admin role.
     *
     * @param userName Name to give to the newly created user.
     * @return Newly created admin user, null if failed to create a user.
     */
    @Nullable
    public UserInfo createNewAdminUser(String userName) {
        UserInfo user = mUserManager.createUser(userName, UserInfo.FLAG_ADMIN);
        if (user == null) {
            // Couldn't create user, most likely because there are too many, but we haven't
            // been able to reload the list yet.
            Log.w(TAG, "can't create admin user.");
            return null;
        }
        assignDefaultIcon(user);
        return user;
    }

    /**
     * Creates a new restricted user on the system.
     *
     * @param userName Name to give to the newly created user.
     * @return Newly created restricted user, null if failed to create a user.
     */
    @Nullable
    public UserInfo createNewNonAdminUser(String userName) {
        UserInfo user = mUserManager.createUser(userName, 0);
        if (user == null) {
            // Couldn't create user, most likely because there are too many, but we haven't
            // been able to reload the list yet.
            Log.w(TAG, "can't create non-admin user.");
            return null;
        }
        assignDefaultIcon(user);
        return user;
    }

    /**
     * Tries to remove the user that's passed in. System user cannot be removed.
     * If the user to be removed is user currently running the process,
     * it switches to the guest user first, and then removes the user.
     *
     * @param userInfo User to be removed
     * @return {@code true} if user is successfully removed, {@code false} otherwise.
     */
    public boolean removeUser(UserInfo userInfo, String guestUserName) {
        if (isSystemUser(userInfo)) {
            Log.w(TAG, "User " + userInfo.id + " is system user, could not be removed.");
            return false;
        }

        // Not allow to delete the default user for now. Since default user is the one to
        // boot into.
        if (isHeadlessSystemUser() && isDefaultUser(userInfo)) {
            Log.w(TAG, "User " + userInfo.id + " is the default user, could not be removed.");
            return false;
        }

        if (userInfo.id == getCurrentForegroundUserId()) {
            startNewGuestSession(guestUserName);
        }

        return mUserManager.removeUser(userInfo.id);
    }

    /**
     * Switches (logs in) to another user given user id.
     *
     * @param id User id to switch to.
     * @return {@code true} if user switching succeed.
     */
    public boolean switchToUserId(int id) {
        if (id == UserHandle.USER_SYSTEM && isHeadlessSystemUser()) {
            // System User doesn't associate with real person, can not be switched to.
            return false;
        }
        return mActivityManager.switchUser(id);
    }

    /**
     * Switches (logs in) to another user.
     *
     * @param userInfo User to switch to.
     * @return {@code true} if user switching succeed.
     */
    public boolean switchToUser(UserInfo userInfo) {
        if (userInfo.id == getCurrentForegroundUserId()) {
            return false;
        }

        return switchToUserId(userInfo.id);
    }

    /**
     * Creates a new guest session and switches into the guest session.
     *
     * @param guestName Username for the guest user.
     * @return {@code true} if switch to guest user succeed.
     */
    public boolean startNewGuestSession(String guestName) {
        UserInfo guest = mUserManager.createGuest(mContext, guestName);
        if (guest == null) {
            // Couldn't create user, most likely because there are too many, but we haven't
            // been able to reload the list yet.
            Log.w(TAG, "can't create user.");
            return false;
        }
        assignDefaultIcon(guest);
        return switchToUserId(guest.id);
    }

    /**
     * Gets a bitmap representing the user's default avatar.
     *
     * @param userInfo User whose avatar should be returned.
     * @return Default user icon
     */
    public Bitmap getUserDefaultIcon(UserInfo userInfo) {
        return UserIcons.convertToBitmap(
            UserIcons.getDefaultUserIcon(mContext.getResources(), userInfo.id, false));
    }

    /**
     * Gets a bitmap representing the default icon for a Guest user.
     *
     * @return Default guest user icon
     */
    public Bitmap getGuestDefaultIcon() {
        if (mDefaultGuestUserIcon == null) {
            mDefaultGuestUserIcon = UserIcons.convertToBitmap(UserIcons.getDefaultUserIcon(
                mContext.getResources(), UserHandle.USER_NULL, false));
        }
        return mDefaultGuestUserIcon;
    }

    /**
     * Gets an icon for the user.
     *
     * @param userInfo User for which we want to get the icon.
     * @return a Bitmap for the icon
     */
    public Bitmap getUserIcon(UserInfo userInfo) {
        Bitmap picture = mUserManager.getUserIcon(userInfo.id);

        if (picture == null) {
            return assignDefaultIcon(userInfo);
        }

        return picture;
    }

    /**
     * Method for scaling a Bitmap icon to a desirable size.
     *
     * @param icon Bitmap to scale.
     * @param desiredSize Wanted size for the icon.
     * @return Drawable for the icon, scaled to the new size.
     */
    public Drawable scaleUserIcon(Bitmap icon, int desiredSize) {
        Bitmap scaledIcon = Bitmap.createScaledBitmap(
                icon, desiredSize, desiredSize, true /* filter */);
        return new BitmapDrawable(mContext.getResources(), scaledIcon);
    }

    /**
     * Sets new Username for the user.
     *
     * @param user User whose name should be changed.
     * @param name New username.
     */
    public void setUserName(UserInfo user, String name) {
        mUserManager.setUserName(user.id, name);
    }

    private void registerReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_USER_REMOVED);
        filter.addAction(Intent.ACTION_USER_ADDED);
        filter.addAction(Intent.ACTION_USER_INFO_CHANGED);
        filter.addAction(Intent.ACTION_USER_SWITCHED);
        filter.addAction(Intent.ACTION_USER_STOPPED);
        filter.addAction(Intent.ACTION_USER_UNLOCKED);
        mContext.registerReceiverAsUser(mUserChangeReceiver, UserHandle.ALL, filter, null, null);
    }

    // Assigns a default icon to a user according to the user's id.
    private Bitmap assignDefaultIcon(UserInfo userInfo) {
        Bitmap bitmap = userInfo.isGuest()
                ? getGuestDefaultIcon() : getUserDefaultIcon(userInfo);
        mUserManager.setUserIcon(userInfo.id, bitmap);
        return bitmap;
    }

    private void unregisterReceiver() {
        mContext.unregisterReceiver(mUserChangeReceiver);
    }

    /**
     * Interface for listeners that want to register for receiving updates to changes to the users
     * on the system including removing and adding users, and changing user info.
     */
    public interface OnUsersUpdateListener {
        /**
         * Method that will get called when users list has been changed.
         */
        void onUsersUpdate();
    }
}
