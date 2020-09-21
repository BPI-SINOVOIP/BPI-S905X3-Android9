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

package com.android.vts.util;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.ProfilingPointRunEntity;
import com.android.vts.entity.TestCaseRunEntity;
import com.android.vts.entity.TestCaseRunEntity.TestCase;
import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.util.UrlUtil.LinkDisplay;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.apache.commons.lang.StringUtils;

/** Helper object for describing test results data. */
public class TestResults {
    private final Logger logger = Logger.getLogger(getClass().getName());

    private List<TestRunEntity> testRuns; // list of all test runs
    private Map<Key, List<TestCaseRunEntity>>
            testCaseRunMap; // map from test run key to the test run information
    private Map<Key, List<DeviceInfoEntity>> deviceInfoMap; // map from test run key to device info
    private Map<String, Integer> testCaseNameMap; // map from test case name to its order
    private Set<String> profilingPointNameSet; // set of profiling point names

    public String testName;
    public String[] headerRow; // row to display above the test results table
    public String[][] timeGrid; // grid of data storing timestamps to render as dates
    public String[][] durationGrid; // grid of data storing timestamps to render as time intervals
    public String[][] summaryGrid; // grid of data displaying a summary of the test run
    public String[][] resultsGrid; // grid of data displaying test case results
    public String[] profilingPointNames; // list of profiling point names in the test run
    public Map<String, List<String[]>> logInfoMap; // map from test run index to url/display pairs
    public int[] totResultCounts; // array of test result counts for the tip-of-tree runs
    public String totBuildId = ""; // build ID of tip-of-tree run
    public long startTime = Long.MAX_VALUE; // oldest timestamp displayed in the results table
    public long endTime = Long.MIN_VALUE; // newest timestamp displayed in the results table

    // Row labels for the test time-formatted information.
    private static final String[] TIME_INFO_NAMES = {"Test Start", "Test End"};

    // Row labels for the test duration information.
    private static final String[] DURATION_INFO_NAMES = {"<b>Test Duration</b>"};

    // Row labels for the test summary grid.
    private static final String[] SUMMARY_NAMES = {
        "Total", "Passing #", "Non-Passing #", "Passing %", "Covered Lines", "Coverage %", "Links"
    };

    // Row labels for the device summary information in the table header.
    private static final String[] HEADER_NAMES = {
        "<b>Stats Type \\ Device Build ID</b>",
        "Branch",
        "Build Target",
        "Device",
        "ABI Target",
        "VTS Build ID",
        "Hostname"
    };

    /**
     * Create a test results object.
     *
     * @param testName The name of the test.
     */
    public TestResults(String testName) {
        this.testName = testName;
        this.testRuns = new ArrayList<>();
        this.deviceInfoMap = new HashMap<>();
        this.testCaseRunMap = new HashMap<>();
        this.testCaseNameMap = new HashMap<>();
        this.logInfoMap = new HashMap<>();
        this.profilingPointNameSet = new HashSet<>();
    }

    /**
     * Add a test run to the test results.
     *
     * @param testRun The Entity containing the test run information.
     * @param testCaseRuns The collection of test case executions within the test run.
     */
    public void addTestRun(Entity testRun, Iterable<Entity> testCaseRuns) {
        TestRunEntity testRunEntity = TestRunEntity.fromEntity(testRun);
        if (testRunEntity == null) return;
        if (testRunEntity.startTimestamp < startTime) {
            startTime = testRunEntity.startTimestamp;
        }
        if (testRunEntity.endTimestamp > endTime) {
            endTime = testRunEntity.endTimestamp;
        }
        testRuns.add(testRunEntity);
        testCaseRunMap.put(testRun.getKey(), new ArrayList<TestCaseRunEntity>());

        // Process the test cases in the test run
        for (Entity e : testCaseRuns) {
            TestCaseRunEntity testCaseRunEntity = TestCaseRunEntity.fromEntity(e);
            if (testCaseRunEntity == null) continue;
            testCaseRunMap.get(testRun.getKey()).add(testCaseRunEntity);
            for (TestCase testCase : testCaseRunEntity.testCases) {
                if (!testCaseNameMap.containsKey(testCase.name)) {
                    testCaseNameMap.put(testCase.name, testCaseNameMap.size());
                }
            }
        }
    }

