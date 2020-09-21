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

import com.android.vts.util.UrlUtil;
import com.android.vts.util.UrlUtil.LinkDisplay;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing test run information. */
public class TestRunEntity implements DashboardEntity {
    protected static final Logger logger = Logger.getLogger(TestRunEntity.class.getName());

    /** Enum for classifying test run types. */
    public enum TestRunType {
        OTHER(0),
        PRESUBMIT(1),
        POSTSUBMIT(2);

        private final int value;

        private TestRunType(int value) {
            this.value = value;
        }

        /**
         * Get the ordinal representation of the type.
         *
         * @return The value associated with the test run type.
         */
        public int getNumber() {
            return value;
        }

        /**
         * Convert an ordinal value to a TestRunType.
         *
         * @param value The orginal value to parse.
         * @return a TestRunType value.
         */
        public static TestRunType fromNumber(int value) {
            if (value == 1) {
                return TestRunType.PRESUBMIT;
            } else if (value == 2) {
                return TestRunType.POSTSUBMIT;
            } else {
                return TestRunType.OTHER;
            }
        }

        /**
         * Determine the test run type based on the build ID.
         *
         * Postsubmit runs are expected to have integer build IDs, while presubmit runs are integers
         * prefixed by the character P. All other runs (e.g. local builds) are classified as OTHER.
         *
         * @param buildId The build ID.
         * @return the TestRunType.
         */
        public static TestRunType fromBuildId(String buildId) {
            try {
                Integer.parseInt(buildId);
                return TestRunType.POSTSUBMIT;
            } catch (NumberFormatException e) {
                // Not an integer
            }
            if (Character.toLowerCase(buildId.charAt(0)) == 'p') {
                try {
                    Integer.parseInt(buildId.substring(1));
                    return TestRunType.PRESUBMIT;
                } catch (NumberFormatException e) {
                    // Not an integer
                }
            }
            return TestRunType.OTHER;
        }
    }

    public static final String KIND = "TestRun";

    // Property keys
    public static final String TEST_NAME = "testName";
    public static final String TYPE = "type";
    public static final String START_TIMESTAMP = "startTimestamp";
    public static final String END_TIMESTAMP = "endTimestamp";
    public static final String TEST_BUILD_ID = "testBuildId";
    public static final String HOST_NAME = "hostName";
    public static final String PASS_COUNT = "passCount";
    public static final String FAIL_COUNT = "failCount";
    public static final String TEST_CASE_IDS = "testCaseIds";
    public static final String LOG_LINKS = "logLinks";
    public static final String HAS_COVERAGE = "hasCoverage";
    public static final String TOTAL_LINE_COUNT = "totalLineCount";
    public static final String COVERED_LINE_COUNT = "coveredLineCount";

    public final Key key;
    public final TestRunType type;
    public final long startTimestamp;
    public final long endTimestamp;
    public final String testBuildId;
    public final String hostName;
    public final long passCount;
    public final long failCount;
    public final boolean hasCoverage;
    public final long coveredLineCount;
    public final long totalLineCount;
    public final List<Long> testCaseIds;
    public final List<String> links;

    /**
     * Create a TestRunEntity object describing a test run.
     *
     * @param parentKey The key to the parent TestEntity.
     * @param type The test run type (e.g. presubmit, postsubmit, other)
     * @param startTimestamp The time in microseconds when the test run started.
     * @param endTimestamp The time in microseconds when the test run ended.
     * @param testBuildId The build ID of the VTS test build.
     * @param hostName The name of host machine.
     * @param passCount The number of passing test cases in the run.
     * @param failCount The number of failing test cases in the run.
     * @param testCaseIds A list of key IDs to the TestCaseRunEntity objects for the test run.
     * @param links A list of links to resource files for the test run, or null if there aren't any.
     * @param coveredLineCount The number of lines covered by the test run.
     * @param totalLineCount The total number of executable lines by the test in the test run.
     */
    public TestRunEntity(Key parentKey, TestRunType type, long startTimestamp, long endTimestamp,
            String testBuildId, String hostName, long passCount, long failCount,
            List<Long> testCaseIds, List<String> links, long coveredLineCount,
            long totalLineCount) {
        this.key = KeyFactory.createKey(parentKey, KIND, startTimestamp);
        this.type = type;
        this.startTimestamp = startTimestamp;
        this.endTimestamp = endTimestamp;
        this.testBuildId = testBuildId;
        this.hostName = hostName;
        this.passCount = passCount;
        this.failCount = failCount;
        this.testCaseIds = testCaseIds == null ? new ArrayList<Long>() : testCaseIds;
        this.links = links == null ? new ArrayList<String>() : links;
        this.coveredLineCount = coveredLineCount;
        this.totalLineCount = totalLineCount;
        this.hasCoverage = totalLineCount > 0;
    }

    /**
     * Create a TestRunEntity object describing a test run.
     *
     * @param parentKey The key to the parent TestEntity.
     * @param type The test run type (e.g. presubmit, postsubmit, other)
     * @param startTimestamp The time in microseconds when the test run started.
     * @param endTimestamp The time in microseconds when the test run ended.
     * @param testBuildId The build ID of the VTS test build.
     * @param hostName The name of host machine.
     * @param passCount The number of passing test cases in the run.
     * @param failCount The number of failing test cases in the run.
     * @param testCaseIds A list of key IDs to the TestCaseRunEntity objects for the test run.
     * @param links A list of links to resource files for the test run, or null if there aren't any.
     */
    public TestRunEntity(Key parentKey, TestRunType type, long startTimestamp, long endTimestamp,
            String testBuildId, String hostName, long passCount, long failCount,
            List<Long> testCaseIds, List<String> links) {
        this(parentKey, type, startTimestamp, endTimestamp, testBuildId, hostName, passCount,
                failCount, testCaseIds, links, 0, 0);
    }

