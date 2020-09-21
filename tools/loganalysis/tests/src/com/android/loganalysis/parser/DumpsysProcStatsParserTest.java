/*
 * Copyright (C) 2015 The Android Open Source Project
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

import com.android.loganalysis.item.DumpsysProcStatsItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link DumpsysProcStatsParser}
 */
public class DumpsysProcStatsParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testDumpsysProcStatsParser() {
        List<String> inputBlock = Arrays.asList(
                "COMMITTED STATS FROM 2015-03-20-02-01-02 (checked in):",
                "  * com.android.systemui / u0a22 / v22:",
                "           TOTAL: 100% (159MB-160MB-161MB/153MB-153MB-154MB over 13)",
                "      Persistent: 100% (159MB-160MB-161MB/153MB-153MB-154MB over 13)",
                "  * com.google.process.gapps / u0a9 / v22:",
                "           TOTAL: 100% (22MB-24MB-25MB/18MB-19MB-20MB over 13)",
                "          Imp Fg: 100% (22MB-24MB-25MB/18MB-19MB-20MB over 13)");

        DumpsysProcStatsItem procstats = new DumpsysProcStatsParser().parse(inputBlock);
        assertEquals("com.android.systemui", procstats.get("u0a22"));
        assertEquals("com.google.process.gapps", procstats.get("u0a9"));
    }
}

