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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.AppVersionItem;
import com.android.loganalysis.item.DumpsysPackageStatsItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/** Unit tests for {@link DumpsysPackageStatsParser} */
public class DumpsysPackageStatsParserTest extends TestCase {

    /** Test that normal input is parsed. */
    public void testDumpsysPackageStatsParser() {
        List<String> inputBlock =
                Arrays.asList(
                        "DUMP OF SERVICE package:",
                        "Package [com.google.android.calculator] (e075c9d):",
                        "  userId=10071",
                        "  secondaryCpuAbi=null",
                        "  versionCode=73000302 minSdk=10000 targetSdk=10000",
                        "  versionName=7.3 (3821978)",
                        "  splits=[base]",
                        " Package [com.google.android.googlequicksearchbox] (607929e):",
                        "  userId=10037",
                        "  pkg=Package{af43294 com.google.android.googlequicksearchbox}",
                        "  versionCode=300734793 minSdk=10000 targetSdk=10000",
                        "  versionName=6.16.35.26.arm64",
                        "  apkSigningVersion=2");

        final DumpsysPackageStatsItem packagestats =
                new DumpsysPackageStatsParser().parse(inputBlock);
        assertEquals(2, packagestats.size());
        assertNotNull(packagestats.get("com.google.android.calculator"));
        final AppVersionItem calculator = packagestats.get("com.google.android.calculator");
        assertEquals(73000302, calculator.getVersionCode());
        assertEquals("7.3 (3821978)", calculator.getVersionName());
        assertNotNull(packagestats.get("com.google.android.googlequicksearchbox"));
        final AppVersionItem googlequicksearchbox =
                packagestats.get("com.google.android.googlequicksearchbox");
        assertEquals(300734793, googlequicksearchbox.getVersionCode());
        assertEquals("6.16.35.26.arm64", googlequicksearchbox.getVersionName());
    }
}
