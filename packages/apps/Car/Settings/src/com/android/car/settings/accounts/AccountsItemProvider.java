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
import android.content.Context;
import android.content.pm.UserInfo;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Implementation of {@link ListItemProvider} for {@link AccountsListFragment}.
 * Creates items that represent current user's accounts.
 */
class AccountsItemProvider extends ListItemProvider {
    private final List<ListItem> mItems = new ArrayList<>();
    private final Context mContext;
    private final AccountClickListener mItemClickListener;
    private final CarUserManagerHelper mCarUserManagerHelper;
    private final AccountManagerHelper mAccountManagerHelper;

    AccountsItemProvider(Context context, AccountClickListener itemClickListener,
            CarUserManagerHelper carUserManagerHelper, AccountManagerHelper accountManagerHelper) {
        mContext = context;
        mItemClickListener = itemClickListener;
        mCarUserManagerHelper = carUserManagerHelper;
        mAccountManagerHelper = accountManagerHelper;
        refreshItems();
    }

    @Override
    public ListItem get(int position) {
        return mItems.get(position);
    }

    @Override
    public int size() {
        return mItems.size();
    }

    /**
     * Clears and recreates the list of items.
     */
    public void refreshItems() {
        mItems.clear();

        UserInfo currUserInfo = mCarUserManagerHelper.getCurrentProcessUserInfo();

        List<Account> accounts = getSortedUserAccounts();

        // Only add account-related items if the User can Modify Accounts
        if (mCarUserManagerHelper.canCurrentProcessModifyAccounts()) {
            // Add "Account for $User" title for a list of accounts.
            mItems.add(createSubtitleItem(
                    mContext.getString(R.string.account_list_title, currUserInfo.name),
                    accounts.isEmpty() ? mContext.getString(R.string.no_accounts_added) : ""));

            // Add an item for each account owned by the current user (1st and 3rd party accounts)
            for (Account account : accounts) {
                mItems.add(createAccountItem(account, account.type, currUserInfo));
            }
        }
    }

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    List<Account> getSortedUserAccounts() {
        List<Account> accounts = mAccountManagerHelper.getAccountsForCurrentUser();

        // Sort accounts
        Collections.sort(accounts, Comparator.comparing(
                (Account a) -> mAccountManagerHelper.getLabelForType(a.type).toString())
                .thenComparing(a -> a.name));

        return accounts;
    }

    // Creates a subtitle line for visual separation in the list
    private ListItem createSubtitleItem(String title, String body) {
        TextListItem item = new TextListItem(mContext);
        item.setTitle(title);
        item.setBody(body);
        item.addViewBinder(viewHolder ->
                viewHolder.getTitle().setTextAppearance(R.style.SettingsListHeader));
        // Hiding the divider after subtitle, since subtitle is a header for a group of items.
        item.setHideDivider(true);
        return item;
    }

    // Creates a line for an account that belongs to a given user
    private ListItem createAccountItem(Account account, String accountType,
            UserInfo userInfo) {
        TextListItem item = new TextListItem(mContext);
        item.setPrimaryActionIcon(mAccountManagerHelper.getDrawableForType(accountType),
                /* useLargeIcon= */ false);
        item.setTitle(account.name);

        // Set item body = account label.
        CharSequence itemBody = mAccountManagerHelper.getLabelForType(accountType);
        if (!TextUtils.isEmpty(itemBody)) {
            item.setBody(itemBody.toString());
        }

        item.setOnClickListener(view -> mItemClickListener.onAccountClicked(account, userInfo));

        // setHideDivider = true will hide the divider to group the items together visually.
        // All of those without a divider between them will be part of the same "group".
        item.setHideDivider(true);
        return item;
    }

    /**
     * Interface for registering clicks on account items.
     */
    interface AccountClickListener {
        /**
         * Invoked when a specific account is clicked on.
         *
         * @param account  Account for which to display details.
         * @param userInfo User who's the owner of the account.
         */
        void onAccountClicked(Account account, UserInfo userInfo);
    }
}
