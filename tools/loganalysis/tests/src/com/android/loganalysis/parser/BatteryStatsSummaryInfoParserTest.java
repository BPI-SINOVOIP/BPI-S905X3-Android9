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

import com.android.loganalysis.item.BatteryStatsSummaryInfoItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.GregorianCalendar;
import java.util.List;
import java.util.TimeZone;

/**
 * Unit tests for {@link BatteryStatsSummaryInfoParser}
 */
public class BatteryStatsSummaryInfoParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testBatteryStatsSummaryInfoParser() {
        List<String> inputBlock = Arrays.asList(
                "Battery History (37% used, 95KB used of 256KB, 166 strings using 15KB):",
                "     0 (9) RESET:TIME: 2014-12-09-11-33-29",
                "     +1s067ms (1) 100 c0500020 -wifi_full_lock -wifi_scan",
                "     +3s297ms (2) 100 80400020 -wake_lock -screen",
                "     +30m02s075ms (1) 100 c0500020 wifi_signal_strength=4 wifi_suppl=completed",
                "     +30m03s012ms (2) 099 c0500020 temp=306 volt=4217",
                "     +33m48s967ms (1) 099 f8400020 +wifi_scan",
                "     +33m49s335ms (2) 098 f0400020 temp=324 -wifi_scan",
                "     +1h07m27s735ms (1) 098 80400020 -wake_lock",
                "     +1h07m27s950ms (2) 097 c0400020",
                "     +1h07m29s000ms (2) 097 c0400020 -sync=u0a41:\"gmail-ls/com.google/a@g",
                "     +1h25m34s877ms (2) 097 00400020 -running wake_reason=0:200:qcom,smd-rpm",
                "     +1h25m41s948ms (2) 096 80400020 wifi_suppl=associated",
                "     +2h13m40s055ms (1) 096 00400018 -running",
                "     +2h13m40s570ms (2) 095 c0400008 temp=304 volt=4167",
                "     +2h56m50s792ms (1) 095 80400020 -wake_lock",
                "     +2h56m50s967ms (2) 094 00400020 temp=317 -running",
                "     +3h38m57s986ms (2) 094 80400020 +running wake_reason=0:289:bcmsdh_sdmmc",
                "     +3h38m58s241ms (2) 093 00400020 temp=327 -running",
                "     +3h56m33s329ms (1) 093 00400020 -running -wake_lock",
                "     +3h56m43s245ms (2) 092 00400020 -running",
                "     +4h13m00s551ms (1) 092 00400020 -running -wake_lock",
                "     +4h13m24s250ms (2) 091 00400020 -running",
                "     +4h34m52s233ms (2) 091 80400020 +running wake_reason=0:289:bcmsdh_sdmmc",
                "     +4h34m52s485ms (3) 090 00400020 -running wake_reason=0:200:qcom,smd-rpm",
                "     +4h57m20s644ms (1) 090 00400020 -running",
                "     +4h57m38s484ms (2) 089 00400020 -running",
                "     +5h20m58s226ms (1) 089 80400020 +running wifi_suppl=associated",
                "     +5h21m03s909ms (1) 088 80400020 -wake_lock -wifi_full_lock",
                "     +5h40m38s169ms (2) 088 c0500020 +top=u0a19:com.google.android.googlequick",
                "     +5h40m38s479ms (2) 087 c0500020 volt=4036",
                "     +6h16m45s248ms (2) 087 d0440020 -sync=u0a41:gmail-ls/com.google/avellore@go",
                "     +6h16m45s589ms (2) 086 d0440020 volt=4096",
                "     +6h52m43s316ms (1) 086 80400020 -wake_lock",
                "     +6h53m18s952ms (2) 085 c0400020",
                "     +7h24m02s415ms (1) 085 80400020 -wake_lock",
                "     +7h24m02s904ms (3) 084 c0400020 volt=4105 +wake_lock=u0a7:NlpWakeLock",
                "     +7h29m10s379ms (1) 084 00400020 -running -wake_lock",
                "     +7h29m11s841ms (2) 083 00400020 temp=317 volt=4047 -running",
                "     +7h41m08s963ms (1) 083 00400020 -running",
                "     +7h41m20s494ms (2) 082 00400020 temp=300 -running",
                "     +7h54m57s288ms (1) 082 52441420 -running",
                "     +7h55m00s801ms (1) 081 52441420 -running",
                "     +8h02m18s594ms (1) 081 50440020 -running",
                "     +8h02m23s493ms (2) 080 50440020 temp=313 -running");

        BatteryStatsSummaryInfoItem summary = new BatteryStatsSummaryInfoParser().parse(inputBlock);

        assertEquals("The battery dropped a level 24 mins on average",
                summary.getBatteryDischargeRate());

        // Get the current timezone short name (PST, GMT) to properly output the time as expected.
        String timezone =
                new GregorianCalendar().getTimeZone().getDisplayName(false, TimeZone.SHORT);
        assertEquals(
                String.format(
                        "The peak discharge time was during Tue Dec 09 16:31:07 %s 2014 to "
                                + "Tue Dec 09 19:35:52 %s 2014 where battery dropped from 89 to 80",
                        timezone, timezone),
                summary.getPeakDischargeTime());
    }

    public void testNoBatteryDischarge() {
        List<String> inputBlock = Arrays.asList(
                "Battery History (37% used, 95KB used of 256KB, 166 strings using 15KB):",
                "     0 (9) RESET:TIME: 2014-12-09-11-33-29");
        BatteryStatsSummaryInfoItem summary = new BatteryStatsSummaryInfoParser().parse(inputBlock);

        assertEquals("The battery did not discharge", summary.getBatteryDischargeRate());
        assertEquals("The battery did not discharge", summary.getPeakDischargeTime());
    }
}

