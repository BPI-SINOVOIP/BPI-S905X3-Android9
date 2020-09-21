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
import android.content.pm.UserInfo;
import android.os.Bundle;
import android.support.annotation.VisibleForTesting;
import android.view.View;
import android.widget.Button;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;

import java.util.Arrays;
import java.util.List;

/**
 * Shows details for a user with the ability to remove user and switch.
 */
public class UserDetailsFragment extends ListItemSettingsFragment implements
        ConfirmRemoveUserDialog.ConfirmRemoveUserListener,
        RemoveUserErrorDialog.RemoveUserErrorListener {
    public static final String EXTRA_USER_INFO = "extra_user_info";
    private static final String ERROR_DIALOG_TAG = "RemoveUserErrorDialogTag";
    private static final String CONFIRM_REMOVE_DIALOG_TAG = "ConfirmRemoveUserDialog";

    private Button mRemoveUserBtn;
    private Button mSwitchUserBtn;

    private UserInfo mUserInfo;
    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    CarUserManagerHelper mCarUserManagerHelper;
    private UserIconProvider mUserIconProvider;
    private ListItemProvider mItemProvider;

    /**
     * Creates instance of UserDetailsFragment.
     */
    public static UserDetailsFragment newInstance(UserInfo userInfo) {
        UserDetailsFragment userDetailsFragment = new UserDetailsFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        bundle.putInt(EXTRA_TITLE_ID, R.string.user_details_title);
        bundle.putParcelable(EXTRA_USER_INFO, userInfo);
        userDetailsFragment.setArguments(bundle);
        return userDetailsFragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mUserInfo = getArguments().getParcelable(EXTRA_USER_INFO);

        if (savedInstanceState != null) {
            RemoveUserErrorDialog removeUserErrorDialog = (RemoveUserErrorDialog)
                    getFragmentManager().findFragmentByTag(ERROR_DIALOG_TAG);
            if (removeUserErrorDialog != null) {
                removeUserErrorDialog.setRetryListener(this);
            }

            ConfirmRemoveUserDialog confirmRemoveUserDialog = (ConfirmRemoveUserDialog)
                    getFragmentManager().findFragmentByTag(CONFIRM_REMOVE_DIALOG_TAG);
            if (confirmRemoveUserDialog != null) {
                confirmRemoveUserDialog.setConfirmRemoveUserListener(this);
            }
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        createUserManagerHelper();
        mUserIconProvider = new UserIconProvider(mCarUserManagerHelper);
        mItemProvider = new ListItemProvider.ListProvider(getListItems());

        // Needs to be called after creation of item provider.
        super.onActivityCreated(savedInstanceState);

        showActionButtons();
    }

    @Override
    public void onRemoveUserConfirmed() {
        removeUser();
    }

    @Override
    public void onRetryRemoveUser() {
        // Retry deleting user.
        removeUser();
    }

    @Override
    public ListItemProvider getItemProvider() {
        return mItemProvider;
    }

    private void removeUser() {
        if (mCarUserManagerHelper.removeUser(
                mUserInfo, getContext().getString(R.string.user_guest))) {
            getActivity().onBackPressed();
        } else {
            // If failed, need to show error dialog for users, can offer retry.
            RemoveUserErrorDialog removeUserErrorDialog = new RemoveUserErrorDialog();
            removeUserErrorDialog.setRetryListener(this);
            removeUserErrorDialog.show(getFragmentManager(), ERROR_DIALOG_TAG);
        }
    }

    private void showActionButtons() {
        if (mCarUserManagerHelper.isForegroundUser(mUserInfo)) {
            // Already foreground user, shouldn't show SWITCH button.
            showRemoveUserButton();
            return;
        }

        showRemoveUserButton();
        showSwitchButton();
    }

    private void createUserManagerHelper() {
        // Null check for testing. Don't want to override it if already set by a test.
        if (mCarUserManagerHelper == null) {
            mCarUserManagerHelper = new CarUserManagerHelper(getContext());
        }
    }

    private void showRemoveUserButton() {
        Button removeUserBtn = (Button) getActivity().findViewById(R.id.action_button1);
        // If the current user is not allowed to remove users, the user trying to be removed
        // cannot be removed, or the current user is a demo user, do not show delete button.
        if (!mCarUserManagerHelper.canCurrentProcessRemoveUsers()
                || !mCarUserManagerHelper.canUserBeRemoved(mUserInfo)
                || mCarUserManagerHelper.isCurrentProcessDemoUser()) {
            removeUserBtn.setVisibility(View.GONE);
            return;
        }
        removeUserBtn.setVisibility(View.VISIBLE);
        removeUserBtn.setText(R.string.delete_button);
        removeUserBtn.setOnClickListener(v -> {
            ConfirmRemoveUserDialog dialog = new ConfirmRemoveUserDialog();
            dialog.setConfirmRemoveUserListener(this);
            dialog.show(getFragmentManager(), CONFIRM_REMOVE_DIALOG_TAG);
        });
    }

    private void showSwitchButton() {
        Button switchUserBtn = (Button) getActivity().findViewById(R.id.action_button2);
        // If the current process is not allowed to switch to another user, doe not show the switch
        // button.
        if (!mCarUserManagerHelper.canCurrentProcessSwitchUsers()) {
            switchUserBtn.setVisibility(View.GONE);
            return;
        }
        switchUserBtn.setVisibility(View.VISIBLE);
        switchUserBtn.setText(R.string.user_switch);
        switchUserBtn.setOnClickListener(v -> {
            mCarUserManagerHelper.switchToUser(mUserInfo);
            getActivity().onBackPressed();
        });
    }

    private List<ListItem> getListItems() {
        TextListItem item = new TextListItem(getContext());
        item.setPrimaryActionIcon(mUserIconProvider.getUserIcon(mUserInfo, getContext()),
                /* useLargeIcon= */ false);
        item.setTitle(mUserInfo.name);

        if (!mUserInfo.isInitialized()) {
            // Indicate that the user has not been initialized yet.
            item.setBody(getContext().getString(R.string.user_summary_not_set_up));
        }

        return Arrays.asList(item);
    }
}