    /** Creates a test case breakdown of the most recent test run. */
    private void generateToTBreakdown() {
        totResultCounts = new int[TestCaseResult.values().length];
        if (testRuns.size() == 0) return;

        TestRunEntity mostRecentRun = testRuns.get(0);
        List<TestCaseRunEntity> testCaseResults = testCaseRunMap.get(mostRecentRun.key);
        List<DeviceInfoEntity> deviceInfos = deviceInfoMap.get(mostRecentRun.key);
        if (deviceInfos.size() > 0) {
            DeviceInfoEntity totDevice = deviceInfos.get(0);
            totBuildId = totDevice.buildId;
        }
        // Count array for each test result
        for (TestCaseRunEntity testCaseRunEntity : testCaseResults) {
            for (TestCase testCase : testCaseRunEntity.testCases) {
                totResultCounts[testCase.result]++;
            }
        }
    }

    /**
     * Get the number of test runs observed.
     *
     * @return The number of test runs observed.
     */
    public int getSize() {
        return testRuns.size();
    }

    /** Fetch and process profiling point names for the set of test runs. */
    private void processProfilingPoints() {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Key testKey = KeyFactory.createKey(TestEntity.KIND, this.testName);
        Query.Filter profilingFilter =
                FilterUtil.getProfilingTimeFilter(
                        testKey, TestRunEntity.KIND, this.startTime, this.endTime);
        Query profilingPointQuery =
                new Query(ProfilingPointRunEntity.KIND)
                        .setAncestor(testKey)
                        .setFilter(profilingFilter)
                        .setKeysOnly();
        Iterable<Entity> profilingPoints = datastore.prepare(profilingPointQuery).asIterable();
        // Process the profiling point observations in the test run
        for (Entity e : profilingPoints) {
            if (e.getKey().getName() != null) {
                profilingPointNameSet.add(e.getKey().getName());
            }
        }
    }

    /** Fetch and process device information for the set of test runs. */
    private void processDeviceInfos() {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Key testKey = KeyFactory.createKey(TestEntity.KIND, this.testName);
        Query.Filter deviceFilter =
                FilterUtil.getDeviceTimeFilter(
                        testKey, TestRunEntity.KIND, this.startTime, this.endTime);
        Query deviceQuery =
                new Query(DeviceInfoEntity.KIND)
                        .setAncestor(testKey)
                        .setFilter(deviceFilter)
                        .setKeysOnly();
        List<Key> deviceGets = new ArrayList<>();
        for (Entity device :
                datastore.prepare(deviceQuery).asIterable(DatastoreHelper.getLargeBatchOptions())) {
            if (testCaseRunMap.containsKey(device.getParent())) {
                deviceGets.add(device.getKey());
            }
        }
        Map<Key, Entity> devices = datastore.get(deviceGets);
        for (Key key : devices.keySet()) {
            Entity device = devices.get(key);
            if (!testCaseRunMap.containsKey(device.getParent())) return;
            DeviceInfoEntity deviceEntity = DeviceInfoEntity.fromEntity(device);
            if (deviceEntity == null) return;
            if (!deviceInfoMap.containsKey(device.getParent())) {
                deviceInfoMap.put(device.getParent(), new ArrayList<DeviceInfoEntity>());
            }
            deviceInfoMap.get(device.getParent()).add(deviceEntity);
        }
    }

