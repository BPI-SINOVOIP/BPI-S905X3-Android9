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

import com.android.vts.entity.TestCaseRunEntity.TestCase;
import com.google.appengine.api.datastore.Entity;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing test status. */
public class TestStatusEntity implements DashboardEntity {
    protected static final Logger logger = Logger.getLogger(TestStatusEntity.class.getName());

    public static final String KIND = "TestStatus";

    // Property keys
    public static final String PASS_COUNT = "passCount";
    public static final String FAIL_COUNT = "failCount";
    public static final String UPDATED_TIMESTAMP = "updatedTimestamp";

    protected static final String FAILING_IDS = "failingTestcaseIds";
    protected static final String FAILING_OFFSETS = "failingTestcaseOffsets";

    public final String testName;
    public final int passCount;
    public final int failCount;
    public final long timestamp;
    public final List<TestCaseReference> failingTestCases;

    /** Object representing a reference to a test case. */
    public static class TestCaseReference {
        public final long parentId;
        public final int offset;

        /**
         * Create a test case reference.
         *
         * @param parentId The ID of the TestCaseRunEntity containing the test case.
         * @param offset The offset of the test case into the TestCaseRunEntity.
         */
        public TestCaseReference(long parentId, int offset) {
            this.parentId = parentId;
            this.offset = offset;
        }

        /**
         * Create a test case reference.
         *
         * @param testCase The TestCase to reference.
         */
        public TestCaseReference(TestCase testCase) {
            this(testCase.parentId, testCase.offset);
        }
    }

    /**
     * Create a TestEntity object with status metadata.
     *
     * @param testName The name of the test.
     * @param timestamp The timestamp indicating the most recent test run event in the test state.
     * @param passCount The number of tests passing up to the timestamp specified.
     * @param failCount The number of tests failing up to the timestamp specified.
     * @param failingTestCases The TestCaseReferences of the last observed failing test cases.
     */
    public TestStatusEntity(
            String testName,
            long timestamp,
            int passCount,
            int failCount,
            List<TestCaseReference> failingTestCases) {
        this.testName = testName;
        this.timestamp = timestamp;
        this.passCount = passCount;
        this.failCount = failCount;
        this.failingTestCases = failingTestCases;
    }

    /**
     * Create a TestEntity object without metadata.
     *
     * @param testName The name of the test.
     */
    public TestStatusEntity(String testName) {
        this(testName, 0, -1, -1, new ArrayList<TestCaseReference>());
    }

    @Override
    public Entity toEntity() {
        Entity testEntity = new Entity(KIND, this.testName);
        if (this.timestamp >= 0 && this.passCount >= 0 && this.failCount >= 0) {
            testEntity.setProperty(UPDATED_TIMESTAMP, this.timestamp);
            testEntity.setProperty(PASS_COUNT, this.passCount);
            testEntity.setProperty(FAIL_COUNT, this.failCount);
            if (this.failingTestCases.size() > 0) {
                List<Long> failingTestcaseIds = new ArrayList<>();
                List<Integer> failingTestcaseOffsets = new ArrayList<>();
                for (TestCaseReference testCase : this.failingTestCases) {
                    failingTestcaseIds.add(testCase.parentId);
                    failingTestcaseOffsets.add(testCase.offset);
                }
                testEntity.setUnindexedProperty(FAILING_IDS, failingTestcaseIds);
                testEntity.setUnindexedProperty(FAILING_OFFSETS, failingTestcaseOffsets);
            }
        }
        return testEntity;
    }

    /**
     * Convert an Entity object to a TestEntity.
     *
     * @param e The entity to process.
     * @return TestEntity object with the properties from e processed, or null if incompatible.
     */
    @SuppressWarnings("unchecked")
    public static TestStatusEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND) || e.getKey().getName() == null) {
            logger.log(Level.WARNING, "Missing test attributes in entity: " + e.toString());
            return null;
        }
        String testName = e.getKey().getName();
        long timestamp = 0;
        int passCount = -1;
        int failCount = -1;
        List<TestCaseReference> failingTestCases = new ArrayList<>();
        try {
            if (e.hasProperty(UPDATED_TIMESTAMP)) {
                timestamp = (long) e.getProperty(UPDATED_TIMESTAMP);
            }
            if (e.hasProperty(PASS_COUNT)) {
                passCount = ((Long) e.getProperty(PASS_COUNT)).intValue();
            }
            if (e.hasProperty(FAIL_COUNT)) {
                failCount = ((Long) e.getProperty(FAIL_COUNT)).intValue();
            }
            if (e.hasProperty(FAILING_IDS) && e.hasProperty(FAILING_OFFSETS)) {
                List<Long> ids = (List<Long>) e.getProperty(FAILING_IDS);
                List<Long> offsets = (List<Long>) e.getProperty(FAILING_OFFSETS);
                if (ids.size() == offsets.size()) {
                    for (int i = 0; i < ids.size(); i++) {
                        failingTestCases.add(
                                new TestCaseReference(ids.get(i), offsets.get(i).intValue()));
                    }
                }
            }
        } catch (ClassCastException exception) {
            // Invalid contents or null values
            logger.log(Level.WARNING, "Error parsing test entity.", exception);
        }
        return new TestStatusEntity(testName, timestamp, passCount, failCount, failingTestCases);
    }
}
