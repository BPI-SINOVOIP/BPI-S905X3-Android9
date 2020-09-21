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

package com.android.vts.servlet;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.ProfilingPointRunEntity;
import com.android.vts.entity.TestCaseRunEntity;
import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.FilterUtil;
import com.android.vts.util.TestRunDetails;
import com.android.vts.util.TestRunMetadata;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.appengine.api.datastore.Query.SortDirection;
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Servlet for handling requests to load individual tables. */
public class ShowTreeServlet extends BaseServlet {
    private static final String TABLE_JSP = "WEB-INF/jsp/show_tree.jsp";
    // Error message displayed on the webpage is tableName passed is null.
    private static final String TABLE_NAME_ERROR = "Error : Table name must be passed!";
    private static final String PROFILING_DATA_ALERT = "No profiling data was found.";
    private static final int MAX_RESULT_COUNT = 60;
    private static final int MAX_PREFETCH_COUNT = 10;

    @Override
    public PageType getNavParentType() {
        return PageType.TOT;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        String testName = request.getParameter("testName");
        links.add(new Page(PageType.TREE, testName, "?testName=" + testName));
        return links;
    }

    /**
     * Get the test run details for a test run.
     *
     * @param metadata The metadata for the test run whose details will be fetched.
     * @return The TestRunDetails object for the provided test run.
     */
    public static TestRunDetails processTestDetails(TestRunMetadata metadata) {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        TestRunDetails details = new TestRunDetails();
        List<Key> gets = new ArrayList<>();
        for (long testCaseId : metadata.testRun.testCaseIds) {
            gets.add(KeyFactory.createKey(TestCaseRunEntity.KIND, testCaseId));
        }
        Map<Key, Entity> entityMap = datastore.get(gets);
        for (int i = 0; i < 1; i++) {
            for (Key key : entityMap.keySet()) {
                TestCaseRunEntity testCaseRun = TestCaseRunEntity.fromEntity(entityMap.get(key));
                if (testCaseRun == null) {
                    continue;
                }
                details.addTestCase(testCaseRun);
            }
        }
        return details;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        boolean unfiltered = request.getParameter("unfiltered") != null;
        boolean showPresubmit = request.getParameter("showPresubmit") != null;
        boolean showPostsubmit = request.getParameter("showPostsubmit") != null;
        Long startTime = null; // time in microseconds
        Long endTime = null; // time in microseconds
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        RequestDispatcher dispatcher = null;

        // message to display if profiling point data is not available
        String profilingDataAlert = "";

        if (request.getParameter("testName") == null) {
            request.setAttribute("testName", TABLE_NAME_ERROR);
            return;
        }
        String testName = request.getParameter("testName");

        if (request.getParameter("startTime") != null) {
            String time = request.getParameter("startTime");
            try {
                startTime = Long.parseLong(time);
                startTime = startTime > 0 ? startTime : null;
            } catch (NumberFormatException e) {
                startTime = null;
            }
        }
        if (request.getParameter("endTime") != null) {
            String time = request.getParameter("endTime");
            try {
                endTime = Long.parseLong(time);
                endTime = endTime > 0 ? endTime : null;
            } catch (NumberFormatException e) {
                endTime = null;
            }
        }

        // If no params are specified, set to default of postsubmit-only.
        if (!(showPresubmit || showPostsubmit)) {
            showPostsubmit = true;
        }

        // If unfiltered, set showPre- and Post-submit to true for accurate UI.
        if (unfiltered) {
            showPostsubmit = true;
            showPresubmit = true;
        }

        // Add result names to list
        List<String> resultNames = new ArrayList<>();
        for (TestCaseResult r : TestCaseResult.values()) {
            resultNames.add(r.name());
        }

        SortDirection dir = SortDirection.DESCENDING;
        if (startTime != null && endTime == null) {
            dir = SortDirection.ASCENDING;
        }
        Key testKey = KeyFactory.createKey(TestEntity.KIND, testName);

        Filter typeFilter = FilterUtil.getTestTypeFilter(showPresubmit, showPostsubmit, unfiltered);
        Filter testFilter =
                FilterUtil.getTimeFilter(
                        testKey, TestRunEntity.KIND, startTime, endTime, typeFilter);

        Map<String, Object> parameterMap = request.getParameterMap();
        List<Filter> userTestFilters = FilterUtil.getUserTestFilters(parameterMap);
        userTestFilters.add(0, testFilter);
        Filter userDeviceFilter = FilterUtil.getUserDeviceFilter(parameterMap);

        List<TestRunMetadata> testRunMetadata = new ArrayList<>();
        Map<Key, TestRunMetadata> metadataMap = new HashMap<>();
        Key minKey = null;
        Key maxKey = null;
        List<Key> gets =
                FilterUtil.getMatchingKeys(
                        testKey,
                        TestRunEntity.KIND,
                        userTestFilters,
                        userDeviceFilter,
                        dir,
                        MAX_RESULT_COUNT);
        Map<Key, Entity> entityMap = datastore.get(gets);
        for (Key key : gets) {
            if (!entityMap.containsKey(key)) {
                continue;
            }
            TestRunEntity testRunEntity = TestRunEntity.fromEntity(entityMap.get(key));
            if (testRunEntity == null) {
                continue;
            }
            if (minKey == null || key.compareTo(minKey) < 0) {
                minKey = key;
            }
            if (maxKey == null || key.compareTo(maxKey) > 0) {
                maxKey = key;
            }
            TestRunMetadata metadata = new TestRunMetadata(testName, testRunEntity);
            testRunMetadata.add(metadata);
            metadataMap.put(key, metadata);
        }

        List<String> profilingPointNames = new ArrayList<>();
        if (minKey != null && maxKey != null) {
            Filter deviceFilter =
                    FilterUtil.getDeviceTimeFilter(
                            testKey, TestRunEntity.KIND, minKey.getId(), maxKey.getId());
            Query deviceQuery =
                    new Query(DeviceInfoEntity.KIND)
                            .setAncestor(testKey)
                            .setFilter(deviceFilter)
                            .setKeysOnly();
            List<Key> deviceGets = new ArrayList<>();
            for (Entity device :
                    datastore
                            .prepare(deviceQuery)
                            .asIterable(DatastoreHelper.getLargeBatchOptions())) {
                if (metadataMap.containsKey(device.getParent())) {
                    deviceGets.add(device.getKey());
                }
            }
            Map<Key, Entity> devices = datastore.get(deviceGets);
            for (Key key : devices.keySet()) {
                if (!metadataMap.containsKey(key.getParent())) continue;
                DeviceInfoEntity device = DeviceInfoEntity.fromEntity(devices.get(key));
                if (device == null) continue;
                TestRunMetadata metadata = metadataMap.get(key.getParent());
                metadata.addDevice(device);
            }

            Filter profilingFilter =
                    FilterUtil.getProfilingTimeFilter(
                            testKey, TestRunEntity.KIND, minKey.getId(), maxKey.getId());

            Set<String> profilingPoints = new HashSet<>();
            Query profilingPointQuery =
                    new Query(ProfilingPointRunEntity.KIND)
                            .setAncestor(testKey)
                            .setFilter(profilingFilter)
                            .setKeysOnly();
            for (Entity e : datastore.prepare(profilingPointQuery).asIterable()) {
                profilingPoints.add(e.getKey().getName());
            }

            if (profilingPoints.size() == 0) {
                profilingDataAlert = PROFILING_DATA_ALERT;
            }
            profilingPointNames.addAll(profilingPoints);
            profilingPointNames.sort(Comparator.naturalOrder());
        }

        testRunMetadata.sort(
                (t1, t2) ->
                        new Long(t2.testRun.startTimestamp).compareTo(t1.testRun.startTimestamp));
        List<JsonObject> testRunObjects = new ArrayList<>();

        int prefetchCount = 0;
        for (TestRunMetadata metadata : testRunMetadata) {
            if (metadata.testRun.failCount > 0 && prefetchCount < MAX_PREFETCH_COUNT) {
                // process
                metadata.addDetails(processTestDetails(metadata));
                ++prefetchCount;
            }
            testRunObjects.add(metadata.toJson());
        }

        int[] topBuildResultCounts = null;
        String topBuild = "";
        if (testRunMetadata.size() > 0) {
            TestRunMetadata firstRun = testRunMetadata.get(0);
            topBuild = firstRun.getDeviceInfo();
            endTime = firstRun.testRun.startTimestamp;
            TestRunDetails topDetails = firstRun.getDetails();
            if (topDetails == null) {
                topDetails = processTestDetails(firstRun);
            }
            topBuildResultCounts = topDetails.resultCounts;

            TestRunMetadata lastRun = testRunMetadata.get(testRunMetadata.size() - 1);
            startTime = lastRun.testRun.startTimestamp;
        }

        FilterUtil.setAttributes(request, parameterMap);

        request.setAttribute("testName", request.getParameter("testName"));

        request.setAttribute("error", profilingDataAlert);

        request.setAttribute("profilingPointNames", profilingPointNames);
        request.setAttribute("resultNames", resultNames);
        request.setAttribute("resultNamesJson", new Gson().toJson(resultNames));
        request.setAttribute("testRuns", new Gson().toJson(testRunObjects));

        // data for pie chart
        request.setAttribute("topBuildResultCounts", new Gson().toJson(topBuildResultCounts));
        request.setAttribute("topBuildId", topBuild);
        request.setAttribute("startTime", new Gson().toJson(startTime));
        request.setAttribute("endTime", new Gson().toJson(endTime));
        request.setAttribute(
                "hasNewer",
                new Gson().toJson(DatastoreHelper.hasNewer(testKey, TestRunEntity.KIND, endTime)));
        request.setAttribute(
                "hasOlder",
                new Gson()
                        .toJson(DatastoreHelper.hasOlder(testKey, TestRunEntity.KIND, startTime)));
        request.setAttribute("unfiltered", unfiltered);
        request.setAttribute("showPresubmit", showPresubmit);
        request.setAttribute("showPostsubmit", showPostsubmit);

        request.setAttribute("branches", new Gson().toJson(DatastoreHelper.getAllBranches()));
        request.setAttribute("devices", new Gson().toJson(DatastoreHelper.getAllBuildFlavors()));

        dispatcher = request.getRequestDispatcher(TABLE_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : " + e.toString());
        }
    }
}
