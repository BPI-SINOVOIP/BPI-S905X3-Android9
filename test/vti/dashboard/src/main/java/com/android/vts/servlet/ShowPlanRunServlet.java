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
import com.android.vts.entity.TestPlanEntity;
import com.android.vts.entity.TestPlanRunEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.util.TestRunMetadata;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.EntityNotFoundException;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
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

/** Servlet for handling requests to load individual plan runs. */
public class ShowPlanRunServlet extends BaseServlet {
    private static final String PLAN_RUN_JSP = "WEB-INF/jsp/show_plan_run.jsp";

    @Override
    public PageType getNavParentType() {
        return PageType.RELEASE;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        String planName = request.getParameter("plan");
        links.add(new Page(PageType.PLAN_RELEASE, planName, "?plan=" + planName));

        String time = request.getParameter("time");
        links.add(new Page(PageType.PLAN_RUN, "?plan=" + planName + "&time=" + time));
        return links;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        Long time = null; // time in microseconds
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        RequestDispatcher dispatcher = null;

        String plan = request.getParameter("plan");

        if (request.getParameter("time") != null) {
            String timeString = request.getParameter("time");
            try {
                time = Long.parseLong(timeString);
                time = time > 0 ? time : null;
            } catch (NumberFormatException e) {
                time = null;
            }
        }

        // Add result names to list
        List<String> resultNames = new ArrayList<>();
        for (TestCaseResult r : TestCaseResult.values()) {
            resultNames.add(r.name());
        }

        List<JsonObject> passingTestObjects = new ArrayList<>();
        List<JsonObject> failingTestObjects = new ArrayList<>();
        List<JsonObject> testRunObjects = new ArrayList<>();

        Key planKey = KeyFactory.createKey(TestPlanEntity.KIND, plan);
        Key planRunKey = KeyFactory.createKey(planKey, TestPlanRunEntity.KIND, time);
        String testBuildId = "";
        int passCount = 0;
        int failCount = 0;
        long startTime = 0;
        long endTime = 0;
        long moduleCount = 0;
        try {
            Entity testPlanRunEntity = datastore.get(planRunKey);
            TestPlanRunEntity testPlanRun = TestPlanRunEntity.fromEntity(testPlanRunEntity);
            Map<Key, Entity> testRuns = datastore.get(testPlanRun.testRuns);
            testBuildId = testPlanRun.testBuildId;
            passCount = (int) testPlanRun.passCount;
            failCount = (int) testPlanRun.failCount;
            startTime = testPlanRun.startTimestamp;
            endTime = testPlanRun.endTimestamp;
            moduleCount = testPlanRun.testRuns.size();

            for (Key key : testPlanRun.testRuns) {
                if (!testRuns.containsKey(key)) continue;
                TestRunEntity testRunEntity = TestRunEntity.fromEntity(testRuns.get(key));
                if (testRunEntity == null) continue;
                Query deviceInfoQuery = new Query(DeviceInfoEntity.KIND).setAncestor(key);
                List<DeviceInfoEntity> devices = new ArrayList<>();
                for (Entity device : datastore.prepare(deviceInfoQuery).asIterable()) {
                    DeviceInfoEntity deviceEntity = DeviceInfoEntity.fromEntity(device);
                    if (deviceEntity == null) continue;
                    devices.add(deviceEntity);
                }
                TestRunMetadata metadata =
                        new TestRunMetadata(key.getParent().getName(), testRunEntity, devices);
                if (metadata.testRun.failCount > 0) {
                    failingTestObjects.add(metadata.toJson());
                } else {
                    passingTestObjects.add(metadata.toJson());
                }
            }
        } catch (EntityNotFoundException e) {
            // Invalid parameters
        }
        testRunObjects.addAll(failingTestObjects);
        testRunObjects.addAll(passingTestObjects);

        int[] topBuildResultCounts = new int[TestCaseResult.values().length];
        topBuildResultCounts[TestCaseResult.TEST_CASE_RESULT_PASS.getNumber()] = passCount;
        topBuildResultCounts[TestCaseResult.TEST_CASE_RESULT_FAIL.getNumber()] = failCount;

        request.setAttribute("plan", request.getParameter("plan"));
        request.setAttribute("time", request.getParameter("time"));

        request.setAttribute("resultNames", resultNames);
        request.setAttribute("resultNamesJson", new Gson().toJson(resultNames));
        request.setAttribute("testRuns", new Gson().toJson(testRunObjects));
        request.setAttribute("testBuildId", new Gson().toJson(testBuildId));
        request.setAttribute("startTime", new Gson().toJson(startTime));
        request.setAttribute("endTime", new Gson().toJson(endTime));
        request.setAttribute("moduleCount", new Gson().toJson(moduleCount));
        request.setAttribute("passingTestCaseCount", new Gson().toJson(passCount));
        request.setAttribute("failingTestCaseCount", new Gson().toJson(failCount));

        // data for pie chart
        request.setAttribute("topBuildResultCounts", new Gson().toJson(topBuildResultCounts));

        dispatcher = request.getRequestDispatcher(PLAN_RUN_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : " + e.toString());
        }
    }
}
