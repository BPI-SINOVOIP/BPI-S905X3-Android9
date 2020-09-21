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

package com.android.car.settings.accounts;

import android.accounts.Account;
import android.car.user.CarUserManagerHelper;
import android.content.pm.UserInfo;
import android.os.Bundle;
import android.os.UserHandle;
import android.view.View;
import android.widget.Button;

import androidx.car.widget.ListItemProvider;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;
import com.android.settingslib.accounts.AuthenticatorHelper;

/**
 * Shows current user and the accounts that belong to the user.
 */
public class AccountsListFragment extends ListItemSettingsFragment
        implements AuthenticatorHelper.OnAccountsUpdateListener,
        CarUserManagerHelper.OnUsersUpdateListener,
        AccountsItemProvider.AccountClickListener {
    private AccountsItemProvider mItemProvider;
    private AccountManagerHelper mAccountManagerHelper;
    private CarUserManagerHelper mCarUserManagerHelper;

    private Button mAddAccountButton;

    /**
     * Creates new instance of CurrentUserDetailsFragment.
     */
    public static AccountsListFragment newInstance() {
        AccountsListFragment accountsFragment = new AccountsListFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.accounts_settings_title);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        accountsFragment.setArguments(bundle);
        return accountsFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mAccountManagerHelper = new AccountManagerHelper(getContext(), this);
        mAccountManagerHelper.startListeningToAccountUpdates();

        mCarUserManagerHelper = new CarUserManagerHelper(getContext());
        mItemProvider = new AccountsItemProvider(getContext(), this,
                mCarUserManagerHelper, mAccountManagerHelper);

        // Register to receive changes to the users.
        mCarUserManagerHelper.registerOnUsersUpdateListener(this);

        // Super class's onActivityCreated need to be called after mContext is initialized.
        // Because getLineItems is called in there.
        super.onActivityCreated(savedInstanceState);

        mAddAccountButton = (Button) getActivity().findViewById(R.id.action_button1);
        if (mCarUserManagerHelper.canCurrentProcessModifyAccounts()) {
            mAddAccountButton.setText(R.string.user_add_account_menu);
            mAddAccountButton.setOnClickListener(v -> onAddAccountClicked());
        } else {
            mAddAccountButton.setVisibility(View.GONE);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mCarUserManagerHelper.unregisterOnUsersUpdateListener();
        mAccountManagerHelper.stopListeningToAccountUpdates();

        // The action button may be hidden at some point, so make it visible again
        mAddAccountButton.setVisibility(View.VISIBLE);
    }

    @Override
    public void onAccountsUpdate(UserHandle userHandle) {
        refreshListItems();
    }

    @Override
    public void onUsersUpdate() {
        refreshListItems();
    }

    @Override
    public void onAccountClicked(Account account, UserInfo userInfo) {
        getFragmentController().launchFragment(
                AccountDetailsFragment.newInstance(account, userInfo));
    }

    @Override
    public ListItemProvider getItemProvider() {
        return mItemProvider;
    }

    private void refreshListItems() {
        mItemProvider.refreshItems();
        refreshList();
    }

    private void onAddAccountClicked() {
        getFragmentController().launchFragment(ChooseAccountFragment.newInstance());
    }
}
