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

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse wifi stats and extract the number of disconnects and scans
 */
public class DumpsysWifiStatsParser implements IParser {

    /**
     * Matches: 01-04 00:16:27.666 - Event [IFNAME=wlan0 CTRL-EVENT-SCAN-STARTED ]
     */
    private static final Pattern WIFI_SCAN = Pattern.compile(
            "^\\d+-\\d+ \\d+:\\d+:\\d+\\.\\d+ - Event \\[IFNAME=wlan0 CTRL-EVENT-SCAN-STARTED \\]");

    /**
     * Matches: 01-04 00:16:27.666 - Event [IFNAME=wlan0 CTRL-EVENT-DISCONNECTED
     * bssid=6c:f3:7f:ae:89:92 reason=0 locally_generated=1]
     */
    private static final Pattern WIFI_DISCONNECT = Pattern.compile(
            "^\\d+-\\d+ \\d+:\\d+:\\d+\\.\\d+ - Event \\[IFNAME=wlan0 CTRL-EVENT-DISCONNECTED "
            + "bssid=\\w+:\\w+:\\w+:\\w+:\\w+:\\w+ reason=\\d+(\\s*locally_generated=\\d+)?\\]");

    /**
     * Matches: 01-21 18:17:23.15 - Event [IFNAME=wlan0 Trying to associate with SSID 'WL-power-2']
     */
    private static final Pattern WIFI_ASSOCIATION = Pattern.compile(
            "^\\d+-\\d+ \\d+:\\d+:\\d+\\.\\d+ - Event \\[IFNAME=wlan0 Trying to associate with "
            + "SSID \\'.*\\'\\]");

    /**
     * {@inheritDoc}
     *
     * @return The {@link DumpsysWifiStatsItem}.
     */
    @Override
    public DumpsysWifiStatsItem parse(List<String> lines) {
        DumpsysWifiStatsItem item = new DumpsysWifiStatsItem();
        int numWifiScans = 0;
        int numWifiDisconnects = 0;
        int numWifiAssociations = 0;
        for (String line : lines) {
            Matcher m = WIFI_SCAN.matcher(line);
            if(m.matches()) {
                numWifiScans++;
                continue;
            }
            m = WIFI_DISCONNECT.matcher(line);
            if (m.matches()) {
                numWifiDisconnects++;
                continue;
            }
            m = WIFI_ASSOCIATION.matcher(line);
            if (m.matches()) {
                numWifiAssociations++;
            }
        }
        item.setNumWifiScan(numWifiScans);
        item.setNumWifiDisconnect(numWifiDisconnects);
        item.setNumWifiAssociation(numWifiAssociations);
        return item;
    }

}
