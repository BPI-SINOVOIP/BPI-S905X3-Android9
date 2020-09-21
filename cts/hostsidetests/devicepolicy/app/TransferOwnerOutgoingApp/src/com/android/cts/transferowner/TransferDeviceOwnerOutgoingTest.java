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

import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertThrows;

import android.app.admin.DevicePolicyManager;
import android.app.admin.SystemUpdatePolicy;
import android.os.PersistableBundle;
import android.support.test.filters.SmallTest;

import org.junit.Test;

import java.util.Collections;

@SmallTest
public class TransferDeviceOwnerOutgoingTest extends DeviceAndProfileOwnerTransferOutgoingTest {

    @Override
    public void setUp() throws Exception {
        super.setUp();
        setupTestParameters(DevicePolicyManager.ACTION_DEVICE_OWNER_CHANGED);
    }

    @Test
    public void testTransferWithPoliciesOutgoing() throws Throwable {
        int passwordLength = 123;
        mDevicePolicyManager.setCameraDisabled(mOutgoingComponentName, true);
        mDevicePolicyManager.setPasswordMinimumLength(mOutgoingComponentName, passwordLength);
        mDevicePolicyManager.setKeepUninstalledPackages(mOutgoingComponentName,
                Collections.singletonList("test.package"));
        mDevicePolicyManager.setSystemUpdatePolicy(mOutgoingComponentName,
                SystemUpdatePolicy.createWindowedInstallPolicy(123, 456));

        PersistableBundle b = new PersistableBundle();
        mDevicePolicyManager.transferOwnership(mOutgoingComponentName, INCOMING_COMPONENT_NAME, b);
    }

    @Test
    public void testTransferOwnership() throws Throwable {
        PersistableBundle b = new PersistableBundle();
        mDevicePolicyManager.transferOwnership(mOutgoingComponentName, INCOMING_COMPONENT_NAME, b);
        assertTrue(mDevicePolicyManager.isAdminActive(INCOMING_COMPONENT_NAME));
        assertTrue(mDevicePolicyManager.isDeviceOwnerApp(INCOMING_COMPONENT_NAME.getPackageName()));
        assertFalse(
                mDevicePolicyManager.isDeviceOwnerApp(mOutgoingComponentName.getPackageName()));
        assertFalse(mDevicePolicyManager.isAdminActive(mOutgoingComponentName));
        assertThrows(SecurityException.class, () -> {
            mDevicePolicyManager.getSecondaryUsers(mOutgoingComponentName);
        });
    }

    @Test
    public void testTransferAffiliatedProfileOwnershipCompleteCallbackIsCalled() {
        assertTrue(getBooleanPref(mContext, KEY_TRANSFER_AFFILIATED_PROFILE_COMPLETED_CALLED));
    }
}
