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

package com.android.settings.intelligence.search.query;

import android.text.TextUtils;

/**
 * Utils for Query-time operations.
 */

public class SearchQueryUtils {

    public static final int NAME_NO_MATCH = -1;

    /**
     * Returns "difference" between resultName and query string. resultName must contain all
     * characters from query as a prefix to a word, in the same order.
     * If not, returns NAME_NO_MATCH.
     * If they do match, returns an int value representing  how different they are,
     * and larger values means they are less similar.
     * <p/>
     * Example:
     * resultName: Abcde, query: Abcde, Returns 0
     * resultName: Abcde, query: abc, Returns 2
     * resultName: Abcde, query: ab, Returns 3
     * resultName: Abcde, query: bc, Returns NAME_NO_MATCH
     * resultName: Abcde, query: xyz, Returns NAME_NO_MATCH
     * resultName: Abc de, query: de, Returns 4
     */
    public static int getWordDifference(String resultName, String query) {
        if (TextUtils.isEmpty(resultName) || TextUtils.isEmpty(query)) {
            return NAME_NO_MATCH;
        }

        final char[] queryTokens = query.toLowerCase().toCharArray();
        final char[] resultTokens = resultName.toLowerCase().toCharArray();
        final int resultLength = resultTokens.length;
        if (queryTokens.length > resultLength) {
            return NAME_NO_MATCH;
        }

        int i = 0;
        int j;

        while (i < resultLength) {
            j = 0;
            // Currently matching a prefix
            while ((i + j < resultLength) && (queryTokens[j] == resultTokens[i + j])) {
                // Matched the entire query
                if (++j >= queryTokens.length) {
                    // Use the diff in length as a proxy of how close the 2 words match.
                    // Value range from 0 to infinity.
                    return resultLength - queryTokens.length;
                }
            }

            i += j;

            // Remaining string is longer that the query or we have search the whole result name.
            if (queryTokens.length > resultLength - i) {
                return NAME_NO_MATCH;
            }

            // This is the first index where result name and query name are different
            // Find the next space in the result name or the end of the result name.
            while ((i < resultLength) && (!Character.isWhitespace(resultTokens[i++]))) ;

            // Find the start of the next word
            while ((i < resultLength) && !(Character.isLetter(resultTokens[i])
                    || Character.isDigit(resultTokens[i]))) {
                // Increment in body because we cannot guarantee which condition was true
                i++;
            }
        }
        return NAME_NO_MATCH;
    }
}
