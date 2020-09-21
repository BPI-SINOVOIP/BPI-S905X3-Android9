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

import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.proto.VtsReportMessage;
import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.FilterUtil;
import com.android.vts.util.TestRunMetadata;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.Query;
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Represents the servlet that is invoked on loading the coverage overview page. */
public class ShowCoverageOverviewServlet extends BaseServlet {
    private static final String COVERAGE_OVERVIEW_JSP = "WEB-INF/jsp/show_coverage_overview.jsp";

    @Override
    public PageType getNavParentType() {
        return PageType.COVERAGE_OVERVIEW;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        return null;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        RequestDispatcher dispatcher = null;
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        boolean unfiltered = request.getParameter("unfiltered") != null;
        boolean showPresubmit = request.getParameter("showPresubmit") != null;
        boolean showPostsubmit = request.getParameter("showPostsubmit") != null;

        // If no params are specified, set to default of postsubmit-only.
        if (!(showPresubmit || showPostsubmit)) {
            showPostsubmit = true;
        }

        // If unfiltered, set showPre- and Post-submit to true for accurate UI.
        if (unfiltered) {
            showPostsubmit = true;
            showPresubmit = true;
        }

        Query q = new Query(TestEntity.KIND).setKeysOnly();
        List<Key> allTests = new ArrayList<>();
        for (Entity test : datastore.prepare(q).asIterable()) {
            allTests.add(test.getKey());
        }

        // Add test names to list
        List<String> resultNames = new ArrayList<>();
        for (VtsReportMessage.TestCaseResult r : VtsReportMessage.TestCaseResult.values()) {
            resultNames.add(r.name());
        }

        List<JsonObject> testRunObjects = new ArrayList<>();

        Query.Filter testFilter =
                new Query.FilterPredicate(
                        TestRunEntity.HAS_COVERAGE, Query.FilterOperator.EQUAL, true);
        Query.Filter timeFilter =
                FilterUtil.getTestTypeFilter(showPresubmit, showPostsubmit, unfiltered);

        if (timeFilter != null) {
            testFilter = Query.CompositeFilterOperator.and(testFilter, timeFilter);
        }
        Map<String, Object> parameterMap = request.getParameterMap();
        List<Query.Filter> userTestFilters = FilterUtil.getUserTestFilters(parameterMap);
        userTestFilters.add(0, testFilter);
        Query.Filter userDeviceFilter = FilterUtil.getUserDeviceFilter(parameterMap);

        int coveredLines = 0;
        int uncoveredLines = 0;
        int passCount = 0;
        int failCount = 0;
        for (Key key : allTests) {
            List<Key> gets =
                    FilterUtil.getMatchingKeys(
                            key,
                            TestRunEntity.KIND,
                            userTestFilters,
                            userDeviceFilter,
                            Query.SortDirection.DESCENDING,
                            1);
            Map<Key, Entity> entityMap = datastore.get(gets);
            for (Key entityKey : gets) {
                if (!entityMap.containsKey(entityKey)) {
                    continue;
                }
                TestRunEntity testRunEntity = TestRunEntity.fromEntity(entityMap.get(entityKey));
                if (testRunEntity == null) {
                    continue;
                }
                TestRunMetadata metadata = new TestRunMetadata(key.getName(), testRunEntity);
                testRunObjects.add(metadata.toJson());
                coveredLines += testRunEntity.coveredLineCount;
                uncoveredLines += testRunEntity.totalLineCount - testRunEntity.coveredLineCount;
                passCount += testRunEntity.passCount;
                failCount += testRunEntity.failCount;
            }
        }

        FilterUtil.setAttributes(request, parameterMap);

        int[] testStats = new int[VtsReportMessage.TestCaseResult.values().length];
        testStats[VtsReportMessage.TestCaseResult.TEST_CASE_RESULT_PASS.getNumber()] = passCount;
        testStats[VtsReportMessage.TestCaseResult.TEST_CASE_RESULT_FAIL.getNumber()] = failCount;

        response.setStatus(HttpServletResponse.SC_OK);
        request.setAttribute("resultNames", resultNames);
        request.setAttribute("resultNamesJson", new Gson().toJson(resultNames));
        request.setAttribute("testRuns", new Gson().toJson(testRunObjects));
        request.setAttribute("coveredLines", new Gson().toJson(coveredLines));
        request.setAttribute("uncoveredLines", new Gson().toJson(uncoveredLines));
        request.setAttribute("testStats", new Gson().toJson(testStats));

        request.setAttribute("unfiltered", unfiltered);
        request.setAttribute("showPresubmit", showPresubmit);
        request.setAttribute("showPostsubmit", showPostsubmit);
        request.setAttribute("branches", new Gson().toJson(DatastoreHelper.getAllBranches()));
        request.setAttribute("devices", new Gson().toJson(DatastoreHelper.getAllBuildFlavors()));
        dispatcher = request.getRequestDispatcher(COVERAGE_OVERVIEW_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : ", e);
        }
    }
}
