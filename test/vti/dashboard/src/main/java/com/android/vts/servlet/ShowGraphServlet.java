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

package com.android.vts.servlet;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.ProfilingPointRunEntity;
import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.FilterUtil;
import com.android.vts.util.Graph;
import com.android.vts.util.GraphSerializer;
import com.android.vts.util.Histogram;
import com.android.vts.util.LineGraph;
import com.android.vts.util.PerformanceUtil;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.commons.lang.StringUtils;

/** Servlet for handling requests to load graphs. */
public class ShowGraphServlet extends BaseServlet {
    private static final String GRAPH_JSP = "WEB-INF/jsp/show_graph.jsp";
    private static final long DEFAULT_FILTER_OPTION = -1;

    private static final String HIDL_HAL_OPTION = "hidl_hal_mode";
    private static final String[] splitKeysArray = new String[] {HIDL_HAL_OPTION};
    private static final Set<String> splitKeySet = new HashSet<>(Arrays.asList(splitKeysArray));
    private static final String PROFILING_DATA_ALERT = "No profiling data was found.";

    @Override
    public PageType getNavParentType() {
        return PageType.TOT;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        String testName = request.getParameter("testName");
        links.add(new Page(PageType.TABLE, testName, "?testName=" + testName));

        String profilingPointName = request.getParameter("profilingPoint");
        links.add(
                new Page(
                        PageType.GRAPH,
                        "?testName=" + testName + "&profilingPoint=" + profilingPointName));
        return links;
    }

    /**
     * Process a profiling report message and add it to the map of graphs.
     *
     * @param profilingRun The Entity of a profiling point run to process.
     * @param idString The ID derived from the test run to identify the profiling report.
     * @param graphMap A map from graph name to Graph object.
     */
    private static void processProfilingRun(
            Entity profilingRun, String idString, Map<String, Graph> graphMap) {
        ProfilingPointRunEntity pt = ProfilingPointRunEntity.fromEntity(profilingRun);
        if (pt == null) return;
        String name = PerformanceUtil.getOptionAlias(pt, splitKeySet);
        Graph g = null;
        if (pt.labels != null && pt.labels.size() == pt.values.size()) {
            g = new LineGraph(name);
        } else if (pt.labels == null && pt.values.size() > 0) {
            g = new Histogram(name);
        } else {
            return;
        }
        if (!graphMap.containsKey(name)) {
            graphMap.put(name, g);
        }
        graphMap.get(name).addData(idString, pt);
    }

