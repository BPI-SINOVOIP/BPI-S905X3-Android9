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
package com.android.cts.devicepolicy;

/**
 * Tests the DPC transfer functionality for device owner. Testing is done by having two dummy DPCs,
 * CtsTransferOwnerOutgoingApp and CtsTransferOwnerIncomingApp. The former is the current DPC
 * and the latter will be the new DPC after transfer. In order to run the tests from the correct
 * process, first we setup some policies in the client side in CtsTransferOwnerOutgoingApp and then
 * we verify the policies are still there in CtsTransferOwnerIncomingApp.
 */
public class MixedDeviceOwnerHostSideTransferTest extends
        DeviceAndProfileOwnerHostSideTransferTest {
    private static final String TRANSFER_DEVICE_OWNER_OUTGOING_TEST =
            "com.android.cts.transferowner.TransferDeviceOwnerOutgoingTest";
    private static final String TRANSFER_DEVICE_OWNER_INCOMING_TEST =
            "com.android.cts.transferowner.TransferDeviceOwnerIncomingTest";
    public static final String TRANSFER_PROFILE_OWNER_OUTGOING_TEST =
            "com.android.cts.transferowner.TransferProfileOwnerOutgoingTest";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        if (mHasFeature) {
            installAppAsUser(TRANSFER_OWNER_OUTGOING_APK, mPrimaryUserId);
            if (setDeviceOwner(TRANSFER_OWNER_OUTGOING_TEST_RECEIVER, mPrimaryUserId,
                    false)) {
                setupTestParameters(mPrimaryUserId, TRANSFER_DEVICE_OWNER_OUTGOING_TEST,
                        TRANSFER_DEVICE_OWNER_INCOMING_TEST);
                installAppAsUser(TRANSFER_OWNER_INCOMING_APK, mUserId);
            } else {
                removeAdmin(TRANSFER_OWNER_OUTGOING_TEST_RECEIVER, mUserId);
                getDevice().uninstallPackage(TRANSFER_OWNER_OUTGOING_PKG);
                fail("Failed to set device owner");
            }
        }
    }

    public void testTransferAffiliatedProfileOwnershipCompleteCallback() throws Exception {
        if (!mHasFeature || !hasDeviceFeature("android.software.managed_users")) {
            return;
        }
        final int profileUserId = setupManagedProfileOnDeviceOwner(TRANSFER_OWNER_OUTGOING_APK,
                TRANSFER_OWNER_OUTGOING_TEST_RECEIVER);

        setSameAffiliationId(profileUserId, mOutgoingTestClassName);

        installAppAsUser(TRANSFER_OWNER_INCOMING_APK, profileUserId);

        runDeviceTestsAsUser(TRANSFER_OWNER_OUTGOING_PKG,
                TRANSFER_PROFILE_OWNER_OUTGOING_TEST,
                "testTransferOwnership", profileUserId);

        waitForBroadcastIdle();

        runDeviceTestsAsUser(TRANSFER_OWNER_OUTGOING_PKG,
                mOutgoingTestClassName,
                "testTransferAffiliatedProfileOwnershipCompleteCallbackIsCalled",
                mUserId);
    }

    public void testTransferAffiliatedProfileOwnershipInComp() throws Exception {
        if (!mHasFeature || !hasDeviceFeature("android.software.managed_users")) {
            return;
        }
        final int profileUserId = setupManagedProfileOnDeviceOwner(TRANSFER_OWNER_OUTGOING_APK,
                TRANSFER_OWNER_OUTGOING_TEST_RECEIVER);

        setSameAffiliationId(profileUserId, mOutgoingTestClassName);

        installAppAsUser(TRANSFER_OWNER_INCOMING_APK, profileUserId);

        runDeviceTestsAsUser(TRANSFER_OWNER_OUTGOING_PKG,
                TRANSFER_PROFILE_OWNER_OUTGOING_TEST,
                "testTransferOwnership", profileUserId);
        waitForBroadcastIdle();

        runDeviceTestsAsUser(TRANSFER_OWNER_OUTGOING_PKG,
                mOutgoingTestClassName,
                "testTransferOwnership", mUserId);
        waitForBroadcastIdle();

        assertAffiliationIdsAreIntact(profileUserId, mIncomingTestClassName);

        runDeviceTestsAsUser(TRANSFER_OWNER_OUTGOING_PKG,
                mOutgoingTestClassName,
                "testTransferAffiliatedProfileOwnershipCompleteCallbackIsCalled",
                mUserId);
    }

    private void assertAffiliationIdsAreIntact(int profileUserId,
            String testClassName) throws Exception {
        runDeviceTestsAsUser(TRANSFER_OWNER_INCOMING_PKG,
                testClassName,
                "testIsAffiliationId1", mPrimaryUserId);
        runDeviceTestsAsUser(TRANSFER_OWNER_INCOMING_PKG,
                testClassName,
                "testIsAffiliationId1", profileUserId);
    }

    private void setSameAffiliationId(int profileUserId, String testClassName)
            throws Exception {
        runDeviceTestsAsUser(TRANSFER_OWNER_OUTGOING_PKG,
                testClassName,
                "testSetAffiliationId1", mPrimaryUserId);
        runDeviceTestsAsUser(TRANSFER_OWNER_OUTGOING_PKG,
                testClassName,
                "testSetAffiliationId1", profileUserId);
    }
}