    /** Post-process the test runs to generate reports of the results. */
    public void processReport() {
        if (getSize() > 0) {
            processDeviceInfos();
            processProfilingPoints();
        }
        testRuns.sort((t1, t2) -> new Long(t2.startTimestamp).compareTo(t1.startTimestamp));
        generateToTBreakdown();

        headerRow = new String[testRuns.size() + 1];
        headerRow[0] = StringUtils.join(HEADER_NAMES, "<br>");

        summaryGrid = new String[SUMMARY_NAMES.length][testRuns.size() + 1];
        for (int i = 0; i < SUMMARY_NAMES.length; i++) {
            summaryGrid[i][0] = "<b>" + SUMMARY_NAMES[i] + "</b>";
        }

        timeGrid = new String[TIME_INFO_NAMES.length][testRuns.size() + 1];
        for (int i = 0; i < TIME_INFO_NAMES.length; i++) {
            timeGrid[i][0] = "<b>" + TIME_INFO_NAMES[i] + "</b>";
        }

        durationGrid = new String[DURATION_INFO_NAMES.length][testRuns.size() + 1];
        for (int i = 0; i < DURATION_INFO_NAMES.length; i++) {
            durationGrid[i][0] = "<b>" + DURATION_INFO_NAMES[i] + "</b>";
        }

        resultsGrid = new String[testCaseNameMap.size()][testRuns.size() + 1];
        // first column for results grid
        for (String testCaseName : testCaseNameMap.keySet()) {
            resultsGrid[testCaseNameMap.get(testCaseName)][0] = testCaseName;
        }

        // Iterate through the test runs
        for (int col = 0; col < testRuns.size(); col++) {
            TestRunEntity testRun = testRuns.get(col);

            // Process the device information
            List<DeviceInfoEntity> devices = deviceInfoMap.get(testRun.key);
            List<String> buildIdList = new ArrayList<>();
            List<String> buildAliasList = new ArrayList<>();
            List<String> buildFlavorList = new ArrayList<>();
            List<String> productVariantList = new ArrayList<>();
            List<String> abiInfoList = new ArrayList<>();
            for (DeviceInfoEntity deviceInfoEntity : devices) {
                buildAliasList.add(deviceInfoEntity.branch);
                buildFlavorList.add(deviceInfoEntity.buildFlavor);
                productVariantList.add(deviceInfoEntity.product);
                buildIdList.add(deviceInfoEntity.buildId);
                String abi = "";
                String abiName = deviceInfoEntity.abiName;
                String abiBitness = deviceInfoEntity.abiBitness;
                if (abiName.length() > 0) {
                    abi += abiName;
                    if (abiBitness.length() > 0) {
                        abi += " (" + abiBitness + " bit)";
                    }
                }
                abiInfoList.add(abi);
            }

            String buildAlias = StringUtils.join(buildAliasList, ",");
            String buildFlavor = StringUtils.join(buildFlavorList, ",");
            String productVariant = StringUtils.join(productVariantList, ",");
            String buildIds = StringUtils.join(buildIdList, ",");
            String abiInfo = StringUtils.join(abiInfoList, ",");
            String vtsBuildId = testRun.testBuildId;

            int totalCount = 0;
            int passCount = (int) testRun.passCount;
            int nonpassCount = (int) testRun.failCount;
            TestCaseResult aggregateStatus = TestCaseResult.UNKNOWN_RESULT;
            long totalLineCount = testRun.totalLineCount;
            long coveredLineCount = testRun.coveredLineCount;

            // Process test case results
            for (TestCaseRunEntity testCaseEntity : testCaseRunMap.get(testRun.key)) {
                // Update the aggregated test run status
                totalCount += testCaseEntity.testCases.size();
                for (TestCase testCase : testCaseEntity.testCases) {
                    int result = testCase.result;
                    String name = testCase.name;
                    if (result == TestCaseResult.TEST_CASE_RESULT_PASS.getNumber()) {
                        if (aggregateStatus == TestCaseResult.UNKNOWN_RESULT) {
                            aggregateStatus = TestCaseResult.TEST_CASE_RESULT_PASS;
                        }
                    } else if (result != TestCaseResult.TEST_CASE_RESULT_SKIP.getNumber()) {
                        aggregateStatus = TestCaseResult.TEST_CASE_RESULT_FAIL;
                    }

                    String systraceUrl = null;

                    if (testCaseEntity.getSystraceUrl() != null) {
                        String url = testCaseEntity.getSystraceUrl();
                        LinkDisplay validatedLink = UrlUtil.processUrl(url);
                        if (validatedLink != null) {
                            systraceUrl = validatedLink.url;
                        } else {
                            logger.log(Level.WARNING, "Invalid systrace URL : " + url);
                        }
                    }

                    int index = testCaseNameMap.get(name);
                    String classNames = "test-case-status ";
                    String glyph = "";
                    TestCaseResult testCaseResult = TestCaseResult.valueOf(result);
                    if (testCaseResult != null) classNames += testCaseResult.toString();
                    else classNames += TestCaseResult.UNKNOWN_RESULT.toString();

                    if (systraceUrl != null) {
                        classNames += " width-1";
                        glyph +=
                                "<a href=\""
                                        + systraceUrl
                                        + "\" "
                                        + "class=\"waves-effect waves-light btn red right inline-btn\">"
                                        + "<i class=\"material-icons inline-icon\">info_outline</i></a>";
                    }
                    resultsGrid[index][col + 1] =
                            "<div class=\"" + classNames + "\">&nbsp;</div>" + glyph;
                }
            }
            String passInfo;
            try {
                double passPct =
                        Math.round((100 * passCount / (passCount + nonpassCount)) * 100f) / 100f;
                passInfo = Double.toString(passPct) + "%";
            } catch (ArithmeticException e) {
                passInfo = " - ";
            }

            // Process coverage metadata
            String coverageInfo;
            String coveragePctInfo;
            try {
                double coveragePct =
                        Math.round((100 * coveredLineCount / totalLineCount) * 100f) / 100f;
                coveragePctInfo =
                        Double.toString(coveragePct)
                                + "%"
                                + "<a href=\"/show_coverage?testName="
                                + testName
                                + "&startTime="
                                + testRun.startTimestamp
                                + "\" class=\"waves-effect waves-light btn red right inline-btn\">"
                                + "<i class=\"material-icons inline-icon\">menu</i></a>";
                coverageInfo = coveredLineCount + "/" + totalLineCount;
            } catch (ArithmeticException e) {
                coveragePctInfo = " - ";
                coverageInfo = " - ";
            }

            // Process log information
            String linkSummary = " - ";
            List<String[]> linkEntries = new ArrayList<>();
            logInfoMap.put(Integer.toString(col), linkEntries);

            if (testRun.links != null) {
                for (String rawUrl : testRun.links) {
                    LinkDisplay validatedLink = UrlUtil.processUrl(rawUrl);
                    if (validatedLink == null) {
                        logger.log(Level.WARNING, "Invalid logging URL : " + rawUrl);
                        continue;
                    }
                    String[] logInfo =
                            new String[] {
                                validatedLink.name,
                                validatedLink.url // TODO: process the name from the URL
                            };
                    linkEntries.add(logInfo);
                }
            }
            if (linkEntries.size() > 0) {
                linkSummary = Integer.toString(linkEntries.size());
                linkSummary +=
                        "<i class=\"waves-effect waves-light btn red right inline-btn"
                                + " info-btn material-icons inline-icon\""
                                + " data-col=\""
                                + Integer.toString(col)
                                + "\""
                                + ">launch</i>";
            }

            String icon = "<div class='status-icon " + aggregateStatus.toString() + "'>&nbsp</div>";
            String hostname = testRun.hostName;

            // Populate the header row
            headerRow[col + 1] =
                    "<span class='valign-wrapper'><b>"
                            + buildIds
                            + "</b>"
                            + icon
                            + "</span>"
                            + buildAlias
                            + "<br>"
                            + buildFlavor
                            + "<br>"
                            + productVariant
                            + "<br>"
                            + abiInfo
                            + "<br>"
                            + vtsBuildId
                            + "<br>"
                            + hostname;

            // Populate the test summary grid
            summaryGrid[0][col + 1] = Integer.toString(totalCount);
            summaryGrid[1][col + 1] = Integer.toString(passCount);
            summaryGrid[2][col + 1] = Integer.toString(nonpassCount);
            summaryGrid[3][col + 1] = passInfo;
            summaryGrid[4][col + 1] = coverageInfo;
            summaryGrid[5][col + 1] = coveragePctInfo;
            summaryGrid[6][col + 1] = linkSummary;

            // Populate the test time info grid
            timeGrid[0][col + 1] = Long.toString(testRun.startTimestamp);
            timeGrid[1][col + 1] = Long.toString(testRun.endTimestamp);

            // Populate the test duration info grid
            durationGrid[0][col + 1] = Long.toString(testRun.endTimestamp - testRun.startTimestamp);
        }

        profilingPointNames =
                profilingPointNameSet.toArray(new String[profilingPointNameSet.size()]);
        Arrays.sort(profilingPointNames);
    }
}
