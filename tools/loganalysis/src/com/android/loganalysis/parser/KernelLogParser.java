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
import com.android.loganalysis.util.LogTailUtil;

import java.io.BufferedReader;
import java.io.IOException;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
* A {@link IParser} to parse {@code /proc/last_kmsg}, {@code /proc/kmsg}, and the output from
* {@code dmesg}.
*/
public class KernelLogParser implements IParser {
    public static final String KERNEL_RESET = "KERNEL_RESET";
    public static final String KERNEL_ERROR = "KERNEL_ERROR";
    public static final String SELINUX_DENIAL = "SELINUX_DENIAL";
    public static final String NORMAL_REBOOT = "NORMAL_REBOOT";
    public static final String PAGE_ALLOC_FAILURE = "PAGE_ALLOC_FAILURE";
    public static final String LOW_MEMORY_KILLER = "LOW_MEMORY_KILLER";

    /**
     * Matches: [     0.000000] Message<br />
     * Matches: &lt;3&gt;[     0.000000] Message
     */
    private static final Pattern LOG_LINE = Pattern.compile(
            "^(<\\d+>)?\\[\\s*(\\d+\\.\\d{6})\\] (.*)$");
    private static final Pattern SELINUX_DENIAL_PATTERN = Pattern.compile(
            ".*avc:\\s.*scontext=\\w*:\\w*:([\\w\\s]*):\\w*\\s.*");
    // Matches: page allocation failure: order:3, mode:0x10c0d0
    private static final Pattern PAGE_ALLOC_FAILURE_PATTERN = Pattern.compile(
            ".*page\\s+allocation\\s+failure:\\s+order:(\\d+).*");
    // Matches: Killing '.qcrilmsgtunnel' (3699), adj 100,
    private static final Pattern LOW_MEMORY_KILLER_PATTERN = Pattern.compile(
            ".*Killing\\s+'(.*)'\\s+\\((\\d+)\\),.*adj\\s+(\\d+).*");

    /**
     * Regular expression representing all known bootreasons which are bad.
     */
    public static final Pattern BAD_BOOTREASONS = Pattern.compile(
            "(?:kernel_panic.*|rpm_err|hw_reset(?:$|\\n)|wdog_.*|tz_err|adsp_err|modem_err|mba_err|"
            + "watchdog.*|Watchdog|Panic|srto:.*|oemerr.*)");

    /**
     * Regular expression representing all known bootreasons which are good.
     */
    public static final Pattern GOOD_BOOTREASONS = Pattern.compile(
            "(?:PowerKey|normal|recovery|reboot.*)");

    private boolean mAddUnknownBootreason = true;

    private KernelLogItem mKernelLog = null;
    private Double mStartTime = null;
    private Double mStopTime = null;

    private LogPatternUtil mPatternUtil = new LogPatternUtil();
    private LogTailUtil mPreambleUtil = new LogTailUtil(500, 50, 50);
    private boolean mBootreasonFound = false;

    public KernelLogParser() {
        initPatterns();
    }

    public void setAddUnknownBootreason(boolean enable) {
        mAddUnknownBootreason = enable;
    }

    /**
     * Parse a kernel log from a {@link BufferedReader} into an {@link KernelLogItem} object.
     *
     * @return The {@link KernelLogItem}.
     * @see #parse(List)
     */
    public KernelLogItem parse(BufferedReader input) throws IOException {
        String line;
        while ((line = input.readLine()) != null) {
            parseLine(line);
        }
        commit();

        return mKernelLog;
    }

    /**
     * {@inheritDoc}
     *
     * @return The {@link KernelLogItem}.
     */
    @Override
    public KernelLogItem parse(List<String> lines) {
        mBootreasonFound = false;
        for (String line : lines) {
            parseLine(line);
        }
        commit();

        return mKernelLog;
    }

    /**
     * Parse a line of input.
     *
     * @param line The line to parse
     */
    private void parseLine(String line) {
        if ("".equals(line.trim())) {
            return;
        }
        if (mKernelLog == null) {
            mKernelLog = new KernelLogItem();
        }
        Matcher m = LOG_LINE.matcher(line);
        if (m.matches()) {
            Double time = Double.parseDouble(m.group(2));
            String msg = m.group(3);

            if (mStartTime == null) {
                mStartTime = time;
            }
            mStopTime = time;

            checkAndAddKernelEvent(msg);

            mPreambleUtil.addLine(null, line);
        } else {
            checkAndAddKernelEvent(line);
        }
    }

