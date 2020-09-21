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
 * An {@link IItem} used to store Java crash info.
 */
public class JavaCrashItem extends MiscLogcatItem {

    /** Constant for JSON output */
    public static final String EXCEPTION = "EXCEPTION";
    /** Constant for JSON output */
    public static final String MESSAGE = "MESSAGE";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            EXCEPTION, MESSAGE));

    /**
     * The constructor for {@link JavaCrashItem}.
     */
    public JavaCrashItem() {
        super(ATTRIBUTES);
        setCategory(LogcatParser.JAVA_CRASH);
    }

    /**
     * Get the exception for the Java crash.
     */
    public String getException() {
        return (String) getAttribute(EXCEPTION);
    }

    /**
     * Get the exception for the Java crash.
     */
    public void setException(String exception) {
        setAttribute(EXCEPTION, exception);
    }

    /**
     * Get the message for the Java crash.
     */
    public String getMessage() {
        return (String) getAttribute(MESSAGE);
    }

    /**
     * Set the message for the Java crash.
     */
    public void setMessage(String message) {
        setAttribute(MESSAGE, message);
    }
}
