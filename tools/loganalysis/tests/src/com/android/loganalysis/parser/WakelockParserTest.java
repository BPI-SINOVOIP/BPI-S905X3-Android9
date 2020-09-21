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

import com.android.loganalysis.item.WakelockItem;
import com.android.loganalysis.item.WakelockItem.WakeLockCategory;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link WakelockParser}
 */
public class WakelockParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testKernelWakelockParser() {
        List<String> inputBlock = Arrays.asList(
               " All kernel wake locks:",
               " Kernel Wake lock PowerManagerService.WakeLocks: 1h 3m 50s 5ms (8 times) realtime",
               " Kernel Wake lock event0-2656 : 3m 49s 268ms (2399 times) realtime",
               " Kernel Wake lock wlan_wd_wake: 3m 34s 639ms (1751 times) realtime",
               " Kernel Wake lock wlan_rx_wake: 3m 19s 887ms (225 times) realtime",
               " Kernel Wake lock wlan_tx_wake: 2m 19s 887ms (225 times) realtime",
               " Kernel Wake lock tx_wake: 1m 19s 887ms (225 times) realtime",
               " "
              );

        WakelockItem wakelock = new WakelockParser().parse(inputBlock);

        assertEquals(WakelockParser.TOP_WAKELOCK_COUNT,
                wakelock.getWakeLocks(WakeLockCategory.KERNEL_WAKELOCK).size());
        assertEquals("event0-2656 ",
                wakelock.getWakeLocks(WakeLockCategory.KERNEL_WAKELOCK).get(0).getName());
        assertEquals(229268, wakelock.getWakeLocks(WakeLockCategory.KERNEL_WAKELOCK).
                get(0).getHeldTime());
        assertEquals(2399, wakelock.getWakeLocks(WakeLockCategory.KERNEL_WAKELOCK).
                get(0).getLockedCount());
    }

    public void testPartialWakelockParser() {
        List<String> inputBlock = Arrays.asList(
                " All partial wake locks:",
                " Wake lock u0a7 NlpWakeLock: 8m 13s 203ms (1479 times) max=0 realtime",
                " Wake lock u0a7 NlpCollectorWakeLock: 6m 29s 18ms (238 times) max=0 realtime",
                " Wake lock u0a7 GCM_CONN_ALARM: 6m 8s 587ms (239 times) max=0 realtime",
                " Wake lock 1000 *alarm*: 5m 11s 316ms (1469 times) max=0 realtime",
                " Wake lock u10 xxx: 4m 11s 316ms (1469 times) max=0 realtime",
                " Wake lock u30 cst: 2m 11s 316ms (1469 times) max=0 realtime",
                "");

        WakelockItem wakelock = new WakelockParser().parse(inputBlock);

        assertEquals(WakelockParser.TOP_WAKELOCK_COUNT,
                wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).size());
        assertEquals("NlpWakeLock", wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).
                get(0).getName());
        assertEquals("u0a7", wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).
                get(0).getProcessUID());
        assertEquals(493203, wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).
                get(0).getHeldTime());
        assertEquals(1479, wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).
                get(0).getLockedCount());
    }

    public void testPartialWakelockParserOnOldFormat() {
        List<String> inputBlock = Arrays.asList(
                " All partial wake locks:",
                " Wake lock u0a7 NlpWakeLock: 8m 13s 203ms (1479 times) realtime",
                " Wake lock u0a7 NlpCollectorWakeLock: 6m 29s 18ms (238 times) realtime",
                " Wake lock u0a7 GCM_CONN_ALARM: 6m 8s 587ms (239 times) realtime",
                " Wake lock 1000 *alarm*: 5m 11s 316ms (1469 times) realtime",
                " Wake lock u10 xxx: 4m 11s 316ms (1469 times) realtime",
                " Wake lock u30 cst: 2m 11s 316ms (1469 times) realtime",
                "");

        WakelockItem wakelock = new WakelockParser().parse(inputBlock);

        assertEquals(WakelockParser.TOP_WAKELOCK_COUNT,
                wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).size());
        assertEquals("NlpWakeLock", wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).
                get(0).getName());
        assertEquals("u0a7", wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).
                get(0).getProcessUID());
        assertEquals(493203, wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).
                get(0).getHeldTime());
        assertEquals(1479, wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).
                get(0).getLockedCount());
    }

    public void testInvalidInputWakelockParser() {
        List<String> inputBlock = Arrays.asList(
             " lock PowerManagerService.WakeLocks: 1h 3m 50s 5ms (8 times) realtime",
             " lock event0-2656 : 3m 49s 268ms (2399 times) realtime",
             " lock wlan_wd_wake: 3m 34s 639ms (1751 times) realtime",
             " lock wlan_rx_wake: 3m 19s 887ms (225 times) realtime",
             " wlan_tx_wake: 2m 19s 887ms (225 times) realtime",
             " tx_wake: 1m 19s 887ms (225 times) realtime",
             " "
            );

        WakelockItem wakelock = new WakelockParser().parse(inputBlock);

        assertEquals(0,
                wakelock.getWakeLocks(WakeLockCategory.KERNEL_WAKELOCK).size());
        assertEquals(0,
                wakelock.getWakeLocks(WakeLockCategory.PARTIAL_WAKELOCK).size());
    }
}

