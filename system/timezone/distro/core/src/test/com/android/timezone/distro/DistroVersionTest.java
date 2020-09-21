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

package com.android.timezone.distro;

import junit.framework.TestCase;

public class DistroVersionTest extends TestCase {

    private static final int INVALID_VERSION_LOW = -1;
    private static final int VALID_VERSION = 23;
    private static final int INVALID_VERSION_HIGH = 1000;
    private static final String VALID_RULES_VERSION = "2016a";
    private static final String INVALID_RULES_VERSION = "A016a";

    public void testConstructorValidation() throws Exception {
        checkConstructorThrows(
                INVALID_VERSION_LOW, VALID_VERSION, VALID_RULES_VERSION, VALID_VERSION);
        checkConstructorThrows(
                INVALID_VERSION_HIGH, VALID_VERSION, VALID_RULES_VERSION, VALID_VERSION);
        checkConstructorThrows(
                VALID_VERSION, INVALID_VERSION_LOW, VALID_RULES_VERSION, VALID_VERSION);
        checkConstructorThrows(
                VALID_VERSION, INVALID_VERSION_HIGH, VALID_RULES_VERSION, VALID_VERSION);
        checkConstructorThrows(VALID_VERSION, VALID_VERSION, INVALID_RULES_VERSION, VALID_VERSION);
        checkConstructorThrows(VALID_VERSION, VALID_VERSION, VALID_RULES_VERSION,
                INVALID_VERSION_LOW);
        checkConstructorThrows(VALID_VERSION, VALID_VERSION, VALID_RULES_VERSION,
                INVALID_VERSION_HIGH);
    }

    private static void checkConstructorThrows(
            int majorVersion, int minorVersion, String rulesVersion, int revision) {
        try {
            new DistroVersion(majorVersion, minorVersion, rulesVersion, revision);
            fail();
        } catch (DistroException expected) {}
    }

    public void testConstructor() throws Exception {
        DistroVersion distroVersion = new DistroVersion(1, 2, VALID_RULES_VERSION, 3);
        assertEquals(1, distroVersion.formatMajorVersion);
        assertEquals(2, distroVersion.formatMinorVersion);
        assertEquals(VALID_RULES_VERSION, distroVersion.rulesVersion);
        assertEquals(3, distroVersion.revision);
    }

    public void testToFromBytesRoundTrip() throws Exception {
        DistroVersion distroVersion = new DistroVersion(1, 2, VALID_RULES_VERSION, 3);
        assertEquals(distroVersion, DistroVersion.fromBytes(distroVersion.toBytes()));
    }

    public void testIsCompatibleWithThisDevice() throws Exception {
        DistroVersion exactMatch = createDistroVersion(
                DistroVersion.CURRENT_FORMAT_MAJOR_VERSION,
                DistroVersion.CURRENT_FORMAT_MINOR_VERSION);
        assertTrue(DistroVersion.isCompatibleWithThisDevice(exactMatch));

        DistroVersion newerMajor = createDistroVersion(
                DistroVersion.CURRENT_FORMAT_MAJOR_VERSION + 1,
                DistroVersion.CURRENT_FORMAT_MINOR_VERSION);
        assertFalse(DistroVersion.isCompatibleWithThisDevice(newerMajor));

        DistroVersion newerMinor = createDistroVersion(
                DistroVersion.CURRENT_FORMAT_MAJOR_VERSION,
                DistroVersion.CURRENT_FORMAT_MINOR_VERSION + 1);
        assertTrue(DistroVersion.isCompatibleWithThisDevice(newerMinor));

        // The constant versions should never be below 1. We allow 0 but want to start version
        // numbers at 1 to allow testing of older version logic.
        assertTrue(DistroVersion.CURRENT_FORMAT_MAJOR_VERSION >= 1);
        assertTrue(DistroVersion.CURRENT_FORMAT_MINOR_VERSION >= 1);

        DistroVersion olderMajor = createDistroVersion(
                DistroVersion.CURRENT_FORMAT_MAJOR_VERSION - 1,
                DistroVersion.CURRENT_FORMAT_MINOR_VERSION);
        assertFalse(DistroVersion.isCompatibleWithThisDevice(olderMajor));

        DistroVersion olderMinor = createDistroVersion(
                DistroVersion.CURRENT_FORMAT_MAJOR_VERSION,
                DistroVersion.CURRENT_FORMAT_MINOR_VERSION - 1);
        assertFalse(DistroVersion.isCompatibleWithThisDevice(olderMinor));
    }

    private DistroVersion createDistroVersion(int majorFormatVersion, int minorFormatVersion)
            throws DistroException {
        return new DistroVersion(majorFormatVersion, minorFormatVersion, VALID_RULES_VERSION, 3);
    }
}
