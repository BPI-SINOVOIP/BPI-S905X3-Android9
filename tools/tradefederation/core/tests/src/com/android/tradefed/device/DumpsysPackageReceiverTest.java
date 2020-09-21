/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.tradefed.device;

import junit.framework.TestCase;

/**
 * Unit tests for {@link DumpsysPackageReceiver}.
 */
public class DumpsysPackageReceiverTest extends TestCase {

    /**
     * Verifies parse correctly handles 'dumpsys package p' output from 4.2 and below.
     */
    public void testParse_classic() throws Exception {
        final String[] froyoPkgTxt = new String[] {"Packages:",
                "Package [com.android.soundrecorder] (462f6b38):",
                "targetSdk=8",
                "versionName=3.1.36 (88)",
                "pkgFlags=0x1 installStatus=1 enabled=0"};

        DumpsysPackageReceiver p = new DumpsysPackageReceiver();
        p.processNewLines(froyoPkgTxt);
        assertEquals("failed to parse package data", 1, p.getPackages().size());
        PackageInfo pkg = p.getPackages().get("com.android.soundrecorder");
        assertEquals("com.android.soundrecorder", pkg.getPackageName());
        assertTrue(pkg.isSystemApp());
        assertFalse(pkg.isUpdatedSystemApp());
        assertEquals("3.1.36 (88)", pkg.getVersionName());
    }

    /**
     * Verifies parse correctly handles new textual 'dumpsys package p' output from newer releases.
     */
    public void testParse_future() throws Exception {
        final String[] pkgTxt = new String[] {"Packages:",
        "Package [com.android.soundrecorder] (462f6b38):",
                "targetSdk=8",
                "pkgFlags=[ SYSTEM HAS_CODE ALLOW_CLEAR_USER_DATA ALLOW_BACKUP ]",
                "installed=true"};

        DumpsysPackageReceiver p = new DumpsysPackageReceiver();
        p.processNewLines(pkgTxt);
        assertEquals("failed to parse package data", 1, p.getPackages().size());
        PackageInfo pkg = p.getPackages().get("com.android.soundrecorder");
        assertNotNull("failed to parse package data", pkg);
        assertEquals("com.android.soundrecorder", pkg.getPackageName());
        assertTrue(pkg.isSystemApp());
        assertFalse(pkg.isUpdatedSystemApp());
    }

    /**
     * Verifies parse correctly handles 'dumpsys package p' output with hidden system package info
     */
    public void testParse_hidden() throws Exception {
        final String[] pkgsTxt = new String[] {"Packages:",
                "Package [com.android.soundrecorder] (462f6b38):",
                "targetSdk=8",
                "pkgFlags=[ SYSTEM HAS_CODE ALLOW_CLEAR_USER_DATA ALLOW_BACKUP ]",
                "installed=true",
                "Hidden system packages:",
                "  Package [com.android.soundrecorder] (686868):"};

        DumpsysPackageReceiver p = new DumpsysPackageReceiver();
        p.processNewLines(pkgsTxt);
        assertEquals("failed to parse package data", 1, p.getPackages().size());
        PackageInfo pkg = p.getPackages().get("com.android.soundrecorder");
        assertEquals("com.android.soundrecorder", pkg.getPackageName());
        assertTrue(pkg.isSystemApp());
        assertTrue(pkg.isUpdatedSystemApp());
    }

    /**
     * Verifies parse handles empty input
     */
    public void testParse_empty() {
        DumpsysPackageReceiver parser = new DumpsysPackageReceiver();
        assertEquals(0,  parser.getPackages().size());
    }
}
