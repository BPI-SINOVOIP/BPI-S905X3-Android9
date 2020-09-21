/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.loganalysis.item.ProcrankItem;
import com.android.loganalysis.util.ArrayUtil;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link ProcrankParser}
 */
public class ProcrankParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testProcRankParserShortLine() {
        List<String> inputBlock = Arrays.asList(
                "  PID      Vss      Rss      Pss      Uss  cmdline",
                "  178   87136K   81684K   52829K   50012K  system_server",
                " 1313   78128K   77996K   48603K   45812K  com.google.android.apps.maps",
                " 3247   61652K   61492K   33122K   30972K  com.android.browser",
                "  334   55740K   55572K   29629K   28360K  com.android.launcher",
                " 2072   51348K   51172K   24263K   22812K  android.process.acore",
                " 1236   51440K   51312K   22911K   20608K  com.android.settings",
                "                 51312K   22911K   20608K  invalid.format",
                "                          ------   ------  ------",
                "                          203624K  163604K  TOTAL",
                "RAM: 731448K total, 415804K free, 9016K buffers, 108548K cached",
                "[procrank: 1.6s elapsed]");

        ProcrankItem procrank = new ProcrankParser().parse(inputBlock);

        // Ensures that only valid lines are parsed. Only 6 of the 11 lines under the header are
        // valid.
        assertEquals(6, procrank.getPids().size());

        // Make sure all expected rows are present, and do a diagonal check of values
        assertEquals((Integer) 87136, procrank.getVss(178));
        assertEquals((Integer) 77996, procrank.getRss(1313));
        assertEquals((Integer) 33122, procrank.getPss(3247));
        assertEquals((Integer) 28360, procrank.getUss(334));
        assertEquals("android.process.acore", procrank.getProcessName(2072));
        assertEquals(ArrayUtil.join("\n", inputBlock), procrank.getText());
    }

    /**
     * Test that normal input is parsed.
     */
    public void testProcRankParserLongLine() {
        List<String> inputBlock = Arrays.asList(
                "  PID       Vss      Rss      Pss      Uss     Swap    PSwap    USwap    ZSwap  cmdline",
                " 6711  3454396K  146300K  108431K  105524K   31540K   20522K   20188K    4546K  com.google.android.GoogleCamera",
                " 1515  2535920K  131984K   93750K   89440K   42676K   31792K   31460K    7043K  system_server",
                "19906  2439540K  130228K   85418K   69296K   11680K     353K       0K      78K  com.android.chrome:sandboxed_process10",
                "13790  2596308K  124424K   75673K   69680K   11336K     334K       0K      74K  com.google.android.youtube",
                " 9288  2437704K  119496K   74288K   69532K   11344K     334K       0K      74K  com.google.android.videos",
                "                           51312K   22911K   20608K       0K       0K       0K  invalid.format",
                "                           ------   ------   ------   ------   ------   ------  ------",
                "                         1061237K  940460K  619796K  225468K  201688K   49950K  TOTAL",
                "ZRAM: 52892K physical used for 238748K in swap (520908K total swap)",
                "RAM: 1857348K total, 51980K free, 3780K buffers, 456272K cached, 29220K shmem, 97560K slab",
                "[/system/xbin/su: 3.260s elapsed]");

        ProcrankItem procrank = new ProcrankParser().parse(inputBlock);

        // Ensures that only valid lines are parsed. Only 6 of the 11 lines under the header are
        // valid.
        assertEquals(5, procrank.getPids().size());

        // Make sure all expected rows are present, and do a diagonal check of values
        assertEquals((Integer) 3454396, procrank.getVss(6711));
        assertEquals((Integer) 146300, procrank.getRss(6711));
        assertEquals((Integer) 108431, procrank.getPss(6711));
        assertEquals((Integer) 105524, procrank.getUss(6711));
        assertEquals("com.google.android.GoogleCamera", procrank.getProcessName(6711));
        assertEquals(ArrayUtil.join("\n", inputBlock), procrank.getText());
    }

    /**
     * Test that an empty input returns {@code null}.
     */
    public void testEmptyInput() {
        ProcrankItem item = new ProcrankParser().parse(Arrays.asList(""));
        assertNull(item);
    }
}

