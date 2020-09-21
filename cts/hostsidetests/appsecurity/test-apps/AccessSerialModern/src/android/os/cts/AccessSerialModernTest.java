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

package android.os.cts;

import static junit.framework.Assert.assertTrue;
import static junit.framework.Assert.fail;

import android.os.Build;
import android.support.test.InstrumentationRegistry;

import org.junit.Test;

/**
 * Test that legacy apps can access the device serial without the phone permission.
 */
public class AccessSerialModernTest {
    @Test
    public void testAccessSerialPermissionNeeded() throws Exception {
        // Build.SERIAL should not provide the device serial for modern apps.
        // We don't know the serial but know that it should be the dummy
        // value returned to unauthorized callers, so make sure that value
        assertTrue("Build.SERIAL must not work for modern apps",
                Build.UNKNOWN.equals(Build.SERIAL));

        // We don't have the read phone state permission, so this should throw
        try {
            Build.getSerial();
            fail("getSerial() must be gated on the READ_PHONE_STATE permission");
        } catch (SecurityException e) {
            /* expected */
        }

        // Now grant ourselves READ_PHONE_STATE
        grantReadPhoneStatePermission();

        // Build.SERIAL should not provide the device serial for modern apps.
        assertTrue("Build.SERIAL must not work for modern apps",
                Build.UNKNOWN.equals(Build.SERIAL));

        // We have the READ_PHONE_STATE permission, so this should not throw
        try {
            assertTrue("Build.getSerial() must work for apps holding READ_PHONE_STATE",
                    !Build.UNKNOWN.equals(Build.getSerial()));
        } catch (SecurityException e) {
            fail("getSerial() must be gated on the READ_PHONE_STATE permission");
        }
    }

    private void grantReadPhoneStatePermission() {
        InstrumentationRegistry.getInstrumentation().getUiAutomation().grantRuntimePermission(
                InstrumentationRegistry.getContext().getPackageName(),
                android.Manifest.permission.READ_PHONE_STATE);
    }
}
