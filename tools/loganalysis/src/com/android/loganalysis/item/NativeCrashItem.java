/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.loganalysis.parser.LogcatParser;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link IItem} used to store native crash info.
 */
public class NativeCrashItem extends MiscLogcatItem {

    /** Constant for JSON output */
    public static final String FINGERPRINT = "FINGERPRINT";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            FINGERPRINT));

    /**
     * The constructor for {@link NativeCrashItem}.
     */
    public NativeCrashItem() {
        super(ATTRIBUTES);
        setCategory(LogcatParser.NATIVE_CRASH);
    }

    /**
     * Get the fingerprint for the crash.
     */
    public String getFingerprint() {
        return (String) getAttribute(FINGERPRINT);
    }

    /**
     * Set the fingerprint for the crash.
     */
    public void setFingerprint(String fingerprint) {
        setAttribute(FINGERPRINT, fingerprint);
    }
}
