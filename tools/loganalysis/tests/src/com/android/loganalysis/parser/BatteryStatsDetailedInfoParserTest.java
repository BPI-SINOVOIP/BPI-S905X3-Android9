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

import com.android.loganalysis.item.BatteryStatsDetailedInfoItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link BatteryStatsDetailedInfoParser}
 */
public class BatteryStatsDetailedInfoParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testBatteryStatsDetailedInfoParser() {
        List<String> inputBlock = Arrays.asList(
                " Time on battery: 2h 21m 5s 622ms (12.0%) realtime, 7m 54s 146ms (0.7%) uptime",
                " Time on battery screen off: 2h 5m 55s 3ms (1%) realtime, 7m 4s 5ms (7%) uptime",
                " Total run time: 19h 38m 43s 650ms realtime, 17h 25m 32s 175ms uptime",
                " All kernel wake locks:",
                " Kernel Wake lock PowerManagerService.WakeLocks: 1h 3m 50s 5ms (8 times) realtime",
                " Kernel Wake lock event0-2656 : 3m 49s 268ms (2399 times) realtime",
                " Kernel Wake lock wlan_wd_wake: 3m 34s 639ms (1751 times) realtime",
                " Kernel Wake lock wlan_rx_wake: 3m 19s 887ms (225 times) realtime",
                " Kernel Wake lock wlan_tx_wake: 2m 19s 887ms (225 times) realtime",
                " Kernel Wake lock tx_wake: 1m 19s 887ms (225 times) realtime",
                " ",
                " All partial wake locks:",
                " Wake lock u0a7 NlpWakeLock: 8m 13s 203ms (1479 times) realtime",
                " Wake lock u0a7 NlpCollectorWakeLock: 6m 29s 18ms (238 times) realtime",
                " Wake lock u0a7 GCM_CONN_ALARM: 6m 8s 587ms (239 times) realtime",
                " Wake lock 1000 *alarm*: 5m 11s 316ms (1469 times) realtime",
                " Wake lock u10 xxx: 4m 11s 316ms (1469 times) realtime",
                " Wake lock u30 cst: 2m 11s 316ms (1469 times) realtime",
                " ",
                " All wakeup reasons:",
                " Wakeup reason 200:qcom,smd-rpm:222:fc4: 11m 49s 332ms (0 times) realtime",
                " Wakeup reason 200:qcom,smd-rpm: 48s 45ms (0 times) realtime",
                " Wakeup reason 2:qcom,smd-rpm:2:f0.qm,mm:22:fc4mi: 3s 417ms (0 times) realtime",
                " Wakeup reason 188:qcom,smd-adsp:200:qcom,smd-rpm: 1s 656ms (0 times) realtime",
                " Wakeup reason 58:qcom,smsm-modem:2:qcom,smd-rpm: 6m 16s 1ms (5 times) realtime",
                " Wakeup reason 57:qcom,smd-modem:200:qcom,smd-rpm: 40s 995ms (0 times) realtime",
                " Wakeup reason unknown: 8s 455ms (0 times) realtime",
                " Wakeup reason 9:bcmsdh_sdmmc:2:qcomd-rpm:240:mso: 8m 5s 9ms (0 times) realtime",
                " ",
                " 0:",
                "    User activity: 2 other",
                "    Wake lock SCREEN_FROZEN realtime",
                "    Sensor 0: 9s 908ms realtime (1 times)",
                "    Sensor 1: 9s 997ms realtime (1 times)",
                "    Foreground for: 2h 21m 5s 622ms",
                "    Apk android:",
                "      24 wakeup alarms",
                " u0a9:",
                "    Mobile network: 8.1KB received, 1.6KB sent (packets 291 received, 342 sent)",
                "    Mobile radio active: 3m 43s 890ms (34.2%) 39x @ 354 mspp",
                "    Sensor 2: 12m 13s 15ms realtime (5 times)",
                "    Sensor 32: (not used)",
                "    Sensor 35: (not used)");

        BatteryStatsDetailedInfoItem stats = new BatteryStatsDetailedInfoParser().parse(inputBlock);

        assertEquals(8465622, stats.getTimeOnBattery());
        assertEquals(910619, stats.getScreenOnTime());
        assertNotNull(stats.getWakelockItem());
        assertNotNull(stats.getInterruptItem());
        assertNotNull(stats.getProcessUsageItem());
    }

    /**
     * Test with missing wakelock section
     */
    public void testMissingWakelockSection() {
        List<String> inputBlock = Arrays.asList(
                " Time on battery: 2h 21m 5s 622ms (12.0%) realtime, 7m 54s 146ms (0.7%) uptime",
                " Time on battery screen off: 2h 5m 55s 3ms (1%) realtime, 7m 4s 5ms (7%) uptime",
                " Total run time: 19h 38m 43s 650ms realtime, 17h 25m 32s 175ms uptime",
                " All wakeup reasons:",
                " Wakeup reason 200:qcom,smd-rpm:222:fc4: 11m 49s 332ms (0 times) realtime",
                " Wakeup reason 200:qcom,smd-rpm: 48s 45ms (0 times) realtime",
                " Wakeup reason 2:qcom,smd-rpm:2:f0.qm,mm:22:fc4mi: 3s 417ms (0 times) realtime",
                " Wakeup reason 188:qcom,smd-adsp:200:qcom,smd-rpm: 1s 656ms (0 times) realtime",
                " Wakeup reason 58:qcom,smsm-modem:2:qcom,smd-rpm: 6m 16s 1ms (5 times) realtime",
                " Wakeup reason 57:qcom,smd-modem:200:qcom,smd-rpm: 40s 995ms (0 times) realtime",
                " Wakeup reason unknown: 8s 455ms (0 times) realtime",
                " Wakeup reason 9:bcmsdh_sdmmc:2:qcomd-rpm:240:mso: 8m 5s 9ms (0 times) realtime",
                " ",
                " 0:",
                "    User activity: 2 other",
                "    Wake lock SCREEN_FROZEN realtime",
                "    Sensor 0: 9s 908ms realtime (1 times)",
                "    Sensor 1: 9s 997ms realtime (1 times)",
                "    Foreground for: 2h 21m 5s 622ms",
                "    Apk android:",
                "      24 wakeup alarms",
                " u0a9:",
                "    Mobile network: 8.1KB received, 1.6KB sent (packets 291 received, 342 sent)",
                "    Mobile radio active: 3m 43s 890ms (34.2%) 39x @ 354 mspp",
                "    Sensor 2: 12m 13s 15ms realtime (5 times)",
                "    Sensor 32: (not used)",
                "    Sensor 35: (not used)");
        BatteryStatsDetailedInfoItem stats = new BatteryStatsDetailedInfoParser().parse(inputBlock);

        assertEquals(8465622, stats.getTimeOnBattery());
        assertEquals(910619, stats.getScreenOnTime());

        assertNull(stats.getWakelockItem());

        assertNotNull(stats.getInterruptItem());
        assertNotNull(stats.getProcessUsageItem());
    }
}

