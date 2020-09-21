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

import com.android.vts.entity.TestCaseRunEntity;
import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.FilterUtil;
import com.android.vts.util.TestResults;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.appengine.api.datastore.Query.SortDirection;
import com.google.gson.Gson;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Servlet for handling requests to load individual tables. */
public class ShowTableServlet extends BaseServlet {
    private static final String TABLE_JSP = "WEB-INF/jsp/show_table.jsp";
    // Error message displayed on the webpage is tableName passed is null.
    private static final String TABLE_NAME_ERROR = "Error : Table name must be passed!";
    private static final String PROFILING_DATA_ALERT = "No profiling data was found.";
    private static final int MAX_BUILD_IDS_PER_PAGE = 10;

    @Override
    public PageType getNavParentType() {
        return PageType.TOT;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        String testName = request.getParameter("testName");
        links.add(new Page(PageType.TABLE, testName, "?testName=" + testName));
        return links;
    }

    public static void processTestRun(TestResults testResults, Entity testRun) {
        TestRunEntity testRunEntity = TestRunEntity.fromEntity(testRun);
        if (testRunEntity == null) {
            return;
        }

        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        List<Key> gets = new ArrayList<>();
        for (long testCaseId : testRunEntity.testCaseIds) {
            gets.add(KeyFactory.createKey(TestCaseRunEntity.KIND, testCaseId));
        }

        List<Entity> testCases = new ArrayList<>();
        Map<Key, Entity> entityMap = datastore.get(gets);
        for (Key key : gets) {
            if (entityMap.containsKey(key)) {
                testCases.add(entityMap.get(key));
            }
        }

        testResults.addTestRun(testRun, testCases);
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

        TestResults testResults = new TestResults(testName);

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

        List<Key> gets =
                FilterUtil.getMatchingKeys(
                        testKey,
                        TestRunEntity.KIND,
                        userTestFilters,
                        userDeviceFilter,
                        dir,
                        MAX_BUILD_IDS_PER_PAGE);
        Map<Key, Entity> entityMap = datastore.get(gets);
        for (Key key : gets) {
            if (!entityMap.containsKey(key)) {
                continue;
            }
            processTestRun(testResults, entityMap.get(key));
        }
        testResults.processReport();

        if (testResults.profilingPointNames.length == 0) {
            profilingDataAlert = PROFILING_DATA_ALERT;
        }

        FilterUtil.setAttributes(request, parameterMap);

        request.setAttribute("testName", request.getParameter("testName"));

        request.setAttribute("error", profilingDataAlert);

        // pass values by converting to JSON
        request.setAttribute("headerRow", new Gson().toJson(testResults.headerRow));
        request.setAttribute("timeGrid", new Gson().toJson(testResults.timeGrid));
        request.setAttribute("durationGrid", new Gson().toJson(testResults.durationGrid));
        request.setAttribute("summaryGrid", new Gson().toJson(testResults.summaryGrid));
        request.setAttribute("resultsGrid", new Gson().toJson(testResults.resultsGrid));
        request.setAttribute("profilingPointNames", testResults.profilingPointNames);
        request.setAttribute("resultNames", resultNames);
        request.setAttribute("resultNamesJson", new Gson().toJson(resultNames));
        request.setAttribute("logInfoMap", new Gson().toJson(testResults.logInfoMap));

        // data for pie chart
        request.setAttribute(
                "topBuildResultCounts", new Gson().toJson(testResults.totResultCounts));
        request.setAttribute("topBuildId", testResults.totBuildId);
        request.setAttribute("startTime", new Gson().toJson(testResults.startTime));
        request.setAttribute("endTime", new Gson().toJson(testResults.endTime));
        request.setAttribute(
                "hasNewer",
                new Gson()
                        .toJson(
                                DatastoreHelper.hasNewer(
                                        testKey, TestRunEntity.KIND, testResults.endTime)));
        request.setAttribute(
                "hasOlder",
                new Gson()
                        .toJson(
                                DatastoreHelper.hasOlder(
                                        testKey, TestRunEntity.KIND, testResults.startTime)));
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
