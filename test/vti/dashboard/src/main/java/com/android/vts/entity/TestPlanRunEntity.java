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

import com.android.vts.entity.TestRunEntity.TestRunType;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing test plan run information. */
public class TestPlanRunEntity implements DashboardEntity {
    protected static final Logger logger = Logger.getLogger(TestPlanRunEntity.class.getName());

    public static final String KIND = "TestPlanRun";

    // Property keys
    public static final String TEST_PLAN_NAME = "testPlanName";
    public static final String TYPE = "type";
    public static final String START_TIMESTAMP = "startTimestamp";
    public static final String END_TIMESTAMP = "endTimestamp";
    public static final String TEST_BUILD_ID = "testBuildId";
    public static final String PASS_COUNT = "passCount";
    public static final String FAIL_COUNT = "failCount";
    public static final String TEST_RUNS = "testRuns";

    public final Key key;
    public final String testPlanName;
    public final TestRunType type;
    public final long startTimestamp;
    public final long endTimestamp;
    public final String testBuildId;
    public final long passCount;
    public final long failCount;
    public final List<Key> testRuns;

    /**
     * Create a TestPlanRunEntity object describing a test plan run.
     *
     * @param parentKey The key for the parent entity in the database.
     * @param type The test run type (e.g. presubmit, postsubmit, other)
     * @param startTimestamp The time in microseconds when the test plan run started.
     * @param endTimestamp The time in microseconds when the test plan run ended.
     * @param testBuildId The build ID of the VTS test build.
     * @param passCount The number of passing test cases in the run.
     * @param failCount The number of failing test cases in the run.
     * @param testRuns A list of keys to the TestRunEntity objects for the plan run run.
     */
    public TestPlanRunEntity(Key parentKey, String testPlanName, TestRunType type,
            long startTimestamp, long endTimestamp, String testBuildId, long passCount,
            long failCount, List<Key> testRuns) {
        this.key = KeyFactory.createKey(parentKey, KIND, startTimestamp);
        this.testPlanName = testPlanName;
        this.type = type;
        this.startTimestamp = startTimestamp;
        this.endTimestamp = endTimestamp;
        this.testBuildId = testBuildId;
        this.passCount = passCount;
        this.failCount = failCount;
        this.testRuns = testRuns;
    }

    @Override
    public Entity toEntity() {
        Entity planRun = new Entity(this.key);
        planRun.setProperty(TEST_PLAN_NAME, this.testPlanName);
        planRun.setProperty(TYPE, this.type.getNumber());
        planRun.setProperty(START_TIMESTAMP, this.startTimestamp);
        planRun.setProperty(END_TIMESTAMP, this.endTimestamp);
        planRun.setProperty(TEST_BUILD_ID, this.testBuildId.toLowerCase());
        planRun.setProperty(PASS_COUNT, this.passCount);
        planRun.setProperty(FAIL_COUNT, this.failCount);
        if (this.testRuns != null && this.testRuns.size() > 0) {
            planRun.setUnindexedProperty(TEST_RUNS, this.testRuns);
        }
        return planRun;
    }

    /**
     * Convert an Entity object to a TestPlanRunEntity.
     *
     * @param e The entity to process.
     * @return TestPlanRunEntity object with the properties from e processed, or null if
     * incompatible.
     */
    @SuppressWarnings("unchecked")
    public static TestPlanRunEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND) || !e.hasProperty(TEST_PLAN_NAME) || !e.hasProperty(TYPE)
                || !e.hasProperty(START_TIMESTAMP) || !e.hasProperty(END_TIMESTAMP)
                || !e.hasProperty(TEST_BUILD_ID) || !e.hasProperty(PASS_COUNT)
                || !e.hasProperty(FAIL_COUNT) || !e.hasProperty(TEST_RUNS)) {
            logger.log(Level.WARNING, "Missing test run attributes in entity: " + e.toString());
            return null;
        }
        try {
            String testPlanName = (String) e.getProperty(TEST_PLAN_NAME);
            TestRunType type = TestRunType.fromNumber((int) (long) e.getProperty(TYPE));
            long startTimestamp = (long) e.getProperty(START_TIMESTAMP);
            long endTimestamp = (long) e.getProperty(END_TIMESTAMP);
            String testBuildId = (String) e.getProperty(TEST_BUILD_ID);
            long passCount = (long) e.getProperty(PASS_COUNT);
            long failCount = (long) e.getProperty(FAIL_COUNT);
            List<Key> testRuns = (List<Key>) e.getProperty(TEST_RUNS);
            return new TestPlanRunEntity(e.getKey().getParent(), testPlanName, type, startTimestamp,
                    endTimestamp, testBuildId, passCount, failCount, testRuns);
        } catch (ClassCastException exception) {
            // Invalid cast
            logger.log(Level.WARNING, "Error parsing test plan run entity.", exception);
        }
        return null;
    }

    public JsonObject toJson() {
        JsonObject json = new JsonObject();
        json.add(TEST_PLAN_NAME, new JsonPrimitive(this.testPlanName));
        json.add(TEST_BUILD_ID, new JsonPrimitive(this.testBuildId));
        json.add(PASS_COUNT, new JsonPrimitive(this.passCount));
        json.add(FAIL_COUNT, new JsonPrimitive(this.failCount));
        json.add(START_TIMESTAMP, new JsonPrimitive(this.startTimestamp));
        json.add(END_TIMESTAMP, new JsonPrimitive(this.endTimestamp));
        return json;
    }
}
