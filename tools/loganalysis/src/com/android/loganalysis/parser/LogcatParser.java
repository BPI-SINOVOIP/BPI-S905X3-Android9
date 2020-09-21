/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.loganalysis.item.LogcatItem;
import com.android.loganalysis.item.MiscLogcatItem;
import com.android.loganalysis.util.ArrayUtil;
import com.android.loganalysis.util.LogPatternUtil;
import com.android.loganalysis.util.LogTailUtil;

import java.io.BufferedReader;
import java.io.IOException;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * An {@link IParser} to handle logcat.  The parser can handle the time and threadtime logcat
 * formats.
 * <p>
 * Since the timestamps in the logcat do not have a year, the year can be set manually when the
 * parser is created or through {@link #setYear(String)}.  If a year is not set, the current year
 * will be used.
 * </p>
 */
public class LogcatParser implements IParser {
    public static final String ANR = "ANR";
    public static final String JAVA_CRASH = "JAVA_CRASH";
    public static final String NATIVE_CRASH = "NATIVE_CRASH";
    public static final String HIGH_CPU_USAGE = "HIGH_CPU_USAGE";
    public static final String HIGH_MEMORY_USAGE = "HIGH_MEMORY_USAGE";
    public static final String RUNTIME_RESTART = "RUNTIME_RESTART";

    /**
     * Match a single line of `logcat -v threadtime`, such as:
     *
     * <pre>05-26 11:02:36.886 5689 5689 D AndroidRuntime: CheckJNI is OFF.</pre>
     */
    private static final Pattern THREADTIME_LINE =
            // UID was added to logcat. It could either be a number or a string.
            Pattern.compile(
                    // timestamp[1]
                    "^(\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}.\\d{3})"
                            // pid/tid and log level [2-4]
                            + "(?:\\s+[0-9A-Za-z]+)?\\s+(\\d+)\\s+(\\d+)\\s+([A-Z])\\s+"
                            // tag and message [5-6]
                            + "(.+?)\\s*: (.*)$");

    /**
     * Match a single line of `logcat -v time`, such as:
     * 06-04 02:32:14.002 D/dalvikvm(  236): GC_CONCURRENT freed 580K, 51% free [...]
     */
    private static final Pattern TIME_LINE = Pattern.compile(
            "^(\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}.\\d{3})\\s+" +  /* timestamp [1] */
                "(\\w)/(.+?)\\(\\s*(\\d+)\\): (.*)$");  /* level, tag, pid, msg [2-5] */

    /**
     * Match: "*** FATAL EXCEPTION IN SYSTEM PROCESS: message"
     */
    private static final Pattern SYSTEM_SERVER_CRASH = Pattern.compile(
            "\\*\\*\\* FATAL EXCEPTION IN SYSTEM PROCESS:.*");
    /**
     * Match "Process: com.android.package, PID: 123" or "PID: 123"
     */
    private static final Pattern JAVA_CRASH_PROCESS_PID = Pattern.compile(
            "^(Process: (\\S+), )?PID: (\\d+)$");


    /**
     * Match a line thats printed when a non app java process starts.
     */
    private static final Pattern JAVA_PROC_START = Pattern.compile("Calling main entry (.+)");

    /**
     * Class for storing logcat meta data for a particular grouped list of lines.
     */
    private class LogcatData {
        public Integer mPid = null;
        public Integer mTid = null;
        public Date mTime = null;
        public String mLevel = null;
        public String mTag = null;
        public String mLastPreamble = null;
        public String mProcPreamble = null;
        public List<String> mLines = new LinkedList<String>();

        public LogcatData(Integer pid, Integer tid, Date time, String level, String tag,
                String lastPreamble, String procPreamble) {
            mPid = pid;
            mTid = tid;
            mTime = time;
            mLevel = level;
            mTag = tag;
            mLastPreamble = lastPreamble;
            mProcPreamble = procPreamble;
        }
    }

    private LogPatternUtil mPatternUtil = new LogPatternUtil();
    private LogTailUtil mPreambleUtil = new LogTailUtil();

    private String mYear = null;

    LogcatItem mLogcat = null;

    Map<String, LogcatData> mDataMap = new HashMap<String, LogcatData>();
    List<LogcatData> mDataList = new LinkedList<LogcatData>();

    private Date mStartTime = null;
    private Date mStopTime = null;

    private boolean mIsParsing = true;

    private Map<Integer, String> mPids = new HashMap<Integer, String>();

    private List<CrashTag> mJavaCrashTags = new ArrayList<>();
    private List<CrashTag> mNativeCrashTags = new ArrayList<>();

    /**
     * Constructor for {@link LogcatParser}.
     */
    public LogcatParser() {
        // Add default tag for java crash
        addJavaCrashTag("E", "AndroidRuntime", JAVA_CRASH);
        addNativeCrashTag("I", "DEBUG");
        addNativeCrashTag("F", "DEBUG");
        initPatterns();
    }

    /**
     * Constructor for {@link LogcatParser}.
     *
     * @param year The year as a string.
     */
    public LogcatParser(String year) {
        this();
        setYear(year);
    }

    /**
     * Sets the year for {@link LogcatParser}.
     *
     * @param year The year as a string.
     */
    public void setYear(String year) {
        mYear = year;
    }

    /**
     * Parse a logcat from a {@link BufferedReader} into an {@link LogcatItem} object.
     *
     * @param input a {@link BufferedReader}.
     * @return The {@link LogcatItem}.
     * @see #parse(List)
     */
    public LogcatItem parse(BufferedReader input) throws IOException {
        String line;
        while ((line = input.readLine()) != null) {
            parseLine(line);
        }
        commit();

        return mLogcat;
    }

    /**
     * {@inheritDoc}
     *
     * @return The {@link LogcatItem}.
     */
    @Override
    public LogcatItem parse(List<String> lines) {
        for (String line : lines) {
            parseLine(line);
        }
        commit();

        return mLogcat;
    }

    /**
     * Clear the existing {@link LogcatItem}. The next parse will create a new one.
     */
    public void clear() {
        mLogcat = null;
        mDataList.clear();
        mDataMap.clear();
    }

    /**
     * Parse a line of input.
     *
     * @param line The line to parse
     */
    private void parseLine(String line) {
        if ("".equals(line.trim())) {
            return;
        }
        if (mLogcat == null) {
            mLogcat = new LogcatItem();
        }
        Integer pid = null;
        Integer tid = null;
        Date time = null;
        String level = null;
        String tag = null;
        String msg = null;

        Matcher m = THREADTIME_LINE.matcher(line);
        Matcher tm = TIME_LINE.matcher(line);
        if (m.matches()) {
            time = parseTime(m.group(1));
            pid = Integer.parseInt(m.group(2));
            tid = Integer.parseInt(m.group(3));
            level = m.group(4);
            tag = m.group(5);
            msg = m.group(6);
        } else if (tm.matches()) {
            time = parseTime(tm.group(1));
            level = tm.group(2);
            tag = tm.group(3);
            pid = Integer.parseInt(tm.group(4));
            msg = tm.group(5);
        }

        if (time != null) {
            if (mStartTime == null) {
                mStartTime = time;
            }
            mStopTime = time;
        }

        // Don't parse any lines after device begins reboot until a new log is detected.
        if ("I".equals(level) && "ShutdownThread".equals(tag) &&
                Pattern.matches("Rebooting, reason: .*", msg)) {
            mIsParsing = false;
        }
        if (Pattern.matches(".*--------- beginning of .*", line)) {
            mIsParsing = true;
        }

        if (!mIsParsing || !(m.matches() || tm.matches())) {
            return;
        }


        // When a non app java process starts add its pid to the map
        Matcher pidMatcher = JAVA_PROC_START.matcher(msg);
        if (pidMatcher.matches()) {
            String name = pidMatcher.group(1);
            mPids.put(pid, name);
        }

        // ANRs are separated either by different PID/TIDs or when AnrParser.START matches a line.
        // The newest entry is kept in the dataMap for quick lookup while all entries are added to
        // the list.
        if ("E".equals(level) && "ActivityManager".equals(tag)) {
            String key = encodeLine(pid, tid, level, tag);
            LogcatData data;
            if (!mDataMap.containsKey(key) || AnrParser.START.matcher(msg).matches()) {
                data = new LogcatData(pid, tid, time, level, tag, mPreambleUtil.getLastTail(),
                        mPreambleUtil.getIdTail(pid));
                mDataMap.put(key, data);
                mDataList.add(data);
            } else {
                data = mDataMap.get(key);
            }
            data.mLines.add(msg);
        }

        // Native crashes are separated either by different PID/TIDs or when
        // NativeCrashParser.FINGERPRINT matches a line.  The newest entry is kept in the dataMap
        // for quick lookup while all entries are added to the list.
        if (anyNativeCrashTagMatches(level, tag)) {
            String key = encodeLine(pid, tid, level, tag);
            LogcatData data;
            if (!mDataMap.containsKey(key) || NativeCrashParser.FINGERPRINT.matcher(msg).matches()) {
                data = new LogcatData(pid, tid, time, level, tag, mPreambleUtil.getLastTail(),
                        mPreambleUtil.getIdTail(pid));
                mDataMap.put(key, data);
                mDataList.add(data);
            } else {
                data = mDataMap.get(key);
            }
            data.mLines.add(msg);
        }

        // PID and TID are enough to separate Java crashes.
        if (anyJavaCrashTagMatches(level, tag)) {
            String key = encodeLine(pid, tid, level, tag);
            LogcatData data;
            if (!mDataMap.containsKey(key)) {
                data = new LogcatData(pid, tid, time, level, tag, mPreambleUtil.getLastTail(),
                        mPreambleUtil.getIdTail(pid));
                mDataMap.put(key, data);
                mDataList.add(data);
            } else {
                data = mDataMap.get(key);
            }
            data.mLines.add(msg);
        }

        // Check the message here but add it in commit()
        if (mPatternUtil.checkMessage(msg, new ExtrasPattern(level, tag)) != null) {
            LogcatData data = new LogcatData(pid, tid, time, level, tag,
                    mPreambleUtil.getLastTail(), mPreambleUtil.getIdTail(pid));
            data.mLines.add(msg);
            mDataList.add(data);
        }

        // After parsing the line, add it the the buffer for the preambles.
        mPreambleUtil.addLine(pid, line);
    }

    /**
     * Signal that the input has finished.
     */
    private void commit() {
        if (mLogcat == null) {
            return;
        }
        for (LogcatData data : mDataList) {
            MiscLogcatItem item = null;
            if ("E".equals(data.mLevel) && "ActivityManager".equals(data.mTag)) {
                item = new AnrParser().parse(data.mLines);
            } else if (anyJavaCrashTagMatches(data.mLevel, data.mTag)) {
                // Get the process name/PID from the Java crash, then pass the rest of the lines to
                // the parser.
                Integer pid = null;
                String app = null;
                for (int i = 0; i < data.mLines.size(); i++) {
                    String line = data.mLines.get(i);
                    Matcher m = JAVA_CRASH_PROCESS_PID.matcher(line);
                    if (m.matches()) {
                        app = m.group(2);
                        pid = Integer.valueOf(m.group(3));
                        data.mLines = data.mLines.subList(i + 1, data.mLines.size());
                        break;
                    }
                    m = SYSTEM_SERVER_CRASH.matcher(line);
                    if (m.matches()) {
                        app = mPids.get(data.mPid);
                        if (app == null) {
                            app = "system_server";
                        }
                        data.mLines = data.mLines.subList(i + 1, data.mLines.size());
                        break;
                    }
                }
                item = new JavaCrashParser().parse(data.mLines);
                if (item != null) {
                    item.setApp(app);
                    item.setPid(pid);
                    item.setCategory(getJavaCrashCategory(data.mLevel, data.mTag));
                }
            } else if (anyNativeCrashTagMatches(data.mLevel, data.mTag)) {
                // CLog.v("Parsing native crash: %s", data.mLines);
                item = new NativeCrashParser().parse(data.mLines);
            } else {
                String msg = ArrayUtil.join("\n", data.mLines);
                String category = mPatternUtil.checkMessage(msg, new ExtrasPattern(
                        data.mLevel, data.mTag));
                if (category != null) {
                    MiscLogcatItem logcatItem = new MiscLogcatItem();
                    logcatItem.setCategory(category);
                    logcatItem.setStack(msg);
                    item = logcatItem;
                }
            }
            if (item != null) {
                item.setEventTime(data.mTime);
                if (item.getPid() == null) {
                    item.setPid(data.mPid);
                    item.setTid(data.mTid);
                }
                item.setLastPreamble(data.mLastPreamble);
                item.setProcessPreamble(data.mProcPreamble);
                item.setTag(data.mTag);
                mLogcat.addEvent(item);
            }
        }

        mLogcat.setStartTime(mStartTime);
        mLogcat.setStopTime(mStopTime);
    }

    /**
     * Create an identifier that "should" be unique for a given logcat. In practice, we do use it as
     * a unique identifier.
     */
    private static String encodeLine(Integer pid, Integer tid, String level, String tag) {
        if (tid == null) {
            return String.format("%d|%s|%s", pid, level, tag);
        }
        return String.format("%d|%d|%s|%s", pid, tid, level, tag);
    }

    /**
     * Parse the timestamp and return a {@link Date}.  If year is not set, the current year will be
     * used.
     *
     * @param timeStr The timestamp in the format {@code MM-dd HH:mm:ss.SSS}.
     * @return The {@link Date}.
     */
    private Date parseTime(String timeStr) {
        // If year is null, just use the current year.
        if (mYear == null) {
            DateFormat yearFormatter = new SimpleDateFormat("yyyy");
            mYear = yearFormatter.format(new Date());
        }

        DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
        try {
            return formatter.parse(String.format("%s-%s", mYear, timeStr));
        } catch (ParseException e) {
            // CLog.e("Could not parse time string %s", timeStr);
            return null;
        }
    }

    private void initPatterns() {
        // High CPU usage
        mPatternUtil.addPattern(Pattern.compile(".* timed out \\(is the CPU pegged\\?\\).*"),
                HIGH_CPU_USAGE);

        // High memory usage
        mPatternUtil.addPattern(Pattern.compile(
                "GetBufferLock timed out for thread \\d+ buffer .*"), HIGH_MEMORY_USAGE);

        // Runtime restarts
        mPatternUtil.addPattern(Pattern.compile("\\*\\*\\* WATCHDOG KILLING SYSTEM PROCESS.*"),
                RUNTIME_RESTART);
    }

    /**
     * Adds a custom, more complex pattern to LogcatParser for parsing out of logcat events.
     * Any matched events are then assigned to the category name provided, and can be grabbed
     * via LogcatParser's .getMiscEvents("yourCustomCategoryName") method.
     * Considers null messages, levels, or tags to be wildcards.
     *
     * @param pattern The regex representing the pattern to match for, or null for wildcard.
     * @param level The name of the level to match for, or null for wildcard.
     * @param tag The name of the tag to match for, or null for wildcard.
     * @param category Assign any matching logcat events to this category name, for later retrieval
     */
    public void addPattern(Pattern pattern, String level, String tag, String category) {
        /* count null message as wildcard */
        if (pattern == null) {
            pattern = Pattern.compile(".*");
        }
        mPatternUtil.addPattern(pattern, new ExtrasPattern(level, tag), category);
    }

    /**
     * Used internally for bundling up extra pattern criteria for more advanced pattern matching.
     */
    private class ExtrasPattern {
        public String mLevel;
        public String mTag;

        public ExtrasPattern(String level, String tag) {
            mLevel = level;
            mTag = tag;
        }

        /**
         * Override Object.equals to match based on the level & tag patterns,
         * while also counting null level & tag patterns as wildcards.
         *
         * @param otherObj the object we're matching the level & tag patterns to.
         * @return true if otherObj's extras match, false otherwise
         */
        @Override
        public boolean equals(Object otherObj) {
            if (otherObj instanceof ExtrasPattern) {
                // Treat objects as equal only if the obj's level and tag match.
                // Treat null as a wildcard.
                ExtrasPattern other = (ExtrasPattern) otherObj;
                if ((mLevel == null || other.mLevel == null || mLevel.equals(other.mLevel)) &&
                        (mTag == null || other.mTag == null || mTag.equals(other.mTag))) {
                    return true;
                }
            }
            return false;
        }

        /** {@inheritdoc} */
        @Override
        public int hashCode() {
            // Since both mLevel and mTag can be wild cards, we can't actually use them to generate
            // a hashcode without potentially violating the hashcode contract. That doesn't leave
            // us with anything to actually use to generate the hashcode, so just return a random
            // static int.
            return 145800969;
        }
    }

    /**
     * Allows Java crashes to be parsed from multiple log levels and tags. Normally the crashes
     * are error level messages from AndroidRuntime, but they could also be from other sources.
     * Use this method to parse java crashes from those other sources.
     *
     * @param level log level on which to look for java crashes
     * @param tag log tag where to look for java crashes
     */
    public void addJavaCrashTag(String level, String tag, String category) {
        mJavaCrashTags.add(new CrashTag(level, tag, category));
    }

    /**
     * Allows native crashes to be parsed from multiple log levels and tags.  The default levels
     * are "I DEBUG" and "F DEBUG".
     *
     * @param level log level on which to look for native crashes
     * @param tag log tag where to look for native crashes
     */
    private void addNativeCrashTag(String level, String tag) {
        mNativeCrashTags.add(new CrashTag(level, tag, NATIVE_CRASH));
    }

    /**
     * Determines if any of the Java crash tags is matching a logcat line.
     *
     * @param level log level of the logcat line
     * @param tag tag of the logcat line
     * @return True if any Java crash tag matches the current level and tag. False otherwise.
     */
    private boolean anyJavaCrashTagMatches(String level, String tag) {
        return findCrashTag(mJavaCrashTags, level, tag) != null;
    }

    /**
     * Determines if any of the native crash tags is matching a logcat line.
     *
     * @param level log level of the logcat line
     * @param tag tag of the logcat line
     * @return True if any native crash tag matches the current level and tag. False otherwise.
     */
    private boolean anyNativeCrashTagMatches(String level, String tag) {
        return findCrashTag(mNativeCrashTags, level, tag) != null;
    }

    /**
     * Finds the {@link CrashTag} matching given level and tag.
     *
     * @param level level to find
     * @param tag tag to find
     * @return the matching {@link CrashTag} or null if no matches exist.
     */
    private CrashTag findCrashTag(List<CrashTag> crashTags, String level, String tag) {
        for (CrashTag t : crashTags) {
            if (t.matches(level, tag)) {
                return t;
            }
        }
        return null;
    }

    /**
     * Returns category for a given {@link CrashTag}.
     *
     * @param level level of the {@link CrashTag}
     * @param tag tag of the {@link CrashTag}
     * @return category of the {@link CrashTag}, matching search criteria. If nothing was found
     * returns {@code JAVA_CRASH}.
     */
    private String getJavaCrashCategory(String level, String tag) {
        CrashTag crashTag = findCrashTag(mJavaCrashTags, level, tag);
        if (crashTag == null) {
            return JAVA_CRASH;
        }
        return crashTag.getCategory();
    }

    /**
     * Class to encapsulate the tags that indicate which crashes should be parsed.
     */
    private class CrashTag {
        private String mLevel;
        private String mTag;
        private String mCategory;

        public CrashTag(String level, String tag, String category) {
            mLevel = level;
            mTag = tag;
            mCategory = category;
        }

        public boolean matches(String level, String tag) {
            return mLevel.equals(level) && mTag.equals(tag);
        }

        public String getCategory() {
            return mCategory;
        }
    }
}
