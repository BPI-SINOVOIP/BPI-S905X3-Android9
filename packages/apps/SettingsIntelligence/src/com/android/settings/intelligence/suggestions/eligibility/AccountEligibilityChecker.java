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

package com.android.settings.intelligence.suggestions.eligibility;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.Context;
import android.content.pm.ResolveInfo;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

public class AccountEligibilityChecker {
    /**
     * If defined, only display this optional step if an account of that type exists.
     */
    @VisibleForTesting
    static final String META_DATA_REQUIRE_ACCOUNT = "com.android.settings.require_account";
    private static final String TAG = "AccountEligibility";

    public static boolean isEligible(Context context, String id, ResolveInfo info) {
        final String requiredAccountType = info.activityInfo.metaData.
                getString(META_DATA_REQUIRE_ACCOUNT);
        if (requiredAccountType == null) {
            return true;
        }
        AccountManager accountManager = AccountManager.get(context);
        Account[] accounts = accountManager.getAccountsByType(requiredAccountType);
        boolean satisfiesRequiredAccount = accounts.length > 0;
        if (!satisfiesRequiredAccount) {
            Log.i(TAG, id + " requires unavailable account type " + requiredAccountType);
        }
        return satisfiesRequiredAccount;
    }
}
