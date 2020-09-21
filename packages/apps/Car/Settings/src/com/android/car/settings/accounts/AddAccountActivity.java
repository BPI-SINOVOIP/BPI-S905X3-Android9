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

import static android.content.Intent.EXTRA_USER;

import android.accounts.AccountManager;
import android.accounts.AccountManagerCallback;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.Activity;
import android.app.PendingIntent;
import android.car.user.CarUserManagerHelper;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.UserManager;
import android.widget.Toast;

import com.android.car.settings.R;
import com.android.car.settings.common.Logger;

import java.io.IOException;
/**
 *
 * Entry point Activity for account setup. Works as follows
 *
 * <ol>
 *   <li> After receiving an account type from ChooseAccountFragment, this Activity launches the
 *        account setup specified by AccountManager.
 *   <li> After the account setup, this Activity finishes without showing anything.
 * </ol>
 *
 */
public class AddAccountActivity extends Activity {
    /**
     * A boolean to keep the state of whether add account has already been called.
     */
    private static final String KEY_ADD_CALLED = "AddAccountCalled";
    /**
     *
     * Extra parameter to identify the caller. Applications may display a
     * different UI if the call is made from Settings or from a specific
     * application.
     *
     */
    private static final String KEY_CALLER_IDENTITY = "pendingIntent";
    private static final String SHOULD_NOT_RESOLVE = "SHOULDN'T RESOLVE!";

    private static final Logger LOG = new Logger(AddAccountActivity.class);
    private static final String ALLOW_SKIP = "allowSkip";

    /* package */ static final String EXTRA_SELECTED_ACCOUNT = "selected_account";

    // Show additional info regarding the use of a device with multiple users
    static final String EXTRA_HAS_MULTIPLE_USERS = "hasMultipleUsers";

    // Need a specific request code for add account activity.
    public static final int ADD_ACCOUNT_REQUEST = 2001;

    private CarUserManagerHelper mCarUserManagerHelper;
    private UserHandle mUserHandle;
    private PendingIntent mPendingIntent;
    private boolean mAddAccountCalled;

    public boolean hasMultipleUsers(Context context) {
        return ((UserManager) context.getSystemService(Context.USER_SERVICE))
                .getUsers().size() > 1;
    }

    private final AccountManagerCallback<Bundle> mCallback = new AccountManagerCallback<Bundle>() {
        @Override
        public void run(AccountManagerFuture<Bundle> future) {
            if (!future.isDone()) {
                LOG.v("Account manager future is not done.");
                finish();
            }
            try {
                Bundle result = future.getResult();

                Intent intent = (Intent) result.getParcelable(AccountManager.KEY_INTENT);
                Bundle addAccountOptions = new Bundle();
                addAccountOptions.putBoolean(EXTRA_HAS_MULTIPLE_USERS,
                        hasMultipleUsers(AddAccountActivity.this));
                addAccountOptions.putParcelable(EXTRA_USER, mUserHandle);
                intent.putExtras(addAccountOptions);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivityForResultAsUser(
                        intent, ADD_ACCOUNT_REQUEST, mUserHandle);
                LOG.v("account added: " + result);
            } catch (OperationCanceledException | IOException | AuthenticatorException e) {
                LOG.v("addAccount error: " + e);
            } finally {
                finish();
            }
        }
    };

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(KEY_ADD_CALLED, mAddAccountCalled);
        LOG.v("saved");
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState != null) {
            mAddAccountCalled = savedInstanceState.getBoolean(KEY_ADD_CALLED);
            LOG.v("Restored from previous add account call: " + mAddAccountCalled);
        }

        mCarUserManagerHelper = new CarUserManagerHelper(this);

        if (mAddAccountCalled) {
            // We already called add account - maybe the callback was lost.
            finish();
            return;
        }

        mUserHandle = mCarUserManagerHelper.getCurrentProcessUserInfo().getUserHandle();
        if (mCarUserManagerHelper.isCurrentProcessUserHasRestriction(
                UserManager.DISALLOW_MODIFY_ACCOUNTS)) {
            // We aren't allowed to add an account.
            Toast.makeText(
                    this, R.string.user_cannot_add_accounts_message, Toast.LENGTH_LONG)
                    .show();
            finish();
            return;
        }
        addAccount(getIntent().getStringExtra(EXTRA_SELECTED_ACCOUNT));
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        setResult(resultCode);
        if (mPendingIntent != null) {
            mPendingIntent.cancel();
            mPendingIntent = null;
        }
        finish();
    }

    private void addAccount(String accountType) {
        Bundle addAccountOptions = new Bundle();
        addAccountOptions.putBoolean(EXTRA_HAS_MULTIPLE_USERS, hasMultipleUsers(this));
        addAccountOptions.putBoolean(ALLOW_SKIP, true);

        /*
         * The identityIntent is for the purposes of establishing the identity
         * of the caller and isn't intended for launching activities, services
         * or broadcasts.
         */
        Intent identityIntent = new Intent();
        identityIntent.setComponent(new ComponentName(SHOULD_NOT_RESOLVE, SHOULD_NOT_RESOLVE));
        identityIntent.setAction(SHOULD_NOT_RESOLVE);
        identityIntent.addCategory(SHOULD_NOT_RESOLVE);

        mPendingIntent = PendingIntent.getBroadcast(this, 0, identityIntent, 0);
        addAccountOptions.putParcelable(KEY_CALLER_IDENTITY, mPendingIntent);

        AccountManager.get(this).addAccountAsUser(
                accountType,
                /* authTokenType= */ null,
                /* requiredFeatures= */ null,
                addAccountOptions,
                null,
                mCallback,
                /* handler= */ null,
                mUserHandle);
        mAddAccountCalled = true;
    }
}
