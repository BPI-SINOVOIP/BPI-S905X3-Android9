/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.tradefed.device;

import com.android.tradefed.device.CpuStatsCollector.CpuStats;
import com.android.tradefed.device.CpuStatsCollector.TimeCategory;
import com.android.tradefed.testtype.DeviceTestCase;

import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link CpuStatsCollector}.
 */
public class CpuStatsCollectorTest extends DeviceTestCase {
    /**
     * Single output for cpustats tool where frequencies are aggregated in the total.
     */
    private final static String[] SINGLE_OUTPUT = {
        "Total,2,3,5,7,11,13,17,350000,19,700000,23,920000,29,1200000,31",
        "cpu0,37,41,43,47,53,59,61,350000,67,700000,71,920000,73,1200000,79",
        "cpu1,83,89,97,101,103,107,109,350000,113,700000,127,920000,131,1200000,137",
        ""};

    /**
     * Single output for cpustats tool where frequencies are not aggregated in the total.
     */
    private final static String[] SINGLE_NON_AGGREGATE_OUTPUT = {
        "Total,2,3,5,7,11,13,17",
        "cpu0,37,41,43,47,53,59,61,350000,67,700000,71,920000,73,1200000,79",
        "cpu1,83,89,97,101,103,107,109,350000,113,700000,127,920000,131,1200000,137",
        ""};

    /**
     * Multiline output for cpustats tool.
     */
    private final static String[] MULTI_OUTPUT = {
        "Total,246,0,69,283,0,0,0,350000,140,700000,22,920000,122,1200000,318",
        "cpu0,68,0,10,221,0,0,0,350000,70,700000,11,920000,61,1200000,159",
        "cpu1,177,0,60,63,0,0,0,350000,70,700000,11,920000,61,1200000,159",
        "",
        "Total,238,0,75,287,0,0,0,350000,124,700000,12,920000,68,1200000,396",
        "cpu0,53,0,9,238,0,0,0,350000,62,700000,6,920000,34,1200000,198",
        "cpu1,186,0,65,49,0,0,0,350000,62,700000,6,920000,34,1200000,198",
        "",
        "Total,230,0,71,299,0,0,0,350000,0,700000,0,920000,230,1200000,370",
        "cpu0,2,0,3,295,0,0,0,350000,0,700000,0,920000,115,1200000,185",
        "cpu1,228,0,69,3,0,0,0,350000,0,700000,0,920000,115,1200000,185",
        "",
        "Total,248,0,59,293,0,0,0,350000,50,700000,4,920000,330,1200000,216",
        "cpu0,28,0,2,270,0,0,0,350000,25,700000,2,920000,165,1200000,108",
        "cpu1,219,0,56,24,0,0,0,350000,25,700000,2,920000,165,1200000,108",
        "",
        "Total,250,0,63,288,0,0,0,350000,184,700000,22,920000,164,1200000,230",
        "cpu0,79,0,21,201,0,0,0,350000,92,700000,11,920000,82,1200000,115",
        "cpu1,171,0,43,86,0,0,0,350000,92,700000,11,920000,82,1200000,115",
        "",
        "Total,231,0,80,289,0,0,0,350000,96,700000,4,920000,176,1200000,322",
        "cpu0,51,0,7,243,0,0,0,350000,48,700000,2,920000,88,1200000,161",
        "cpu1,181,0,73,47,0,0,0,350000,48,700000,2,920000,88,1200000,161",
        "",
        "Total,248,0,69,283,0,0,0,350000,404,700000,22,920000,26,1200000,150",
        "cpu0,161,0,13,125,0,0,0,350000,202,700000,11,920000,13,1200000,75",
        "cpu1,86,0,55,158,0,0,0,350000,202,700000,11,920000,13,1200000,75",
        "",
        "Total,269,0,97,233,0,0,0,350000,214,700000,18,920000,18,1200000,350",
        "cpu0,40,0,42,218,0,0,0,350000,107,700000,9,920000,9,1200000,175",
        "cpu1,230,0,55,15,0,0,0,350000,107,700000,9,920000,9,1200000,175",
        "",
        "Total,260,0,100,235,0,0,0,350000,152,700000,10,920000,438,1200000,0",
        "cpu0,207,0,72,20,0,0,0,350000,76,700000,5,920000,219,1200000,0",
        "cpu1,53,0,29,214,0,0,0,350000,76,700000,5,920000,219,1200000,0",
        "",
        "Total,248,0,65,287,0,0,0,350000,50,700000,30,920000,336,1200000,184",
        "cpu0,26,0,14,259,0,0,0,350000,25,700000,15,920000,168,1200000,92",
        "cpu1,221,0,51,28,0,0,0,350000,25,700000,15,920000,168,1200000,92",
        ""};

