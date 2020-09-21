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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.AnrItem;
import com.android.loganalysis.item.MiscLogcatItem;
import com.android.loganalysis.item.MonkeyLogItem;
import com.android.loganalysis.item.MonkeyLogItem.DroppedCategory;
import com.android.loganalysis.item.NativeCrashItem;
import com.android.loganalysis.item.TracesItem;

import java.io.BufferedReader;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse monkey logs.
 */
public class MonkeyLogParser implements IParser {
    private static final Pattern THROTTLE = Pattern.compile(
            "adb shell monkey.* --throttle (\\d+).*");
    private static final Pattern SEED_AND_TARGET_COUNT = Pattern.compile(
            ":Monkey: seed=(\\d+) count=(\\d+)");
    private static final Pattern SECURITY_EXCEPTIONS = Pattern.compile(
            "adb shell monkey.* --ignore-security-exceptions.*");

    private static final Pattern PACKAGES = Pattern.compile(":AllowPackage: (\\S+)");
    private static final Pattern CATEGORIES = Pattern.compile(":IncludeCategory: (\\S+)");

    private static final Pattern START_UPTIME = Pattern.compile(
            "# (.*) - device uptime = (\\d+\\.\\d+): Monkey command used for this test:");
    private static final Pattern STOP_UPTIME = Pattern.compile(
            "# (.*) - device uptime = (\\d+\\.\\d+): Monkey command ran for: " +
            "(\\d+):(\\d+) \\(mm:ss\\)");

    private static final Pattern INTERMEDIATE_COUNT = Pattern.compile(
            "\\s+// Sending event #(\\d+)");
    private static final Pattern FINISHED = Pattern.compile("// Monkey finished");
    private static final Pattern FINAL_COUNT = Pattern.compile("Events injected: (\\d+)");
    private static final Pattern NO_ACTIVITIES = Pattern.compile(
            "\\*\\* No activities found to run, monkey aborted.");

    private static final Pattern DROPPED_KEYS = Pattern.compile(":Dropped: .*keys=(\\d+).*");
    private static final Pattern DROPPED_POINTERS = Pattern.compile(
            ":Dropped: .*pointers=(\\d+).*");
    private static final Pattern DROPPED_TRACKBALLS = Pattern.compile(
            ":Dropped: .*trackballs=(\\d+).*");
    private static final Pattern DROPPED_FLIPS = Pattern.compile(":Dropped: .*flips=(\\d+).*");
    private static final Pattern DROPPED_ROTATIONS = Pattern.compile(
            ":Dropped: .*rotations=(\\d+).*");

    // Log messages can get intermixed in crash message, ignore those in the crash context.
    private static final Pattern MONKEY_LOG_MESSAGE = Pattern.compile("$(:|Sleeping|    //)");

    private static final Pattern ANR = Pattern.compile(
            "// NOT RESPONDING: (\\S+) \\(pid (\\d+)\\)");
    private static final Pattern CRASH = Pattern.compile(
            "// CRASH: (\\S+) \\(pid (\\d+)\\)");
    private static final Pattern EMPTY_NATIVE_CRASH = Pattern.compile("" +
            "\\*\\* New native crash detected.");
    private static final Pattern ABORTED = Pattern.compile("\\*\\* Monkey aborted due to error.");

    private static final Pattern TRACES_START = Pattern.compile("anr traces:");
    private static final Pattern TRACES_STOP = Pattern.compile("// anr traces status was \\d+");

    private boolean mMatchingAnr = false;
    private boolean mMatchingCrash = false;
    private boolean mMatchingJavaCrash = false;
    private boolean mMatchingNativeCrash = false;
    private boolean mMatchingTraces = false;
    private boolean mMatchedTrace = false;
    private List<String> mBlock = null;
    private String mApp = null;
    private Integer mPid = null;

    private MonkeyLogItem mMonkeyLog = new MonkeyLogItem();

