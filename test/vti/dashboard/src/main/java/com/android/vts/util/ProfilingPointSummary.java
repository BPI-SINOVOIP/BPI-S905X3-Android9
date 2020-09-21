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

package com.android.vts.util;

import com.android.vts.entity.ProfilingPointSummaryEntity;
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/** Represents statistical summaries of each profiling point. */
public class ProfilingPointSummary implements Iterable<StatSummary> {
    private List<StatSummary> statSummaries;
    private Map<String, Integer> labelIndices;
    private List<String> labels;
    private VtsProfilingRegressionMode regressionMode;
    public String xLabel;
    public String yLabel;

    /** Initializes the summary with empty arrays. */
    public ProfilingPointSummary(
            String xLabel, String yLabel, VtsProfilingRegressionMode regressionMode) {
        statSummaries = new ArrayList<>();
        labelIndices = new HashMap<>();
        labels = new ArrayList<>();
        this.regressionMode = regressionMode;
        this.xLabel = xLabel;
        this.yLabel = yLabel;
    }

    /**
     * Determines if a data label is present in the profiling point.
     *
     * @param label The name of the label.
     * @return true if the label is present, false otherwise.
     */
    public boolean hasLabel(String label) {
        return labelIndices.containsKey(label);
    }

    /**
     * Gets the statistical summary for a specified data label.
     *
     * @param label The name of the label.
     * @return The StatSummary object if it exists, or null otherwise.
     */
    public StatSummary getStatSummary(String label) {
        if (!hasLabel(label)) return null;
        return statSummaries.get(labelIndices.get(label));
    }

    /**
     * Gets the last-seen regression mode.
     *
     * @return The VtsProfilingRegressionMode value.
     */
    public VtsProfilingRegressionMode getRegressionMode() {
        return regressionMode;
    }

    /**
     * Updates the profiling summary with the data from a new profiling report.
     *
     * @param ppSummary The profiling point run entity object containing profiling data.
     */
    public void update(ProfilingPointSummaryEntity ppSummary) {
        for (String label : ppSummary.labels) {
            if (!ppSummary.labelStats.containsKey(label)) continue;
            if (!labelIndices.containsKey(label)) {
                labelIndices.put(label, statSummaries.size());
                labels.add(label);
                statSummaries.add(ppSummary.labelStats.get(label));
            } else {
                StatSummary summary = getStatSummary(label);
                summary.merge(ppSummary.labelStats.get(label));
            }
        }
    }

    /**
     * Updates the profiling summary at a label with the data from a new profiling report.
     *
     * <p>Updates the summary specified by the label with all values provided in the report. If
     * labels are provided in the report, they will be ignored -- all values are updated only to the
     * provided label.
     *
     * @param ppSummary The ProfilingPointSummaryEntity object containing profiling data.
     * @param label The String label for which all values in the report will be updated.
     */
    public void updateLabel(ProfilingPointSummaryEntity ppSummary, String label) {
        if (!labelIndices.containsKey(label)) {
            labelIndices.put(label, labels.size());
            labels.add(label);
            StatSummary stat =
                    new StatSummary(
                            label,
                            ppSummary.globalStats.getMin(),
                            ppSummary.globalStats.getMax(),
                            ppSummary.globalStats.getMean(),
                            ppSummary.globalStats.getSumSq(),
                            ppSummary.globalStats.getCount(),
                            ppSummary.globalStats.getRegressionMode());
            statSummaries.add(stat);
        } else {
            StatSummary summary = getStatSummary(label);
            summary.merge(ppSummary.globalStats);
        }
    }

    /**
     * Gets an iterator that returns stat summaries in the ordered the labels were specified in the
     * ProfilingReportMessage objects.
     */
    @Override
    public Iterator<StatSummary> iterator() {
        Iterator<StatSummary> it =
                new Iterator<StatSummary>() {
                    private int currentIndex = 0;

                    @Override
                    public boolean hasNext() {
                        return labels != null && currentIndex < labels.size();
                    }

                    @Override
                    public StatSummary next() {
                        String label = labels.get(currentIndex++);
                        return statSummaries.get(labelIndices.get(label));
                    }

                    @Override
                    public void remove() {
                        // Not supported
                        throw new UnsupportedOperationException();
                    }
                };
        return it;
    }
}
