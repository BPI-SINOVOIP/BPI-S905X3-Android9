/*
 * Copyright (c) 2017 Google Inc. All Rights Reserved.
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

import com.android.vts.entity.ProfilingPointRunEntity;
import com.android.vts.proto.VtsReportMessage;
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Helper object for describing time-series box plot data. */
public class BoxPlot extends Graph {
    private static final String LABEL_KEY = "label";
    private static final String SERIES_KEY = "seriesList";
    private static final String MEAN_KEY = "mean";
    private static final String STD_KEY = "std";
    private static final String COUNT_KEY = "count";

    private static final String DAY = "Day";

    private final GraphType type = GraphType.BOX_PLOT;
    private final String name;
    private final String xLabel;
    private final String yLabel;
    private final Map<String, Map<String, StatSummary>> labelSeriesMap;
    private final Set<String> seriesSet;

    private int count;
    private List<String> labels;

    public BoxPlot(String name, String xLabel, String yLabel) {
        this.name = name;
        this.xLabel = xLabel == null ? DAY : xLabel;
        this.yLabel = yLabel;
        this.count = 0;
        this.labelSeriesMap = new HashMap<>();
        this.seriesSet = new HashSet<>();
        this.labels = new ArrayList<>();
    }

    /**
     * Get the x axis label.
     *
     * @return The x axis label.
     */
    @Override
    public String getXLabel() {
        return xLabel;
    }

    /**
     * Get the graph type.
     *
     * @return The graph type.
     */
    @Override
    public GraphType getType() {
        return type;
    }

    /**
     * Get the name of the graph.
     *
     * @return The name of the graph.
     */
    @Override
    public String getName() {
        return name;
    }

    /**
     * Get the y axis label.
     *
     * @return The y axis label.
     */
    @Override
    public String getYLabel() {
        return yLabel;
    }

    /**
     * Get the number of data points stored in the graph.
     *
     * @return The number of data points stored in the graph.
     */
    @Override
    public int size() {
        return this.count;
    }

    /**
     * Add data to the graph.
     *
     * @param label The name of the category.
     * @param profilingPoint The ProfilingPointRunEntity containing data to add.
     */
    @Override
    public void addData(String label, ProfilingPointRunEntity profilingPoint) {
        StatSummary stat =
                new StatSummary(
                        label, VtsReportMessage.VtsProfilingRegressionMode.UNKNOWN_REGRESSION_MODE);
        for (long value : profilingPoint.values) {
            stat.updateStats(value);
        }
        addSeriesData(label, "", stat);
    }

    public void addSeriesData(String label, String series, StatSummary stats) {
        if (!labelSeriesMap.containsKey(label)) {
            labelSeriesMap.put(label, new HashMap<>());
        }
        Map<String, StatSummary> seriesMap = labelSeriesMap.get(label);
        seriesMap.put(series, stats);
        seriesSet.add(series);
        count += stats.getCount();
    }

    public void setLabels(List<String> labels) {
        this.labels = labels;
    }

    /**
     * Serializes the graph to json format.
     *
     * @return A JsonElement object representing the graph object.
     */
    @Override
    public JsonObject toJson() {
        JsonObject json = super.toJson();
        List<JsonObject> stats = new ArrayList<>();
        List<String> seriesList = new ArrayList<>(seriesSet);
        seriesList.sort(Comparator.naturalOrder());
        for (String label : labels) {
            JsonObject statJson = new JsonObject();
            String boxLabel = label;
            List<JsonObject> statList = new ArrayList<>(seriesList.size());
            Map<String, StatSummary> seriesMap = labelSeriesMap.get(label);
            for (String series : seriesList) {
                JsonObject statSummary = new JsonObject();
                Double mean = null;
                Double std = null;
                Integer count = null;
                if (seriesMap.containsKey(series) && seriesMap.get(series).getCount() > 0) {
                    StatSummary stat = seriesMap.get(series);
                    mean = stat.getMean();
                    std = 0.;
                    if (stat.getCount() > 1) {
                        std = stat.getStd();
                    }
                    count = stat.getCount();
                }
                statSummary.addProperty(MEAN_KEY, mean);
                statSummary.addProperty(STD_KEY, std);
                statSummary.addProperty(COUNT_KEY, count);
                statList.add(statSummary);
            }
            statJson.addProperty(LABEL_KEY, boxLabel);
            statJson.add(VALUE_KEY, new Gson().toJsonTree(statList));
            stats.add(statJson);
        }
        json.add(VALUE_KEY, new Gson().toJsonTree(stats));
        json.add(SERIES_KEY, new Gson().toJsonTree(seriesList));
        return json;
    }
}