    /**
     * Multiline output for cpustats tool where frequencies are not aggregated in the total.
     */
    private final static String[] MULTI_NON_AGGREGATE_OUTPUT = {
        "Total,246,0,69,283,0,0,0",
        "cpu0,68,0,10,221,0,0,0,350000,70,700000,11,920000,61,1200000,159",
        "cpu1,177,0,60,63,0,0,0,350000,70,700000,11,920000,61,1200000,159",
        "",
        "Total,238,0,75,287,0,0,0",
        "cpu0,53,0,9,238,0,0,0,350000,62,700000,6,920000,34,1200000,198",
        "cpu1,186,0,65,49,0,0,0,350000,62,700000,6,920000,34,1200000,198",
        "",
        "Total,230,0,71,299,0,0,0",
        "cpu0,2,0,3,295,0,0,0,350000,0,700000,0,920000,115,1200000,185",
        "cpu1,228,0,69,3,0,0,0,350000,0,700000,0,920000,115,1200000,185",
        "",
        "Total,248,0,59,293,0,0,0",
        "cpu0,28,0,2,270,0,0,0,350000,25,700000,2,920000,165,1200000,108",
        "cpu1,219,0,56,24,0,0,0,350000,25,700000,2,920000,165,1200000,108",
        "",
        "Total,250,0,63,288,0,0,0",
        "cpu0,79,0,21,201,0,0,0,350000,92,700000,11,920000,82,1200000,115",
        "cpu1,171,0,43,86,0,0,0,350000,92,700000,11,920000,82,1200000,115",
        "",
        "Total,231,0,80,289,0,0,0",
        "cpu0,51,0,7,243,0,0,0,350000,48,700000,2,920000,88,1200000,161",
        "cpu1,181,0,73,47,0,0,0,350000,48,700000,2,920000,88,1200000,161",
        "",
        "Total,248,0,69,283,0,0,0",
        "cpu0,161,0,13,125,0,0,0,350000,202,700000,11,920000,13,1200000,75",
        "cpu1,86,0,55,158,0,0,0,350000,202,700000,11,920000,13,1200000,75",
        "",
        "Total,269,0,97,233,0,0,0",
        "cpu0,40,0,42,218,0,0,0,350000,107,700000,9,920000,9,1200000,175",
        "cpu1,230,0,55,15,0,0,0,350000,107,700000,9,920000,9,1200000,175",
        "",
        "Total,260,0,100,235,0,0,0",
        "cpu0,207,0,72,20,0,0,0,350000,76,700000,5,920000,219,1200000,0",
        "cpu1,53,0,29,214,0,0,0,350000,76,700000,5,920000,219,1200000,0",
        "",
        "Total,248,0,65,287,0,0,0",
        "cpu0,26,0,14,259,0,0,0,350000,25,700000,15,920000,168,1200000,92",
        "cpu1,221,0,51,28,0,0,0,350000,25,700000,15,920000,168,1200000,92",
        ""};

