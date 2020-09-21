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

package com.android.vts.entity;

import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.KeyFactory;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing the execution of a test case. */
public class TestCaseRunEntity implements DashboardEntity {
    protected static final Logger logger = Logger.getLogger(TestCaseRunEntity.class.getName());

    public static final String KIND = "TestCaseRun";

    // Property keys
    public static final String TEST_CASE_NAME = "testCaseName";
    public static final String RESULT = "result";
    public static final String TEST_CASE_NAMES = "testCaseNames";
    public static final String RESULTS = "results";
    public static final String SYSTRACE_URL = "systraceUrl";

    // Maximum number of test cases in the entity.
    private static final int SIZE_LIMIT = 500;

    public final long id;
    public final List<TestCase> testCases;
    private String systraceUrl;

    /**
     * Class describing an individual test case run.
     */
    public static class TestCase {
        public final long parentId;
        public final int offset;
        public final String name;
        public final int result;

        /**
         * Create a test case run.
         * @param parentId The ID of the TestCaseRunEntity containing the test case.
         * @param offset The offset of the TestCase into the TestCaseRunEntity.
         * @param name The name of the test case.
         * @param result The result of the test case.
         */
        public TestCase(long parentId, int offset, String name, int result) {
            this.parentId = parentId;
            this.offset = offset;
            this.name = name;
            this.result = result;
        }
    }

    /**
     * Create a TestCaseRunEntity.
     */
    public TestCaseRunEntity() {
        this.id = -1;
        this.testCases = new ArrayList<>();
        this.systraceUrl = null;
    }

    /**
     * Create a TestCaseRunEntity with the specified id.
     * @param id The entity id.
     */
    public TestCaseRunEntity(long id) {
        this.id = id;
        this.testCases = new ArrayList<>();
        this.systraceUrl = null;
    }

    /**
     * Determine if the TestCaseRunEntity is full.
     * @return True if the entity is full, false otherwise.
     */
    public boolean isFull() {
        return this.testCases.size() >= SIZE_LIMIT;
    }

    /**
     * Set the systrace url.
     * @param url The systrace url, or null.
     */
    private void setSystraceUrl(String url) {
        this.systraceUrl = url;
    }

    /**
     * Get the systrace url.
     * returns The systrace url, or null.
     */
    public String getSystraceUrl() {
        return this.systraceUrl;
    }

    /**
     * Add a test case to the test case run entity.
     * @param name The name of the test case.
     * @param result The result of the test case.
     * @return true if added, false otherwise.
     */
    public boolean addTestCase(String name, int result) {
        if (isFull())
            return false;
        this.testCases.add(new TestCase(this.id, this.testCases.size(), name, result));
        return true;
    }

    @Override
    public Entity toEntity() {
        Entity testCaseRunEntity;
        if (this.id >= 0) {
            testCaseRunEntity = new Entity(KeyFactory.createKey(KIND, id));
        } else {
            testCaseRunEntity = new Entity(KIND);
        }

        if (this.testCases.size() > 0) {
            List<String> testCaseNames = new ArrayList<>();
            List<Integer> results = new ArrayList<>();
            for (TestCase testCase : this.testCases) {
                testCaseNames.add(testCase.name);
                results.add(testCase.result);
            }
            testCaseRunEntity.setUnindexedProperty(TEST_CASE_NAMES, testCaseNames);
            testCaseRunEntity.setUnindexedProperty(RESULTS, results);
        }
        if (systraceUrl != null) {
            testCaseRunEntity.setUnindexedProperty(SYSTRACE_URL, this.systraceUrl);
        }

        return testCaseRunEntity;
    }

    /**
     * Convert an Entity object to a TestCaseRunEntity.
     *
     * @param e The entity to process.
     * @return TestCaseRunEntity object with the properties from e, or null if incompatible.
     */
    @SuppressWarnings("unchecked")
    public static TestCaseRunEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND)) {
            logger.log(Level.WARNING, "Wrong kind: " + e.getKey());
            return null;
        }
        try {
            TestCaseRunEntity testCaseRun = new TestCaseRunEntity(e.getKey().getId());
            if (e.hasProperty(TEST_CASE_NAMES) && e.hasProperty(RESULTS)) {
                List<String> testCaseNames = (List<String>) e.getProperty(TEST_CASE_NAMES);
                List<Long> results = (List<Long>) e.getProperty(RESULTS);
                if (testCaseNames.size() == results.size()) {
                    for (int i = 0; i < testCaseNames.size(); i++) {
                        testCaseRun.addTestCase(testCaseNames.get(i), results.get(i).intValue());
                    }
                }
            }
            if (e.hasProperty(TEST_CASE_NAME) && e.hasProperty(RESULT)) {
                testCaseRun.addTestCase(
                        (String) e.getProperty(TEST_CASE_NAME), (int) (long) e.getProperty(RESULT));
            }
            if (e.hasProperty(SYSTRACE_URL)) {
                String systraceUrl = (String) e.getProperty(SYSTRACE_URL);
                testCaseRun.setSystraceUrl(systraceUrl);
            }
            return testCaseRun;
        } catch (ClassCastException exception) {
            // Invalid cast
            logger.log(Level.WARNING, "Error parsing test case run entity.", exception);
        }
        return null;
    }
}
