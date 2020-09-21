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
package com.android.cts.transferowner;

import static junit.framework.Assert.assertTrue;

import static org.junit.Assert.assertEquals;

import android.app.admin.DevicePolicyManager;
import android.support.test.filters.SmallTest;

import org.junit.Test;

@SmallTest
public class TransferProfileOwnerIncomingTest extends DeviceAndProfileOwnerTransferIncomingTest {
    @Test
    public void testTransferPoliciesAreRetainedAfterTransfer() {
        int passwordLength = 123;
        int passwordExpirationTimeout = 456;
        assertTrue(mDevicePolicyManager.isAdminActive(mIncomingComponentName));
        assertTrue(mDevicePolicyManager.isProfileOwnerApp(mIncomingComponentName.getPackageName()));
        assertTrue(mDevicePolicyManager.getCameraDisabled(mIncomingComponentName));
        assertTrue(mDevicePolicyManager.getCrossProfileCallerIdDisabled(mIncomingComponentName));
        assertEquals(
                passwordLength,
                mDevicePolicyManager.getPasswordMinimumLength(mIncomingComponentName));

        DevicePolicyManager targetParentProfileInstance =
                mDevicePolicyManager.getParentProfileInstance(mIncomingComponentName);
        assertEquals(
                passwordExpirationTimeout,
                targetParentProfileInstance.getPasswordExpirationTimeout(mIncomingComponentName));
    }
}
