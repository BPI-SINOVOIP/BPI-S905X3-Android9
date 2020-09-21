/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.util.account;

import android.accounts.Account;
import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.support.annotation.Nullable;

/** Helper methods for getting and selecting a user account. */
public class AccountHelperImpl implements com.android.tv.util.account.AccountHelper {
    private static final String SELECTED_ACCOUNT = "android.tv.livechannels.selected_account";

    protected final Context mContext;
    private final SharedPreferences mDefaultPreferences;

    @Nullable private Account mSelectedAccount;

    public AccountHelperImpl(Context context) {
        mContext = context.getApplicationContext();
        mDefaultPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);
    }

    /** Returns the currently selected account or {@code null} if none is selected. */
    @Override
    @Nullable
    public final Account getSelectedAccount() {
        String accountId = mDefaultPreferences.getString(SELECTED_ACCOUNT, null);
        if (accountId == null) {
            return null;
        }
        if (mSelectedAccount == null || !mSelectedAccount.name.equals((accountId))) {
            mSelectedAccount = null;
            for (Account account : getEligibleAccounts()) {
                if (account.name.equals(accountId)) {
                    mSelectedAccount = account;
                    break;
                }
            }
        }
        return mSelectedAccount;
    }

    /**
     * Returns all eligible accounts.
     *
     * <p>Override this method to return the accounts needed.
     */
    protected Account[] getEligibleAccounts() {
        return new Account[0];
    }

    /**
     * Selects the first account available.
     *
     * @return selected account or {@code null} if none is selected.
     */
    @Override
    @Nullable
    public final Account selectFirstAccount() {
        Account account = getFirstEligibleAccount();
        if (account != null) {
            selectAccount(account);
        }
        return account;
    }

    /**
     * Gets the first account eligible.
     *
     * @return first account or {@code null} if none is eligible.
     */
    @Override
    @Nullable
    public final Account getFirstEligibleAccount() {
        Account[] accounts = getEligibleAccounts();
        return accounts.length == 0 ? null : accounts[0];
    }

    /** Sets the given account as the selected account. */
    private void selectAccount(Account account) {
        SharedPreferences defaultPreferences =
                PreferenceManager.getDefaultSharedPreferences(mContext);
        defaultPreferences.edit().putString(SELECTED_ACCOUNT, account.name).commit();
    }
}
