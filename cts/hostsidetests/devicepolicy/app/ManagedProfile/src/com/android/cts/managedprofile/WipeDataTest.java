/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.cts.managedprofile;

public class WipeDataTest extends BaseManagedProfileTest {
    private static final String TEST_WIPE_DATA_REASON = "cts test for WipeDataWithReason";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // Make sure we are running in a managed profile, otherwise risk wiping the primary user's
        // data.
        assertTrue(mDevicePolicyManager.isAdminActive(ADMIN_RECEIVER_COMPONENT));
        assertTrue(mDevicePolicyManager.isProfileOwnerApp(ADMIN_RECEIVER_COMPONENT.getPackageName()));
    }

    /**
     * Test wipeData() for use in managed profile. If called from a managed profile, wipeData()
     * should remove the current managed profile. Also, no erasing of external storage should be
     * allowed.
     */
    public void testWipeData() throws InterruptedException {
        mDevicePolicyManager.wipeData(0);
        // The test that the profile will indeed be removed is done in the host.
    }

    /**
     * Test wipeDataWithReason() for use in managed profile. If called from a managed profile,
     * wipeDataWithReason() should remove the current managed profile.In the mean time, it should
     * send out a notification containing the reason for wiping data to user. Also, no erasing of
     * external storage should be allowed.
     */
    public void testWipeDataWithReason() throws InterruptedException {
        mDevicePolicyManager.wipeData(0, TEST_WIPE_DATA_REASON);
        // The test that the profile will indeed be removed is done in the host.
        // Notification verification is done in another test.
    }
}
