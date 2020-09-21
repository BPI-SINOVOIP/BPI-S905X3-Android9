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
package com.android.loganalysis.item;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/** An {@link IItem} used to store an app's version code and name. */
public class AppVersionItem extends GenericItem {

    /** Constants for JSON output */
    public static final String VERSION_CODE = "VERSION_CODE";

    public static final String VERSION_NAME = "VERSION_NAME";

    private static final Set<String> ATTRIBUTES =
            new HashSet<String>(Arrays.asList(VERSION_CODE, VERSION_NAME));

    /**
     * The constructor for {@link AppVersionItem}
     *
     * @param versionCode the version code
     * @param versionName the version name
     */
    public AppVersionItem(int versionCode, String versionName) {
        super(ATTRIBUTES);

        setAttribute(VERSION_CODE, versionCode);
        setAttribute(VERSION_NAME, versionName);
    }

    public int getVersionCode() {
        return (Integer) getAttribute(VERSION_CODE);
    }

    public String getVersionName() {
        return (String) getAttribute(VERSION_NAME);
    }
}
