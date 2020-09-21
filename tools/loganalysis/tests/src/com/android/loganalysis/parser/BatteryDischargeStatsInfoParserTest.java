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

import com.android.loganalysis.item.BatteryDischargeStatsInfoItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link BatteryDischargeStatsInfoParser}
 */
public class BatteryDischargeStatsInfoParserTest extends TestCase {

    /**
     * Test that normal input is parsed correctly.
     */
    public void testBatteryDischargeStats() {
        List<String> input = Arrays.asList(
            "  #0: +4m53s738ms to 0 (screen-on, power-save-off, device-idle-off)",
            "  #1: +4m5s586ms to 1 (screen-on, power-save-off, device-idle-off)",
            "  #2: +3m0s157ms to 2 (screen-on, power-save-off, device-idle-off)",
            "  #3: +2m52s243ms to 3 (screen-on, power-save-off, device-idle-off)",
            "  #4: +2m27s599ms to 4 (screen-on, power-save-off, device-idle-off)",
            "  #5: +5m0s172ms to 5 (screen-on, power-save-off, device-idle-off)",
            "  #6: +2m21s664ms to 6 (screen-on, power-save-off, device-idle-off)",
            "  #7: +5m18s811ms to 7 (screen-on, power-save-off, device-idle-off)",
            "  #8: +3m35s622ms to 8 (screen-on, power-save-off, device-idle-off)",
            "  #9: +4m52s605ms to 9 (screen-on, power-save-off, device-idle-off)",
            "  #10: +4m46s779ms to 10 (screen-on, power-save-off, device-idle-off)",
            "  #11: +4m0s200ms to 11 (screen-on, power-save-off, device-idle-off)",
            "  #12: +4m44s941ms to 12 (screen-on, power-save-off, device-idle-off)",
            "  #13: +3m31s163ms to 13 (screen-on, power-save-off, device-idle-off)",
            "  #14: +4m17s293ms to 14 (screen-on, power-save-off, device-idle-off)",
            "  #15: +3m11s584ms to 15 (screen-on, power-save-off, device-idle-off)",
            "  #16: +5m52s923ms to 16 (screen-on, power-save-off, device-idle-off)",
            "  #17: +3m7s34ms to 17 (screen-on, power-save-off, device-idle-off)",
            "  #18: +5m59s810ms to 18 (screen-on, power-save-off, device-idle-off)",
            "  #19: +6m15s275ms to 19 (screen-on, power-save-off, device-idle-off)",
            "  #20: +4m0s55ms to 20 (screen-on, power-save-off, device-idle-off)",
            "  #21: +5m21s911ms to 21 (screen-on, power-save-off, device-idle-off)",
            "  #22: +4m0s171ms to 22 (screen-on, power-save-off, device-idle-off)",
            "  #23: +5m22s820ms to 23 (screen-on, power-save-off, device-idle-off)",
            "  #24: +3m44s752ms to 24 (screen-on, power-save-off, device-idle-off)",
            "  #25: +4m15s130ms to 25 (screen-on, power-save-off, device-idle-off)",
            "  #26: +3m48s654ms to 26 (screen-on, power-save-off, device-idle-off)",
            "  #27: +5m0s294ms to 27 (screen-on, power-save-off, device-idle-off)",
            "  #28: +3m11s169ms to 28 (screen-on, power-save-off, device-idle-off)",
            "  #29: +4m48s194ms to 29 (screen-on, power-save-off, device-idle-off)",
            "  #30: +5m0s319ms to 30 (screen-on, power-save-off, device-idle-off)",
            "  #31: +2m42s209ms to 31 (screen-on, power-save-off, device-idle-off)",
            "  #32: +5m29s187ms to 32 (screen-on, power-save-off, device-idle-off)",
            "  #33: +3m32s392ms to 33 (screen-on, power-save-off, device-idle-off)",
            "  #34: +5m27s578ms to 34 (screen-on, power-save-off, device-idle-off)",
            "  #35: +3m47s37ms to 35 (screen-on, power-save-off, device-idle-off)",
            "  #36: +5m18s916ms to 36 (screen-on, power-save-off, device-idle-off)",
            "  #37: +2m54s111ms to 37 (screen-on, power-save-off, device-idle-off)",
            "  #38: +6m32s480ms to 38 (screen-on, power-save-off, device-idle-off)",
            "  #39: +5m24s906ms to 39 (screen-on, power-save-off, device-idle-off)",
            "  #40: +3m2s451ms to 40 (screen-on, power-save-off, device-idle-off)",
            "  #41: +6m29s762ms to 41 (screen-on, power-save-off, device-idle-off)",
            "  #42: +3m31s933ms to 42 (screen-on, power-save-off, device-idle-off)",
            "  #43: +4m58s520ms to 43 (screen-on, power-save-off, device-idle-off)",
            "  #44: +4m31s130ms to 44 (screen-on, power-save-off, device-idle-off)",
            "  #45: +5m28s870ms to 45 (screen-on, power-save-off, device-idle-off)",
            "  #46: +3m54s809ms to 46 (screen-on, power-save-off, device-idle-off)",
            "  #47: +5m5s105ms to 47 (screen-on, power-save-off, device-idle-off)",
            "  #48: +3m50s427ms to 48 (screen-on, power-save-off, device-idle-off)",
            "  #49: +6m0s344ms to 49 (screen-on, power-save-off, device-idle-off)",
            "  #50: +5m2s952ms to 50 (screen-on, power-save-off, device-idle-off)",
            "  #51: +3m6s120ms to 51 (screen-on, power-save-off, device-idle-off)",
            "  #52: +5m34s839ms to 52 (screen-on, power-save-off, device-idle-off)",
            "  #53: +2m33s473ms to 53 (screen-on, power-save-off, device-idle-off)",
            "  #54: +4m51s873ms to 54 (screen-on, power-save-off, device-idle-off)",
            "  #55: +3m30s41ms to 55 (screen-on, power-save-off, device-idle-off)",
            "  #56: +4m29s879ms to 56 (screen-on, power-save-off, device-idle-off)",
            "  #57: +3m41s722ms to 57 (screen-on, power-save-off, device-idle-off)",
            "  #58: +4m29s72ms to 58 (screen-on, power-save-off, device-idle-off)",
            "  #59: +4m49s351ms to 59 (screen-on, power-save-off, device-idle-off)",
            "  #60: +3m51s605ms to 60 (screen-on, power-save-off, device-idle-off)",
            "  #61: +5m8s334ms to 61 (screen-on, power-save-off, device-idle-off)",
            "  #62: +2m53s153ms to 62 (screen-on, power-save-off, device-idle-off)",
            "  #63: +6m0s234ms to 63 (screen-on, power-save-off, device-idle-off)",
            "  #64: +3m20s345ms to 64 (screen-on, power-save-off, device-idle-off)",
            "  #65: +5m46s211ms to 65 (screen-on, power-save-off, device-idle-off)",
            "  #66: +3m40s147ms to 66 (screen-on, power-save-off, device-idle-off)",
            "  #67: +5m14s559ms to 67 (screen-on, power-save-off, device-idle-off)",
            "  #68: +4m0s183ms to 68 (screen-on, power-save-off, device-idle-off)",
            "  #69: +5m23s334ms to 69 (screen-on, power-save-off, device-idle-off)",
            "  #70: +5m45s493ms to 70 (screen-on, power-save-off, device-idle-off)",
            "  #71: +4m0s179ms to 71 (screen-on, power-save-off, device-idle-off)",
            "  #72: +5m45s462ms to 72 (screen-on, power-save-off, device-idle-off)",
            "  #73: +3m10s449ms to 73 (screen-on, power-save-off, device-idle-off)",
            "  #74: +6m29s370ms to 74 (screen-on, power-save-off, device-idle-off)",
            "  #75: +3m20s414ms to 75 (screen-on, power-save-off, device-idle-off)",
            "  #76: +5m10s462ms to 76 (screen-on, power-save-off, device-idle-off)",
            "  #77: +4m20s500ms to 77 (screen-on, power-save-off, device-idle-off)",
            "  #78: +5m39s504ms to 78 (screen-on, power-save-off, device-idle-off)",
            "  #79: +5m59s819ms to 79 (screen-on, power-save-off, device-idle-off)",
            "  #80: +3m0s126ms to 80 (screen-on, power-save-off, device-idle-off)",
            "  #81: +6m20s912ms to 81 (screen-on, power-save-off, device-idle-off)",
            "  #82: +4m0s199ms to 82 (screen-on, power-save-off, device-idle-off)",
            "  #83: +5m23s470ms to 83 (screen-on, power-save-off, device-idle-off)",
            "  #84: +3m15s368ms to 84 (screen-on, power-save-off, device-idle-off)",
            "  #85: +6m18s625ms to 85 (screen-on, power-save-off, device-idle-off)",
            "  #86: +3m41s417ms to 86 (screen-on, power-save-off, device-idle-off)",
            "  #87: +5m32s257ms to 87 (screen-on, power-save-off, device-idle-off)",
            "  #88: +4m0s212ms to 88 (screen-on, power-save-off, device-idle-off)",
            "  #89: +5m41s218ms to 89 (screen-on, power-save-off, device-idle-off)",
            "  #90: +5m46s333ms to 90 (screen-on, power-save-off, device-idle-off)",
            "  #91: +3m58s362ms to 91 (screen-on, power-save-off, device-idle-off)",
            "  #92: +5m1s593ms to 92 (screen-on, power-save-off, device-idle-off)",
            "  #93: +4m47s33ms to 93 (screen-on, power-save-off, device-idle-off)",
            "  #94: +6m0s417ms to 94 (screen-on, power-save-off, device-idle-off)",
            "  #95: +3m9s77ms to 95 (screen-on, power-save-off, device-idle-off)",
            "  #96: +7m0s308ms to 96 (screen-on, power-save-off, device-idle-off)",
            "  #97: +3m29s741ms to 97 (screen-on, power-save-off, device-idle-off)",
            "  #98: +7m12s748ms to 98 (screen-on, power-save-off, device-idle-off)",
            "  Estimated screen on time: 7h 36m 13s 0ms ");

        BatteryDischargeStatsInfoItem infoItem = new BatteryDischargeStatsInfoParser().parse(input);
        assertEquals(99, infoItem.getDischargePercentage());
        assertEquals(27099330, infoItem.getDischargeDuration());
        assertEquals(27207848, infoItem.getProjectedBatteryLife());
    }

    /**
     * Test that input with only a few discharge stats.
     */
    public void testBatteryDischargeStatsWithTop5Percentages() {
        List<String> input = Arrays.asList(
            "  #95: +3m9s77ms to 95 (screen-on, power-save-off, device-idle-off)",
            "  #96: +7m0s308ms to 96 (screen-on, power-save-off, device-idle-off)",
            "  #97: +3m29s741ms to 97 (screen-on, power-save-off, device-idle-off)",
            "  #98: +7m12s748ms to 98 (screen-on, power-save-off, device-idle-off)",
            "  Estimated screen on time: 7h 36m 13s 0ms ");

        BatteryDischargeStatsInfoItem infoItem = new BatteryDischargeStatsInfoParser().parse(input);

        try {
            infoItem.getProjectedBatteryLife();
            fail("Projected battery life is expected to be undefined when there are not enough" +
                " samples of battery discharge below 95 percent.");
        } catch (NullPointerException e) {
            // NullPointerException expected.
        }
    }
}
