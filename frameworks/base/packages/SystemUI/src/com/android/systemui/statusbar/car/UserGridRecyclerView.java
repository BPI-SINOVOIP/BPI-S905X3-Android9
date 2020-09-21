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
 * limitations under the License
 */

package com.android.systemui.statusbar.car;

import static android.content.DialogInterface.BUTTON_NEGATIVE;
import static android.content.DialogInterface.BUTTON_POSITIVE;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.UserInfo;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.AsyncTask;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawableFactory;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.car.widget.PagedListView;

import com.android.internal.util.UserIcons;
import com.android.settingslib.users.UserManagerHelper;
import com.android.systemui.R;

import com.android.systemui.statusbar.phone.SystemUIDialog;
import java.util.ArrayList;
import java.util.List;

/**
 * Displays a GridLayout with icons for the users in the system to allow switching between users.
 * One of the uses of this is for the lock screen in auto.
 */
public class UserGridRecyclerView extends PagedListView implements
        UserManagerHelper.OnUsersUpdateListener {
    private UserSelectionListener mUserSelectionListener;
    private UserAdapter mAdapter;
    private UserManagerHelper mUserManagerHelper;
    private Context mContext;

    public UserGridRecyclerView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mUserManagerHelper = new UserManagerHelper(mContext);
    }

    /**
     * Register listener for any update to the users
     */
    @Override
    public void onFinishInflate() {
        super.onFinishInflate();
        mUserManagerHelper.registerOnUsersUpdateListener(this);
    }

    /**
     * Unregisters listener checking for any change to the users
     */
    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mUserManagerHelper.unregisterOnUsersUpdateListener();
    }

    /**
     * Initializes the adapter that populates the grid layout
     *
     * @return the adapter
     */
    public void buildAdapter() {
        List<UserRecord> userRecords = createUserRecords(mUserManagerHelper
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
                mUserManagerHelper.getForegroundUserId() == userInfo.id;
            UserRecord record = new UserRecord(userInfo, false /* isStartGuestSession */,
                    false /* isAddUser */, isForeground);
            userRecords.add(record);
        }

        // Add guest user record if the foreground user is not a guest
        if (!mUserManagerHelper.foregroundUserIsGuestUser()) {
            userRecords.add(addGuestUserRecord());
        }

        // Add add user record if the foreground user can add users
        if (mUserManagerHelper.foregroundUserCanAddUsers()) {
            userRecords.add(addUserRecord());
        }

        return userRecords;
    }

    /**
     * Create guest user record
     */
    private UserRecord addGuestUserRecord() {
        UserInfo userInfo = new UserInfo();
        userInfo.name = mContext.getString(R.string.car_guest);
        return new UserRecord(userInfo, true /* isStartGuestSession */,
                false /* isAddUser */, false /* isForeground */);
    }

    /**
     * Create add user record
     */
    private UserRecord addUserRecord() {
        UserInfo userInfo = new UserInfo();
        userInfo.name = mContext.getString(R.string.car_add_user);
        return new UserRecord(userInfo, false /* isStartGuestSession */,
                true /* isAddUser */, false /* isForeground */);
    }

    public void setUserSelectionListener(UserSelectionListener userSelectionListener) {
        mUserSelectionListener = userSelectionListener;
    }

    @Override
    public void onUsersUpdate() {
        mAdapter.clearUsers();
        mAdapter.updateUsers(createUserRecords(mUserManagerHelper.getAllUsers()));
        mAdapter.notifyDataSetChanged();
    }

    /**
     * Adapter to populate the grid layout with the available user profiles
     */
    public final class UserAdapter extends RecyclerView.Adapter<UserAdapter.UserAdapterViewHolder>
            implements Dialog.OnClickListener {

        private final Context mContext;
        private List<UserRecord> mUsers;
        private final Resources mRes;
        private final String mGuestName;
        private final String mNewUserName;
        private AlertDialog mDialog;
        // View that holds the add user button.  Used to enable/disable the view
        private View mAddUserView;
        // User record for the add user.  Need to call notifyUserSelected only if the user
        // confirms adding a user
        private UserRecord mAddUserRecord;

        public UserAdapter(Context context, List<UserRecord> users) {
            mRes = context.getResources();
            mContext = context;
            updateUsers(users);
            mGuestName = mRes.getString(R.string.car_guest);
            mNewUserName = mRes.getString(R.string.car_new_user);
        }

        public void clearUsers() {
            mUsers.clear();
        }

        public void updateUsers(List<UserRecord> users) {
            mUsers = users;
        }

        @Override
        public UserAdapterViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(mContext)
                    .inflate(R.layout.car_fullscreen_user_pod, parent, false);
            view.setAlpha(1f);
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

            holder.mView.setOnClickListener(v -> {
                if (userRecord == null) {
                    return;
                }


                // If the user selects Guest, start the guest session.
                if (userRecord.mIsStartGuestSession) {
                    notifyUserSelected(userRecord);
                    mUserManagerHelper.startNewGuestSession(mGuestName);
                    return;
                }

                // If the user wants to add a user, show dialog to confirm adding a user
                if (userRecord.mIsAddUser) {
                    // Disable button so it cannot be clicked multiple times
                    mAddUserView = holder.mView;
                    mAddUserView.setEnabled(false);

                    String message = mRes.getString(R.string.user_add_user_message_setup)
                        .concat(System.getProperty("line.separator"))
                        .concat(System.getProperty("line.separator"))
                        .concat(mRes.getString(R.string.user_add_user_message_update));

                    mAddUserRecord = userRecord;
                    mDialog = new Builder(mContext, R.style.Theme_Car_Dark_Dialog_Alert)
                        .setTitle(R.string.user_add_user_title)
                        .setMessage(message)
                        .setNegativeButton(android.R.string.cancel, this)
                        .setPositiveButton(android.R.string.ok, this)
                        .create();
                    // Sets window flags for the SysUI dialog
                    SystemUIDialog.applyFlags(mDialog);
                    mDialog.show();
                    return;
                }
                // If the user doesn't want to be a guest or add a user, switch to the user selected
                notifyUserSelected(userRecord);
                mUserManagerHelper.switchToUser(userRecord.mInfo);
            });

        }

        private void notifyUserSelected(UserRecord userRecord) {
            // Notify the listener which user was selected
            if (mUserSelectionListener != null) {
                mUserSelectionListener.onUserSelected(userRecord);
            }
        }

        private Bitmap getUserRecordIcon(UserRecord userRecord) {
            if (userRecord.mIsStartGuestSession) {
                return mUserManagerHelper.getGuestDefaultIcon();
            }

            if (userRecord.mIsAddUser) {
                return UserIcons.convertToBitmap(mContext
                    .getDrawable(R.drawable.car_add_circle_round));
            }

            return mUserManagerHelper.getUserIcon(userRecord.mInfo);
        }

        @Override
        public void onClick(DialogInterface dialog, int which) {
            if (which == BUTTON_POSITIVE) {
                notifyUserSelected(mAddUserRecord);
                new AddNewUserTask().execute(mNewUserName);
            } else if (which == BUTTON_NEGATIVE) {
                // Enable the add button only if cancel
                if (mAddUserView != null) {
                    mAddUserView.setEnabled(true);
                }
            }
        }

        private class AddNewUserTask extends AsyncTask<String, Void, UserInfo> {

            @Override
            protected UserInfo doInBackground(String... userNames) {
                return mUserManagerHelper.createNewUser(userNames[0]);
            }

            @Override
            protected void onPreExecute() {
            }

            @Override
            protected void onPostExecute(UserInfo user) {
                if (user != null) {
                    mUserManagerHelper.switchToUser(user);
                }
            }
        }

        @Override
        public int getItemCount() {
            return mUsers.size();
        }

        public class UserAdapterViewHolder extends RecyclerView.ViewHolder {

            public ImageView mUserAvatarImageView;
            public TextView mUserNameTextView;
            public View mView;

            public UserAdapterViewHolder(View view) {
                super(view);
                mView = view;
                mUserAvatarImageView = (ImageView) view.findViewById(R.id.user_avatar);
                mUserNameTextView = (TextView) view.findViewById(R.id.user_name);
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

    /**
     * Listener used to notify when a user has been selected
     */
    interface UserSelectionListener {

        void onUserSelected(UserRecord record);
    }
}
