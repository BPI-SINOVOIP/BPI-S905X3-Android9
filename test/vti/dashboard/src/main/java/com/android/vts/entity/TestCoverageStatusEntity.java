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
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing test coverage status. */
public class TestCoverageStatusEntity implements DashboardEntity {
    protected static final Logger logger =
            Logger.getLogger(TestCoverageStatusEntity.class.getName());

    public static final String KIND = "TestCoverageStatus";

    // Property keys
    public static final String TOTAL_LINE_COUNT = "totalLineCount";
    public static final String COVERED_LINE_COUNT = "coveredLineCount";
    public static final String UPDATED_TIMESTAMP = "updatedTimestamp";

    public final String testName;
    public final long coveredLineCount;
    public final long totalLineCount;
    public final long timestamp;

    /**
     * Create a TestCoverageStatusEntity object with status metadata.
     *
     * @param testName The name of the test.
     * @param timestamp The timestamp indicating the most recent test run event in the test state.
     * @param coveredLineCount The number of lines covered.
     * @param totalLineCount The total number of lines.
     */
    public TestCoverageStatusEntity(
            String testName, long timestamp, long coveredLineCount, long totalLineCount) {
        this.testName = testName;
        this.timestamp = timestamp;
        this.coveredLineCount = coveredLineCount;
        this.totalLineCount = totalLineCount;
    }

    @Override
    public Entity toEntity() {
        Entity testEntity = new Entity(KIND, this.testName);
        testEntity.setProperty(UPDATED_TIMESTAMP, this.timestamp);
        testEntity.setProperty(COVERED_LINE_COUNT, this.coveredLineCount);
        testEntity.setProperty(TOTAL_LINE_COUNT, this.totalLineCount);
        return testEntity;
    }

    /**
     * Convert an Entity object to a TestCoverageStatusEntity.
     *
     * @param e The entity to process.
     * @return TestCoverageStatusEntity object with the properties from e processed, or null if
     *     incompatible.
     */
    @SuppressWarnings("unchecked")
    public static TestCoverageStatusEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND)
                || e.getKey().getName() == null
                || !e.hasProperty(UPDATED_TIMESTAMP)
                || !e.hasProperty(COVERED_LINE_COUNT)
                || !e.hasProperty(TOTAL_LINE_COUNT)) {
            logger.log(Level.WARNING, "Missing test attributes in entity: " + e.toString());
            return null;
        }
        String testName = e.getKey().getName();
        long timestamp = 0;
        long coveredLineCount = -1;
        long totalLineCount = -1;
        try {
            timestamp = (long) e.getProperty(UPDATED_TIMESTAMP);
            coveredLineCount = (Long) e.getProperty(COVERED_LINE_COUNT);
            totalLineCount = (Long) e.getProperty(TOTAL_LINE_COUNT);
        } catch (ClassCastException exception) {
            // Invalid contents or null values
            logger.log(Level.WARNING, "Error parsing test entity.", exception);
            return null;
        }
        return new TestCoverageStatusEntity(testName, timestamp, coveredLineCount, totalLineCount);
    }
}
