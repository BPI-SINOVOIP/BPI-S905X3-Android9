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

package com.android.car.settings.accounts;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.when;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.car.user.CarUserManagerHelper;
import android.content.Context;
import android.content.pm.UserInfo;
import android.os.UserHandle;
import android.os.UserManager;

import com.android.car.settings.CarSettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

@RunWith(CarSettingsRobolectricTestRunner.class)
public class AccountsItemProviderTest {
    @Mock
    private AccountManagerHelper mAccountManagerHelper;
    @Mock
    private UserManager mUserManager;
    @Mock
    private AccountManager mAccountManager;
    @Mock
    private Context mContext;
    @Mock
    private CarUserManagerHelper mCarUserManagerHelper;

    private AccountsItemProvider mProvider;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        when(mContext.getSystemService(Context.USER_SERVICE)).thenReturn(mUserManager);
        when(mContext.getSystemService(Context.ACCOUNT_SERVICE)).thenReturn(mAccountManager);
        when(mContext.getApplicationContext()).thenReturn(mContext);
        when(mUserManager.getUserInfo(UserHandle.myUserId())).thenReturn(new UserInfo());
        when(mAccountManagerHelper.getAccountsForCurrentUser()).thenReturn(new ArrayList<>());

        mCarUserManagerHelper = new CarUserManagerHelper(mContext);
        mProvider = new AccountsItemProvider(
                RuntimeEnvironment.application,
                null,
                mCarUserManagerHelper,
                mAccountManagerHelper);
    }

    @Test
    public void testAccountsAreSorted() {
        Account account1 = new Account("g name", "g test type");
        Account account2 = new Account("b name", "t test type");
        Account account3 = new Account("a name", "g test type");
        Account account4 = new Account("a name", "p test type");
        List<Account> accounts = Arrays.asList(account1, account2, account3, account4);

        when(mAccountManagerHelper.getAccountsForCurrentUser()).thenReturn(accounts);
        when(mAccountManagerHelper.getLabelForType("g test type")).thenReturn("g test type");
        when(mAccountManagerHelper.getLabelForType("t test type")).thenReturn("t test type");
        when(mAccountManagerHelper.getLabelForType("p test type")).thenReturn("p test type");

        assertThat(mProvider.getSortedUserAccounts())
                .containsExactly(account3, account1, account4, account2).inOrder();
    }

}
