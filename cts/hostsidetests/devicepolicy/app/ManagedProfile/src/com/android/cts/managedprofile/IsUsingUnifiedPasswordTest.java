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

/**
 * Test to check reporting the state of profile challenge.
 */
public class IsUsingUnifiedPasswordTest extends BaseManagedProfileTest {

    public void testNotUsingUnifiedPassword() {
        assertFalse(mDevicePolicyManager.isUsingUnifiedPassword(ADMIN_RECEIVER_COMPONENT));
    }

    public void testUsingUnifiedPassword() {
        assertTrue(mDevicePolicyManager.isUsingUnifiedPassword(ADMIN_RECEIVER_COMPONENT));
    }
}