    /**
     * Get a summary string describing the devices in the test run.
     *
     * @param devices The list of device descriptors for a particular test run.
     * @return A string describing the devices in the test run.
     */
    private static String getDeviceSummary(List<DeviceInfoEntity> devices) {
        if (devices == null) return null;
        List<String> buildInfos = new ArrayList<>();
        for (DeviceInfoEntity device : devices) {
            buildInfos.add(device.product + " (" + device.buildId + ")");
        }
        return StringUtils.join(buildInfos, ", ");
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        RequestDispatcher dispatcher = null;
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        String testName = request.getParameter("testName");
        String profilingPointName = request.getParameter("profilingPoint");
        String selectedDevice = request.getParameter("device");
        Long endTime = null;
        if (request.getParameter("endTime") != null) {
            String time = request.getParameter("endTime");
            try {
                endTime = Long.parseLong(time);
            } catch (NumberFormatException e) {
            }
        }
        if (endTime == null) {
            endTime = TimeUnit.MILLISECONDS.toMicros(System.currentTimeMillis());
        }
        Long startTime = endTime - TimeUnit.DAYS.toMicros(1);

        // Set of device names
        List<String> devices = DatastoreHelper.getAllBuildFlavors();
        if (!devices.contains(selectedDevice)) selectedDevice = null;

        Map<String, Graph> graphMap = new HashMap<>();

        // Create a query for test runs matching the time window filter
        Key parentKey = KeyFactory.createKey(TestEntity.KIND, testName);
        Filter timeFilter =
                FilterUtil.getTimeFilter(parentKey, TestRunEntity.KIND, startTime, endTime);
        Query testRunQuery =
                new Query(TestRunEntity.KIND)
                        .setAncestor(parentKey)
                        .setFilter(timeFilter)
                        .setKeysOnly();

        // Process the test runs in the query
        List<Key> gets = new ArrayList<>();
        for (Entity testRun :
                datastore
                        .prepare(testRunQuery)
                        .asIterable(DatastoreHelper.getLargeBatchOptions())) {
            gets.add(
                    KeyFactory.createKey(
                            testRun.getKey(), ProfilingPointRunEntity.KIND, profilingPointName));
        }
        Map<Key, Entity> profilingPoints = datastore.get(gets);
        Map<Key, Entity> testRunProfiling = new HashMap<>();
        for (Key key : profilingPoints.keySet()) {
            testRunProfiling.put(key.getParent(), profilingPoints.get(key));
        }

        Filter deviceFilter =
                FilterUtil.getDeviceTimeFilter(parentKey, TestRunEntity.KIND, startTime, endTime);
        if (selectedDevice != null) {
            deviceFilter =
                    Query.CompositeFilterOperator.and(
                            deviceFilter,
                            new Query.FilterPredicate(
                                    DeviceInfoEntity.BUILD_FLAVOR,
                                    Query.FilterOperator.EQUAL,
                                    selectedDevice));
        }
        Query deviceQuery =
                new Query(DeviceInfoEntity.KIND)
                        .setAncestor(parentKey)
                        .setFilter(deviceFilter)
                        .setKeysOnly();
        gets = new ArrayList<>();
        for (Entity device :
                datastore.prepare(deviceQuery).asIterable(DatastoreHelper.getLargeBatchOptions())) {
            if (testRunProfiling.containsKey(device.getParent())) {
                gets.add(device.getKey());
            }
        }

        Map<Key, Entity> deviceInfos = datastore.get(gets);
        Map<Key, List<DeviceInfoEntity>> testRunDevices = new HashMap<>();
        for (Key deviceKey : deviceInfos.keySet()) {
            if (!testRunDevices.containsKey(deviceKey.getParent())) {
                testRunDevices.put(deviceKey.getParent(), new ArrayList<DeviceInfoEntity>());
            }
            DeviceInfoEntity device = DeviceInfoEntity.fromEntity(deviceInfos.get(deviceKey));
            if (device == null) continue;
            testRunDevices.get(deviceKey.getParent()).add(device);
        }

        for (Key runKey : testRunProfiling.keySet()) {
            String idString = getDeviceSummary(testRunDevices.get(runKey));
            if (idString != null) {
                processProfilingRun(testRunProfiling.get(runKey), idString, graphMap);
            }
        }

        // Get the names of the graphs to render
        String[] names = graphMap.keySet().toArray(new String[graphMap.size()]);
        Arrays.sort(names);

        List<Graph> graphList = new ArrayList<>();
        boolean hasHistogram = false;
        for (String name : names) {
            Graph g = graphMap.get(name);
            if (g.size() > 0) {
                graphList.add(g);
                if (g instanceof Histogram) hasHistogram = true;
            }
        }

        String filterVal = request.getParameter("filterVal");
        try {
            Long.parseLong(filterVal);
        } catch (NumberFormatException e) {
            filterVal = Long.toString(DEFAULT_FILTER_OPTION);
        }
        request.setAttribute("testName", request.getParameter("testName"));
        request.setAttribute("filterVal", filterVal);
        request.setAttribute("endTime", new Gson().toJson(endTime));
        request.setAttribute("devices", devices);
        request.setAttribute("selectedDevice", selectedDevice);
        request.setAttribute("showFilterDropdown", hasHistogram);
        if (graphList.size() == 0) request.setAttribute("error", PROFILING_DATA_ALERT);

        Gson gson =
                new GsonBuilder()
                        .registerTypeHierarchyAdapter(Graph.class, new GraphSerializer())
                        .create();
        request.setAttribute("graphs", gson.toJson(graphList));

        request.setAttribute("profilingPointName", profilingPointName);
        dispatcher = request.getRequestDispatcher(GRAPH_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Excpetion caught : ", e);
        }
    }
}
