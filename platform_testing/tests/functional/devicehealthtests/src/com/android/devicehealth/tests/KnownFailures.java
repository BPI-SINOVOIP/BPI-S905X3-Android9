/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.devicehealth.tests;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A class to manage and match known failures
 */
public class KnownFailures {

    private Map<String, List<KnownFailureItem>> mKnownFailures = new HashMap<>();
    {
        // add known failures per dropbox label below:
        // Example:
        // addKnownFailure(
        //         "system_app_anr",  // dropbox label
        //         Pattern.compile("^.*Process:.*com\\.google\\.android\\.googlequicksearchbox"
        //                 + ":search.*$", Pattern.MULTILINE),  // regex pattern
        //         "73254069");  // bug number
        // add known failures for additional dropbox labels here
        // It's recommended to use multiline matching and a snippet will be matched if it's found
        // to contain the pattern described by regex
    }

    /**
     * Convenience method to add a known failure to list
     * @param dropboxLabel
     * @param pattern
     * @param bugNumber
     */
    public void addKnownFailure(String dropboxLabel, Pattern pattern, String bugNumber) {
        List<KnownFailureItem> knownFailures = mKnownFailures.get(dropboxLabel);
        if (knownFailures == null) {
            knownFailures = new ArrayList<>();
            mKnownFailures.put(dropboxLabel, knownFailures);
        }
        knownFailures.add(new KnownFailureItem(pattern, bugNumber));
    }

    /**
     * Check if a dropbox snippet of the specificed label type contains a known failure
     * @param dropboxLabel
     * @param dropboxSnippet
     * @return the first known failure pattern matched, or <code>null</code> if none matched
     */
    public KnownFailureItem findMatchedKnownFailure(String dropboxLabel, String dropboxSnippet) {
        List<KnownFailureItem> knownFailureItems = mKnownFailures.get(dropboxLabel);
        if (knownFailureItems != null) {
            for (KnownFailureItem k : knownFailureItems) {
                Matcher m = k.failurePattern.matcher(dropboxSnippet);
                if (m.find()) {
                    return k;
                }
            }
        }
        return null;
    }
}
