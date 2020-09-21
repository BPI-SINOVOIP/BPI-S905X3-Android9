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

package com.android.settings.deviceinfo;

import android.accounts.Account;
import android.content.Context;
import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.accounts.AccountDetailDashboardFragment;
import com.android.settings.accounts.AccountFeatureProvider;
import com.android.settings.core.BasePreferenceController;
import com.android.settings.core.SubSettingLauncher;
import com.android.settings.overlay.FeatureFactory;

public class BrandedAccountPreferenceController extends BasePreferenceController {
    private static final String KEY_PREFERENCE_TITLE = "branded_account";
    private final Account[] mAccounts;

    public BrandedAccountPreferenceController(Context context) {
        super(context, KEY_PREFERENCE_TITLE);
        final AccountFeatureProvider accountFeatureProvider = FeatureFactory.getFactory(
                mContext).getAccountFeatureProvider();
        mAccounts = accountFeatureProvider.getAccounts(mContext);
    }

    @Override
    public int getAvailabilityStatus() {
        if (mAccounts != null && mAccounts.length > 0) {
            return AVAILABLE;
        }
        return DISABLED_FOR_USER;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        final AccountFeatureProvider accountFeatureProvider = FeatureFactory.getFactory(
                mContext).getAccountFeatureProvider();
        final Preference accountPreference = screen.findPreference(KEY_PREFERENCE_TITLE);
        if (accountPreference != null && (mAccounts == null || mAccounts.length == 0)) {
            screen.removePreference(accountPreference);
            return;
        }

        accountPreference.setSummary(mAccounts[0].name);
        accountPreference.setOnPreferenceClickListener(preference -> {
            final Bundle args = new Bundle();
            args.putParcelable(AccountDetailDashboardFragment.KEY_ACCOUNT,
                    mAccounts[0]);
            args.putParcelable(AccountDetailDashboardFragment.KEY_USER_HANDLE,
                    android.os.Process.myUserHandle());
            args.putString(AccountDetailDashboardFragment.KEY_ACCOUNT_TYPE,
                    accountFeatureProvider.getAccountType());

            new SubSettingLauncher(mContext)
                    .setDestination(AccountDetailDashboardFragment.class.getName())
                    .setTitle(R.string.account_sync_title)
                    .setArguments(args)
                    .setSourceMetricsCategory(MetricsEvent.DEVICEINFO)
                    .launch();
            return true;
        });
    }
}
