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

import com.android.loganalysis.item.BatteryUsageItem;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse battery usage statistics
 */
public class BatteryUsageParser implements IParser {

    /**
     * Matches: Capacity: 3220, Computed drain: 11.0, actual drain: 0
     */
    private static final Pattern Capacity = Pattern.compile(
            "^\\s*Capacity: (\\d+), Computed drain: \\d+.*");

    private static final Pattern Usage = Pattern.compile("^\\s*(.*): (\\d+(\\.\\d*)?)");
    private BatteryUsageItem mItem = new BatteryUsageItem();

    /**
     * {@inheritDoc}
     *
     * @return The {@link BatteryUsageItem}.
     */
    @Override
    public BatteryUsageItem parse(List<String> lines) {
        for (String line : lines) {
            Matcher m = Capacity.matcher(line);
            if(m.matches()) {
                mItem.setBatteryCapacity(Integer.parseInt(m.group(1)));
            } else {
                m = Usage.matcher(line);
                if (m.matches()) {
                    mItem.addBatteryUsage(m.group(1), Double.parseDouble(m.group(2)));
                }
            }
        }
        return mItem;
    }

    /**
     * Get the {@link BatteryUsageItem}.
     * <p>
     * Exposed for unit testing.
     * </p>
     */
    BatteryUsageItem getItem() {
        return mItem;
    }
}
