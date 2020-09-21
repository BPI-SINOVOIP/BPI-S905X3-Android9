/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.managedprovisioning.manageduser;

import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;

import android.os.UserHandle;
import android.support.test.filters.SmallTest;

import com.android.managedprovisioning.task.nonrequiredapps.SystemAppsSnapshot;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * Unit tests for {@link ManagedUserCreationController}.
 */
@SmallTest
public class ManagedUserCreationControllerTest {

    private static final int MANAGED_USER_USER_ID = 12;

    @Mock
    private SystemAppsSnapshot mSystemAppsSnapshot;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
    }

    @Test
    public void testNullUserId() {
        ManagedUserCreationController controller = new ManagedUserCreationController(
                UserHandle.USER_NULL, false, mSystemAppsSnapshot);

        controller.run();

        verifyZeroInteractions(mSystemAppsSnapshot);
    }

    @Test
    public void testLeaveAllSystemApps() {
        ManagedUserCreationController controller = new ManagedUserCreationController(
                MANAGED_USER_USER_ID, true, mSystemAppsSnapshot);

        controller.run();

        verifyZeroInteractions(mSystemAppsSnapshot);
    }

    @Test
    public void testTakeSnapshot() {
        ManagedUserCreationController controller = new ManagedUserCreationController(
                MANAGED_USER_USER_ID, false, mSystemAppsSnapshot);

        controller.run();

        verify(mSystemAppsSnapshot).takeNewSnapshot(MANAGED_USER_USER_ID);
    }
}
