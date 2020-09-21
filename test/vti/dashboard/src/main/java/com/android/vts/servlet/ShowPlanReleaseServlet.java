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
import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.FilterUtil;
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
import com.google.gson.JsonPrimitive;
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
import org.apache.commons.lang.StringUtils;

public class ShowPlanReleaseServlet extends BaseServlet {
    private static final String PLAN_RELEASE_JSP = "WEB-INF/jsp/show_plan_release.jsp";
    private static final int MAX_RUNS_PER_PAGE = 90;

    @Override
    public PageType getNavParentType() {
        return PageType.RELEASE;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        String planName = request.getParameter("plan");
        links.add(new Page(PageType.PLAN_RELEASE, planName, "?plan=" + planName));
        return links;
    }

    /** Model to describe each test plan run . */
    private class TestPlanRunMetadata implements Comparable<TestPlanRunMetadata> {
        public final TestPlanRunEntity testPlanRun;
        public final List<String> devices;
        public final Set<DeviceInfoEntity> deviceSet;

        public TestPlanRunMetadata(TestPlanRunEntity testPlanRun) {
            this.testPlanRun = testPlanRun;
            this.devices = new ArrayList<>();
            this.deviceSet = new HashSet<>();
        }

        public void addDevice(DeviceInfoEntity device) {
            if (device == null || deviceSet.contains(device)) return;
            devices.add(device.branch + "/" + device.buildFlavor + " (" + device.buildId + ")");
            deviceSet.add(device);
        }

        public JsonObject toJson() {
            JsonObject obj = new JsonObject();
            obj.add("testPlanRun", testPlanRun.toJson());
            obj.add("deviceInfo", new JsonPrimitive(StringUtils.join(devices, ", ")));
            return obj;
        }

        @Override
        public int compareTo(TestPlanRunMetadata o) {
            return new Long(o.testPlanRun.startTimestamp)
                    .compareTo(this.testPlanRun.startTimestamp);
        }
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Long startTime = null; // time in microseconds
        Long endTime = null; // time in microseconds
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
        SortDirection dir = SortDirection.DESCENDING;
        if (startTime != null && endTime == null) {
            dir = SortDirection.ASCENDING;
        }
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
        Filter typeFilter = FilterUtil.getTestTypeFilter(showPresubmit, showPostsubmit, unfiltered);
        String testPlan = request.getParameter("plan");
        Key testPlanKey = KeyFactory.createKey(TestPlanEntity.KIND, testPlan);
        Filter testPlanRunFilter =
                FilterUtil.getTimeFilter(
                        testPlanKey, TestPlanRunEntity.KIND, startTime, endTime, typeFilter);
        Map<String, Object> parameterMap = request.getParameterMap();
        List<Filter> userTestFilters =
                FilterUtil.getUserTestFilters(parameterMap);
        userTestFilters.add(0, testPlanRunFilter);
        Filter userDeviceFilter = FilterUtil.getUserDeviceFilter(parameterMap);

        List<TestPlanRunMetadata> testPlanRuns = new ArrayList<>();
        Map<Key, TestPlanRunMetadata> testPlanMap = new HashMap<>();
        Key minKey = null;
        Key maxKey = null;
        List<Key> gets =
                FilterUtil.getMatchingKeys(
                        testPlanKey,
                        TestPlanRunEntity.KIND,
                        userTestFilters,
                        userDeviceFilter,
                        dir,
                        MAX_RUNS_PER_PAGE);
        Map<Key, Entity> entityMap = datastore.get(gets);
        for (Key key : gets) {
            if (!entityMap.containsKey(key)) {
                continue;
            }
            TestPlanRunEntity testPlanRun = TestPlanRunEntity.fromEntity(entityMap.get(key));
            if (testPlanRun == null) {
                continue;
            }
            TestPlanRunMetadata metadata = new TestPlanRunMetadata(testPlanRun);
            testPlanRuns.add(metadata);
            testPlanMap.put(key, metadata);
            if (minKey == null || key.compareTo(minKey) < 0) {
                minKey = key;
            }
            if (maxKey == null || key.compareTo(maxKey) > 0) {
                maxKey = key;
            }
        }
        if (minKey != null && maxKey != null) {
            Filter deviceFilter =
                    FilterUtil.getDeviceTimeFilter(
                            testPlanKey, TestPlanRunEntity.KIND, minKey.getId(), maxKey.getId());
            Query deviceQuery =
                    new Query(DeviceInfoEntity.KIND)
                            .setAncestor(testPlanKey)
                            .setFilter(deviceFilter)
                            .setKeysOnly();
            List<Key> deviceGets = new ArrayList<>();
            for (Entity device :
                    datastore
                            .prepare(deviceQuery)
                            .asIterable(DatastoreHelper.getLargeBatchOptions())) {
                if (testPlanMap.containsKey(device.getParent())) {
                    deviceGets.add(device.getKey());
                }
            }
            Map<Key, Entity> devices = datastore.get(deviceGets);
            for (Key key : devices.keySet()) {
                if (!testPlanMap.containsKey(key.getParent())) continue;
                DeviceInfoEntity device = DeviceInfoEntity.fromEntity(devices.get(key));
                if (device == null) continue;
                TestPlanRunMetadata metadata = testPlanMap.get(key.getParent());
                metadata.addDevice(device);
            }
        }

        testPlanRuns.sort(Comparator.naturalOrder());

        if (testPlanRuns.size() > 0) {
            TestPlanRunMetadata firstRun = testPlanRuns.get(0);
            endTime = firstRun.testPlanRun.startTimestamp;

            TestPlanRunMetadata lastRun = testPlanRuns.get(testPlanRuns.size() - 1);
            startTime = lastRun.testPlanRun.startTimestamp;
        }

        List<JsonObject> testPlanRunObjects = new ArrayList<>();
        for (TestPlanRunMetadata metadata : testPlanRuns) {
            testPlanRunObjects.add(metadata.toJson());
        }

        FilterUtil.setAttributes(request, parameterMap);

        request.setAttribute("plan", request.getParameter("plan"));
        request.setAttribute(
                "hasNewer",
                new Gson()
                        .toJson(
                                DatastoreHelper.hasNewer(
                                        testPlanKey, TestPlanRunEntity.KIND, endTime)));
        request.setAttribute(
                "hasOlder",
                new Gson()
                        .toJson(
                                DatastoreHelper.hasOlder(
                                        testPlanKey, TestPlanRunEntity.KIND, startTime)));
        request.setAttribute("planRuns", new Gson().toJson(testPlanRunObjects));

        request.setAttribute("unfiltered", unfiltered);
        request.setAttribute("showPresubmit", showPresubmit);
        request.setAttribute("showPostsubmit", showPostsubmit);
        request.setAttribute("startTime", new Gson().toJson(startTime));
        request.setAttribute("endTime", new Gson().toJson(endTime));
        request.setAttribute("branches", new Gson().toJson(DatastoreHelper.getAllBranches()));
        request.setAttribute("devices", new Gson().toJson(DatastoreHelper.getAllBuildFlavors()));
        response.setStatus(HttpServletResponse.SC_OK);
        RequestDispatcher dispatcher = request.getRequestDispatcher(PLAN_RELEASE_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Excpetion caught : ", e);
        }
    }
}
