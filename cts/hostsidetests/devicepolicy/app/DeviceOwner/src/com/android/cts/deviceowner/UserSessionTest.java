/*
 * Copyright (C) 2018 The Android Open Source Project
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

/** Test for user session APIs. */
public class UserSessionTest extends BaseDeviceOwnerTest {

    private static final String START_SESSION_MESSAGE = "Start session";
    private static final String END_SESSION_MESSAGE = "End session";

    public void testSetLogoutEnabled() {
        mDevicePolicyManager.setLogoutEnabled(getWho(), true);
        assertTrue(mDevicePolicyManager.isLogoutEnabled());
    }

    public void testSetLogoutDisabled() {
        mDevicePolicyManager.setLogoutEnabled(getWho(), false);
        assertFalse(mDevicePolicyManager.isLogoutEnabled());
    }

    public void testSetStartUserSessionMessage() {
        mDevicePolicyManager.setStartUserSessionMessage(getWho(), START_SESSION_MESSAGE);
        assertEquals(START_SESSION_MESSAGE,
                mDevicePolicyManager.getStartUserSessionMessage(getWho()));
    }

    public void testSetEndUserSessionMessage() {
        mDevicePolicyManager.setEndUserSessionMessage(getWho(), END_SESSION_MESSAGE);
        assertEquals(END_SESSION_MESSAGE, mDevicePolicyManager.getEndUserSessionMessage(getWho()));
    }

    public void testClearStartUserSessionMessage() {
        mDevicePolicyManager.setStartUserSessionMessage(getWho(), START_SESSION_MESSAGE);
        mDevicePolicyManager.setStartUserSessionMessage(getWho(), null);
        assertNull(mDevicePolicyManager.getStartUserSessionMessage(getWho()));
    }


    public void testClearEndUserSessionMessage() {
        mDevicePolicyManager.setEndUserSessionMessage(getWho(), END_SESSION_MESSAGE);
        mDevicePolicyManager.setEndUserSessionMessage(getWho(), null);
        assertNull(mDevicePolicyManager.getEndUserSessionMessage(getWho()));
    }
}
