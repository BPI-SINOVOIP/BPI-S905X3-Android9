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

import com.android.loganalysis.item.WakelockItem;
import com.android.loganalysis.item.WakelockItem.WakeLockCategory;
import com.android.loganalysis.util.NumberFormattingUtil;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to handle the parsing of wakelock information
 */
public class WakelockParser implements IParser {

    private static final String WAKE_LOCK_PAT_SUFFIX =
            "(?:(\\d+)d)?\\s?(?:(\\d+)h)?\\s?(?:(\\d+)m)?\\s?(?:(\\d+)s)?\\s?(?:(\\d+)ms)?"
            + "\\s?\\((\\d+) times\\)(?: max=\\d+)? realtime";

    /**
     * Match a valid line such as:
     * "  Kernel Wake lock PowerManagerService.WakeLocks: 1h 13m 50s 950ms (2858 times) realtime"
     */
    private static final Pattern KERNEL_WAKE_LOCK_PAT = Pattern.compile(
            "^\\s*Kernel Wake lock (.+): " + WAKE_LOCK_PAT_SUFFIX);
    /**
     * Match a valid line such as:
     * "  Wake lock u0a7 NlpWakeLock: 8m 13s 203ms (1479 times) realtime";
     */
    private static final Pattern PARTIAL_WAKE_LOCK_PAT = Pattern.compile(
            "^\\s*Wake lock (.*)\\s(.+): " + WAKE_LOCK_PAT_SUFFIX);

    private WakelockItem mItem = new WakelockItem();

    public static final int TOP_WAKELOCK_COUNT = 5;

    /**
     * {@inheritDoc}
     */
    @Override
    public WakelockItem parse(List<String> lines) {
        Matcher m = null;
        int wakelockCounter = 0;
        for (String line : lines) {
            if (wakelockCounter >= TOP_WAKELOCK_COUNT || "".equals(line.trim())) {
                // Done with wakelock parsing
                break;
            }

            m = KERNEL_WAKE_LOCK_PAT.matcher(line);
            if (m.matches() && !line.contains("PowerManagerService.WakeLocks")) {
                parseKernelWakeLock(line, WakeLockCategory.KERNEL_WAKELOCK);
                wakelockCounter++;
                continue;
            }

            m = PARTIAL_WAKE_LOCK_PAT.matcher(line);
            if (m.matches()) {
                parsePartialWakeLock(line, WakeLockCategory.PARTIAL_WAKELOCK);
                wakelockCounter++;
            }
        }

        return mItem;
    }

    /**
     * Parse a line of output and add it to wakelock section
     * <p>
     * Exposed for unit testing.
     * </p>
     */
    void parseKernelWakeLock(String line, WakeLockCategory category) {
        Matcher m = KERNEL_WAKE_LOCK_PAT.matcher(line);
        if (!m.matches()) {
            return;
        }
        final String name = m.group(1);
        final long wakelockTime = NumberFormattingUtil.getMs(
                NumberFormattingUtil.parseIntOrZero(m.group(2)),
                NumberFormattingUtil.parseIntOrZero(m.group(3)),
                NumberFormattingUtil.parseIntOrZero(m.group(4)),
                NumberFormattingUtil.parseIntOrZero(m.group(5)),
                NumberFormattingUtil.parseIntOrZero(m.group(6)));

        final int timesCalled = Integer.parseInt(m.group(7));

        mItem.addWakeLock(name, wakelockTime, timesCalled, category);
    }

    /**
     * Parse a line of output and add it to wake lock section.
     * <p>
     * Exposed for unit testing.
     * </p>
     */
    void parsePartialWakeLock(String line, WakeLockCategory category) {
        Matcher m = PARTIAL_WAKE_LOCK_PAT.matcher(line);
        if (!m.matches()) {
            return;
        }
        final String processUID = m.group(1);
        final String name = m.group(2);
        final long wakelockTime = NumberFormattingUtil.getMs(
                NumberFormattingUtil.parseIntOrZero(m.group(3)),
                NumberFormattingUtil.parseIntOrZero(m.group(4)),
                NumberFormattingUtil.parseIntOrZero(m.group(5)),
                NumberFormattingUtil.parseIntOrZero(m.group(6)),
                NumberFormattingUtil.parseIntOrZero(m.group(7)));
        final int timesCalled = Integer.parseInt(m.group(8));

        mItem.addWakeLock(name, processUID, wakelockTime, timesCalled, category);
    }

    /**
     * Get the {@link WakelockItem}.
     * <p>
     * Exposed for unit testing.
     * </p>
     */
    WakelockItem getItem() {
        return mItem;
    }
}
