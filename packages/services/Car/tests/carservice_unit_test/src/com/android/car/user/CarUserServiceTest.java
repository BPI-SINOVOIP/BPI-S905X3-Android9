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

package com.android.car.user;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.user.CarUserManagerHelper;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.UserInfo;
import android.support.test.runner.AndroidJUnit4;

import java.util.ArrayList;
import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * This class contains unit tests for the {@link CarUserService}.
 *
 * The following mocks are used:
 * <ol>
 *   <li> {@link Context} provides system services and resources.
 *   <li> {@link CarUserManagerHelper} provides user info and actions.
 * <ol/>
 */
@RunWith(AndroidJUnit4.class)
public class CarUserServiceTest {
    private CarUserService mCarUserService;

    @Mock
    private Context mMockContext;

    @Mock
    private Context mApplicationContext;

    @Mock
    private CarUserManagerHelper mCarUserManagerHelper;

    /**
     * Initialize all of the objects with the @Mock annotation.
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        when(mMockContext.getApplicationContext()).thenReturn(mApplicationContext);

        mCarUserService = new CarUserService(mMockContext, mCarUserManagerHelper);
    }

    /**
     * Test that the {@link CarUserService} registers to receive the locked boot completed
     * intent.
     */
    @Test
    public void testRegistersToReceiveEvents() {
        ArgumentCaptor<IntentFilter> argument = ArgumentCaptor.forClass(IntentFilter.class);
        mCarUserService.init();
        verify(mMockContext).registerReceiver(eq(mCarUserService), argument.capture());
        IntentFilter intentFilter = argument.getValue();
        assertThat(intentFilter.countActions()).isEqualTo(1);

        assertThat(intentFilter.getAction(0)).isEqualTo(Intent.ACTION_LOCKED_BOOT_COMPLETED);
    }

    /**
     * Test that the {@link CarUserService} unregisters its event receivers.
     */
    @Test
    public void testUnregistersEventReceivers() {
        mCarUserService.release();
        verify(mMockContext).unregisterReceiver(mCarUserService);
    }

    /**
     * Test that the {@link CarUserService} starts up a secondary admin user upon first run.
     */
    @Test
    public void testStartsSecondaryAdminUserOnFirstRun() {
        List<UserInfo> users = new ArrayList<>();

        int adminUserId = 10;
        UserInfo admin = new UserInfo(adminUserId, CarUserService.OWNER_NAME, UserInfo.FLAG_ADMIN);

        doReturn(users).when(mCarUserManagerHelper).getAllUsers();
        // doReturn(users).when(mCarUserManagerHelper.getAllUsers());
        doReturn(admin).when(mCarUserManagerHelper).createNewAdminUser(CarUserService.OWNER_NAME);
        doReturn(true).when(mCarUserManagerHelper).switchToUser(admin);

        mCarUserService.onReceive(mMockContext,
                new Intent(Intent.ACTION_LOCKED_BOOT_COMPLETED));

        verify(mCarUserManagerHelper).createNewAdminUser(CarUserService.OWNER_NAME);
        verify(mCarUserManagerHelper).switchToUser(admin);
    }
}
