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

package com.android.cts.managedprofile;

import android.app.KeyguardManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.test.InstrumentationTestCase;

import com.android.cts.managedprofile.BaseManagedProfileTest.BasicAdminReceiver;

/**
 * Helper to set lock timeouts and check if the profile is locked.
 */
public class ProfileTimeoutTestHelper extends InstrumentationTestCase {
    // This should be sufficiently smaller than ManagedProfileTest.PROFILE_TIMEOUT_DELAY_SEC.
    private static final int TIMEOUT_MS = 5_000;
    private static final ComponentName ADMIN_COMPONENT = new ComponentName(
            BasicAdminReceiver.class.getPackage().getName(), BasicAdminReceiver.class.getName());

    private KeyguardManager mKm;
    private DevicePolicyManager mDpm;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        final Context context = getInstrumentation().getContext();
        mDpm = (DevicePolicyManager) context.getSystemService(Context.DEVICE_POLICY_SERVICE);
        mKm = (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);
    }

    public void testSetWorkProfileTimeout() {
        assertProfileOwner();
        mDpm.setMaximumTimeToLock(ADMIN_COMPONENT, TIMEOUT_MS);
        assertEquals("Failed to set timeout",
                TIMEOUT_MS, mDpm.getMaximumTimeToLock(ADMIN_COMPONENT));
    }

    public void testDeviceLocked() {
        assertTrue("Device not locked", mKm.isDeviceLocked());
    }

    public void testDeviceNotLocked() {
        assertFalse("Device locked", mKm.isDeviceLocked());
    }

    private void assertProfileOwner() {
        assertTrue(mDpm.isProfileOwnerApp(ADMIN_COMPONENT.getPackageName()));
        assertTrue(mDpm.isManagedProfile(ADMIN_COMPONENT));
    }
}
