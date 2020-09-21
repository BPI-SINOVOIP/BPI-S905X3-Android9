/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.vts.entity;

import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import com.android.vts.util.StatSummary;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing a profiling point summary. */
public class ProfilingPointSummaryEntity implements DashboardEntity {
    protected static final Logger logger =
            Logger.getLogger(ProfilingPointSummaryEntity.class.getName());
    protected static final String DELIMITER = "#";

    public static final String KIND = "ProfilingPointSummary";
    public static final String ALL = "ALL";

    // Property keys
    public static final String START_TIME = "startTime";
    public static final String MEAN = "mean";
    public static final String SUMSQ = "sumSq";
    public static final String MIN = "min";
    public static final String MAX = "max";
    public static final String LABELS = "labels";
    public static final String LABEL_MEANS = "labelMeans";
    public static final String LABEL_SUMSQS = "labelSumSqs";
    public static final String LABEL_MINS = "labelMins";
    public static final String LABEL_MAXES = "labelMaxes";
    public static final String LABEL_COUNTS = "labelCounts";
    public static final String COUNT = "count";
    public static final String BRANCH = "branch";
    public static final String BUILD_FLAVOR = "buildFlavor";
    public static final String SERIES = "series";

    private final Key key;

    public final StatSummary globalStats;
    public final List<String> labels;
    public final Map<String, StatSummary> labelStats;
    public final String branch;
    public final String buildFlavor;
    public final String series;
    public final long startTime;

    /**
     * Create a ProfilingPointSummaryEntity object.
     *
     * @param parentKey The Key object for the parent TestRunEntity in the database.
     * @param globalStats The StatSummary object recording global statistics about the profiling
     *     point.
     * @param labels The list of data labels.
     * @param labelStats The map from data label to StatSummary object for the label.
     * @param branch The branch.
     * @param buildFlavor The device build flavor.
     * @param series The string describing the profiling point series (e.g. binder or passthrough).
     * @param startTime The timestamp indicating the beginning of the summary.
     */
    public ProfilingPointSummaryEntity(
            Key parentKey,
            StatSummary globalStats,
            List<String> labels,
            Map<String, StatSummary> labelStats,
            String branch,
            String buildFlavor,
            String series,
            long startTime) {
        this.globalStats = globalStats;
        this.labels = labels;
        this.labelStats = labelStats;
        this.buildFlavor = buildFlavor == null ? ALL : buildFlavor;
        this.branch = branch == null ? ALL : branch;
        this.series = series == null ? "" : series;
        this.startTime = startTime;
        this.key = createKey(parentKey, this.branch, this.buildFlavor, this.series, this.startTime);
    }

    /**
     * Create a new ProfilingPointSummaryEntity object.
     *
     * @param parentKey The Key object for the parent TestRunEntity in the database.
     * @param branch The branch.
     * @param buildFlavor The buildFlavor name.
     * @param series The string describing the profiling point series (e.g. binder or passthrough).
     * @param startTime The timestamp indicating the beginning of the summary.
     */
    public ProfilingPointSummaryEntity(
            Key parentKey, String branch, String buildFlavor, String series, long startTime) {
        this(
                parentKey,
                new StatSummary(null, VtsProfilingRegressionMode.UNKNOWN_REGRESSION_MODE),
                new ArrayList<>(),
                new HashMap<>(),
                branch,
                buildFlavor,
                series,
                startTime);
    }

    /**
     * Create a key for a ProfilingPointSummaryEntity.
     *
     * @param parentKey The Key object for the parent TestRunEntity in the database.
     * @param branch The branch.
     * @param buildFlavor The device build flavor.
     * @param series The string describing the profiling point series (e.g. binder or passthrough).
     * @param startTime The timestamp indicating the beginning of the summary.
     * @return a Key object for the ProfilingPointSummaryEntity in the database.
     */
    public static Key createKey(
            Key parentKey, String branch, String buildFlavor, String series, long startTime) {
        StringBuilder sb = new StringBuilder();
        sb.append(branch);
        sb.append(DELIMITER);
        sb.append(buildFlavor);
        sb.append(DELIMITER);
        sb.append(series);
        sb.append(DELIMITER);
        sb.append(startTime);
        return KeyFactory.createKey(parentKey, KIND, sb.toString());
    }

    /**
     * Updates the profiling summary with the data from a new profiling report.
     *
     * @param profilingRun The profiling point run entity object containing profiling data.
     */
    public void update(ProfilingPointRunEntity profilingRun) {
        if (profilingRun.labels != null
                && profilingRun.labels.size() == profilingRun.values.size()) {
            for (int i = 0; i < profilingRun.labels.size(); i++) {
                String label = profilingRun.labels.get(i);
                if (!this.labelStats.containsKey(label)) {
                    StatSummary summary = new StatSummary(label, profilingRun.regressionMode);
                    this.labelStats.put(label, summary);
                }
                StatSummary summary = this.labelStats.get(label);
                summary.updateStats(profilingRun.values.get(i));
            }
            this.labels.clear();
            this.labels.addAll(profilingRun.labels);
        }
        for (long value : profilingRun.values) {
            this.globalStats.updateStats(value);
        }
    }

