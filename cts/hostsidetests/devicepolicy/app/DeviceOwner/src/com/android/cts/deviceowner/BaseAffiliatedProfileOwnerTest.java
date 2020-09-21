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
package com.android.cts.deviceowner;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.test.AndroidTestCase;

/**
 * Base class for affiliated profile-owner based tests.
 *
 * This class handles making sure that the test is the affiliated profile owner and that it has an
 * active admin registered, so that all tests may assume these are done. The admin component can be
 * accessed through {@link #getWho()}.
 */
public abstract class BaseAffiliatedProfileOwnerTest extends AndroidTestCase {

    protected DevicePolicyManager mDevicePolicyManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mDevicePolicyManager = mContext.getSystemService(DevicePolicyManager.class);
        assertDeviceOrAffiliatedProfileOwner();
    }

    private void assertDeviceOrAffiliatedProfileOwner() {
        assertNotNull(mDevicePolicyManager);
        assertTrue(mDevicePolicyManager.isAdminActive(getWho()));
        boolean isDeviceOwner = mDevicePolicyManager.isDeviceOwnerApp(mContext.getPackageName());
        boolean isAffiliatedProfileOwner = mDevicePolicyManager.isProfileOwnerApp(
                mContext.getPackageName())
                && mDevicePolicyManager.isAffiliatedUser();
        assertTrue(isDeviceOwner || isAffiliatedProfileOwner);
    }

    protected ComponentName getWho() {
        return BasicAdminReceiver.getComponentName(mContext);
    }
}
