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

import android.accounts.AccountManager;
import android.accounts.AuthenticatorDescription;
import android.app.ActivityManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SyncAdapterType;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.UserHandle;
import android.os.UserManager;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.common.Logger;
import com.android.internal.util.CharSequences;
import com.android.settingslib.accounts.AuthenticatorHelper;

import libcore.util.Nullable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Implementation of ListItemProvider for the ChooseAccountFragment.
 */
class ChooseAccountItemProvider extends ListItemProvider {
    private static final Logger LOG = new Logger(ChooseAccountItemProvider.class);
    private static final String AUTHORITIES_FILTER_KEY = "authorities";

    private final List<ListItem> mItems = new ArrayList<>();
    private final Context mContext;
    private final ChooseAccountFragment mFragment;
    private final UserManager mUserManager;
    private final ArrayList<ProviderEntry> mProviderList = new ArrayList<>();
    @Nullable private AuthenticatorDescription[] mAuthDescs;
    @Nullable private HashMap<String, ArrayList<String>> mAccountTypeToAuthorities;
    private Map<String, AuthenticatorDescription> mTypeToAuthDescription = new HashMap<>();
    private final AuthenticatorHelper mAuthenticatorHelper;

    // The UserHandle of the user we are choosing an account for
    private final UserHandle mUserHandle;
    private final String[] mAuthorities;

    ChooseAccountItemProvider(Context context, ChooseAccountFragment fragment) {
        mContext = context;
        mFragment = fragment;
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mUserHandle =
                mUserManager.getUserInfo(ActivityManager.getCurrentUser()).getUserHandle();

        mAuthorities = fragment.getActivity().getIntent().getStringArrayExtra(
                AUTHORITIES_FILTER_KEY);

        // create auth helper
        UserInfo currUserInfo = mUserManager.getUserInfo(ActivityManager.getCurrentUser());
        mAuthenticatorHelper =
                new AuthenticatorHelper(mContext, currUserInfo.getUserHandle(), mFragment);

        updateAuthDescriptions();
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
     * Clears the existing item list and rebuilds it with new data.
     */
    public void refreshItems() {
        mItems.clear();

        for (int i = 0; i < mProviderList.size(); i++) {
            String accountType = mProviderList.get(i).type;
            Drawable icon = mAuthenticatorHelper.getDrawableForType(mContext, accountType);

            TextListItem item = new TextListItem(mContext);
            item.setPrimaryActionIcon(icon, /* useLargeIcon= */ false);
            item.setTitle(mProviderList.get(i).name.toString());
            item.setOnClickListener(v -> onItemSelected(accountType));
            mItems.add(item);
        }
    }

    // Starts a AddAccountActivity for the accountType that was clicked on.
    private void onItemSelected(String accountType) {
        Intent intent = new Intent(mContext, AddAccountActivity.class);
        intent.putExtra(AddAccountActivity.EXTRA_SELECTED_ACCOUNT, accountType);
        mContext.startActivity(intent);
    }

    /**
     * Updates provider icons.
     */
    private void updateAuthDescriptions() {
        mAuthDescs = AccountManager.get(mContext).getAuthenticatorTypesAsUser(
                mUserHandle.getIdentifier());
        for (int i = 0; i < mAuthDescs.length; i++) {
            mTypeToAuthDescription.put(mAuthDescs[i].type, mAuthDescs[i]);
        }
        onAuthDescriptionsUpdated();
    }

    private void onAuthDescriptionsUpdated() {
        // Create list of providers to show on page.
        for (int i = 0; i < mAuthDescs.length; i++) {
            String accountType = mAuthDescs[i].type;
            CharSequence providerName = getLabelForType(accountType);

            // Get the account authorities implemented by the account type.
            ArrayList<String> accountAuths = getAuthoritiesForAccountType(accountType);

            // If there are specific authorities required, we need to check whether it's
            // included in the account type.
            if (mAuthorities != null && mAuthorities.length > 0 && accountAuths != null) {
                for (int k = 0; k < mAuthorities.length; k++) {
                    if (accountAuths.contains(mAuthorities[k])) {
                        mProviderList.add(
                                new ProviderEntry(providerName, accountType));
                        break;
                    }
                }
            } else {
                mProviderList.add(
                        new ProviderEntry(providerName, accountType));
            }
        }
        Collections.sort(mProviderList);
    }

    private ArrayList<String> getAuthoritiesForAccountType(String type) {
        if (mAccountTypeToAuthorities == null) {
            mAccountTypeToAuthorities = new HashMap<>();
            SyncAdapterType[] syncAdapters = ContentResolver.getSyncAdapterTypesAsUser(
                    mUserHandle.getIdentifier());
            for (int i = 0, n = syncAdapters.length; i < n; i++) {
                final SyncAdapterType adapterType = syncAdapters[i];
                ArrayList<String> authorities =
                        mAccountTypeToAuthorities.get(adapterType.accountType);
                if (authorities == null) {
                    authorities = new ArrayList<String>();
                    mAccountTypeToAuthorities.put(adapterType.accountType, authorities);
                }
                LOG.v("added authority " + adapterType.authority + " to accountType "
                        + adapterType.accountType);
                authorities.add(adapterType.authority);
            }
        }
        return mAccountTypeToAuthorities.get(type);
    }

    /**
     * Gets the label associated with a particular account type. If none found, return null.
     *
     * @param accountType the type of account
     * @return a CharSequence for the label or null if one cannot be found.
     */
    private CharSequence getLabelForType(final String accountType) {
        CharSequence label = null;
        if (mTypeToAuthDescription.containsKey(accountType)) {
            try {
                AuthenticatorDescription desc = mTypeToAuthDescription.get(accountType);
                Context authContext = mFragment.getActivity().createPackageContextAsUser(
                        desc.packageName, 0 /* flags */, mUserHandle);
                label = authContext.getResources().getText(desc.labelId);
            } catch (PackageManager.NameNotFoundException e) {
                LOG.w("No label name for account type " + accountType);
            } catch (Resources.NotFoundException e) {
                LOG.w("No label resource for account type " + accountType);
            }
        }
        return label;
    }

    private static class ProviderEntry implements Comparable<ProviderEntry> {
        private final CharSequence name;
        private final String type;
        ProviderEntry(CharSequence providerName, String accountType) {
            name = providerName;
            type = accountType;
        }

        @Override
        public int compareTo(ProviderEntry another) {
            if (name == null) {
                return -1;
            }
            if (another.name == null) {
                return 1;
            }
            return CharSequences.compareToIgnoreCase(name, another.name);
        }
    }
}
