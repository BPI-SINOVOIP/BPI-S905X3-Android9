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
package com.android.loganalysis.item;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * An {@link IItem} used to store monkey log info.
 */
public class SmartMonkeyLogItem extends GenericItem {

    @SuppressWarnings("serial")
    private class DateSet extends HashSet<Date> {}

    /** Constant for JSON output */
    public static final String START_TIME = "START_TIME";
    /** Constant for JSON output */
    public static final String STOP_TIME = "STOP_TIME";
    /** Constant for JSON output */
    public static final String APPLICATIONS = "APPS";
    /** Constant for JSON output */
    public static final String PACKAGES = "PACKAGES";
    /** Constant for JSON output */
    public static final String THROTTLE = "THROTTLE";
    /** Constant for JSON output */
    public static final String TARGET_INVOCATIONS = "TARGET_INVOCATIONS";
    /** Constant for JSON output */
    public static final String TOTAL_DURATION = "TOTAL_TIME";
    /** Constant for JSON output */
    public static final String START_UPTIME_DURATION = "START_UPTIME";
    /** Constant for JSON output */
    public static final String STOP_UPTIME_DURATION = "STOP_UPTIME";
    /** Constant for JSON output */
    public static final String IS_FINISHED = "IS_FINISHED";
    /** Constant for JSON output */
    public static final String ABORTED = "ABORTED";
    /** Constant for JSON output */
    public static final String INTERMEDIATE_COUNT = "INTERMEDIATE_COUNT";
    /** Constant for JSON output */
    public static final String FINAL_COUNT = "FINAL_COUNT";
    /** Constant for JSON output */
    public static final String ANR_TIMES = "ANR_TIMES";
    /** Constant for JSON output */
    public static final String CRASH_TIMES = "CRASH_TIMES";
    /** Constant for JSON output */
    public static final String INTERMEDIATE_TIME = "INTERMEDIATE_TIME";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            START_TIME, STOP_TIME, PACKAGES, THROTTLE, TARGET_INVOCATIONS, ABORTED,
            TOTAL_DURATION, START_UPTIME_DURATION, STOP_UPTIME_DURATION, APPLICATIONS,
            IS_FINISHED, INTERMEDIATE_COUNT, FINAL_COUNT, ANR_TIMES, CRASH_TIMES,
            INTERMEDIATE_TIME));

    /**
     * The constructor for {@link MonkeyLogItem}.
     */
    public SmartMonkeyLogItem() {
        super(ATTRIBUTES);

        setAttribute(APPLICATIONS, new ArrayList<String>());
        setAttribute(PACKAGES, new ArrayList<String>());
        setAttribute(CRASH_TIMES, new DateSet());
        setAttribute(ANR_TIMES, new DateSet());
        setAttribute(INTERMEDIATE_TIME, new DateSet());
        setAttribute(THROTTLE, 0);
        setAttribute(FINAL_COUNT, 0);
        setAttribute(IS_FINISHED, false);
        setAttribute(ABORTED, false);
        setAttribute(INTERMEDIATE_COUNT, 0);
        setAttribute(START_UPTIME_DURATION, 0L);
        setAttribute(STOP_UPTIME_DURATION, 0L);
    }

    /**
     * Get the start time of the monkey log.
     */
    public Date getStartTime() {
        return (Date) getAttribute(START_TIME);
    }

    /**
     * Set the start time of the monkey log.
     */
    public void setStartTime(Date time) {
        setAttribute(START_TIME, time);
    }

    /**
     * Set the last time reported for a monkey event
     */
    public void setIntermediateTime(Date time) {
        setAttribute(INTERMEDIATE_TIME, time);
    }

    /**
     * Get the last time reported for a monkey event
     */
    public Date getIntermediateTime() {
        return (Date) getAttribute(INTERMEDIATE_TIME);
    }

    /**
     * Get the stop time of the monkey log.
     */
    public Date getStopTime() {
        return (Date) getAttribute(STOP_TIME);
    }

    /**
     * Set the stop time of the monkey log.
     */
    public void setStopTime(Date time) {
        setAttribute(STOP_TIME, time);
    }

    /**
     * Get the set of packages that the monkey is run on.
     */
    @SuppressWarnings("unchecked")
    public List<String> getPackages() {
        return (List<String>) getAttribute(PACKAGES);
    }

    /**
     * Add a package to the set that the monkey is run on.
     */
    @SuppressWarnings("unchecked")
    public void addPackage(String thePackage) {
        ((List<String>) getAttribute(PACKAGES)).add(thePackage);
    }

    /**
     * Get the set of packages that the monkey is run on.
     */
    @SuppressWarnings("unchecked")
    public List<String> getApplications() {
        return (List<String>) getAttribute(APPLICATIONS);
    }

    /**
     * Add a package to the set that the monkey is run on.
     */
    @SuppressWarnings("unchecked")
    public void addApplication(String theApp) {
        ((List<String>) getAttribute(APPLICATIONS)).add(theApp);
    }

    /**
     * Get the throttle for the monkey run.
     */
    public int getThrottle() {
        return (Integer) getAttribute(THROTTLE);
    }

    /**
     * Set the throttle for the monkey run.
     */
    public void setThrottle(int throttle) {
        setAttribute(THROTTLE, throttle);
    }

    /**
     * Get the target sequence invocations for the monkey run.
     */
    public int getTargetInvocations() {
        return (Integer) getAttribute(TARGET_INVOCATIONS);
    }

    /**
     * Set the target sequence invocations for the monkey run.
     */
    public void setTargetInvocations(int count) {
        setAttribute(TARGET_INVOCATIONS, count);
    }

    /**
     * Get the total duration of the monkey run in milliseconds.
     */
    public long getTotalDuration() {
        if (getIsFinished() || getIsAborted())
            return (Long) getAttribute(TOTAL_DURATION);
        // else it crashed
        Date startTime = getStartTime();
        Date endTime = getIntermediateTime();
        return endTime.getTime() - startTime.getTime() / 1000;
    }

    /**
     * Set the total duration of the monkey run in milliseconds.
     */
    public void setTotalDuration(long time) {
        setAttribute(TOTAL_DURATION, time);
    }

    /**
     * Get the start uptime duration of the monkey run in milliseconds.
     */
    public long getStartUptimeDuration() {
        return (Long) getAttribute(START_UPTIME_DURATION);
    }

    /**
     * Set the start uptime duration of the monkey run in milliseconds.
     */
    public void setStartUptimeDuration(long uptime) {
        setAttribute(START_UPTIME_DURATION, uptime);
    }

    /**
     * Get the stop uptime duration of the monkey run in milliseconds.
     */
    public long getStopUptimeDuration() {
        return (Long) getAttribute(STOP_UPTIME_DURATION);
    }

    /**
     * Set the stop uptime duration of the monkey run in milliseconds.
     */
    public void setStopUptimeDuration(long uptime) {
        setAttribute(STOP_UPTIME_DURATION, uptime);
    }

    /**
     * Get if the monkey run finished without crashing.
     */
    public boolean getIsFinished() {
        return (Boolean) getAttribute(IS_FINISHED);
    }

    /**
     * Set if the monkey run finished without crashing.
     */
    public void setIsFinished(boolean finished) {
        setAttribute(IS_FINISHED, finished);
    }

    /**
     * Get the intermediate count for the monkey run.
     * <p>
     * This count starts at 0 and increments every 100 events. This number should be within 100 of
     * the final count.
     * </p>
     */
    public int getIntermediateCount() {
        return (Integer) getAttribute(INTERMEDIATE_COUNT);
    }

    /**
     * Set the intermediate count for the monkey run.
     * <p>
     * This count starts at 0 and increments every 100 events. This number should be within 100 of
     * the final count.
     * </p>
     */
    public void setIntermediateCount(int count) {
        setAttribute(INTERMEDIATE_COUNT, count);
    }

    /**
     * Get the final count for the monkey run.
     */
    public int getFinalCount() {
        if (getIsFinished())
            return (Integer) getAttribute(FINAL_COUNT);
        return getIntermediateCount();
    }

    /**
     * Set the final count for the monkey run.
     */
    public void setFinalCount(int count) {
        setAttribute(FINAL_COUNT, count);
    }

    /**
     * Get ANR times
     */
    public Set<Date> getAnrTimes() {
        return (DateSet) getAttribute(ANR_TIMES);
    }

    /**
     * Add ANR time
     */
    public void addAnrTime(Date time) {
        ((DateSet) getAttribute(ANR_TIMES)).add(time);
    }

    /**
     * Get Crash times
     */
    public Set<Date> getCrashTimes() {
        return (DateSet) getAttribute(CRASH_TIMES);
    }

    /**
     * Add Crash time
     */
    public void addCrashTime(Date time) {
        ((DateSet) getAttribute(CRASH_TIMES)).add(time);
    }

    /**
     * Get the status of no sequences abort
     */
    public boolean getIsAborted() {
        return (Boolean) getAttribute(ABORTED);
    }

    /**
     * Set the status of no sequences abort
     * @param noSeq
     */
    public void setIsAborted(boolean noSeq) {
        setAttribute(ABORTED, noSeq);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public JSONObject toJson() {
        JSONObject object = super.toJson();

        // Override application, packages, and ANR and crash times.
        put(object, APPLICATIONS, new JSONArray(getApplications()));
        put(object, PACKAGES, new JSONArray(getPackages()));
        put(object, ANR_TIMES, new JSONArray(getAnrTimes()));
        put(object, CRASH_TIMES, new JSONArray(getCrashTimes()));

        return object;
    }

    /**
     * Try to put an {@link Object} in a {@link JSONObject} and remove the existing key if it fails.
     */
    private static void put(JSONObject object, String key, Object value) {
        try {
            object.put(key, value);
        } catch (JSONException e) {
            object.remove(key);
        }
    }
}
