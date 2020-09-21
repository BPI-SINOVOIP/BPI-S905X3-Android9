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
package com.android.cts.deviceowner;

import android.app.Instrumentation;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;
import android.test.AndroidTestCase;

/**
 * Base class for device-owner based tests.
 *
 * This class handles making sure that the test is the device owner
 * and that it has an active admin registered, so that all tests may
 * assume these are done. The admin component can be accessed through
 * {@link #getWho()}.
 */
public abstract class BaseDeviceOwnerTest extends AndroidTestCase {

    protected DevicePolicyManager mDevicePolicyManager;
    protected Instrumentation mInstrumentation;
    protected UiDevice mDevice;
    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mDevice = UiDevice.getInstance(mInstrumentation);
        mDevicePolicyManager = mContext.getSystemService(DevicePolicyManager.class);
        assertDeviceOwner();
    }

    private void assertDeviceOwner() {
        assertNotNull(mDevicePolicyManager);
        assertTrue(mDevicePolicyManager.isAdminActive(getWho()));
        assertTrue(mDevicePolicyManager.isDeviceOwnerApp(mContext.getPackageName()));
        assertFalse(mDevicePolicyManager.isManagedProfile(getWho()));
    }

    protected ComponentName getWho() {
        return BasicAdminReceiver.getComponentName(mContext);
    }

    protected String executeShellCommand(String... command) throws Exception {
        return mDevice.executeShellCommand(String.join(" ", command));
    }
}