    @Override
    public Entity toEntity() {
        Entity testRunEntity = new Entity(this.key);
        testRunEntity.setProperty(TEST_NAME, this.key.getParent().getName());
        testRunEntity.setProperty(TYPE, this.type.getNumber());
        testRunEntity.setProperty(START_TIMESTAMP, this.startTimestamp);
        testRunEntity.setUnindexedProperty(END_TIMESTAMP, this.endTimestamp);
        testRunEntity.setProperty(TEST_BUILD_ID, this.testBuildId.toLowerCase());
        testRunEntity.setProperty(HOST_NAME, this.hostName.toLowerCase());
        testRunEntity.setProperty(PASS_COUNT, this.passCount);
        testRunEntity.setProperty(FAIL_COUNT, this.failCount);
        testRunEntity.setUnindexedProperty(TEST_CASE_IDS, this.testCaseIds);
        boolean hasCoverage = this.totalLineCount > 0 && this.coveredLineCount >= 0;
        testRunEntity.setProperty(HAS_COVERAGE, hasCoverage);
        if (hasCoverage) {
            testRunEntity.setProperty(COVERED_LINE_COUNT, this.coveredLineCount);
            testRunEntity.setProperty(TOTAL_LINE_COUNT, this.totalLineCount);
        }
        if (this.links != null && this.links.size() > 0) {
            testRunEntity.setUnindexedProperty(LOG_LINKS, this.links);
        }
        return testRunEntity;
    }

    /**
     * Convert an Entity object to a TestRunEntity.
     *
     * @param e The entity to process.
     * @return TestRunEntity object with the properties from e processed, or null if incompatible.
     */
    @SuppressWarnings("unchecked")
    public static TestRunEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND) || !e.hasProperty(TYPE) || !e.hasProperty(START_TIMESTAMP)
                || !e.hasProperty(END_TIMESTAMP) || !e.hasProperty(TEST_BUILD_ID)
                || !e.hasProperty(HOST_NAME) || !e.hasProperty(PASS_COUNT)
                || !e.hasProperty(FAIL_COUNT) || !e.hasProperty(TEST_CASE_IDS)) {
            logger.log(Level.WARNING, "Missing test run attributes in entity: " + e.toString());
            return null;
        }
        try {
            TestRunType type = TestRunType.fromNumber((int) (long) e.getProperty(TYPE));
            long startTimestamp = (long) e.getProperty(START_TIMESTAMP);
            long endTimestamp = (long) e.getProperty(END_TIMESTAMP);
            String testBuildId = (String) e.getProperty(TEST_BUILD_ID);
            String hostName = (String) e.getProperty(HOST_NAME);
            long passCount = (long) e.getProperty(PASS_COUNT);
            long failCount = (long) e.getProperty(FAIL_COUNT);
            List<Long> testCaseIds = (List<Long>) e.getProperty(TEST_CASE_IDS);
            List<String> links = null;
            long coveredLineCount = 0;
            long totalLineCount = 0;
            if (e.hasProperty(TOTAL_LINE_COUNT) && e.hasProperty(COVERED_LINE_COUNT)) {
                coveredLineCount = (long) e.getProperty(COVERED_LINE_COUNT);
                totalLineCount = (long) e.getProperty(TOTAL_LINE_COUNT);
            }
            if (e.hasProperty(LOG_LINKS)) {
                links = (List<String>) e.getProperty(LOG_LINKS);
            }
            return new TestRunEntity(e.getKey().getParent(), type, startTimestamp, endTimestamp,
                    testBuildId, hostName, passCount, failCount, testCaseIds, links,
                    coveredLineCount, totalLineCount);
        } catch (ClassCastException exception) {
            // Invalid cast
            logger.log(Level.WARNING, "Error parsing test run entity.", exception);
        }
        return null;
    }

    public JsonObject toJson() {
        JsonObject json = new JsonObject();
        json.add(TEST_NAME, new JsonPrimitive(this.key.getParent().getName()));
        json.add(TEST_BUILD_ID, new JsonPrimitive(this.testBuildId));
        json.add(HOST_NAME, new JsonPrimitive(this.hostName));
        json.add(PASS_COUNT, new JsonPrimitive(this.passCount));
        json.add(FAIL_COUNT, new JsonPrimitive(this.failCount));
        json.add(START_TIMESTAMP, new JsonPrimitive(this.startTimestamp));
        json.add(END_TIMESTAMP, new JsonPrimitive(this.endTimestamp));
        if (this.totalLineCount > 0 && this.coveredLineCount >= 0) {
            json.add(COVERED_LINE_COUNT, new JsonPrimitive(this.coveredLineCount));
            json.add(TOTAL_LINE_COUNT, new JsonPrimitive(this.totalLineCount));
        }
        if (this.links != null && this.links.size() > 0) {
            List<JsonElement> links = new ArrayList<>();
            for (String rawUrl : this.links) {
                LinkDisplay validatedLink = UrlUtil.processUrl(rawUrl);
                if (validatedLink == null) {
                    logger.log(Level.WARNING, "Invalid logging URL : " + rawUrl);
                    continue;
                }
                String[] logInfo = new String[] {validatedLink.name, validatedLink.url};
                links.add(new Gson().toJsonTree(logInfo));
            }
            if (links.size() > 0) {
                json.add(LOG_LINKS, new Gson().toJsonTree(links));
            }
        }
        return json;
    }
}
