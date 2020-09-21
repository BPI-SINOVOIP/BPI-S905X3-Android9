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
 * limitations under the License
 */
package com.android.car.settings.accounts;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerCallback;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.UserInfo;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.UserHandle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.widget.Button;

import androidx.car.app.CarAlertDialog;
import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;
import com.android.car.settings.common.Logger;
import com.android.settingslib.accounts.AuthenticatorHelper;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Shows account details, and delete account option.
 */
public class AccountDetailsFragment extends ListItemSettingsFragment
        implements AuthenticatorHelper.OnAccountsUpdateListener {
    public static final String EXTRA_ACCOUNT_INFO = "extra_account_info";
    public static final String EXTRA_USER_INFO = "extra_user_info";

    private Account mAccount;
    private UserInfo mUserInfo;
    private ListItemProvider mItemProvider;
    private AccountManagerHelper mAccountManagerHelper;

    public static AccountDetailsFragment newInstance(
            Account account, UserInfo userInfo) {
        AccountDetailsFragment
                accountDetailsFragment = new AccountDetailsFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        bundle.putInt(EXTRA_TITLE_ID, R.string.account_details_title);
        bundle.putParcelable(EXTRA_ACCOUNT_INFO, account);
        bundle.putParcelable(EXTRA_USER_INFO, userInfo);
        accountDetailsFragment.setArguments(bundle);
        return accountDetailsFragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mAccount = getArguments().getParcelable(EXTRA_ACCOUNT_INFO);
        mUserInfo = getArguments().getParcelable(EXTRA_USER_INFO);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        // Should be created before calling getListItem().
        mAccountManagerHelper = new AccountManagerHelper(getContext(), this);
        mAccountManagerHelper.startListeningToAccountUpdates();

        mItemProvider = new ListItemProvider.ListProvider(getListItems());

        // Super is called only AFTER item provider is instantiated, because
        // super.onActivityCreated calls getItemProvider().
        super.onActivityCreated(savedInstanceState);

        // Title was set in super.onActivityCreated, but override if account label is available.
        setFragmentTitle();

        showRemoveButton();
    }

    @Override
    public void onAccountsUpdate(UserHandle userHandle) {
        if (!mAccountManagerHelper.accountExists(mAccount)) {
            // The account was deleted. Pop back.
            getFragmentController().goBack();
        }
    }

    @Override
    public ListItemProvider getItemProvider() {
        return mItemProvider;
    }

    private void showRemoveButton() {
        Button removeAccountBtn = getActivity().findViewById(R.id.action_button1);
        removeAccountBtn.setText(R.string.remove_button);
        removeAccountBtn.setOnClickListener(v -> removeAccount());
    }

    private void setFragmentTitle() {
        CharSequence accountLabel = mAccountManagerHelper.getLabelForType(mAccount.type);
        if (!TextUtils.isEmpty(accountLabel)) {
            setTitle(accountLabel);
        }
    }

    private List<ListItem> getListItems() {
        Drawable icon = mAccountManagerHelper.getDrawableForType(mAccount.type);

        TextListItem item = new TextListItem(getContext());
        item.setPrimaryActionIcon(icon, /* useLargeIcon= */ false);
        item.setTitle(mAccount.name);

        List<ListItem> items = new ArrayList<>();
        items.add(item);
        return items;
    }

    public void removeAccount() {
        ConfirmRemoveAccountDialog.show(this, mAccount, mUserInfo.getUserHandle());
    }

    /**
     * Dialog to confirm with user about account removal
     */
    public static class ConfirmRemoveAccountDialog extends DialogFragment implements
            DialogInterface.OnClickListener {
        private static final String KEY_ACCOUNT = "account";
        private static final String DIALOG_TAG = "confirmRemoveAccount";
        private static final Logger LOG = new Logger(ConfirmRemoveAccountDialog.class);
        private Account mAccount;
        private UserHandle mUserHandle;

        private final AccountManagerCallback<Bundle> mCallback =
            new AccountManagerCallback<Bundle>() {
                @Override
                public void run(AccountManagerFuture<Bundle> future) {
                    // If already out of this screen, don't proceed.
                    if (!getTargetFragment().isResumed()) {
                        return;
                    }

                    boolean success = false;
                    try {
                        success =
                                future.getResult().getBoolean(AccountManager.KEY_BOOLEAN_RESULT);
                    } catch (OperationCanceledException | IOException | AuthenticatorException e) {
                        LOG.v("removeAccount error: " + e);
                    }
                    final Activity activity = getTargetFragment().getActivity();
                    if (!success && activity != null && !activity.isFinishing()) {
                        RemoveAccountFailureDialog.show(getTargetFragment());
                    } else {
                        getTargetFragment().getFragmentManager().popBackStack();
                    }
                }
            };

        public static void show(
                Fragment parent, Account account, UserHandle userHandle) {
            final ConfirmRemoveAccountDialog dialog = new ConfirmRemoveAccountDialog();
            Bundle bundle = new Bundle();
            bundle.putParcelable(KEY_ACCOUNT, account);
            bundle.putParcelable(Intent.EXTRA_USER, userHandle);
            dialog.setArguments(bundle);
            dialog.setTargetFragment(parent, 0);
            dialog.show(parent.getFragmentManager(), DIALOG_TAG);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            final Bundle arguments = getArguments();
            mAccount = arguments.getParcelable(KEY_ACCOUNT);
            mUserHandle = arguments.getParcelable(Intent.EXTRA_USER);
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            return new CarAlertDialog.Builder(getContext())
                    .setTitle(R.string.really_remove_account_title)
                    .setBody(R.string.really_remove_account_message)
                    .setNegativeButton(android.R.string.cancel, null)
                    .setPositiveButton(R.string.remove_account_title, this)
                    .create();
        }

        @Override
        public void onClick(DialogInterface dialog, int which) {
            Activity activity = getTargetFragment().getActivity();
            AccountManager.get(activity).removeAccountAsUser(
                    mAccount, activity, mCallback, null, mUserHandle);
            dialog.dismiss();
        }
    }

    /**
     * Dialog to tell user about account removal failure
     */
    public static class RemoveAccountFailureDialog extends DialogFragment {

        private static final String DIALOG_TAG = "removeAccountFailed";

        public static void show(Fragment parent) {
            final RemoveAccountFailureDialog dialog = new RemoveAccountFailureDialog();
            dialog.setTargetFragment(parent, 0);
            dialog.show(parent.getFragmentManager(), DIALOG_TAG);
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            return new CarAlertDialog.Builder(getContext())
                    .setTitle(R.string.really_remove_account_title)
                    .setBody(R.string.remove_account_failed)
                    .setPositiveButton(android.R.string.ok, null)
                    .create();
        }

    }
}
