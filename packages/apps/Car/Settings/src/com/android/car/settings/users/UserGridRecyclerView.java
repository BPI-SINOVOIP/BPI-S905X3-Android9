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

import android.car.user.CarUserManagerHelper;
import android.content.Context;
import android.content.pm.UserInfo;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawableFactory;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.car.widget.PagedListView;

import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.car.settings.users.ConfirmCreateNewUserDialog.CancelCreateNewUserListener;
import com.android.car.settings.users.ConfirmCreateNewUserDialog.ConfirmCreateNewUserListener;
import com.android.internal.util.UserIcons;

import java.util.ArrayList;
import java.util.List;

/**
 * Displays a GridLayout with icons for the users in the system to allow switching between users.
 * One of the uses of this is for the lock screen in auto.
 */
public class UserGridRecyclerView extends PagedListView implements
        CarUserManagerHelper.OnUsersUpdateListener {
    private UserAdapter mAdapter;
    private CarUserManagerHelper mCarUserManagerHelper;
    private Context mContext;
    private BaseFragment mBaseFragment;
    public AddNewUserTask mAddNewUserTask;
    public boolean mEnableAddUserButton;

    public UserGridRecyclerView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mCarUserManagerHelper = new CarUserManagerHelper(mContext);
        mEnableAddUserButton = true;
    }

    /**
     * Register listener for any update to the users
     */
    @Override
    public void onFinishInflate() {
        super.onFinishInflate();
        mCarUserManagerHelper.registerOnUsersUpdateListener(this);
    }

    /**
     * Unregisters listener checking for any change to the users
     */
    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mCarUserManagerHelper.unregisterOnUsersUpdateListener();
        if (mAddNewUserTask != null) {
            mAddNewUserTask.cancel(/* mayInterruptIfRunning= */ false);
        }
    }

    /**
     * Initializes the adapter that populates the grid layout
     */
    public void buildAdapter() {
        List<UserRecord> userRecords = createUserRecords(mCarUserManagerHelper
                .getAllUsers());
        mAdapter = new UserAdapter(mContext, userRecords);
        super.setAdapter(mAdapter);
    }

    private List<UserRecord> createUserRecords(List<UserInfo> userInfoList) {
        List<UserRecord> userRecords = new ArrayList<>();
        for (UserInfo userInfo : userInfoList) {
            if (userInfo.isGuest()) {
                // Don't display guests in the switcher.
                continue;
            }
            boolean isForeground =
                    mCarUserManagerHelper.getCurrentForegroundUserId() == userInfo.id;
            UserRecord record = new UserRecord(userInfo, false /* isStartGuestSession */,
                    false /* isAddUser */, isForeground);
            userRecords.add(record);
        }

        // Add guest user record if the foreground user is not a guest
        if (!mCarUserManagerHelper.isForegroundUserGuest()) {
            userRecords.add(addGuestUserRecord());
        }

        // Add "add user" record if the foreground user can add users
        if (mCarUserManagerHelper.canForegroundUserAddUsers()) {
            userRecords.add(addUserRecord());
        }

        return userRecords;
    }

    /**
     * Show the "Add User" Button
     */
    public void enableAddUser() {
        mEnableAddUserButton = true;
        onUsersUpdate();
    }

    /**
     * Hide the "Add User" Button
     */
    public void disableAddUser() {
        mEnableAddUserButton = false;
        onUsersUpdate();
    }

    /**
     * Create guest user record
     */
    private UserRecord addGuestUserRecord() {
        UserInfo userInfo = new UserInfo();
        userInfo.name = mContext.getString(R.string.user_guest);
        return new UserRecord(userInfo, true /* isStartGuestSession */,
                false /* isAddUser */, false /* isForeground */);
    }

    /**
     * Create add user record
     */
    private UserRecord addUserRecord() {
        UserInfo userInfo = new UserInfo();
        userInfo.name = mContext.getString(R.string.user_add_user_menu);
        return new UserRecord(userInfo, false /* isStartGuestSession */,
                true /* isAddUser */, false /* isForeground */);
    }

    public void setFragment(BaseFragment fragment) {
        mBaseFragment = fragment;
    }

    @Override
    public void onUsersUpdate() {
        // If you can show the add user button, there is no restriction
        mAdapter.setAddUserRestricted(mEnableAddUserButton ? false : true);
        mAdapter.clearUsers();
        mAdapter.updateUsers(createUserRecords(mCarUserManagerHelper
                .getAllUsers()));
        mAdapter.notifyDataSetChanged();
    }

    /**
     * Adapter to populate the grid layout with the available user profiles
     */
    public final class UserAdapter extends RecyclerView.Adapter<UserAdapter.UserAdapterViewHolder>
            implements ConfirmCreateNewUserListener, CancelCreateNewUserListener,
            AddNewUserTask.AddNewUserListener {

        private final Context mContext;
        private List<UserRecord> mUsers;
        private final Resources mRes;
        private final String mGuestName;
        private final String mNewUserName;
        // View that holds the add user button.  Used to enable/disable the view
        private View mAddUserView;
        private float mOpacityDisabled;
        private float mOpacityEnabled;
        private boolean mIsAddUserRestricted;

        public UserAdapter(Context context, List<UserRecord> users) {
            mRes = context.getResources();
            mContext = context;
            updateUsers(users);
            mGuestName = mRes.getString(R.string.user_guest);
            mNewUserName = mRes.getString(R.string.user_new_user_name);
            mOpacityDisabled = mRes.getFloat(R.dimen.opacity_disabled);
            mOpacityEnabled = mRes.getFloat(R.dimen.opacity_enabled);
        }

        /**
         * Removes all the users from the User Grid.
         */
        public void clearUsers() {
            mUsers.clear();
        }

        /**
         * Refreshes the User Grid with the new List of users.
         * @param users
         */
        public void updateUsers(List<UserRecord> users) {
            mUsers = users;
        }

        @Override
        public UserAdapterViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(mContext)
                    .inflate(R.layout.car_user_switcher_pod, parent, false);
            view.setAlpha(mOpacityEnabled);
            view.bringToFront();
            return new UserAdapterViewHolder(view);
        }

        @Override
        public void onBindViewHolder(UserAdapterViewHolder holder, int position) {
            UserRecord userRecord = mUsers.get(position);
            RoundedBitmapDrawable circleIcon = RoundedBitmapDrawableFactory.create(mRes,
                    getUserRecordIcon(userRecord));
            circleIcon.setCircular(true);
            holder.mUserAvatarImageView.setImageDrawable(circleIcon);
            holder.mUserNameTextView.setText(userRecord.mInfo.name);

            // Show the current user frame if current user
            if (userRecord.mIsForeground) {
                holder.mFrame.setBackgroundResource(R.drawable.car_user_avatar_bg_circle);
            } else {
                holder.mFrame.setBackgroundResource(0);
            }

            // If there are restrictions, show a 50% opaque "add user" view
            if (userRecord.mIsAddUser && mIsAddUserRestricted) {
                holder.mView.setAlpha(mOpacityDisabled);
            } else {
                holder.mView.setAlpha(mOpacityEnabled);
            }

            holder.mView.setOnClickListener(v -> {
                if (userRecord == null) {
                    return;
                }

                // If the user selects Guest, start the guest session.
                if (userRecord.mIsStartGuestSession) {
                    mCarUserManagerHelper.startNewGuestSession(mGuestName);
                    return;
                }

                // If the user wants to add a user, show dialog to confirm adding a user
                if (userRecord.mIsAddUser) {
                    if (mIsAddUserRestricted) {
                        mBaseFragment.getFragmentController().showDOBlockingMessage();
                    } else {
                        // Disable button so it cannot be clicked multiple times
                        mAddUserView = holder.mView;
                        mAddUserView.setEnabled(false);

                        ConfirmCreateNewUserDialog dialog =
                                new ConfirmCreateNewUserDialog();
                        dialog.setConfirmCreateNewUserListener(this);
                        dialog.setCancelCreateNewUserListener(this);
                        if (mBaseFragment != null) {
                            dialog.show(mBaseFragment);
                        }
                    }
                    return;
                }
                // If the user doesn't want to be a guest or add a user, switch to the user selected
                mCarUserManagerHelper.switchToUser(userRecord.mInfo);
            });

        }

        /**
         * Specify if adding a user should be restricted.
         *
         * @param isAddUserRestricted should adding a user be restricted
         *
         */
        public void setAddUserRestricted(boolean isAddUserRestricted) {
            mIsAddUserRestricted = isAddUserRestricted;
        }

        private Bitmap getUserRecordIcon(UserRecord userRecord) {
            if (userRecord.mIsStartGuestSession) {
                return mCarUserManagerHelper.getGuestDefaultIcon();
            }

            if (userRecord.mIsAddUser) {
                return UserIcons.convertToBitmap(mContext
                    .getDrawable(R.drawable.car_add_circle_round));
            }

            return mCarUserManagerHelper.getUserIcon(userRecord.mInfo);
        }

        @Override
        public void onCreateNewUserConfirmed() {
            mAddNewUserTask =
                    new AddNewUserTask(mCarUserManagerHelper, /* addNewUserListener= */ this);
            mAddNewUserTask.execute(mNewUserName);
        }

        /**
         * Enable the "add user" button if the user cancels adding an user
         */
        @Override
        public void onCreateNewUserCancelled() {
            if (mAddUserView != null) {
                mAddUserView.setEnabled(true);
            }
        }

        @Override
        public void onUserAdded() {
            if (mAddUserView != null) {
                mAddUserView.setEnabled(true);
            }
        }

        @Override
        public int getItemCount() {
            return mUsers.size();
        }

        /**
         * Layout for each individual pod in the Grid RecyclerView
         */
        public class UserAdapterViewHolder extends RecyclerView.ViewHolder {

            public ImageView mUserAvatarImageView;
            public TextView mUserNameTextView;
            public View mView;
            public FrameLayout mFrame;

            public UserAdapterViewHolder(View view) {
                super(view);
                mView = view;
                mUserAvatarImageView = (ImageView) view.findViewById(R.id.user_avatar);
                mUserNameTextView = (TextView) view.findViewById(R.id.user_name);
                mFrame = (FrameLayout) view.findViewById(R.id.current_user_frame);
            }
        }
    }

    /**
     * Object wrapper class for the userInfo.  Use it to distinguish if a profile is a
     * guest profile, add user profile, or the foreground user.
     */
    public static final class UserRecord {

        public final UserInfo mInfo;
        public final boolean mIsStartGuestSession;
        public final boolean mIsAddUser;
        public final boolean mIsForeground;

        public UserRecord(UserInfo userInfo, boolean isStartGuestSession, boolean isAddUser,
                boolean isForeground) {
            mInfo = userInfo;
            mIsStartGuestSession = isStartGuestSession;
            mIsAddUser = isAddUser;
            mIsForeground = isForeground;
        }
    }
}
