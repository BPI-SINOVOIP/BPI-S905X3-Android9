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

package com.android.vts.api;

import com.android.vts.entity.TestCaseRunEntity;
import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.util.FilterUtil;
import com.android.vts.util.TestRunDetails;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.EntityNotFoundException;
import com.google.appengine.api.datastore.FetchOptions;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.gson.Gson;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.logging.Logger;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Servlet for handling requests to fetch test case results. */
public class TestRunRestServlet extends HttpServlet {
    private static final String LATEST = "latest";
    protected static final Logger logger = Logger.getLogger(TestRunRestServlet.class.getName());

    /**
     * Get the test case results for the specified run of the specified test.
     *
     * @param test The test whose test cases to get.
     * @param timeString The string representation of the test run timestamp (in microseconds).
     * @return A TestRunDetails object with the test case details for the specified run.
     */
    private static TestRunDetails getTestRunDetails(String test, String timeString) {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        long timestamp;
        try {
            timestamp = Long.parseLong(timeString);
            if (timestamp <= 0) throw new NumberFormatException();
            timestamp = timestamp > 0 ? timestamp : null;
        } catch (NumberFormatException e) {
            return null;
        }

        Key testKey = KeyFactory.createKey(TestEntity.KIND, test);
        Key testRunKey = KeyFactory.createKey(testKey, TestRunEntity.KIND, timestamp);
        TestRunEntity testRunEntity;
        try {
            Entity testRun = datastore.get(testRunKey);
            testRunEntity = TestRunEntity.fromEntity(testRun);
            if (testRunEntity == null) {
                throw new EntityNotFoundException(testRunKey);
            }
        } catch (EntityNotFoundException e) {
            return null;
        }
        TestRunDetails details = new TestRunDetails();
        List<Key> gets = new ArrayList<>();
        for (long testCaseId : testRunEntity.testCaseIds) {
            gets.add(KeyFactory.createKey(TestCaseRunEntity.KIND, testCaseId));
        }
        Map<Key, Entity> entityMap = datastore.get(gets);
        for (Key key : entityMap.keySet()) {
            TestCaseRunEntity testCaseRun = TestCaseRunEntity.fromEntity(entityMap.get(key));
            if (testCaseRun == null) {
                continue;
            }
            details.addTestCase(testCaseRun);
        }
        return details;
    }

    /**
     * Get the test case results for the latest run of the specified test.
     *
     * @param test The test whose test cases to get.
     * @return A TestRunDetails object with the test case details for the latest run.
     */
    private static TestRunDetails getLatestTestRunDetails(String test) {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Key testKey = KeyFactory.createKey(TestEntity.KIND, test);
        Query.Filter typeFilter = FilterUtil.getTestTypeFilter(false, true, false);
        Query testRunQuery =
                new Query(TestRunEntity.KIND)
                        .setAncestor(testKey)
                        .setFilter(typeFilter)
                        .addSort(Entity.KEY_RESERVED_PROPERTY, Query.SortDirection.DESCENDING);
        TestRunEntity testRun = null;
        for (Entity testRunEntity :
                datastore.prepare(testRunQuery).asIterable(FetchOptions.Builder.withLimit(1))) {
            testRun = TestRunEntity.fromEntity(testRunEntity);
        }
        if (testRun == null) return null;
        TestRunDetails details = new TestRunDetails();

        List<Key> gets = new ArrayList<>();
        for (long testCaseId : testRun.testCaseIds) {
            gets.add(KeyFactory.createKey(TestCaseRunEntity.KIND, testCaseId));
        }
        Map<Key, Entity> entityMap = datastore.get(gets);
        for (Key key : entityMap.keySet()) {
            TestCaseRunEntity testCaseRun = TestCaseRunEntity.fromEntity(entityMap.get(key));
            if (testCaseRun == null) {
                continue;
            }
            details.addTestCase(testCaseRun);
        }
        return details;
    }

    /**
     * Get the test case details for a test run.
     *
     * Expected format: (1) /api/test_run?test=[test name]&timestamp=[timestamp] to the details
     * for a specific run, or (2) /api/test_run?test=[test name]&timestamp=latest -- the details for
     * the latest test run.
     */
    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        String test = request.getParameter("test");
        String timeString = request.getParameter("timestamp");
        TestRunDetails details = null;

        if (timeString != null && timeString.equals(LATEST)) {
            details = getLatestTestRunDetails(test);
        } else if (timeString != null) {
            details = getTestRunDetails(test, timeString);
        }

        if (details == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
        } else {
            response.setStatus(HttpServletResponse.SC_OK);
            response.setContentType("application/json");
            PrintWriter writer = response.getWriter();
            writer.print(new Gson().toJson(details.toJson()));
            writer.flush();
        }
    }
}
