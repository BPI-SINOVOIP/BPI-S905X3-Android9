/*
 * Copyright (C) 2016 The Android Open Source Project
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

import com.android.loganalysis.item.DumpsysWifiStatsItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link DumpsysWifiStatsParser}
 */
public class DumpsysWifiStatsParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testDumpsysWifiStatsParser() {
        List<String> inputBlock = Arrays.asList(
                "Wi-Fi is enabled",
                "rec[0]: time=10-09 00:25:16.737 processed=DefaultState org=DeviceActiveState "
                + "dest=<null> what=155654(0x26006)",
                "mScreenOff true",
                " rec[0]: time=10-08 16:49:55.034 processed=WatchdogEnabledState org=OnlineState "
                + "dest=OnlineWatchState what=135170(0x21002)",
                "rec[30]: time=10-08 13:06:50.379 processed=DriverStartedState org=ConnectedState "
                + "dest=<null> what=131143(0x20047) (1)CMD_START_SCAN  rt=4720806/7884852 10013 51 "
                + "ic=0 proc(ms):14 onGoing full started:1444334810368,11 cnt=24 rssi=-127 f=-1 "
                + "sc=0 link=-1 tx=1.5, 1.7, 0.0  rx=8.4 fiv=20000 [on:3266 tx:65 rx:556 "
                + "period:1268] from screen [on:266962 period:266959]",
                "rec[357]: time=10-08 13:10:13.199 processed=DriverStartedState org=ConnectedState "
                + "dest=<null> what=131143(0x20047) (-2)CMD_START_SCAN  rt=4923625/8087671 10013 "
                + "175 ic=0 proc(ms):2 onGoing full started:1444335011199,1999 cnt=24 rssi=-127 "
                + "f=-1 sc=0 link=-1 tx=8.4, 1.7, 0.0  rx=6.7 fiv=20000 [on:0 tx:0 rx:0 period:990] "
                + "from screen [on:467747 period:469779]",
                "WifiConfigStore - Log Begin ----",
                "10-08 11:09:14.653 - Event [IFNAME=wlan0 CTRL-EVENT-SCAN-STARTED ]",
                "10-08 13:06:29.489 - Event [IFNAME=wlan0 CTRL-EVENT-DISCONNECTED "
                + "bssid=9c:1c:12:c3:d0:72 reason=0]",
                "10-08 13:06:22.280 - Event [IFNAME=wlan0 CTRL-EVENT-SCAN-STARTED ]",
                "10-08 13:06:25.363 - Event [IFNAME=wlan0 CTRL-EVENT-SCAN-STARTED ]",
                "10-08 13:08:15.018 - Event [IFNAME=wlan0 CTRL-EVENT-DISCONNECTED "
                + "bssid=9c:1c:12:e8:72:d2 reason=3 locally_generated=1]",
                "10-08 13:08:15.324 - wlan0: 442:IFNAME=wlan0 ENABLE_NETWORK 0 -> true",
                "01-21 13:17:23.1 - Event [IFNAME=wlan0 Trying to associate with SSID 'WL-power']",
                "01-21 13:18:23.1 - Event [IFNAME=wlan0 Trying to associate with SSID 'WL-power']",
                "01-21 13:18:23.1 - Event [IFNAME=wlan0 Trying to associate with SSID 'WL-power']");

        DumpsysWifiStatsItem wifiStats = new DumpsysWifiStatsParser().parse(inputBlock);
        assertEquals(2, wifiStats.getNumWifiDisconnects());
        assertEquals(3, wifiStats.getNumWifiScans());
        assertEquals(3, wifiStats.getNumWifiAssociations());
    }

    /**
     * Test that input with no wifi disconnect and wifi scans.
     */
    public void testDumpsysWifiStatsParser_no_wifi_scan_disconnect() {
        List<String> inputBlock = Arrays.asList(
                "Wi-Fi is enabled",
                "rec[0]: time=10-09 00:25:16.737 processed=DefaultState org=DeviceActiveState "
                + "dest=<null> what=155654(0x26006)",
                "mScreenOff true",
                " rec[0]: time=10-08 16:49:55.034 processed=WatchdogEnabledState org=OnlineState "
                + "dest=OnlineWatchState what=135170(0x21002)",
                "rec[30]: time=10-08 13:06:50.379 processed=DriverStartedState org=ConnectedState "
                + "dest=<null> what=131143(0x20047) (1)CMD_START_SCAN  rt=4720806/7884852 10013 51 "
                + "ic=0 proc(ms):14 onGoing full started:1444334810368,11 cnt=24 rssi=-127 f=-1 "
                + "sc=0 link=-1 tx=1.5, 1.7, 0.0  rx=8.4 fiv=20000 [on:3266 tx:65 rx:556 "
                + "period:1268] from screen [on:266962 period:266959]",
                "rec[357]: time=10-08 13:10:13.199 processed=DriverStartedState org=ConnectedState "
                + "dest=<null> what=131143(0x20047) (-2)CMD_START_SCAN  rt=4923625/8087671 10013 "
                + "175 ic=0 proc(ms):2 onGoing full started:1444335011199,1999 cnt=24 rssi=-127 "
                + "f=-1 sc=0 link=-1 tx=8.4, 1.7, 0.0  rx=6.7 fiv=20000 [on:0 tx:0 rx:0 period:990]"
                + "from screen [on:467747 period:469779]",
                "WifiConfigStore - Log Begin ----",
                "10-08 13:07:37.777 - wlan0: 384:IFNAME=wlan0 ENABLE_NETWORK 4 -> true",
                "10-08 13:07:37.789 - wlan0: 388:IFNAME=wlan0 SAVE_CONFIG -> true",
                "10-08 13:08:15.065 - wlan0: 427:IFNAME=wlan0 SET ps 1 -> true",
                "10-08 13:08:15.179 - wlan0: 432:IFNAME=wlan0 SET pno 1 -> true");

        DumpsysWifiStatsItem wifiStats = new DumpsysWifiStatsParser().parse(inputBlock);
        assertEquals(0, wifiStats.getNumWifiDisconnects());
        assertEquals(0, wifiStats.getNumWifiScans());
        assertEquals(0, wifiStats.getNumWifiAssociations());
    }
}

