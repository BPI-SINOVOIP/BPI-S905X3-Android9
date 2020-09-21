/**
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.vts.util;

import com.android.vts.entity.ProfilingPointEntity;
import com.android.vts.entity.ProfilingPointSummaryEntity;
import com.google.appengine.api.datastore.Entity;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.logging.Logger;

/** PerformanceSummary, an object summarizing performance across profiling points for a test run. */
public class PerformanceSummary {
    protected static Logger logger = Logger.getLogger(PerformanceSummary.class.getName());
    private Map<String, ProfilingPointSummary> summaryMap;

    public final long startTime;
    public final long endTime;
    public final String label;

    /** Creates a performance summary object. */
    public PerformanceSummary(long startTime, long endTime, String label) {
        this.summaryMap = new HashMap<>();
        this.startTime = startTime;
        this.endTime = endTime;
        this.label = label;
    }

    /** Creates a performance summary object. */
    public PerformanceSummary(long startTime, long endTime) {
        this(
                startTime,
                endTime,
                "<span class='date-label'>"
                        + Long.toString(TimeUnit.MICROSECONDS.toMillis(endTime))
                        + "</span>");
    }

    /**
     * Determine if the performance summary contains the provided time.
     *
     * @param time The time (unix timestamp, microseconds) to check.
     * @return True if the time is within the performance summary window, false otherwise.
     */
    public boolean contains(long time) {
        return time >= startTime && time <= endTime;
    }

    /**
     * Add the profiling data from a ProfilingPointRunEntity to the performance summary.
     *
     * @param profilingRun The Entity object whose data to add.
     */
    public void addData(ProfilingPointEntity profilingPoint, Entity profilingRun) {
        ProfilingPointSummaryEntity ppSummary =
                ProfilingPointSummaryEntity.fromEntity(profilingRun);
        if (ppSummary == null) return;

        String name = profilingPoint.profilingPointName;
        if (ppSummary.labels != null && ppSummary.labels.size() > 0) {
            if (!ppSummary.series.equals("")) {
                name += " (" + ppSummary.series + ")";
            }
            if (!summaryMap.containsKey(name)) {
                summaryMap.put(
                        name,
                        new ProfilingPointSummary(
                                profilingPoint.xLabel,
                                profilingPoint.yLabel,
                                profilingPoint.regressionMode));
            }
            summaryMap.get(name).update(ppSummary);
        } else {
            // Use the option suffix as the table name.
            // Group all profiling points together into one table
            if (!summaryMap.containsKey(ppSummary.series)) {
                summaryMap.put(
                        ppSummary.series,
                        new ProfilingPointSummary(
                                profilingPoint.xLabel,
                                profilingPoint.yLabel,
                                profilingPoint.regressionMode));
            }
            summaryMap.get(ppSummary.series).updateLabel(ppSummary, name);
        }
    }

    /**
     * Adds a ProfilingPointSummary object into the summary map only if the key doesn't exist.
     *
     * @param key The name of the profiling point.
     * @param summary The ProfilingPointSummary object to add into the summary map.
     * @return True if the data was inserted into the performance summary, false otherwise.
     */
    public boolean insertProfilingPointSummary(String key, ProfilingPointSummary summary) {
        if (!summaryMap.containsKey(key)) {
            summaryMap.put(key, summary);
            return true;
        }
        return false;
    }

    /**
     * Gets the number of profiling points.
     *
     * @return The number of profiling points in the performance summary.
     */
    public int size() {
        return summaryMap.size();
    }

    /**
     * Gets the names of the profiling points.
     *
     * @return A string array of profiling point names.
     */
    public String[] getProfilingPointNames() {
        String[] profilingNames = summaryMap.keySet().toArray(new String[summaryMap.size()]);
        Arrays.sort(profilingNames);
        return profilingNames;
    }

    /**
     * Determines if a profiling point is described by the performance summary.
     *
     * @param profilingPointName The name of the profiling point.
     * @return True if the profiling point is contained in the performance summary, else false.
     */
    public boolean hasProfilingPoint(String profilingPointName) {
        return summaryMap.containsKey(profilingPointName);
    }

    /**
     * Gets the profiling point summary by name.
     *
     * @param profilingPointName The name of the profiling point to fetch.
     * @return The ProfilingPointSummary object describing the specified profiling point.
     */
    public ProfilingPointSummary getProfilingPointSummary(String profilingPointName) {
        return summaryMap.get(profilingPointName);
    }
}
