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

package com.android.cts.deviceandprofileowner;

import android.os.AsyncTask;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Test class that calls DPM.clearApplicationUserData and verifies that it doesn't time out.
 */
public class ClearApplicationDataTest extends BaseDeviceAdminTest {
    private static final String TEST_PKG = "com.android.cts.intent.receiver";
    private static final String DEVICE_ADMIN_PKG = "com.android.cts.deviceandprofileowner";
    private static final Semaphore mSemaphore = new Semaphore(0);
    private static final long CLEAR_APPLICATION_DATA_TIMEOUT_S = 10;

    public void testClearApplicationData_testPkg() throws Exception {
        clearApplicationDataTest(TEST_PKG, /* shouldSucceed */ true);
    }

    public void testClearApplicationData_deviceProvisioning() throws Exception {
        String deviceProvisioningPackageName = getDeviceProvisioningPackageName();
        if (deviceProvisioningPackageName == null) {
            return;
        }
        clearApplicationDataTest(deviceProvisioningPackageName, /* shouldSucceed */ false);
    }

    public void testClearApplicationData_activeAdmin() throws Exception {
        clearApplicationDataTest(DEVICE_ADMIN_PKG, /* shouldSucceed */ false);
    }

    private void clearApplicationDataTest(String packageName, boolean shouldSucceed)
            throws Exception {
        mDevicePolicyManager.clearApplicationUserData(ADMIN_RECEIVER_COMPONENT,
                packageName, AsyncTask.THREAD_POOL_EXECUTOR,
                (String pkg, boolean succeeded) -> {
                    assertEquals(packageName, pkg);
                    assertEquals(shouldSucceed, succeeded);
                    mSemaphore.release();
                });
        assertTrue("Clearing application data took too long",
                mSemaphore.tryAcquire(CLEAR_APPLICATION_DATA_TIMEOUT_S, TimeUnit.SECONDS));
    }

    private String getDeviceProvisioningPackageName() {
        final int provisioning_app_id = mContext.getResources().getIdentifier(
                "config_deviceProvisioningPackage", "string", "android");
        if (provisioning_app_id > 0) {
            return mContext.getResources().getString(provisioning_app_id);
        } else {
            return null;
        }
    }
}
