/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.cts.devicepolicy;

import com.android.tradefed.device.DeviceNotAvailableException;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

/**
 * Base class for DeviceAdmin host side tests.
 *
 * We have subclasses for device admins for different API levels.
 */
public abstract class BaseDeviceAdminHostSideTest extends BaseDevicePolicyTest {

    protected int mUserId;

    protected abstract int getTargetApiVersion();

    protected final String getDeviceAdminApkFileName() {
        return DeviceAdminHelper.getDeviceAdminApkFileName(getTargetApiVersion());
    }

    protected final String getDeviceAdminApkPackage() {
        return DeviceAdminHelper.getDeviceAdminApkPackage(getTargetApiVersion());
    }

    protected final String getAdminReceiverComponent() {
        return DeviceAdminHelper.getAdminReceiverComponent(getTargetApiVersion());
    }

    protected final String getUnprotectedAdminReceiverComponent() {
        return DeviceAdminHelper.getUnprotectedAdminReceiverComponent(getTargetApiVersion());
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mUserId = mPrimaryUserId;

        if (mHasFeature) {
            installAppAsUser(getDeviceAdminApkFileName(), mUserId);
            setDeviceAdmin(getAdminReceiverComponent(), mUserId);
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (mHasFeature) {
            assertTrue("Failed to remove admin", removeAdmin(getAdminReceiverComponent(), mUserId));
            getDevice().uninstallPackage(getDeviceAdminApkPackage());
        }

        super.tearDown();
    }

    protected void runTests(@Nonnull String apk, @Nonnull String className,
            @Nullable String method) throws DeviceNotAvailableException {
        runDeviceTestsAsUser(apk,
                DeviceAdminHelper.getDeviceAdminJavaPackage() + "." + className, method, mUserId);
    }

    protected void runTests(@Nonnull String apk, @Nonnull String className)
            throws DeviceNotAvailableException {
        runTests(apk, className, null);
    }

    /**
     * Run all tests in DeviceAdminTest.java (as device admin).
     */
    public void testRunDeviceAdminTest() throws Exception {
        if (!mHasFeature) {
            return;
        }
        runTests(getDeviceAdminApkPackage(), "DeviceAdminTest");
    }

    public void testResetPassword_nycRestrictions() throws Exception {
        if (!mHasFeature) {
            return;
        }

        try {
            runTests(getDeviceAdminApkPackage(), "DeviceAdminPasswordTest",
                            "testResetPassword_nycRestrictions");
        } finally {
            changeUserCredential(null, "1234", 0);
        }
    }

    private void clearPasswordForDeviceOwner() throws Exception {
        runTests(getDeviceAdminApkPackage(), "ClearPasswordTest");
    }

    /**
     * Run the tests in DeviceOwnerPasswordTest.java (as device owner).
     */
    public void testRunDeviceOwnerPasswordTest() throws Exception {
        if (!mHasFeature) {
            return;
        }

        setDeviceOwner(getAdminReceiverComponent(), mUserId, /*expectFailure*/ false);
        try {
            runTests(getDeviceAdminApkPackage(), "DeviceOwnerPasswordTest");
        } finally {
            clearPasswordForDeviceOwner();
        }
    }
}
