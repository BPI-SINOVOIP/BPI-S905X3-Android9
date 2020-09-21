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

package com.android.car.settings.accounts;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.UserHandle;
import android.os.UserManager;

import com.android.settingslib.accounts.AuthenticatorHelper;

import java.util.ArrayList;
import java.util.List;

/**
 * Helper class for managing accounts that belong to a single user.
 */
class AccountManagerHelper {
    private final Context mContext;
    private final AuthenticatorHelper.OnAccountsUpdateListener mUpdateListener;
    private final AuthenticatorHelper mAuthenticatorHelper;

    private final UserHandle mCurrentUserHandle;

    public AccountManagerHelper(Context context,
            AuthenticatorHelper.OnAccountsUpdateListener listener) {
        mContext = context;
        mUpdateListener = listener;
        UserManager userManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mCurrentUserHandle = userManager.getUserInfo(UserHandle.myUserId()).getUserHandle();

        // Listen to account updates for this user.
        mAuthenticatorHelper =
                new AuthenticatorHelper(mContext, mCurrentUserHandle, mUpdateListener);
    }

    /**
     * Starts listening to account updates. Every registered listener should be unregistered.
     */
    public void startListeningToAccountUpdates() {
        mAuthenticatorHelper.listenToAccountUpdates();
    }

    /**
     * Stops listening to account updates. Should be called on cleanup/destroy.
     */
    public void stopListeningToAccountUpdates() {
        mAuthenticatorHelper.stopListeningToAccountUpdates();
    }

    /**
     * Returns all the enabled accounts that exist for the current user. These include 1st and 3rd
     * party accounts.
     *
     * @return List of {@code Account} for the currently logged in user.
     */
    public List<Account> getAccountsForCurrentUser() {
        List<Account> accounts = new ArrayList<>();

        String[] accountTypes = mAuthenticatorHelper.getEnabledAccountTypes();
        for (int i = 0; i < accountTypes.length; i++) {
            String accountType = accountTypes[i];
            Account[] accountsForType = AccountManager.get(mContext)
                    .getAccountsByTypeAsUser(accountType, mCurrentUserHandle);
            for (Account account : accountsForType) {
                accounts.add(account);
            }
        }
        return accounts;
    }

    /**
     * Returns whether the given account is in the list of accounts for the current user.
     * Useful for checking whether an account has been deleted.
     *
     * @param account Account which existence we're checking for.
     * @return {@code true} if it exists, {@code false} if it doesn't.
     */
    public boolean accountExists(Account account) {
        if (account == null) {
            return false;
        }

        Account[] accounts = AccountManager.get(mContext).getAccountsByTypeAsUser(
                account.type, mCurrentUserHandle);
        for (Account other : accounts) {
            if (other.equals(account)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Wrapper for {@code AuthenticatorHelper.getDrawableForType}.
     * Gets an icon associated with a particular account type.
     *
     * @param accountType the type of account
     * @return a drawable for the icon
     */
    public Drawable getDrawableForType(final String accountType) {
        return mAuthenticatorHelper.getDrawableForType(mContext, accountType);
    }

    /**
     * Wrapper for {@code AuthenticatorHelper.getLabelForType}.
     * Gets the label associated with a particular account type. If none found, return {@code null}.
     *
     * @param accountType the type of account
     * @return a CharSequence for the label or null if one cannot be found.
     */
    public CharSequence getLabelForType(final String accountType) {
        return mAuthenticatorHelper.getLabelForType(mContext, accountType);
    }
}
