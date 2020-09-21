/*
 * Copyright (c) 2016 Google Inc. All Rights Reserved.
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
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Helper object for describing graph data. */
public class LineGraph extends Graph {
    public static final String TICKS_KEY = "ticks";

    private List<ProfilingPointRunEntity> profilingRuns;
    private List<String> ids;
    private String xLabel;
    private String yLabel;
    private String name;
    private GraphType type = GraphType.LINE_GRAPH;

    public LineGraph(String name) {
        this.name = name;
        profilingRuns = new ArrayList<>();
        ids = new ArrayList<>();
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
        return profilingRuns.size();
    }

    /**
     * Add data to the graph.
     *
     * @param id The name of the graph.
     * @param profilingPoint The ProfilingPointRunEntity containing data to add.
     */
    @Override
    public void addData(String id, ProfilingPointRunEntity profilingPoint) {
        if (profilingPoint.values.size() == 0
                || profilingPoint.values.size() != profilingPoint.labels.size())
            return;
        ids.add(id);
        profilingRuns.add(profilingPoint);
        xLabel = profilingPoint.xLabel;
        yLabel = profilingPoint.yLabel;
    }

    /**
     * Serializes the graph to json format.
     *
     * @return A JsonElement object representing the graph object.
     */
    @Override
    public JsonObject toJson() {
        JsonObject json = super.toJson();
        // Use the most recent profiling vector to generate the labels
        ProfilingPointRunEntity profilingRun = profilingRuns.get(profilingRuns.size() - 1);

        List<String> axisTicks = new ArrayList<>();
        Map<String, Integer> tickIndexMap = new HashMap<>();
        for (int i = 0; i < profilingRun.labels.size(); i++) {
            String label = profilingRun.labels.get(i);
            axisTicks.add(label);
            tickIndexMap.put(label, i);
        }

        long[][] lineGraphValues = new long[axisTicks.size()][profilingRuns.size()];
        for (int reportIndex = 0; reportIndex < profilingRuns.size(); reportIndex++) {
            ProfilingPointRunEntity pt = profilingRuns.get(reportIndex);
            for (int i = 0; i < pt.labels.size(); i++) {
                String label = pt.labels.get(i);

                // Skip value if its label is not present
                if (!tickIndexMap.containsKey(label))
                    continue;
                int labelIndex = tickIndexMap.get(label);

                lineGraphValues[labelIndex][reportIndex] = pt.values.get(i);
            }
        }
        json.add(VALUE_KEY, new Gson().toJsonTree(lineGraphValues).getAsJsonArray());
        json.add(IDS_KEY, new Gson().toJsonTree(ids).getAsJsonArray());
        json.add(TICKS_KEY, new Gson().toJsonTree(axisTicks));
        return json;
    }
}
