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

package com.android.car.settings.accounts;

import android.os.Bundle;
import android.os.UserHandle;

import androidx.car.widget.ListItemProvider;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;
import com.android.car.settings.common.Logger;
import com.android.settingslib.accounts.AuthenticatorHelper;

/**
 * Activity asking a user to select an account to be set up.
 *
 * <p>An extra {@link UserHandle} can be specified in the intent as {@link EXTRA_USER},
 * if the user for which the action needs to be performed is different to the one the
 * Settings App will run in.
 */
public class ChooseAccountFragment extends ListItemSettingsFragment
        implements AuthenticatorHelper.OnAccountsUpdateListener {
    private static final Logger LOG = new Logger(ChooseAccountFragment.class);

    private ChooseAccountItemProvider mItemProvider;

    public static ChooseAccountFragment newInstance() {
        ChooseAccountFragment
                chooseAccountFragment = new ChooseAccountFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.add_an_account);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        chooseAccountFragment.setArguments(bundle);
        return chooseAccountFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mItemProvider = new ChooseAccountItemProvider(getContext(), this);
        super.onActivityCreated(savedInstanceState);
    }

    @Override
    public void onAccountsUpdate(UserHandle userHandle) {
        LOG.v("Accounts changed, refreshing the account list.");
        mItemProvider.refreshItems();
        refreshList();
    }

    @Override
    public ListItemProvider getItemProvider() {
        return mItemProvider;
    }
}
