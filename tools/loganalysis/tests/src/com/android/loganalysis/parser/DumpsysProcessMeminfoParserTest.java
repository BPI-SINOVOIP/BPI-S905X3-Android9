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
package com.android.loganalysis.parser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.android.loganalysis.item.DumpsysProcessMeminfoItem;

import org.junit.Test;

import java.util.Arrays;
import java.util.List;

/** Unit tests for {@link DumpsysProcessMeminfoParser} */
public class DumpsysProcessMeminfoParserTest {
    private static final String TEST_INPUT =
            "time,28506638,177086152\n"
                    + "4,938,system_server,11,22,N/A,44,0,0,N/A,0,0,0,N/A,0,27613,14013,176602,"
                    + "218228,0,0,122860,122860,1512,1412,5740,8664,0,0,154924,154924,27568,"
                    + "13972,11916,53456,0,0,123008,123008,0,0,0,0,0,0,0,0,Dalvik Other,3662,0,"
                    + "104,0,3660,0,0,0,Stack,1576,0,8,0,1576,0,0,0,Cursor,0,0,0,0,0,0,0,0,"
                    + "Ashmem,156,0,20,0,148,0,0,0,Gfx dev,100,0,48,0,76,0,0,0,Other dev,116,0,"
                    + "164,0,0,96,0,0,.so mmap,7500,2680,3984,21864,904,2680,0,0,.jar mmap,0,0,0,"
                    + "0,0,0,0,0,.apk mmap,72398,71448,0,11736,0,71448,0,0,.ttf mmap,0,0,0,0,0,0,"
                    + "0,0,.dex mmap,76874,46000,0,83644,40,46000,0,0,.oat mmap,8127,2684,64,"
                    + "26652,0,2684,0,0,.art mmap,1991,48,972,10004,1544,48,0,0,Other mmap,137,0,"
                    + "44,1024,4,52,0,0,EGL mtrack,0,0,0,0,0,0,0,0,GL mtrack,111,222,333,444,555,"
                    + "666,777,888,";

    private static final String INVALID_TEST_INPUT = "RANDOM,TEST,DATA\n234235345345";

    // Test that normal input is parsed
    @Test
    public void testDumpsysProcessMeminfoParser() {
        List<String> inputBlock = Arrays.asList(TEST_INPUT.split("\n"));
        DumpsysProcessMeminfoItem dump = new DumpsysProcessMeminfoParser().parse(inputBlock);
        assertEquals(938, dump.getPid());
        assertEquals("system_server", dump.getProcessName());
        assertEquals(
                Long.valueOf(11L),
                dump.get(DumpsysProcessMeminfoItem.NATIVE).get(DumpsysProcessMeminfoItem.MAX));
        assertEquals(
                Long.valueOf(22L),
                dump.get(DumpsysProcessMeminfoItem.DALVIK).get(DumpsysProcessMeminfoItem.MAX));
        assertFalse(
                dump.get(DumpsysProcessMeminfoItem.OTHER)
                        .containsKey(DumpsysProcessMeminfoItem.MAX));
        assertEquals(
                Long.valueOf(44L),
                dump.get(DumpsysProcessMeminfoItem.TOTAL).get(DumpsysProcessMeminfoItem.MAX));
        assertEquals(
                Long.valueOf(218228L),
                dump.get(DumpsysProcessMeminfoItem.TOTAL).get(DumpsysProcessMeminfoItem.PSS));
        assertEquals(
                Long.valueOf(3662L), dump.get("Dalvik Other").get(DumpsysProcessMeminfoItem.PSS));
        assertEquals(Long.valueOf(111L), dump.get("GL mtrack").get(DumpsysProcessMeminfoItem.PSS));
        assertEquals(
                Long.valueOf(222L),
                dump.get("GL mtrack").get(DumpsysProcessMeminfoItem.SWAPPABLE_PSS));
        assertEquals(
                Long.valueOf(333L),
                dump.get("GL mtrack").get(DumpsysProcessMeminfoItem.SHARED_DIRTY));
        assertEquals(
                Long.valueOf(444L),
                dump.get("GL mtrack").get(DumpsysProcessMeminfoItem.SHARED_CLEAN));
        assertEquals(
                Long.valueOf(555L),
                dump.get("GL mtrack").get(DumpsysProcessMeminfoItem.PRIVATE_DIRTY));
        assertEquals(
                Long.valueOf(666L),
                dump.get("GL mtrack").get(DumpsysProcessMeminfoItem.PRIVATE_CLEAN));
        assertEquals(
                Long.valueOf(777L),
                dump.get("GL mtrack").get(DumpsysProcessMeminfoItem.SWAPPED_OUT));
        assertEquals(
                Long.valueOf(888L),
                dump.get("GL mtrack").get(DumpsysProcessMeminfoItem.SWAPPED_OUT_PSS));
    }

    // Test that the parser does not crash on invalid input and returns empty data
    @Test
    public void testDumpsysProcessMeminfoParserInvalid() {
        List<String> inputBlock = Arrays.asList(INVALID_TEST_INPUT.split("\n"));
        DumpsysProcessMeminfoItem dump = new DumpsysProcessMeminfoParser().parse(inputBlock);
        assertNull(dump.getProcessName());
        assertTrue(dump.get(DumpsysProcessMeminfoItem.TOTAL).isEmpty());
    }
}
