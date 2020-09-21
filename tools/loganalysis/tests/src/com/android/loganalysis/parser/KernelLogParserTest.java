/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.loganalysis.item.KernelLogItem;
import com.android.loganalysis.item.LowMemoryKillerItem;
import com.android.loganalysis.item.MiscKernelLogItem;
import com.android.loganalysis.item.PageAllocationFailureItem;
import com.android.loganalysis.item.SELinuxItem;
import com.android.loganalysis.util.LogPatternUtil;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link KernelLogParser}.
 */
public class KernelLogParserTest extends TestCase {
    /**
     * Test that log lines formatted by last kmsg are able to be parsed.
     */
    public void testParseLastKmsg() {
        List<String> lines = Arrays.asList(
                "[    0.000000] Start",
                "[    1.000000] Kernel panic",
                "[    2.000000] End");

        KernelLogItem kernelLog = new KernelLogParser().parse(lines);
        assertNotNull(kernelLog);
        assertEquals(0.0, kernelLog.getStartTime(), 0.0000005);
        assertEquals(2.0, kernelLog.getStopTime(), 0.0000005);
        assertEquals(1, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).size());

        MiscKernelLogItem item = kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).get(0);
        assertEquals(1.0, item.getEventTime(), 0.0000005);
        assertEquals("[    0.000000] Start", item.getPreamble());
        assertEquals("Kernel panic", item.getStack());
    }

    /**
     * Test that log lines formatted by dmsg are able to be parsed.
     */
    public void testParseDmesg() {
        List<String> lines = Arrays.asList(
                "<1>[    0.000000] Start",
                "<1>[    1.000000] Kernel panic",
                "<1>[    2.000000] End");

        KernelLogItem kernelLog = new KernelLogParser().parse(lines);
        assertNotNull(kernelLog);
        assertEquals(0.0, kernelLog.getStartTime(), 0.0000005);
        assertEquals(2.0, kernelLog.getStopTime(), 0.0000005);
        assertEquals(1, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).size());

        MiscKernelLogItem item = kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).get(0);
        assertEquals(1.0, item.getEventTime(), 0.0000005);
        assertEquals("<1>[    0.000000] Start", item.getPreamble());
        assertEquals("Kernel panic", item.getStack());
    }

    /**
     * Test that last boot reasons are parsed.
     */
    public void testParseLastMessage() {
        List<String> lines = Arrays.asList(
                "[    0.000000] Start",
                "[    2.000000] End",
                "Last boot reason: hw_reset");

        KernelLogItem kernelLog = new KernelLogParser().parse(lines);
        assertNotNull(kernelLog);
        assertEquals(0.0, kernelLog.getStartTime(), 0.0000005);
        assertEquals(2.0, kernelLog.getStopTime(), 0.0000005);
        assertEquals(1, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).size());

        MiscKernelLogItem item = kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).get(0);
        assertEquals(2.0, item.getEventTime(), 0.0000005);
        assertEquals("[    0.000000] Start\n[    2.000000] End", item.getPreamble());
        assertEquals("Last boot reason: hw_reset", item.getStack());
    }

    /**
     * Test that unknown last boot reasons are parsed.
     */
    public void testParseUnknownLastMessage() {
        List<String> lines = Arrays.asList(
                "[    0.000000] Start",
                "[    2.000000] End",
                "Last boot reason: unknown failure");

        KernelLogItem kernelLog = new KernelLogParser().parse(lines);
        assertNotNull(kernelLog);
        assertEquals(0.0, kernelLog.getStartTime(), 0.0000005);
        assertEquals(2.0, kernelLog.getStopTime(), 0.0000005);
        assertEquals(1, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).size());

        MiscKernelLogItem item = kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).get(0);
        assertEquals(2.0, item.getEventTime(), 0.0000005);
        assertEquals("[    0.000000] Start\n[    2.000000] End", item.getPreamble());
        assertNotNull(item.getStack());
    }

    public void testParseKnownGoodLastMessage() {
        List<String> lines = Arrays.asList(
                "[    0.000000] Start",
                "[    2.000000] End",
                "Last boot reason: reboot");

        KernelLogItem kernelLog = new KernelLogParser().parse(lines);
        assertNotNull(kernelLog);
        assertEquals(0.0, kernelLog.getStartTime(), 0.0000005);
        assertEquals(2.0, kernelLog.getStopTime(), 0.0000005);
        assertEquals(0, kernelLog.getEvents().size());
    }

    /**
     * Test that reset reasons don't crash if times are set.
     */
    public void testNoPreviousLogs() {
        List<String> lines = Arrays.asList(
                "Last boot reason: hw_reset");

        KernelLogItem kernelLog = new KernelLogParser().parse(lines);
        assertNotNull(kernelLog);
        assertNull(kernelLog.getStartTime());
        assertNull(kernelLog.getStopTime());
        assertEquals(1, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).size());

        MiscKernelLogItem item = kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).get(0);
        assertNull(item.getEventTime());
        assertEquals("", item.getPreamble());
        assertEquals("Last boot reason: hw_reset", item.getStack());
    }

    /**
     * Test that an empty input returns {@code null}.
     */
    public void testEmptyInput() {
        KernelLogItem item = new KernelLogParser().parse(Arrays.asList(""));
        assertNull(item);
    }

    /**
     * Test that kernel patterns are matched.
     */
    public void testPatterns() {
        List<String> kernelResetPatterns = Arrays.asList(
                "smem: DIAG",
                "smsm: AMSS FATAL ERROR",
                "kernel BUG at ",
                "PVR_K:(Fatal): Debug assertion failed! []",
                "Kernel panic",
                "BP panicked",
                "WROTE DSP RAMDUMP",
                "tegra_wdt: last reset due to watchdog timeout",
                "tegra_wdt tegra_wdt.0: last reset is due to watchdog timeout.",
                "Last reset was MPU Watchdog Timer reset",
                "[MODEM_IF] CRASH",
                "Last boot reason: kernel_panic",
                "Last boot reason: rpm_err",
                "Last boot reason: hw_reset",
                "Last boot reason: wdog_",
                "Last boot reason: tz_err",
                "Last boot reason: adsp_err",
                "Last boot reason: modem_err",
                "Last boot reason: mba_err",
                "Last boot reason: watchdog",
                "Last boot reason: watchdogr",
                "Last boot reason: Watchdog",
                "Last boot reason: Panic",
                "Last reset was system watchdog timer reset");

        LogPatternUtil patternUtil = new KernelLogParser().getLogPatternUtil();

        for (String pattern : kernelResetPatterns) {
            assertEquals(String.format("Message \"%s\" was not matched.", pattern),
                    KernelLogParser.KERNEL_RESET, patternUtil.checkMessage(pattern));
        }

        assertEquals(KernelLogParser.KERNEL_ERROR, patternUtil.checkMessage("Internal error:"));
        assertEquals(KernelLogParser.SELINUX_DENIAL, patternUtil.checkMessage(
                    "avc: denied scontext=0:0:domain:0 "));
    }

    /**
     * Test that an SELinux Denial can be parsed out of a list of log lines.
     */
    public void testSelinuxDenialParse() {
        final String SELINUX_DENIAL_STACK = "type=1400 audit(1384544483.730:10): avc:  denied  " +
                "{ getattr } for  pid=797 comm=\"Binder_5\" path=\"/dev/pts/1\" + " +
                "dev=devpts ino=4 scontext=u:r:system_server:s0 " +
                "tcontext=u:object_r:devpts:s0 tclass=chr_file";
        List<String> lines = Arrays.asList(
                "<4>[    0.000000] Memory policy: ECC disabled, Data cache writealloc",
                "<7>[    7.896355] SELinux: initialized (dev cgroup, type cgroup)" +
                        ", uses genfs_contexts",
                "<5>[   43.399164] " + SELINUX_DENIAL_STACK);
        KernelLogItem kernelLog = new KernelLogParser().parse(lines);

        assertNotNull(kernelLog);
        assertEquals(0.0, kernelLog.getStartTime(), 0.0000005);
        assertEquals(43.399164, kernelLog.getStopTime(), 0.0000005);
        assertEquals(2, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.SELINUX_DENIAL).size());
        assertEquals(1, kernelLog.getSELinuxEvents().size());

        MiscKernelLogItem item = kernelLog.getMiscEvents(KernelLogParser.SELINUX_DENIAL).get(0);
        assertEquals(43.399164, item.getEventTime(), 0.0000005);
        assertEquals(KernelLogParser.SELINUX_DENIAL, item.getCategory());
        assertEquals(SELINUX_DENIAL_STACK, item.getStack());

        SELinuxItem selinuxItem = kernelLog.getSELinuxEvents().get(0);
        assertEquals("system_server", selinuxItem.getSContext());
        assertEquals(43.399164, selinuxItem.getEventTime(), 0.0000005);
        assertEquals(KernelLogParser.SELINUX_DENIAL, selinuxItem.getCategory());
        assertEquals(SELINUX_DENIAL_STACK, selinuxItem.getStack());
    }

    /**
     * Test that an LowMemoryKiller event can be parsed out of a list of log lines.
     */
    public void testLowMemoryKillerParse() {
        final String LMK_MESSAGE = "Killing '.qcrilmsgtunnel' (3699), adj 100,";
        List<String> lines = Arrays.asList(
                "<4>[    0.000000] Memory policy: ECC disabled, Data cache writealloc",
                "<7>[    7.896355] SELinux: initialized (dev cgroup, type cgroup)" +
                        ", uses genfs_contexts",
                "<3>[   43.399164] " + LMK_MESSAGE);
        KernelLogItem kernelLog = new KernelLogParser().parse(lines);

        assertNotNull(kernelLog);
        assertEquals(0.0, kernelLog.getStartTime(), 0.0000005);
        assertEquals(43.399164, kernelLog.getStopTime(), 0.0000005);
        assertEquals(2, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.LOW_MEMORY_KILLER).size());
        assertEquals(1, kernelLog.getLowMemoryKillerEvents().size());

        MiscKernelLogItem miscItem = kernelLog.getMiscEvents(KernelLogParser.LOW_MEMORY_KILLER)
                .get(0);
        assertEquals(43.399164, miscItem.getEventTime(), 0.0000005);
        assertEquals(KernelLogParser.LOW_MEMORY_KILLER, miscItem.getCategory());
        assertEquals(LMK_MESSAGE, miscItem.getStack());

        LowMemoryKillerItem lmkItem = kernelLog.getLowMemoryKillerEvents().get(0);
        assertEquals(KernelLogParser.LOW_MEMORY_KILLER, lmkItem.getCategory());
        assertEquals(3699, lmkItem.getPid());
        assertEquals(".qcrilmsgtunnel", lmkItem.getProcessName());
        assertEquals(100, lmkItem.getAdjustment());
        assertEquals(LMK_MESSAGE, lmkItem.getStack());
    }

    /**
     * Test that page allocation failures can be parsed out of a list of log lines.
     */
    public void testPageAllocationFailureParse() {
        final String ALLOC_FAILURE = "page allocation failure: order:3, mode:0x10c0d0";
        List<String> lines = Arrays.asList(
                "<4>[    0.000000] Memory policy: ECC disabled, Data cache writealloc",
                "<7>[    7.896355] SELinux: initialized (dev cgroup, type cgroup)" +
                        ", uses genfs_contexts",
                "<3>[   43.399164] " + ALLOC_FAILURE);
        KernelLogItem kernelLog = new KernelLogParser().parse(lines);

        assertNotNull(kernelLog);
        assertEquals(0.0, kernelLog.getStartTime(), 0.0000005);
        assertEquals(43.399164, kernelLog.getStopTime(), 0.0000005);
        assertEquals(2, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.PAGE_ALLOC_FAILURE).size());
        assertEquals(1, kernelLog.getPageAllocationFailureEvents().size());

        MiscKernelLogItem miscItem = kernelLog.getMiscEvents(KernelLogParser.PAGE_ALLOC_FAILURE)
                .get(0);
        assertEquals(43.399164, miscItem.getEventTime(), 0.0000005);
        assertEquals(KernelLogParser.PAGE_ALLOC_FAILURE, miscItem.getCategory());
        assertEquals(ALLOC_FAILURE, miscItem.getStack());

        PageAllocationFailureItem failItem = kernelLog.getPageAllocationFailureEvents().get(0);
        assertEquals(KernelLogParser.PAGE_ALLOC_FAILURE, failItem.getCategory());
        assertEquals(3, failItem.getOrder());
        assertEquals(ALLOC_FAILURE, failItem.getStack());
    }

    public void testMantaReset() {
        final List<String> lines = Arrays.asList("[ 3281.347296] ---fimc_is_ischain_close(0)",
                "[ 3281.432055] fimc_is_scalerc_video_close",
                "[ 3281.432270] fimc_is_scalerp_video_close",
                "[ 3287.688303] wm8994-codec wm8994-codec: FIFO error",
                "",
                "No errors detected",
                "Last reset was system watchdog timer reset (RST_STAT=0x100000)");

        KernelLogItem kernelLog = new KernelLogParser().parse(lines);
        assertEquals(1, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).size());
    }

    /**
     * Test that only the first kernel reset is taken but other signatures can have multiple
     */
    public void testMultipleKernelResets() {
        final String SELINUX_DENIAL_STACK = "type=1400 audit(1384544483.730:10): avc:  denied  " +
                "{ getattr } for  pid=797 comm=\"Binder_5\" path=\"/dev/pts/1\" + " +
                "dev=devpts ino=4 scontext=u:r:system_server:s0 " +
                "tcontext=u:object_r:devpts:s0 tclass=chr_file";
        final List<String> lines = Arrays.asList(
                "[ 0.000000] Kernel panic",
                "[ 0.000000] Internal error:",
                "[ 0.000000] " + SELINUX_DENIAL_STACK,
                "[ 1.000000] Kernel panic",
                "[ 1.000000] Internal error:",
                "[ 1.000000] " + SELINUX_DENIAL_STACK,
                "[ 2.000000] Kernel panic",
                "[ 2.000000] Internal error:",
                "[ 2.000000] " + SELINUX_DENIAL_STACK);

        KernelLogItem kernelLog = new KernelLogParser().parse(lines);
        assertEquals(7, kernelLog.getEvents().size());
        assertEquals(1, kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).size());
        assertEquals(0.0,
                kernelLog.getMiscEvents(KernelLogParser.KERNEL_RESET).get(0).getEventTime());
    }
}
