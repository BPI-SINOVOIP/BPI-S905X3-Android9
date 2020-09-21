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

package com.android.tradefed.device;

import com.android.tradefed.device.TopHelper.TopStats;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.List;

/**
 * Unit tests for {@link TopHelper}
 */
public class TopHelperTest extends TestCase {
    private ITestDevice mMockDevice;
    private TopHelper mTop;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mTop = new TopHelper(mMockDevice);
    }

    /**
     * Test that all the output from top invoked once is supported and parsed (or not parsed).
     */
    public void testTopParser_parse() {
        final String lines = ("User 7%, System 5%, IOW 3%, IRQ 2%\r\n" +
                "User 3 + Nice 0 + Sys 6 + Idle 100 + IOW 0 + IRQ 0 + SIRQ 0 = 109\r\n" +
                "\r\n" +
                "  PID   TID PR CPU% S     VSS     RSS PCY UID      Thread          Proc\r\n" +
                " 1388  1388  0  11% R   1160K    576K  fg shell    top             top\r\n" +
                "    2     2  0   0% S      0K      0K  fg root     kthreadd\r\n" +
                "\r\n" +
                "\r\n" +
                "\r\n");

        mTop.getReceiver().processNewLines(lines.split("\r\n"));
        List<TopStats> stats = mTop.getTopStats();

        assertEquals(1, stats.size());

        assertEquals(17.0, stats.get(0).mTotalPercent, 0.0001);
        assertEquals(7.0, stats.get(0).mUserPercent, 0.0001);
        assertEquals(5.0, stats.get(0).mSystemPercent, 0.0001);
        assertEquals(3.0, stats.get(0).mIowPercent, 0.0001);
        assertEquals(2.0, stats.get(0).mIrqPercent, 0.0001);
    }

    /**
     * Test that the parser returns the correct averages for various ranges.
     */
    public void testTopParser_stats() {
        final String lines = (
                "User 15%, System 11%, IOW 7%, IRQ 3%\r\n" +
                "User 16%, System 12%, IOW 8%, IRQ 4%\r\n" +
                "User 17%, System 13%, IOW 9%, IRQ 5%\r\n" +
                "User 18%, System 14%, IOW 10%, IRQ 6%\r\n" +
                "User 19%, System 15%, IOW 11%, IRQ 7%\r\n" +
                "User 20%, System 16%, IOW 12%, IRQ 8%\r\n" +
                "User 21%, System 17%, IOW 13%, IRQ 9%\r\n");

        List<TopStats> stats = mTop.getTopStats();

        assertEquals(0, stats.size());

        assertNull(TopHelper.getTotalAverage(stats));
        assertNull(TopHelper.getUserAverage(stats));
        assertNull(TopHelper.getSystemAverage(stats));
        assertNull(TopHelper.getIowAverage(stats));
        assertNull(TopHelper.getIrqAverage(stats));

        mTop.getReceiver().processNewLines(lines.split("\r\n"));
        stats = mTop.getTopStats();

        assertEquals(7, mTop.getTopStats().size());

        assertEquals(48.0, TopHelper.getTotalAverage(stats.subList(0, 7)), 0.001);
        assertEquals(18.0, TopHelper.getUserAverage(stats.subList(0, 7)), 0.001);
        assertEquals(14.0, TopHelper.getSystemAverage(stats.subList(0, 7)), 0.001);
        assertEquals(10.0, TopHelper.getIowAverage(stats.subList(0, 7)), 0.001);
        assertEquals(6.0, TopHelper.getIrqAverage(stats.subList(0, 7)), 0.001);

        assertEquals(44.0, TopHelper.getTotalAverage(stats.subList(0, 7 - 2)), 0.001);
        assertEquals(17.0, TopHelper.getUserAverage(stats.subList(0, 7 - 2)), 0.001);
        assertEquals(13.0, TopHelper.getSystemAverage(stats.subList(0, 7 - 2)), 0.001);
        assertEquals(9.0, TopHelper.getIowAverage(stats.subList(0, 7 - 2)), 0.001);
        assertEquals(5.0, TopHelper.getIrqAverage(stats.subList(0, 7 - 2)), 0.001);

        assertEquals(52.0, TopHelper.getTotalAverage(stats.subList(2, 7)), 0.001);
        assertEquals(19.0, TopHelper.getUserAverage(stats.subList(2, 7)), 0.001);
        assertEquals(15.0, TopHelper.getSystemAverage(stats.subList(2, 7)), 0.001);
        assertEquals(11.0, TopHelper.getIowAverage(stats.subList(2, 7)), 0.001);
        assertEquals(7.0, TopHelper.getIrqAverage(stats.subList(2, 7)), 0.001);

        assertNull(TopHelper.getTotalAverage(stats.subList(3, 3)));
        assertNull(TopHelper.getUserAverage(stats.subList(3, 3)));
        assertNull(TopHelper.getSystemAverage(stats.subList(3, 3)));
        assertNull(TopHelper.getIowAverage(stats.subList(3, 3)));
        assertNull(TopHelper.getIrqAverage(stats.subList(3, 3)));
    }
}
