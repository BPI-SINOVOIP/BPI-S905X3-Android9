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
package com.android.loganalysis.util;

import java.util.HashSet;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A utility class for matching a message against a set of patterns.
 * <p>
 * This is used to match a message against a set of patterns, and optionally, an extra object. If
 * the message is matched, a category will be returned. This means that a single object can be used
 * to match many different categories.
 */
public class LogPatternUtil {

    /**
     * A class used to store pattern, extras, and category.
     */
    private class PatternInfo {
        public Pattern mPattern;
        public Object mExtras;
        public String mCategory;

        /**
         * Constructor for {@link PatternInfo}
         *
         * @param pattern the {@link Pattern} to match against.
         * @param extras the {@link Object} to additionally match against.  If extras is null, it
         * will be treated as wildcard
         * @param category the category to return if there is a match.
         */
        public PatternInfo(Pattern pattern, Object extras, String category) {
            mPattern = pattern;
            mExtras = extras;
            mCategory = category;
        }
    }

    private Set<PatternInfo> mPatterns = new HashSet<PatternInfo>();

    /**
     * Add a pattern to this list of patterns to match against.
     *
     * @param pattern the {@link Pattern} object to match against.
     * @param category the category to return if there is a match.
     */
    public void addPattern(Pattern pattern, String category) {
        addPattern(pattern, null, category);
    }

    /**
     * Add a pattern to this list of patterns to match against.
     *
     * @param pattern the {@link Pattern} to match against.
     * @param extras the {@link Object} to additionally match against.  If extras is null, it will
     * be treated as wildcard
     * @param category the category to return if there is a match.
     */
    public void addPattern(Pattern pattern, Object extras, String category) {
        mPatterns.add(new PatternInfo(pattern, extras, category));
    }

    /**
     * Checks to see if the message matches any patterns.
     *
     * @param message the message to match against
     * @return The category of the match.
     */
    public String checkMessage(String message) {
        return checkMessage(message, null);
    }

    /**
     * Checks to see if the message matches any patterns.
     *
     * @param message the message to match against
     * @param extras the extras to match against
     * @return The category of the match.
     */
    public String checkMessage(String message, Object extras) {
        for (PatternInfo patternInfo : mPatterns) {
            Matcher m = patternInfo.mPattern.matcher(message);

            // Return the category if the pattern matches and the extras are equal. Treat a null
            // patternInfo.mExtras as a wildcard.
            if (m.matches() &&
                    (patternInfo.mExtras == null || patternInfo.mExtras.equals(extras))) {
                return patternInfo.mCategory;
            }
        }
        return null;
    }
}