    @Override
    public Entity toEntity() {
        Entity profilingSummary;
        profilingSummary = new Entity(this.key);
        profilingSummary.setUnindexedProperty(MEAN, this.globalStats.getMean());
        profilingSummary.setUnindexedProperty(SUMSQ, this.globalStats.getSumSq());
        profilingSummary.setUnindexedProperty(MIN, this.globalStats.getMin());
        profilingSummary.setUnindexedProperty(MAX, this.globalStats.getMax());
        profilingSummary.setUnindexedProperty(COUNT, this.globalStats.getCount());
        profilingSummary.setIndexedProperty(START_TIME, this.startTime);
        profilingSummary.setIndexedProperty(BRANCH, this.branch);
        profilingSummary.setIndexedProperty(BUILD_FLAVOR, this.buildFlavor);
        profilingSummary.setIndexedProperty(SERIES, this.series);
        if (this.labels.size() != 0) {
            List<Double> labelMeans = new ArrayList<>();
            List<Double> labelSumsqs = new ArrayList<>();
            List<Double> labelMins = new ArrayList<>();
            List<Double> labelMaxes = new ArrayList<>();
            List<Long> labelCounts = new ArrayList<>();
            for (String label : this.labels) {
                if (!this.labelStats.containsKey(label)) continue;
                StatSummary labelStat = this.labelStats.get(label);
                labelMeans.add(labelStat.getMean());
                labelSumsqs.add(labelStat.getSumSq());
                labelMins.add(labelStat.getMin());
                labelMaxes.add(labelStat.getMax());
                labelCounts.add(new Long(labelStat.getCount()));
            }
            profilingSummary.setUnindexedProperty(LABELS, this.labels);
            profilingSummary.setUnindexedProperty(LABEL_MEANS, labelMeans);
            profilingSummary.setUnindexedProperty(LABEL_SUMSQS, labelSumsqs);
            profilingSummary.setUnindexedProperty(LABEL_MINS, labelMins);
            profilingSummary.setUnindexedProperty(LABEL_MAXES, labelMaxes);
            profilingSummary.setUnindexedProperty(LABEL_COUNTS, labelCounts);
        }

        return profilingSummary;
    }

    /**
     * Convert an Entity object to a ProfilingPointSummaryEntity.
     *
     * @param e The entity to process.
     * @return ProfilingPointSummaryEntity object with the properties from e, or null if
     *     incompatible.
     */
    @SuppressWarnings("unchecked")
    public static ProfilingPointSummaryEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND)
                || !e.hasProperty(MEAN)
                || !e.hasProperty(SUMSQ)
                || !e.hasProperty(MIN)
                || !e.hasProperty(MAX)
                || !e.hasProperty(COUNT)
                || !e.hasProperty(START_TIME)
                || !e.hasProperty(BRANCH)
                || !e.hasProperty(BUILD_FLAVOR)
                || !e.hasProperty(SERIES)) {
            logger.log(
                    Level.WARNING, "Missing profiling point attributes in entity: " + e.toString());
            return null;
        }
        try {
            Key parentKey = e.getParent();
            double mean = (double) e.getProperty(MEAN);
            double sumsq = (double) e.getProperty(SUMSQ);
            double min = (double) e.getProperty(MIN);
            double max = (double) e.getProperty(MAX);
            int count = (int) (long) e.getProperty(COUNT);
            StatSummary globalStats =
                    new StatSummary(
                            null,
                            min,
                            max,
                            mean,
                            sumsq,
                            count,
                            VtsProfilingRegressionMode.UNKNOWN_REGRESSION_MODE);
            Map<String, StatSummary> labelStats = new HashMap<>();
            List<String> labels = new ArrayList<>();
            if (e.hasProperty(LABELS)) {
                labels = (List<String>) e.getProperty(LABELS);
                List<Double> labelMeans = (List<Double>) e.getProperty(LABEL_MEANS);
                List<Double> labelSumsqs = (List<Double>) e.getProperty(LABEL_SUMSQS);
                List<Double> labelMins = (List<Double>) e.getProperty(LABEL_MINS);
                List<Double> labelMaxes = (List<Double>) e.getProperty(LABEL_MAXES);
                List<Long> labelCounts = (List<Long>) e.getProperty(LABEL_COUNTS);
                if (labels.size() != labelMeans.size()
                        || labels.size() != labelSumsqs.size()
                        || labels.size() != labelMins.size()
                        || labels.size() != labelMaxes.size()
                        || labels.size() != labelCounts.size()) {
                    logger.log(Level.WARNING, "Jagged label information for entity: " + e.getKey());
                    return null;
                }
                for (int i = 0; i < labels.size(); ++i) {
                    StatSummary labelStat =
                            new StatSummary(
                                    labels.get(i),
                                    labelMins.get(i),
                                    labelMaxes.get(i),
                                    labelMeans.get(i),
                                    labelSumsqs.get(i),
                                    labelCounts.get(i).intValue(),
                                    VtsProfilingRegressionMode.UNKNOWN_REGRESSION_MODE);
                    labelStats.put(labels.get(i), labelStat);
                }
            }
            String branch = (String) e.getProperty(BRANCH);
            String buildFlavor = (String) e.getProperty(BUILD_FLAVOR);
            String series = (String) e.getProperty(SERIES);
            long startTime = (long) e.getProperty(START_TIME);
            return new ProfilingPointSummaryEntity(
                    parentKey,
                    globalStats,
                    labels,
                    labelStats,
                    branch,
                    buildFlavor,
                    series,
                    startTime);
        } catch (ClassCastException exception) {
            // Invalid cast
            logger.log(Level.WARNING, "Error parsing profiling point summary entity.", exception);
        }
        return null;
    }
}