    /**
     * Parse a monkey log from a {@link BufferedReader} into an {@link MonkeyLogItem} object.
     *
     * @param input a {@link BufferedReader}.
     * @return The {@link MonkeyLogItem}.
     * @see #parse(List)
     */
    public MonkeyLogItem parse(BufferedReader input) throws IOException {
        String line;
        while ((line = input.readLine()) != null) {
            parseLine(line);
        }

        return mMonkeyLog;
    }

    /**
     * {@inheritDoc}
     *
     * @return The {@link MonkeyLogItem}.
     */
    @Override
    public MonkeyLogItem parse(List<String> lines) {
        for (String line : lines) {
            parseLine(line);
        }

        return mMonkeyLog;
    }

    /**
     * Parse a line of input.
     */
    private void parseLine(String line) {
        Matcher m;

        if (mMatchingAnr) {
            if ("".equals(line)) {
                AnrItem crash = new AnrParser().parse(mBlock);
                addCrashAndReset(crash);
            } else {
                m = MONKEY_LOG_MESSAGE.matcher(line);
                if (!m.matches()) {
                    mBlock.add(line);
                }
                return;
            }
        }

        if (mMatchingCrash) {
            if (!mMatchingJavaCrash && !mMatchingNativeCrash && line.startsWith("// Short Msg: ")) {
                if (line.contains("Native crash")) {
                    mMatchingNativeCrash = true;
                } else {
                    mMatchingJavaCrash = true;
                }
            }
            m = ABORTED.matcher(line);
            if (m.matches()) {
                MiscLogcatItem crash = null;
                if (mMatchingJavaCrash) {
                    crash = new JavaCrashParser().parse(mBlock);
                } else if (mMatchingNativeCrash) {
                    crash = new NativeCrashParser().parse(mBlock);
                }
                addCrashAndReset(crash);
            } else {
                m = MONKEY_LOG_MESSAGE.matcher(line);
                if (!m.matches() && line.startsWith("// ") && !line.startsWith("// ** ")) {
                    line = line.replace("// ", "");
                    mBlock.add(line);
                }
                return;
            }
        }

        if (mMatchingTraces) {
            m = TRACES_STOP.matcher(line);
            if (m.matches()) {
                TracesItem traces = new TracesParser().parse(mBlock);

                // Set the trace if the crash is an ANR and if the app for the crash and trace match
                if (traces != null && traces.getApp() != null && traces.getStack() != null &&
                        mMonkeyLog.getCrash() instanceof AnrItem &&
                        traces.getApp().equals(mMonkeyLog.getCrash().getApp())) {
                    ((AnrItem) mMonkeyLog.getCrash()).setTrace(traces.getStack());
                }

                reset();
                mMatchedTrace = true;
            } else {
                m = MONKEY_LOG_MESSAGE.matcher(line);
                if (!m.matches()) {
                    mBlock.add(line);
                }
                return;
            }
        }

        m = THROTTLE.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setThrottle(Integer.parseInt(m.group(1)));
        }
        m = SEED_AND_TARGET_COUNT.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setSeed(Long.parseLong(m.group(1)));
            mMonkeyLog.setTargetCount(Integer.parseInt(m.group(2)));
        }
        m = SECURITY_EXCEPTIONS.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setIgnoreSecurityExceptions(true);
        }
        m = PACKAGES.matcher(line);
        if (m.matches()) {
            mMonkeyLog.addPackage(m.group(1));
        }
        m = CATEGORIES.matcher(line);
        if (m.matches()) {
            mMonkeyLog.addCategory(m.group(1));
        }
        m = START_UPTIME.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setStartTime(parseTime(m.group(1)));
            mMonkeyLog.setStartUptimeDuration((long) (Double.parseDouble(m.group(2)) * 1000));
        }
        m = STOP_UPTIME.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setStopTime(parseTime(m.group(1)));
            mMonkeyLog.setStopUptimeDuration((long) (Double.parseDouble(m.group(2)) * 1000));
            mMonkeyLog.setTotalDuration(60 * 1000 * Integer.parseInt(m.group(3)) +
                    1000 *Integer.parseInt(m.group(4)));
        }
        m = INTERMEDIATE_COUNT.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setIntermediateCount(Integer.parseInt(m.group(1)));
        }
        m = FINAL_COUNT.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setFinalCount(Integer.parseInt(m.group(1)));
        }
        m = FINISHED.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setIsFinished(true);
        }
        m = NO_ACTIVITIES.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setNoActivities(true);
        }
        m = DROPPED_KEYS.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setDroppedCount(DroppedCategory.KEYS, Integer.parseInt(m.group(1)));
        }
        m = DROPPED_POINTERS.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setDroppedCount(DroppedCategory.POINTERS, Integer.parseInt(m.group(1)));
        }
        m = DROPPED_TRACKBALLS.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setDroppedCount(DroppedCategory.TRACKBALLS, Integer.parseInt(m.group(1)));
        }
        m = DROPPED_FLIPS.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setDroppedCount(DroppedCategory.FLIPS, Integer.parseInt(m.group(1)));
        }
        m = DROPPED_ROTATIONS.matcher(line);
        if (m.matches()) {
            mMonkeyLog.setDroppedCount(DroppedCategory.ROTATIONS, Integer.parseInt(m.group(1)));
        }
        m = ANR.matcher(line);
        if (mMonkeyLog.getCrash() == null && m.matches()) {
            mApp = m.group(1);
            mPid = Integer.parseInt(m.group(2));
            mBlock = new LinkedList<String>();
            mMatchingAnr = true;
        }
        m = CRASH.matcher(line);
        if (mMonkeyLog.getCrash() == null && m.matches()) {
            mApp = m.group(1);
            mPid = Integer.parseInt(m.group(2));
            mBlock = new LinkedList<String>();
            mMatchingCrash = true;
        }
        m = EMPTY_NATIVE_CRASH.matcher(line);
        if (mMonkeyLog.getCrash() == null && m.matches()) {
            MiscLogcatItem crash = new NativeCrashItem();
            crash.setStack("");
            addCrashAndReset(crash);
        }
        m = TRACES_START.matcher(line);
        if (!mMatchedTrace && m.matches()) {
            mBlock = new LinkedList<String>();
            mMatchingTraces = true;
        }
    }

    /**
     * Add a crash to the monkey log item and reset the parser state for crashes.
     */
    private void addCrashAndReset(MiscLogcatItem crash) {
        if (crash != null) {
            if (crash.getPid() == null) {
                crash.setPid(mPid);
            }
            if (crash.getApp() == null) {
                crash.setApp(mApp);
            }
            mMonkeyLog.setCrash(crash);
        }

        reset();
    }

    /**
     * Reset the parser state for crashes.
     */
    private void reset() {
        mApp = null;
        mPid = null;
        mMatchingAnr = false;
        mMatchingCrash = false;
        mMatchingJavaCrash = false;
        mMatchingNativeCrash = false;
        mMatchingTraces = false;
        mBlock = null;
    }

    /**
     * Parse the timestamp and return a date.
     *
     * @param timeStr The timestamp in the format {@code E, MM/dd/yyyy hh:mm:ss a} or
     * {@code EEE MMM dd HH:mm:ss zzz yyyy}.
     * @return The {@link Date}.
     */
    private Date parseTime(String timeStr) {
        try {
            return new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy").parse(timeStr);
        } catch (ParseException e) {
            // CLog.v("Could not parse date %s with format EEE MMM dd HH:mm:ss zzz yyyy", timeStr);
        }

        try {
            return new SimpleDateFormat("E, MM/dd/yyyy hh:mm:ss a").parse(timeStr);
        } catch (ParseException e) {
            // CLog.v("Could not parse date %s with format E, MM/dd/yyyy hh:mm:ss a", timeStr);
        }

        // CLog.e("Could not parse date %s", timeStr);
        return null;
    }

}
