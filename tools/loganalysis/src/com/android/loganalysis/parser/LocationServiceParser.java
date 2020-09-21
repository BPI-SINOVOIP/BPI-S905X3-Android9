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

import com.android.loganalysis.item.LocationDumpsItem;
import com.android.loganalysis.util.NumberFormattingUtil;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to handle the parsing of location request information
 */
public class LocationServiceParser implements IParser {
    /**
     * Match a valid line such as:
     * "Interval effective/min/max 1/0/0[s] Duration: 140[minutes] [com.google.android.gms,
     * PRIORITY_NO_POWER, UserLocationProducer] Num requests: 2 Active: true"
     */
    private static final Pattern LOCATION_PAT = Pattern.compile(
            "^\\s*Interval effective/min/max (\\d+)/(\\d+)/(\\d+)\\[s\\] Duration: (\\d+)"
            + "\\[minutes\\]\\s*\\[([\\w.]+), (\\w+)(,.*)?\\] Num requests: \\d+ Active: \\w*");

    private LocationDumpsItem mItem = new LocationDumpsItem();

    /**
     * {@inheritDoc}
     */
    @Override
    public LocationDumpsItem parse(List<String> lines) {
        Matcher m = null;
        for (String line : lines) {
            m = LOCATION_PAT.matcher(line);
            if (m.matches()) {
                mItem.addLocationClient(m.group(5), NumberFormattingUtil.parseIntOrZero(m.group(1)),
                        NumberFormattingUtil.parseIntOrZero(m.group(2)),
                        NumberFormattingUtil.parseIntOrZero(m.group(3)), m.group(6),
                        NumberFormattingUtil.parseIntOrZero(m.group(4)));
                continue;
            }
        }
        return mItem;
    }

    /**
     * Get the {@link LocationDumpsItem}.
     * <p>
     * Exposed for unit testing.
     * </p>
     */
    LocationDumpsItem getItem() {
        return mItem;
    }
}