    /**
     * Checks if a kernel log message matches a pattern and add a kernel event if it does.
     */
    private void checkAndAddKernelEvent(String message) {
        String category = mPatternUtil.checkMessage(message);
        if (category == null) {
            return;
        }

        if (category.equals(KERNEL_RESET) || category.equals(NORMAL_REBOOT)) {
            mBootreasonFound = true;
        }

        if (category.equals(NORMAL_REBOOT)) {
            return;
        }

        MiscKernelLogItem kernelLogItem;
        if (category.equals(SELINUX_DENIAL)) {
            SELinuxItem selinuxItem = new SELinuxItem();
            Matcher m = SELINUX_DENIAL_PATTERN.matcher(message);
            if (m.matches()) {
                selinuxItem.setSContext(m.group(1));
            }
            kernelLogItem = selinuxItem;
        } else if (category.equals(PAGE_ALLOC_FAILURE)) {
            PageAllocationFailureItem allocItem = new PageAllocationFailureItem();
            Matcher m = PAGE_ALLOC_FAILURE_PATTERN.matcher(message);
            if (m.matches()) {
                allocItem.setOrder(Integer.parseInt(m.group(1)));
            }
            kernelLogItem = allocItem;
        } else if (category.equals(LOW_MEMORY_KILLER)) {
            LowMemoryKillerItem lmkItem = new LowMemoryKillerItem();
            Matcher m = LOW_MEMORY_KILLER_PATTERN.matcher(message);
            if (m.matches()) {
                lmkItem.setProcessName(m.group(1));
                lmkItem.setPid(Integer.parseInt(m.group(2)));
                lmkItem.setAdjustment(Integer.parseInt(m.group(3)));
            }
            kernelLogItem = lmkItem;
        } else {
            kernelLogItem = new MiscKernelLogItem();
        }
        kernelLogItem.setEventTime(mStopTime);
        kernelLogItem.setPreamble(mPreambleUtil.getLastTail());
        kernelLogItem.setStack(message);
        kernelLogItem.setCategory(category);
        mKernelLog.addEvent(kernelLogItem);
    }

    /**
     * Signal that the input has finished.
     */
    private void commit() {
        if (mKernelLog == null) {
            return;
        }
        mKernelLog.setStartTime(mStartTime);
        mKernelLog.setStopTime(mStopTime);

        if (mAddUnknownBootreason && !mBootreasonFound) {
            MiscKernelLogItem unknownReset = new MiscKernelLogItem();
            unknownReset.setEventTime(mStopTime);
            unknownReset.setPreamble(mPreambleUtil.getLastTail());
            unknownReset.setCategory(KERNEL_RESET);
            unknownReset.setStack("Unknown reason");
            mKernelLog.addEvent(unknownReset);
        }
    }

    private void initPatterns() {
        // Kernel resets
        // TODO: Separate out device specific patterns
        final String[] kernelResets = {
            "smem: DIAG.*",
            "smsm: AMSS FATAL ERROR.*",
            "kernel BUG at .*",
            "BUG: failure at .*",
            "PVR_K:\\(Fatal\\): Debug assertion failed! \\[.*\\]",
            "Kernel panic.*",
            "Unable to handle kernel paging request.*",
            "BP panicked",
            "WROTE DSP RAMDUMP",
            "tegra_wdt: last reset due to watchdog timeout.*",
            "tegra_wdt tegra_wdt.0: last reset is due to watchdog timeout.*",
            "Last reset was MPU Watchdog Timer reset.*",
            "\\[MODEM_IF\\].*CRASH.*",
            "Last boot reason: " + BAD_BOOTREASONS,
            "Last reset was system watchdog timer reset.*",
        };
        final String[] goodSignatures = {
                "Restarting system.*",
                "Power down.*",
                "Last boot reason: " + GOOD_BOOTREASONS,
        };
        for (String pattern : kernelResets) {
            mPatternUtil.addPattern(Pattern.compile(pattern), KERNEL_RESET);
        }
        for (String pattern : goodSignatures) {
            mPatternUtil.addPattern(Pattern.compile(pattern), NORMAL_REBOOT);
        }

        mPatternUtil.addPattern(Pattern.compile("Internal error:.*"), KERNEL_ERROR);

        // SELINUX denials
        mPatternUtil.addPattern(SELINUX_DENIAL_PATTERN, SELINUX_DENIAL);
        // Page allocation failures
        mPatternUtil.addPattern(PAGE_ALLOC_FAILURE_PATTERN, PAGE_ALLOC_FAILURE);
        // Lowmemorykiller kills
        mPatternUtil.addPattern(LOW_MEMORY_KILLER_PATTERN, LOW_MEMORY_KILLER);
    }

    /**
     * Get the internal {@link LogPatternUtil}. Exposed for unit testing.
     */
    LogPatternUtil getLogPatternUtil() {
        return mPatternUtil;
    }
}
