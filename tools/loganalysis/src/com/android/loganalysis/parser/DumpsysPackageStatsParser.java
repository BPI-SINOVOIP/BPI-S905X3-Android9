/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.loganalysis.item.AppVersionItem;
import com.android.loganalysis.item.DumpsysPackageStatsItem;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** A {@link IParser} to parse package stats and create a mapping table of packages and versions */
public class DumpsysPackageStatsParser implements IParser {

    /** Matches: Package [com.google.android.googlequicksearchbox] (607929e): */
    private static final Pattern PACKAGE_NAME = Pattern.compile("^\\s*Package \\[(.*)\\].*");

    /** Matches: versionCode=300734793 minSdk=10000 targetSdk=10000 */
    private static final Pattern VERSION_CODE = Pattern.compile("^\\s*versionCode=(\\d+).*");

    /** Matches: versionName=6.16.35.26.arm64 */
    private static final Pattern VERSION_NAME = Pattern.compile("^\\s*versionName=(.*)$");

    /**
     * {@inheritDoc}
     *
     * @return The {@link DumpsysPackageStatsItem}.
     */
    @Override
    public DumpsysPackageStatsItem parse(List<String> lines) {
        DumpsysPackageStatsItem item = new DumpsysPackageStatsItem();
        String packageName = null;
        String versionCode = null;
        String versionName = null;

        for (String line : lines) {
            Matcher m = PACKAGE_NAME.matcher(line);
            if (m.matches()) {
                packageName = m.group(1);
                versionCode = null;
                versionName = null;
                continue;
            }
            m = VERSION_CODE.matcher(line);
            if (m.matches()) {
                versionCode = m.group(1);
                continue;
            }
            m = VERSION_NAME.matcher(line);
            if (m.matches()) {
                versionName = m.group(1).trim();
                if (packageName != null && versionCode != null) {
                    item.put(
                            packageName,
                            new AppVersionItem(Integer.parseInt(versionCode), versionName));
                }
                packageName = null;
            }
        }
        return item;
    }
}