    private CpuStatsCollector mCollector;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mCollector = new CpuStatsCollector(null, 1);
    }

    /**
     * Test that a single output from {@code cpustats} is parsed correctly and that {@link CpuStats}
     * contains the correct data and calculates the correct data.
     */
    public void testCpuStatsParser_single() {
        mCollector.getReceiver().processNewLines(SINGLE_OUTPUT);

        Map<String, List<CpuStats>> cpuStats = mCollector.getCpuStats();
        assertEquals(3, cpuStats.size());
        assertTrue(cpuStats.containsKey("Total"));
        assertTrue(cpuStats.containsKey("cpu0"));
        assertTrue(cpuStats.containsKey("cpu1"));

        assertEquals(1, cpuStats.get("Total").size());
        CpuStats stats = cpuStats.get("Total").get(0);

        // Time info
        assertEquals(2, stats.mTimeStats.get(TimeCategory.USER).intValue());
        assertEquals(3, stats.mTimeStats.get(TimeCategory.NICE).intValue());
        assertEquals(5, stats.mTimeStats.get(TimeCategory.SYS).intValue());
        assertEquals(7, stats.mTimeStats.get(TimeCategory.IDLE).intValue());
        assertEquals(11, stats.mTimeStats.get(TimeCategory.IOW).intValue());
        assertEquals(13, stats.mTimeStats.get(TimeCategory.IRQ).intValue());
        assertEquals(17, stats.mTimeStats.get(TimeCategory.SIRQ).intValue());

        // Percent info
        assertEquals(100.0 * 2 / 58, stats.getPercentage(TimeCategory.USER), 0.01);
        assertEquals(100.0 * 3 / 58, stats.getPercentage(TimeCategory.NICE), 0.01);
        assertEquals(100.0 * 5 / 58, stats.getPercentage(TimeCategory.SYS), 0.01);
        assertEquals(100.0 * 7 / 58, stats.getPercentage(TimeCategory.IDLE), 0.01);
        assertEquals(100.0 * 11 / 58, stats.getPercentage(TimeCategory.IOW), 0.01);
        assertEquals(100.0 * 13 / 58, stats.getPercentage(TimeCategory.IRQ), 0.01);
        assertEquals(100.0 * 17 / 58, stats.getPercentage(TimeCategory.SIRQ), 0.01);

        // Freq info
        assertEquals(4, stats.mFreqStats.size());
        assertEquals(19, stats.mFreqStats.get(350000).intValue());
        assertEquals(23, stats.mFreqStats.get(700000).intValue());
        assertEquals(29, stats.mFreqStats.get(920000).intValue());
        assertEquals(31, stats.mFreqStats.get(1200000).intValue());
        assertEquals((51 / 58.0) * (86630.0 / 102), stats.getEstimatedMhz(), 0.01);
        assertEquals(100.0 * 86630.0 / (102 * 1200), stats.getUsedMhzPercentage(), 0.01);

        // cpu0 raw stats
        assertEquals(1, cpuStats.get("cpu0").size());
        stats = cpuStats.get("cpu0").get(0);
        assertEquals(37, stats.mTimeStats.get(TimeCategory.USER).intValue());
        assertEquals(41, stats.mTimeStats.get(TimeCategory.NICE).intValue());
        assertEquals(43, stats.mTimeStats.get(TimeCategory.SYS).intValue());
        assertEquals(47, stats.mTimeStats.get(TimeCategory.IDLE).intValue());
        assertEquals(53, stats.mTimeStats.get(TimeCategory.IOW).intValue());
        assertEquals(59, stats.mTimeStats.get(TimeCategory.IRQ).intValue());
        assertEquals(61, stats.mTimeStats.get(TimeCategory.SIRQ).intValue());
        assertEquals(4, stats.mFreqStats.size());
        assertEquals(67, stats.mFreqStats.get(350000).intValue());
        assertEquals(71, stats.mFreqStats.get(700000).intValue());
        assertEquals(73, stats.mFreqStats.get(920000).intValue());
        assertEquals(79, stats.mFreqStats.get(1200000).intValue());

        // cpu1 raw stats
        assertEquals(1, cpuStats.get("cpu1").size());
        stats = cpuStats.get("cpu1").get(0);
        assertEquals(83, stats.mTimeStats.get(TimeCategory.USER).intValue());
        assertEquals(89, stats.mTimeStats.get(TimeCategory.NICE).intValue());
        assertEquals(97, stats.mTimeStats.get(TimeCategory.SYS).intValue());
        assertEquals(101, stats.mTimeStats.get(TimeCategory.IDLE).intValue());
        assertEquals(103, stats.mTimeStats.get(TimeCategory.IOW).intValue());
        assertEquals(107, stats.mTimeStats.get(TimeCategory.IRQ).intValue());
        assertEquals(109, stats.mTimeStats.get(TimeCategory.SIRQ).intValue());
        assertEquals(4, stats.mFreqStats.size());
        assertEquals(113, stats.mFreqStats.get(350000).intValue());
        assertEquals(127, stats.mFreqStats.get(700000).intValue());
        assertEquals(131, stats.mFreqStats.get(920000).intValue());
        assertEquals(137, stats.mFreqStats.get(1200000).intValue());
    }

    /**
     * Test that a single output from {@code cpustats} is parsed correctly when frequencies are not
     * aggregated and that {@link CpuStats} contains the correct data and calculates the correct
     * data.
     */
    public void testCpuStatsParser_single_non_aggregate() {
        mCollector.getReceiver().processNewLines(SINGLE_NON_AGGREGATE_OUTPUT);

        Map<String, List<CpuStats>> cpuStats = mCollector.getCpuStats();
        assertEquals(3, cpuStats.size());
        assertTrue(cpuStats.containsKey("Total"));
        assertTrue(cpuStats.containsKey("cpu0"));
        assertTrue(cpuStats.containsKey("cpu1"));

        assertEquals(1, cpuStats.get("Total").size());
        CpuStats stats = cpuStats.get("Total").get(0);

        // Time info
        assertEquals(2, stats.mTimeStats.get(TimeCategory.USER).intValue());
        assertEquals(3, stats.mTimeStats.get(TimeCategory.NICE).intValue());
        assertEquals(5, stats.mTimeStats.get(TimeCategory.SYS).intValue());
        assertEquals(7, stats.mTimeStats.get(TimeCategory.IDLE).intValue());
        assertEquals(11, stats.mTimeStats.get(TimeCategory.IOW).intValue());
        assertEquals(13, stats.mTimeStats.get(TimeCategory.IRQ).intValue());
        assertEquals(17, stats.mTimeStats.get(TimeCategory.SIRQ).intValue());

        // Percent info
        assertEquals(100.0 * 2 / 58, stats.getPercentage(TimeCategory.USER), 0.01);
        assertEquals(100.0 * 3 / 58, stats.getPercentage(TimeCategory.NICE), 0.01);
        assertEquals(100.0 * 5 / 58, stats.getPercentage(TimeCategory.SYS), 0.01);
        assertEquals(100.0 * 7 / 58, stats.getPercentage(TimeCategory.IDLE), 0.01);
        assertEquals(100.0 * 11 / 58, stats.getPercentage(TimeCategory.IOW), 0.01);
        assertEquals(100.0 * 13 / 58, stats.getPercentage(TimeCategory.IRQ), 0.01);
        assertEquals(100.0 * 17 / 58, stats.getPercentage(TimeCategory.SIRQ), 0.01);

        // Freq info
        assertEquals(0, stats.mFreqStats.size());
        assertNull(stats.getEstimatedMhz());
        assertNull(stats.getUsedMhzPercentage());
    }

    /**
     * Tests that multiple lines of {@code cpustats} output are parsed correctly and that
     * {@link CpuStatsCollector} calculates the correct means from the output.
     */
    public void testCpuStatsParser_multi() {
        mCollector.getReceiver().processNewLines(MULTI_OUTPUT);

        Map<String, List<CpuStats>> stats = mCollector.getCpuStats();
        assertEquals(3, stats.size());
        assertTrue(stats.containsKey("Total"));
        assertTrue(stats.containsKey("cpu0"));
        assertTrue(stats.containsKey("cpu1"));

        assertEquals(10, stats.get("Total").size());
        assertEquals(10, stats.get("cpu0").size());
        assertEquals(10, stats.get("cpu1").size());

        assertEquals(53.67, CpuStatsCollector.getTotalPercentageMean(stats.get("Total")), 0.01);
        assertEquals(41.18, CpuStatsCollector.getUserPercentageMean(stats.get("Total")), 0.01);
        assertEquals(12.49, CpuStatsCollector.getSystemPercentageMean(stats.get("Total")), 0.01);
        assertEquals(0.0, CpuStatsCollector.getIowPercentageMean(stats.get("Total")), 0.01);
        assertEquals(0.0, CpuStatsCollector.getIrqPercentageMean(stats.get("Total")), 0.01);
        assertEquals(480.46, CpuStatsCollector.getEstimatedMhzMean(stats.get("Total")), 0.01);
        assertEquals(74.91, CpuStatsCollector.getUsedMhzPercentageMean(stats.get("Total")), 0.01);

        assertEquals(30.31, CpuStatsCollector.getTotalPercentageMean(stats.get("cpu0")), 0.01);
        assertEquals(23.87, CpuStatsCollector.getUserPercentageMean(stats.get("cpu0")), 0.01);
        assertEquals(6.44, CpuStatsCollector.getSystemPercentageMean(stats.get("cpu0")), 0.01);
        assertEquals(0.0, CpuStatsCollector.getIowPercentageMean(stats.get("cpu0")), 0.01);
        assertEquals(0.0, CpuStatsCollector.getIrqPercentageMean(stats.get("cpu0")), 0.01);
        assertEquals(246.38, CpuStatsCollector.getEstimatedMhzMean(stats.get("cpu0")), 0.01);
        assertEquals(74.91, CpuStatsCollector.getUsedMhzPercentageMean(stats.get("cpu0")), 0.01);

        assertEquals(76.99, CpuStatsCollector.getTotalPercentageMean(stats.get("cpu1")), 0.01);
        assertEquals(58.44, CpuStatsCollector.getUserPercentageMean(stats.get("cpu1")), 0.01);
        assertEquals(18.55, CpuStatsCollector.getSystemPercentageMean(stats.get("cpu1")), 0.01);
        assertEquals(0.0, CpuStatsCollector.getIowPercentageMean(stats.get("cpu1")), 0.01);
        assertEquals(0.0, CpuStatsCollector.getIrqPercentageMean(stats.get("cpu1")), 0.01);
        assertEquals(714.29, CpuStatsCollector.getEstimatedMhzMean(stats.get("cpu1")), 0.01);
        assertEquals(74.91, CpuStatsCollector.getUsedMhzPercentageMean(stats.get("cpu1")), 0.01);
    }

    /**
     * Tests that multiple lines of {@code cpustats} output are parsed correctly when frequencies
     * are not aggregated and that {@link CpuStatsCollector} calculates the correct means from the
     * output.
     */
    public void testCpuStatsParser_multi_non_aggregate() {
        mCollector.getReceiver().processNewLines(MULTI_NON_AGGREGATE_OUTPUT);

        Map<String, List<CpuStats>> stats = mCollector.getCpuStats();
        assertEquals(3, stats.size());
        assertTrue(stats.containsKey("Total"));
        assertTrue(stats.containsKey("cpu0"));
        assertTrue(stats.containsKey("cpu1"));

        assertEquals(10, stats.get("Total").size());
        assertEquals(10, stats.get("cpu0").size());
        assertEquals(10, stats.get("cpu1").size());

        assertEquals(53.67, CpuStatsCollector.getTotalPercentageMean(stats.get("Total")), 0.01);
        assertEquals(41.18, CpuStatsCollector.getUserPercentageMean(stats.get("Total")), 0.01);
        assertEquals(12.49, CpuStatsCollector.getSystemPercentageMean(stats.get("Total")), 0.01);
        assertEquals(0.0, CpuStatsCollector.getIowPercentageMean(stats.get("Total")), 0.01);
        assertEquals(0.0, CpuStatsCollector.getIrqPercentageMean(stats.get("Total")), 0.01);
        assertNull(CpuStatsCollector.getEstimatedMhzMean(stats.get("Total")));
        assertNull(CpuStatsCollector.getUsedMhzPercentageMean(stats.get("Total")));
    }
}
