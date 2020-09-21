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
package android.content.syncmanager.cts.app;

import static com.android.compatibility.common.util.TestUtils.waitUntil;

import static org.junit.Assert.fail;

import android.accounts.AbstractAccountAuthenticator;
import android.accounts.Account;
import android.accounts.AccountAuthenticatorResponse;
import android.accounts.AccountManager;
import android.accounts.NetworkErrorException;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

import java.util.Arrays;

/**
 * Authenticator for the sync test.
 */
public class SyncManagerCtsAuthenticator extends Service {
    private static final String TAG = "SyncManagerCtsAuthenticator";

    private static Authenticator sInstance;

    @Override
    public IBinder onBind(Intent intent) {
        if (sInstance == null) {
            sInstance = new Authenticator(getApplicationContext());

        }
        return sInstance.getIBinder();
    }

    static AccountManager getAccountManager() {
        return ContextHolder.getContext().getSystemService(AccountManager.class);
    }

    static String getAccountType() {
        return ContextHolder.getContext().getPackageName();
    }

    /**
     * Adds a test account, if it doesn't exist yet.
     */
    public static void ensureTestAccount(String name) {

        final Account account = new Account(name, getAccountType());

        Bundle result = new Bundle();
        result.putString(AccountManager.KEY_ACCOUNT_TYPE, account.type);
        result.putString(AccountManager.KEY_ACCOUNT_NAME, account.name);

        if (!Arrays.asList(getAccountManager().getAccountsByType(account.type)).contains(account)) {
            Log.i(TAG, "Adding account: " + account);
            getAccountManager().addAccountExplicitly(account, "password", new Bundle());
        }
    }

    /**
     * Remove the test account.
     */
    public static void removeAllAccounts() throws Exception {
        final AccountManager am = getAccountManager();

        for (Account account : am.getAccountsByType(getAccountType())) {
            Log.i(TAG, "Removing " + account + "...");
            am.removeAccountExplicitly(account);
        }
        Log.i(TAG, "Removed all accounts for " + getAccountType());

        waitUntil("Accounts still exist.",
                () -> am.getAccountsByType(getAccountType()).length == 0);
    }

    public static class Authenticator extends AbstractAccountAuthenticator {

        private final Context mContext;

        public Authenticator(Context context) {
            super(context);
            mContext = context;
        }

        @Override
        public Bundle addAccount(AccountAuthenticatorResponse response, String accountType,
                String authTokenType, String[] requiredFeatures, Bundle options)
                throws NetworkErrorException {
            return new Bundle();
        }

        @Override
        public Bundle editProperties(AccountAuthenticatorResponse response, String accountType) {
            return new Bundle();
        }

        @Override
        public Bundle updateCredentials(AccountAuthenticatorResponse response, Account account,
                String authTokenType, Bundle options) throws NetworkErrorException {
            return new Bundle();
        }

        @Override
        public Bundle confirmCredentials(AccountAuthenticatorResponse response, Account account,
                Bundle options) throws NetworkErrorException {
            return new Bundle();
        }

        @Override
        public Bundle getAuthToken(AccountAuthenticatorResponse response, Account account,
                String authTokenType, Bundle options) throws NetworkErrorException {
            return new Bundle();
        }

        @Override
        public String getAuthTokenLabel(String authTokenType) {
            return "token_label";
        }

        @Override
        public Bundle hasFeatures(AccountAuthenticatorResponse response, Account account,
                String[] features) throws NetworkErrorException {
            return new Bundle();
        }
    }
}
