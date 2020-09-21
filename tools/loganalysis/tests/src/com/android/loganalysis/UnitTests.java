/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License",
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

package com.android.loganalysis;

import com.android.loganalysis.item.BatteryDischargeItemTest;
import com.android.loganalysis.item.BatteryUsageItemTest;
import com.android.loganalysis.item.DumpsysPackageStatsItemTest;
import com.android.loganalysis.item.DvmLockSampleItemTest;
import com.android.loganalysis.item.GenericItemTest;
import com.android.loganalysis.item.InterruptItemTest;
import com.android.loganalysis.item.LocationDumpsItemTest;
import com.android.loganalysis.item.MemInfoItemTest;
import com.android.loganalysis.item.MonkeyLogItemTest;
import com.android.loganalysis.item.ProcrankItemTest;
import com.android.loganalysis.item.SmartMonkeyLogItemTest;
import com.android.loganalysis.item.SystemPropsItemTest;
import com.android.loganalysis.item.TopItemTest;
import com.android.loganalysis.item.WakelockItemTest;
import com.android.loganalysis.parser.AbstractSectionParserTest;
import com.android.loganalysis.parser.ActivityServiceParserTest;
import com.android.loganalysis.parser.AnrParserTest;
import com.android.loganalysis.parser.BatteryDischargeStatsInfoParserTest;
import com.android.loganalysis.parser.BatteryStatsDetailedInfoParserTest;
import com.android.loganalysis.parser.BatteryStatsSummaryInfoParserTest;
import com.android.loganalysis.parser.BatteryUsageParserTest;
import com.android.loganalysis.parser.BugreportParserTest;
import com.android.loganalysis.parser.CompactMemInfoParserTest;
import com.android.loganalysis.parser.CpuInfoParserTest;
import com.android.loganalysis.parser.DmesgParserTest;
import com.android.loganalysis.parser.DumpsysBatteryStatsParserTest;
import com.android.loganalysis.parser.DumpsysPackageStatsParserTest;
import com.android.loganalysis.parser.DumpsysParserTest;
import com.android.loganalysis.parser.DumpsysProcStatsParserTest;
import com.android.loganalysis.parser.DumpsysProcessMeminfoParserTest;
import com.android.loganalysis.parser.DumpsysWifiStatsParserTest;
import com.android.loganalysis.parser.DvmLockSampleParserTest;
import com.android.loganalysis.parser.EventsLogParserTest;
import com.android.loganalysis.parser.GfxInfoParserTest;
import com.android.loganalysis.parser.InterruptParserTest;
import com.android.loganalysis.parser.JavaCrashParserTest;
import com.android.loganalysis.parser.KernelLogParserTest;
import com.android.loganalysis.parser.LocationServiceParserTest;
import com.android.loganalysis.parser.LogcatParserTest;
import com.android.loganalysis.parser.MemHealthParserTest;
import com.android.loganalysis.parser.MemInfoParserTest;
import com.android.loganalysis.parser.MonkeyLogParserTest;
import com.android.loganalysis.parser.NativeCrashParserTest;
import com.android.loganalysis.parser.ProcessUsageParserTest;
import com.android.loganalysis.parser.ProcrankParserTest;
import com.android.loganalysis.parser.QtaguidParserTest;
import com.android.loganalysis.parser.SmartMonkeyLogParserTest;
import com.android.loganalysis.parser.SystemPropsParserTest;
import com.android.loganalysis.parser.TopParserTest;
import com.android.loganalysis.parser.TraceFormatParserTest;
import com.android.loganalysis.parser.TracesParserTest;
import com.android.loganalysis.parser.WakelockParserTest;
import com.android.loganalysis.rule.InterruptRuleTest;
import com.android.loganalysis.rule.LocationUsageRuleTest;
import com.android.loganalysis.rule.ProcessUsageRuleTest;
import com.android.loganalysis.rule.WakelockRuleTest;
import com.android.loganalysis.rule.WifiStatsRuleTest;
import com.android.loganalysis.util.ArrayUtilTest;
import com.android.loganalysis.util.LogPatternUtilTest;
import com.android.loganalysis.util.LogTailUtilTest;
import com.android.loganalysis.util.RegexTrieTest;
import com.android.loganalysis.util.config.ArgsOptionParserTest;
import com.android.loganalysis.util.config.OptionSetterTest;
import com.android.loganalysis.util.config.OptionUpdateRuleTest;

import org.junit.runner.RunWith;
import org.junit.runners.Suite;
import org.junit.runners.Suite.SuiteClasses;

/**
 * A test suite for all log analysis unit tests.
 *
 * <p>All tests listed here should be self-contained, and should not require any external
 * dependencies.
 */
@RunWith(Suite.class)
@SuiteClasses({
    // item
    BatteryDischargeItemTest.class,
    BatteryUsageItemTest.class,
    DumpsysPackageStatsItemTest.class,
    DvmLockSampleItemTest.class,
    GenericItemTest.class,
    InterruptItemTest.class,
    LocationDumpsItemTest.class,
    MemInfoItemTest.class,
    MonkeyLogItemTest.class,
    ProcrankItemTest.class,
    SmartMonkeyLogItemTest.class,
    SystemPropsItemTest.class,
    TopItemTest.class,
    WakelockItemTest.class,

    // rule
    InterruptRuleTest.class,
    LocationUsageRuleTest.class,
    ProcessUsageRuleTest.class,
    WakelockRuleTest.class,
    WifiStatsRuleTest.class,

    // parser
    AbstractSectionParserTest.class,
    ActivityServiceParserTest.class,
    AnrParserTest.class,
    BatteryDischargeStatsInfoParserTest.class,
    BatteryStatsDetailedInfoParserTest.class,
    BatteryStatsSummaryInfoParserTest.class,
    BatteryUsageParserTest.class,
    BugreportParserTest.class,
    CompactMemInfoParserTest.class,
    CpuInfoParserTest.class,
    DmesgParserTest.class,
    DumpsysBatteryStatsParserTest.class,
    DumpsysPackageStatsParserTest.class,
    DumpsysParserTest.class,
    DumpsysProcessMeminfoParserTest.class,
    DumpsysProcStatsParserTest.class,
    DumpsysWifiStatsParserTest.class,
    DvmLockSampleParserTest.class,
    EventsLogParserTest.class,
    GfxInfoParserTest.class,
    InterruptParserTest.class,
    JavaCrashParserTest.class,
    KernelLogParserTest.class,
    LocationServiceParserTest.class,
    LogcatParserTest.class,
    MemHealthParserTest.class,
    MemInfoParserTest.class,
    MonkeyLogParserTest.class,
    NativeCrashParserTest.class,
    ProcessUsageParserTest.class,
    ProcrankParserTest.class,
    QtaguidParserTest.class,
    SmartMonkeyLogParserTest.class,
    SystemPropsParserTest.class,
    TopParserTest.class,
    TraceFormatParserTest.class,
    TracesParserTest.class,
    WakelockParserTest.class,

    // util
    ArrayUtilTest.class,
    LogPatternUtilTest.class,
    LogTailUtilTest.class,
    RegexTrieTest.class,

    // util.config
    ArgsOptionParserTest.class,
    OptionSetterTest.class,
    OptionUpdateRuleTest.class,
})
public class UnitTests {
    // empty of purpose
}
