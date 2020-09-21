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
import static org.testng.Assert.assertThrows;

import android.app.admin.SystemUpdatePolicy;
import android.support.test.filters.SmallTest;

import org.junit.Test;

import java.util.Collections;

@SmallTest
public class TransferDeviceOwnerIncomingTest extends DeviceAndProfileOwnerTransferIncomingTest {
    @Test
    public void testTransferPoliciesAreRetainedAfterTransfer() {
        assertTrue(mDevicePolicyManager.isAdminActive(mIncomingComponentName));
        assertTrue(mDevicePolicyManager.isDeviceOwnerApp(mIncomingComponentName.getPackageName()));
        assertTrue(mDevicePolicyManager.getCameraDisabled(mIncomingComponentName));
        assertEquals(Collections.singletonList("test.package"),
                mDevicePolicyManager.getKeepUninstalledPackages(mIncomingComponentName));
        assertEquals(123, mDevicePolicyManager.getPasswordMinimumLength(mIncomingComponentName));
        assertSystemPoliciesEqual(SystemUpdatePolicy.createWindowedInstallPolicy(123, 456),
                mDevicePolicyManager.getSystemUpdatePolicy());
        assertThrows(SecurityException.class, () -> {
            mDevicePolicyManager.getParentProfileInstance(mIncomingComponentName);
        });
    }

    private void assertSystemPoliciesEqual(SystemUpdatePolicy policy1, SystemUpdatePolicy policy2) {
        assertTrue(policy1.getPolicyType() == policy2.getPolicyType()
                && policy1.getInstallWindowStart() == policy2.getInstallWindowStart()
                && policy1.getInstallWindowEnd() == policy2.getInstallWindowEnd());
    }
}
