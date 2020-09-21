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

package com.android.car.settings.users;

import android.car.drivingstate.CarUxRestrictions;
import android.car.user.CarUserManagerHelper;
import android.content.Intent;
import android.content.pm.UserInfo;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;

import androidx.car.widget.ListItemProvider;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;

/**
 * Lists all Users available on this device.
 */
public class UsersListFragment extends ListItemSettingsFragment
        implements CarUserManagerHelper.OnUsersUpdateListener,
        UsersItemProvider.UserClickListener,
        ConfirmCreateNewUserDialog.ConfirmCreateNewUserListener,
        ConfirmExitRetailModeDialog.ConfirmExitRetailModeListener,
        AddNewUserTask.AddNewUserListener {
    private static final String FACTORY_RESET_PACKAGE_NAME = "android";
    private static final String FACTORY_RESET_REASON = "ExitRetailModeConfirmed";

    private UsersItemProvider mItemProvider;
    private CarUserManagerHelper mCarUserManagerHelper;

    private ProgressBar mProgressBar;
    private Button mAddUserButton;

    private AsyncTask mAddNewUserTask;

    public static UsersListFragment newInstance() {
        UsersListFragment usersListFragment = new UsersListFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.users_list_title);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        usersListFragment.setArguments(bundle);
        return usersListFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mCarUserManagerHelper = new CarUserManagerHelper(getContext());
        mItemProvider =
                new UsersItemProvider(getContext(), this, mCarUserManagerHelper);

        // Register to receive changes to the users.
        mCarUserManagerHelper.registerOnUsersUpdateListener(this);

        // Super class's onActivityCreated need to be called after itemProvider is initialized.
        // Because getItemProvider is called in there.
        super.onActivityCreated(savedInstanceState);

        mProgressBar = getActivity().findViewById(R.id.progress_bar);

        mAddUserButton = (Button) getActivity().findViewById(R.id.action_button1);
        if (mCarUserManagerHelper.isCurrentProcessDemoUser()) {
            // If the user is a demo user, show a dialog asking if they want to exit retail/demo
            // mode
            mAddUserButton.setText(R.string.exit_retail_button_text);
            mAddUserButton.setOnClickListener(v -> {
                ConfirmExitRetailModeDialog dialog = new ConfirmExitRetailModeDialog();
                dialog.setConfirmExitRetailModeListener(this);
                dialog.show(this);
            });
        } else if (mCarUserManagerHelper.canCurrentProcessAddUsers()) {
            // Only add the add user button if the current user is allowed to add a user.
            mAddUserButton.setText(R.string.user_add_user_menu);
            mAddUserButton.setOnClickListener(v -> {
                ConfirmCreateNewUserDialog dialog =
                        new ConfirmCreateNewUserDialog();
                dialog.setConfirmCreateNewUserListener(this);
                dialog.show(this);
            });
        }
    }

    @Override
    public void onCreateNewUserConfirmed() {
        mAddUserButton.setEnabled(false);
        mProgressBar.setVisibility(View.VISIBLE);
        mAddNewUserTask =
                new AddNewUserTask(mCarUserManagerHelper, /* addNewUserListener= */ this)
                        .execute(getContext().getString(R.string.user_new_user_name));
    }

    /**
     * Will perform a factory reset. Copied from
     * {@link com.android.settings.MasterClearConfirm#doMasterClear()}
     */
    @Override
    public void onExitRetailModeConfirmed() {
        Intent intent = new Intent(Intent.ACTION_FACTORY_RESET);
        intent.setPackage(FACTORY_RESET_PACKAGE_NAME);
        intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        intent.putExtra(Intent.EXTRA_REASON, FACTORY_RESET_REASON);
        intent.putExtra(Intent.EXTRA_WIPE_EXTERNAL_STORAGE, true);
        intent.putExtra(Intent.EXTRA_WIPE_ESIMS, true);
        getActivity().sendBroadcast(intent);
        // Intent handling is asynchronous -- assume it will happen soon.
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mAddNewUserTask != null) {
            mAddNewUserTask.cancel(/* mayInterruptIfRunning= */ false);
        }

        mCarUserManagerHelper.unregisterOnUsersUpdateListener();
    }

    @Override
    public void onUsersUpdate() {
        refreshListItems();
    }

    @Override
    public void onUserClicked(UserInfo userInfo) {
        if (mCarUserManagerHelper.isForegroundUser(userInfo)) {
            // If it's the foreground user, launch fragment that allows them to edit their name.
            getFragmentController().launchFragment(EditUsernameFragment.newInstance(userInfo));
        } else {
            // If it's another user, launch fragment that displays their information
            getFragmentController().launchFragment(UserDetailsFragment.newInstance(userInfo));
        }
    }

    /**
     * User list fragment is distraction optimized, so is allowed at all times.
     */
    @Override
    public boolean canBeShown(CarUxRestrictions carUxRestrictions) {
        return true;
    }

    @Override
    public void onGuestClicked() {
        getFragmentController().launchFragment(GuestFragment.newInstance());
    }

    @Override
    public ListItemProvider getItemProvider() {
        return mItemProvider;
    }

    private void refreshListItems() {
        mItemProvider.refreshItems();
        refreshList();
    }

    @Override
    public void onUserAdded() {
        mAddUserButton.setEnabled(true);
        mProgressBar.setVisibility(View.GONE);
    }
}
