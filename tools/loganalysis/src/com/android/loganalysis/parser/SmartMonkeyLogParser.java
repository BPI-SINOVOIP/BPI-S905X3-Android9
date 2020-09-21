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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.SmartMonkeyLogItem;

import java.io.BufferedReader;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse monkey logs.
 */
public class SmartMonkeyLogParser implements IParser {

    private static final String TIME_STAMP_GROUP =
            "^(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}.\\d{3}): ";
    private static final String INVOKE_NUM_GROUP = "\\[.*?(\\d+)\\]";
    private static final String SEQ_NUM_GROUP = "\\(Seq:.*?(\\d+)\\)";

    private static final Pattern START_TIME = Pattern.compile(
            TIME_STAMP_GROUP + "Starting.*");

    private static final Pattern START_UPTIME = Pattern.compile(
            TIME_STAMP_GROUP + "Device uptime: (\\d+) sec$");

    private static final Pattern STOP_UPTIME = Pattern.compile(
            TIME_STAMP_GROUP + "Device uptime: (\\d+) sec, Monkey run duration: (\\d+) sec$");

    private static final Pattern THROTTLE = Pattern.compile(
            TIME_STAMP_GROUP + "Throttle: (\\d+).*");

    private static final Pattern TARGET_INVOCATIONS = Pattern.compile(
            TIME_STAMP_GROUP + "Target invocation count: (\\d+)");

    private static final Pattern INTERMEDIATE_COUNT = Pattern.compile(
            TIME_STAMP_GROUP + INVOKE_NUM_GROUP + SEQ_NUM_GROUP + ".*");

    private static final Pattern INTERMEDIATE_TIME = Pattern.compile(TIME_STAMP_GROUP + ".*");

    private static final Pattern FINISHED = Pattern.compile(
            TIME_STAMP_GROUP + "Monkey finished");

    private static final Pattern FINAL_COUNT = Pattern.compile(
            TIME_STAMP_GROUP + "Invocations completed: (\\d+)");

    private static final Pattern APPS_PACKAGES = Pattern.compile(
            TIME_STAMP_GROUP + "Starting \\[(.*)\\]\\[(.*)\\]");

    private static final Pattern ABORTED = Pattern.compile(
            TIME_STAMP_GROUP + "Monkey aborted.");

    private static final Pattern UI_ANR = Pattern.compile(
            TIME_STAMP_GROUP + INVOKE_NUM_GROUP + SEQ_NUM_GROUP + "-UI Exception: ANR: (.*)");

    private static final Pattern UI_CRASH = Pattern.compile(
            TIME_STAMP_GROUP + INVOKE_NUM_GROUP + SEQ_NUM_GROUP + "-UI Exception: CRASH: (.*)");

    private final SmartMonkeyLogItem mSmartMonkeyLog = new SmartMonkeyLogItem();

    /**
     * Parse a monkey log from a {@link BufferedReader} into an {@link SmartMonkeyLogItem}
     * object.
     *
     * @param input a {@link BufferedReader}.
     * @return The {@link SmartMonkeyLogItem}.
     * @see #parse(List)
     */
    public SmartMonkeyLogItem parse(BufferedReader input) throws IOException {
        String line;
        while ((line = input.readLine()) != null) {
            parseLine(line);
        }
        return mSmartMonkeyLog;
    }

    /**
     * {@inheritDoc}
     *
     * @return The {@link SmartMonkeyLogItem}.
     */
    @Override
    public SmartMonkeyLogItem parse(List<String> lines) {
        for (String line : lines) {
            parseLine(line);
        }

        if (mSmartMonkeyLog.getStopUptimeDuration() == 0)
            mSmartMonkeyLog.setIsFinished(false);
        else
            mSmartMonkeyLog.setIsFinished(true);

        return mSmartMonkeyLog;
    }

    /**
     * Parse a line of input.
     */
    private void parseLine(String line) {
        Matcher m = THROTTLE.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setThrottle(Integer.parseInt(m.group(2)));
        }
        m = TARGET_INVOCATIONS.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setTargetInvocations(Integer.parseInt(m.group(2)));
        }
        m = APPS_PACKAGES.matcher(line);
        if (m.matches()) {
            String apps = m.group(2);
            String packages = m.group(3);

            String[] appsArray = apps.split("\\|");
            for (String a : appsArray) {
                mSmartMonkeyLog.addApplication(a);
            }

            String[] pkgsArray = packages.split("\\|");
            for (String p : pkgsArray) {
                mSmartMonkeyLog.addPackage(p);
            }
        }
        m = INTERMEDIATE_COUNT.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setIntermediateCount(Integer.parseInt(m.group(2)));
        }
        m = START_TIME.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setStartTime(parseTime(m.group(1)));
        }
        m = START_UPTIME.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setStartUptimeDuration((Long.parseLong(m.group(2))));
        }
        m = STOP_UPTIME.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setStopTime(parseTime(m.group(1)));
            mSmartMonkeyLog.setStopUptimeDuration(Long.parseLong(m.group(2)));
            mSmartMonkeyLog.setTotalDuration(Long.parseLong(m.group(3)));
        }
        m = INTERMEDIATE_TIME.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setIntermediateTime(parseTime(m.group(1)));
        }
        m = FINAL_COUNT.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setFinalCount(Integer.parseInt(m.group(2)));
        }
        m = FINISHED.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setIsFinished(true);
        }
        m = ABORTED.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.setIsAborted(true);
        }
        m = UI_CRASH.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.addCrashTime(parseTime(m.group(1)));
        }
        m = UI_ANR.matcher(line);
        if (m.matches()) {
            mSmartMonkeyLog.addAnrTime(parseTime(m.group(1)));
        }
    }

    /**
     * Parse the timestamp and return a date.
     *
     * @param timeStr The timestamp in the format {@code yyyy-MM-dd HH:mm:ss.SSS}
     * @return The {@link Date}.
     */
    public static Date parseTime(String timeStr) {
        try {
            return new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS").parse(timeStr);
        } catch (ParseException e) {
        }
        return null;
    }

}
