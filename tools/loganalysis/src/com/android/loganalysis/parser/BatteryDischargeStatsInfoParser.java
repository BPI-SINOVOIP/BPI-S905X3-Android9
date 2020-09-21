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
import com.android.loganalysis.util.NumberFormattingUtil;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse the battery discharge section.
 */
public class BatteryDischargeStatsInfoParser extends AbstractSectionParser {

    /** Matches; #47: +5m5s105ms to 47 (screen-on, power-save-off, device-idle-off) */
    private static final Pattern DISCHARGE_STEP_PATTERN =
            Pattern.compile("^.*: \\+((\\d+)h)?((\\d+)m)?((\\d+)s)?(\\d+)ms.* to (\\d+).*");

    /**
     * {@inheritDoc}
     *
     * @return The {@link BatteryDischargeStatsInfoItem}.
     */
    @Override
    public BatteryDischargeStatsInfoItem parse(List<String> lines) {
        long totalDuration = 0;
        long projectionDuration = 0;
        Integer minPercent = null;
        Integer maxPercent = null;
        Integer minProjectionPercent = null;
        Integer maxProjectionPercent = null;

        for (String line : lines) {
            Matcher m = DISCHARGE_STEP_PATTERN.matcher(line);

            if (m.matches()) {
                int percent = Integer.parseInt(m.group(8));

                if (minPercent == null || percent < minPercent) {
                    minPercent = percent;
                }

                if (maxPercent == null || maxPercent < percent) {
                    maxPercent = percent;
                }

                long duration = NumberFormattingUtil.getMs(
                    NumberFormattingUtil.parseIntOrZero(m.group(2)),
                    NumberFormattingUtil.parseIntOrZero(m.group(4)),
                    NumberFormattingUtil.parseIntOrZero(m.group(6)),
                    NumberFormattingUtil.parseIntOrZero(m.group(7)));

                totalDuration += duration;

                // For computing the projected battery life we drop the first 5% of the battery
                // charge because these discharge 'slower' and are not reliable for the projection.
                if (percent > 94) {
                    continue;
                }

                if (minProjectionPercent == null || percent < minProjectionPercent) {
                    minProjectionPercent = percent;
                }

                if (maxProjectionPercent == null || maxProjectionPercent < percent) {
                    maxProjectionPercent = percent;
                }

                projectionDuration += duration;
            }
        }

        if (minPercent == null) {
            return null;
        }

        int dischargePercent = maxPercent - minPercent + 1;

        BatteryDischargeStatsInfoItem item = new BatteryDischargeStatsInfoItem();
        item.setDischargeDuration(totalDuration);
        item.setDischargePercentage(dischargePercent);
        item.setMaxPercentage(maxPercent);
        item.setMinPercentage(minPercent);

        if (minProjectionPercent == null) {
            return item;
        }

        int projectionDischargePercent = maxProjectionPercent - minProjectionPercent + 1;
        item.setProjectedBatteryLife((projectionDuration * 100) / projectionDischargePercent);
        return item;
    }
}
