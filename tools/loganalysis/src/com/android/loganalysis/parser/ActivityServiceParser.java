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
import com.android.loganalysis.item.ActivityServiceItem;

import java.util.List;

/**
 * A {@link IParser} to parse the activity service dump section of the bugreport
 */
public class ActivityServiceParser extends AbstractSectionParser {

    private static final String LOCATION_SECTION_REGEX =
            "^\\s*SERVICE com.google.android.gms/"
            + "com.google.android.location.internal.GoogleLocationManagerService \\w+ pid=\\d+";

    private static final String NOOP_SECTION_REGEX = "^\\s*SERVICE .*/.*";

    private LocationServiceParser mLocationParser = new LocationServiceParser();

    private ActivityServiceItem mActivityServiceItem= null;
    private boolean mParsedInput = false;

    /**
     * {@inheritDoc}
     *
     * @return The {@link ActivityServiceItem}
     */
    @Override
    public ActivityServiceItem parse(List<String> lines) {
        setup();
        for (String line : lines) {
            if (!mParsedInput && !"".equals(line.trim())) {
                mParsedInput = true;
            }
            parseLine(line);
        }
        commit();
        return mActivityServiceItem;
    }

    /**
     * Sets up the parser by adding the section parsers.
     */
    protected void setup() {
        addSectionParser(mLocationParser, LOCATION_SECTION_REGEX);
        addSectionParser(new NoopParser(), NOOP_SECTION_REGEX);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void commit() {
        // signal EOF
        super.commit();
        if (mParsedInput) {
            if (mActivityServiceItem == null) {
                mActivityServiceItem = new ActivityServiceItem();
            }
        }

        if (mActivityServiceItem != null) {
            mActivityServiceItem.setLocationDumps(
                    (LocationDumpsItem) getSection(mLocationParser));
        }
    }
}
