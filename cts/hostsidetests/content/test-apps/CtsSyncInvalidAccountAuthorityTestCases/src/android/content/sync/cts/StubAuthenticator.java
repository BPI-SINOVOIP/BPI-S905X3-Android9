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

package android.content.sync.cts;

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

public class StubAuthenticator extends Service {

    private Authenticator mAuthenticator;

    @Override
    public void onCreate() {
        mAuthenticator = new Authenticator(this);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mAuthenticator.getIBinder();
    }

    public class Authenticator extends AbstractAccountAuthenticator {
        public Authenticator(Context context) {
            super(context);
        }

        @Override
        public Bundle editProperties(AccountAuthenticatorResponse response,
                String accountType) {
            return null;
        }

        @Override
        public Bundle addAccount(AccountAuthenticatorResponse response,
                String accountType, String tokenType, String[] strings,
                Bundle bundle) throws NetworkErrorException {
            Bundle result = new Bundle();
            result.putString(AccountManager.KEY_ERROR_MESSAGE, "Unsupported operation");
            response.onResult(result);
            return null;
        }

        @Override
        public Bundle confirmCredentials(AccountAuthenticatorResponse response,
                Account account, Bundle bundle) throws NetworkErrorException {
            return null;
        }

        @Override
        public Bundle getAuthToken(AccountAuthenticatorResponse response,
                Account account, String type, Bundle bundle) throws NetworkErrorException {
            return null;
        }

        @Override
        public String getAuthTokenLabel(String tokenName) {
            return null;
        }

        @Override
        public Bundle updateCredentials(AccountAuthenticatorResponse response,
                Account account, String tokenType, Bundle bundle)
                throws NetworkErrorException {
            return null;
        }

        @Override
        public Bundle hasFeatures(AccountAuthenticatorResponse response,
                Account account, String[] options) throws NetworkErrorException {
            return null;
        }

        @Override
        public Bundle getAccountRemovalAllowed(AccountAuthenticatorResponse response,
                Account account) throws NetworkErrorException {
            Bundle result = new Bundle();
            result.putBoolean(AccountManager.KEY_BOOLEAN_RESULT, true);
            return result;
        }
    }
}
