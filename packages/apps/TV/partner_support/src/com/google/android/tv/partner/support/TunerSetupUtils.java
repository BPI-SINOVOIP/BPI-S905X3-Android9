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

package com.google.android.tv.partner.support;

import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.regex.Pattern;

/** Utility class for providing tuner setup. */
public class TunerSetupUtils {
    private static final String TAG = "TunerSetupUtils";

    private static final Pattern CHANNEL_NUMBER_DELIMITER = Pattern.compile("([ .-])");

    public static List<Pair<Lineup, Integer>> lineupChannelMatchCount(
            List<Lineup> lineups, List<String> localChannels) {
        List<Pair<Lineup, Integer>> result = new ArrayList<>();
        List<List<String>> parsedLocalChannels = parseChannelNumbers(localChannels);
        for (Lineup lineup : lineups) {
            result.add(
                    new Pair<>(lineup, getMatchCount(lineup.getChannels(), parsedLocalChannels)));
        }
        // sort in decreasing order
        Collections.sort(
                result,
                new Comparator<Pair<Lineup, Integer>>() {
                    @Override
                    public int compare(Pair<Lineup, Integer> pair, Pair<Lineup, Integer> other) {
                        return Integer.compare(other.second, pair.second);
                    }
                });
        return result;
    }

    @VisibleForTesting
    static int getMatchCount(List<String> lineupChannels, List<List<String>> parsedLocalChannels) {
        int count = 0;
        List<List<String>> parsedLineupChannels = parseChannelNumbers(lineupChannels);
        for (List<String> parsedLineupChannel : parsedLineupChannels) {
            for (List<String> parsedLocalChannel : parsedLocalChannels) {
                if (matchChannelNumber(parsedLineupChannel, parsedLocalChannel)) {
                    count++;
                    break;
                }
            }
        }
        return count;
    }

    /**
     * Parses the channel number string to a list of numbers (major number, minor number, etc.).
     *
     * @param channelNumber the display number of the channel
     * @return a list of numbers
     */
    @VisibleForTesting
    static List<String> parseChannelNumber(String channelNumber) {
        List<String> numbers =
                new ArrayList<>(
                        Arrays.asList(TextUtils.split(channelNumber, CHANNEL_NUMBER_DELIMITER)));
        numbers.removeAll(Collections.singleton(""));
        if (numbers.size() < 1 || numbers.size() > 2) {
            Log.w(TAG, "unsupported channel number format: " + channelNumber);
            return new ArrayList<>();
        }
        return numbers;
    }

    /**
     * Parses a list of channel numbers. See {@link #parseChannelNumber(String)}.
     *
     * @param channelNumbers a list of channel display numbers
     */
    @VisibleForTesting
    static List<List<String>> parseChannelNumbers(List<String> channelNumbers) {
        List<List<String>> numbers = new ArrayList<>(channelNumbers.size());
        for (String channelNumber : channelNumbers) {
            if (!TextUtils.isEmpty(channelNumber)) {
                numbers.add(parseChannelNumber(channelNumber));
            }
        }
        return numbers;
    }

    /**
     * Checks whether two lists of channel numbers match or not. If the sizes are different,
     * additional elements are ignore.
     */
    @VisibleForTesting
    static boolean matchChannelNumber(List<String> numbers, List<String> other) {
        if (numbers.isEmpty() || other.isEmpty()) {
            return false;
        }
        int i = 0;
        int j = 0;
        while (i < numbers.size() && j < other.size()) {
            if (!numbers.get(i++).equals(other.get(j++))) {
                return false;
            }
        }
        return true;
    }
}
