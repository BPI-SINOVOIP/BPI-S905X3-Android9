/**
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.vts.util;

import com.android.vts.entity.BranchEntity;
import com.android.vts.entity.BuildTargetEntity;
import com.android.vts.entity.CoverageEntity;
import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.ProfilingPointRunEntity;
import com.android.vts.entity.TestCaseRunEntity;
import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestPlanEntity;
import com.android.vts.entity.TestPlanRunEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.entity.TestRunEntity.TestRunType;
import com.android.vts.job.VtsAlertJobServlet;
import com.android.vts.job.VtsCoverageAlertJobServlet;
import com.android.vts.job.VtsProfilingStatsJobServlet;
import com.android.vts.proto.VtsReportMessage.AndroidDeviceInfoMessage;
import com.android.vts.proto.VtsReportMessage.CoverageReportMessage;
import com.android.vts.proto.VtsReportMessage.LogMessage;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.proto.VtsReportMessage.TestPlanReportMessage;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.proto.VtsReportMessage.UrlResourceMessage;
import com.google.appengine.api.datastore.DatastoreFailureException;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.DatastoreTimeoutException;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.EntityNotFoundException;
import com.google.appengine.api.datastore.FetchOptions;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.appengine.api.datastore.Query.FilterOperator;
import com.google.appengine.api.datastore.Query.FilterPredicate;
import com.google.appengine.api.datastore.Transaction;
import com.google.appengine.api.datastore.TransactionOptions;
import com.google.common.collect.Lists;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.ConcurrentModificationException;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

/** DatastoreHelper, a helper class for interacting with Cloud Datastore. */
public class DatastoreHelper {
    /** The default kind name for datastore */
    public static final String NULL_ENTITY_KIND = "nullEntity";

    public static final int MAX_WRITE_RETRIES = 5;
    /**
     * This variable is for maximum number of entities per transaction You can find the detail here
     * (https://cloud.google.com/datastore/docs/concepts/limits)
     */
    public static final int MAX_ENTITY_SIZE_PER_TRANSACTION = 300;

    protected static final Logger logger = Logger.getLogger(DatastoreHelper.class.getName());
    private static final DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

    /**
     * Get query fetch options for large batches of entities.
     *
     * @return FetchOptions with a large chunk and prefetch size.
     */
    public static FetchOptions getLargeBatchOptions() {
        return FetchOptions.Builder.withChunkSize(1000).prefetchSize(1000);
    }

    /**
     * Returns true if there are data points newer than lowerBound in the results table.
     *
     * @param parentKey The parent key to use in the query.
     * @param kind The query entity kind.
     * @param lowerBound The (exclusive) lower time bound, long, microseconds.
     * @return boolean True if there are newer data points.
     * @throws IOException
     */
    public static boolean hasNewer(Key parentKey, String kind, Long lowerBound) throws IOException {
        if (lowerBound == null || lowerBound <= 0) return false;
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Key startKey = KeyFactory.createKey(parentKey, kind, lowerBound);
        Filter startFilter =
                new FilterPredicate(
                        Entity.KEY_RESERVED_PROPERTY, FilterOperator.GREATER_THAN, startKey);
        Query q = new Query(kind).setAncestor(parentKey).setFilter(startFilter).setKeysOnly();
        return datastore.prepare(q).countEntities(FetchOptions.Builder.withLimit(1)) > 0;
    }

    /**
     * Returns true if there are data points older than upperBound in the table.
     *
     * @param parentKey The parent key to use in the query.
     * @param kind The query entity kind.
     * @param upperBound The (exclusive) upper time bound, long, microseconds.
     * @return boolean True if there are older data points.
     * @throws IOException
     */
    public static boolean hasOlder(Key parentKey, String kind, Long upperBound) throws IOException {
        if (upperBound == null || upperBound <= 0) return false;
        Key endKey = KeyFactory.createKey(parentKey, kind, upperBound);
        Filter endFilter =
                new FilterPredicate(Entity.KEY_RESERVED_PROPERTY, FilterOperator.LESS_THAN, endKey);
        Query q = new Query(kind).setAncestor(parentKey).setFilter(endFilter).setKeysOnly();
        return datastore.prepare(q).countEntities(FetchOptions.Builder.withLimit(1)) > 0;
    }

    /**
     * Get all of the devices branches.
     *
     * @return a list of all branches.
     */
    public static List<String> getAllBranches() {
        Query query = new Query(BranchEntity.KIND).setKeysOnly();
        List<String> branches = new ArrayList<>();
        for (Entity e : datastore.prepare(query).asIterable(getLargeBatchOptions())) {
            branches.add(e.getKey().getName());
        }
        return branches;
    }

    /**
     * Get all of the device build flavors.
     *
     * @return a list of all device build flavors.
     */
    public static List<String> getAllBuildFlavors() {
        Query query = new Query(BuildTargetEntity.KIND).setKeysOnly();
        List<String> devices = new ArrayList<>();
        for (Entity e : datastore.prepare(query).asIterable(getLargeBatchOptions())) {
            devices.add(e.getKey().getName());
        }
        return devices;
    }

    /**
     * Upload data from a test report message
     *
     * @param report The test report containing data to upload.
     */
    public static void insertTestReport(TestReportMessage report) {

        List<Entity> testEntityList = new ArrayList<>();
        List<Entity> branchEntityList = new ArrayList<>();
        List<Entity> buildTargetEntityList = new ArrayList<>();
        List<Entity> coverageEntityList = new ArrayList<>();
        List<Entity> profilingPointRunEntityList = new ArrayList<>();

        if (!report.hasStartTimestamp()
                || !report.hasEndTimestamp()
                || !report.hasTest()
                || !report.hasHostInfo()
                || !report.hasBuildInfo()) {
            // missing information
            return;
        }
        long startTimestamp = report.getStartTimestamp();
        long endTimestamp = report.getEndTimestamp();
        String testName = report.getTest().toStringUtf8();
        String testBuildId = report.getBuildInfo().getId().toStringUtf8();
        String hostName = report.getHostInfo().getHostname().toStringUtf8();

        TestEntity testEntity = new TestEntity(testName);

        Key testRunKey =
                KeyFactory.createKey(
                        testEntity.key, TestRunEntity.KIND, report.getStartTimestamp());

        long passCount = 0;
        long failCount = 0;
        long coveredLineCount = 0;
        long totalLineCount = 0;

        Set<Key> buildTargetKeys = new HashSet<>();
        Set<Key> branchKeys = new HashSet<>();
        List<TestCaseRunEntity> testCases = new ArrayList<>();
        List<Key> profilingPointKeys = new ArrayList<>();
        List<String> links = new ArrayList<>();

        // Process test cases
        for (TestCaseReportMessage testCase : report.getTestCaseList()) {
            String testCaseName = testCase.getName().toStringUtf8();
            TestCaseResult result = testCase.getTestResult();
            // Track global pass/fail counts
            if (result == TestCaseResult.TEST_CASE_RESULT_PASS) {
                ++passCount;
            } else if (result != TestCaseResult.TEST_CASE_RESULT_SKIP) {
                ++failCount;
            }
            if (testCase.getSystraceCount() > 0
                    && testCase.getSystraceList().get(0).getUrlCount() > 0) {
                String systraceLink = testCase.getSystraceList().get(0).getUrl(0).toStringUtf8();
                links.add(systraceLink);
            }
            // Process coverage data for test case
            for (CoverageReportMessage coverage : testCase.getCoverageList()) {
                CoverageEntity coverageEntity =
                        CoverageEntity.fromCoverageReport(testRunKey, testCaseName, coverage);
                if (coverageEntity == null) {
                    logger.log(Level.WARNING, "Invalid coverage report in test run " + testRunKey);
                } else {
                    coveredLineCount += coverageEntity.coveredLineCount;
                    totalLineCount += coverageEntity.totalLineCount;
                    coverageEntityList.add(coverageEntity.toEntity());
                }
            }
            // Process profiling data for test case
            for (ProfilingReportMessage profiling : testCase.getProfilingList()) {
                ProfilingPointRunEntity profilingPointRunEntity =
                        ProfilingPointRunEntity.fromProfilingReport(testRunKey, profiling);
                if (profilingPointRunEntity == null) {
                    logger.log(Level.WARNING, "Invalid profiling report in test run " + testRunKey);
                } else {
                    profilingPointRunEntityList.add(profilingPointRunEntity.toEntity());
                    profilingPointKeys.add(profilingPointRunEntity.key);
                    testEntity.setHasProfilingData(true);
                }
            }

            int lastIndex = testCases.size() - 1;
            if (lastIndex < 0 || testCases.get(lastIndex).isFull()) {
                testCases.add(new TestCaseRunEntity());
                ++lastIndex;
            }
            TestCaseRunEntity testCaseEntity = testCases.get(lastIndex);
            testCaseEntity.addTestCase(testCaseName, result.getNumber());
        }

        List<Entity> testCasePuts = new ArrayList<>();
        for (TestCaseRunEntity testCaseEntity : testCases) {
            testCasePuts.add(testCaseEntity.toEntity());
        }
        List<Key> testCaseKeys = datastore.put(testCasePuts);

        List<Long> testCaseIds = new ArrayList<>();
        for (Key key : testCaseKeys) {
            testCaseIds.add(key.getId());
        }

        // Process device information
        TestRunType testRunType = null;
        for (AndroidDeviceInfoMessage device : report.getDeviceInfoList()) {
            DeviceInfoEntity deviceInfoEntity =
                    DeviceInfoEntity.fromDeviceInfoMessage(testRunKey, device);
            if (deviceInfoEntity == null) {
                logger.log(Level.WARNING, "Invalid device info in test run " + testRunKey);
            } else {
                // Run type on devices must be the same, else set to OTHER
                TestRunType runType = TestRunType.fromBuildId(deviceInfoEntity.buildId);
                if (testRunType == null) {
                    testRunType = runType;
                } else if (runType != testRunType) {
                    testRunType = TestRunType.OTHER;
                }
                testEntityList.add(deviceInfoEntity.toEntity());
                BuildTargetEntity target = new BuildTargetEntity(deviceInfoEntity.buildFlavor);
                if (buildTargetKeys.add(target.key)) {
                    buildTargetEntityList.add(target.toEntity());
                }
                BranchEntity branch = new BranchEntity(deviceInfoEntity.branch);
                if (branchKeys.add(branch.key)) {
                    branchEntityList.add(branch.toEntity());
                }
            }
        }

        // Overall run type should be determined by the device builds unless test build is OTHER
        if (testRunType == null) {
            testRunType = TestRunType.fromBuildId(testBuildId);
        } else if (TestRunType.fromBuildId(testBuildId) == TestRunType.OTHER) {
            testRunType = TestRunType.OTHER;
        }

        // Process global coverage data
        for (CoverageReportMessage coverage : report.getCoverageList()) {
            CoverageEntity coverageEntity =
                    CoverageEntity.fromCoverageReport(testRunKey, new String(), coverage);
            if (coverageEntity == null) {
                logger.log(Level.WARNING, "Invalid coverage report in test run " + testRunKey);
            } else {
                coveredLineCount += coverageEntity.coveredLineCount;
                totalLineCount += coverageEntity.totalLineCount;
                coverageEntityList.add(coverageEntity.toEntity());
            }
        }

        // Process global profiling data
        for (ProfilingReportMessage profiling : report.getProfilingList()) {
            ProfilingPointRunEntity profilingPointRunEntity =
                    ProfilingPointRunEntity.fromProfilingReport(testRunKey, profiling);
            if (profilingPointRunEntity == null) {
                logger.log(Level.WARNING, "Invalid profiling report in test run " + testRunKey);
            } else {
                profilingPointRunEntityList.add(profilingPointRunEntity.toEntity());
                profilingPointKeys.add(profilingPointRunEntity.key);
                testEntity.setHasProfilingData(true);
            }
        }

        // Process log data
        for (LogMessage log : report.getLogList()) {
            if (log.hasUrl()) links.add(log.getUrl().toStringUtf8());
        }
        // Process url resource
        for (UrlResourceMessage resource : report.getLinkResourceList()) {
            if (resource.hasUrl()) links.add(resource.getUrl().toStringUtf8());
        }

        TestRunEntity testRunEntity =
                new TestRunEntity(
                        testEntity.key,
                        testRunType,
                        startTimestamp,
                        endTimestamp,
                        testBuildId,
                        hostName,
                        passCount,
                        failCount,
                        testCaseIds,
                        links,
                        coveredLineCount,
                        totalLineCount);
        testEntityList.add(testRunEntity.toEntity());

        Entity test = testEntity.toEntity();

        if (datastoreTransactionalRetry(test, testEntityList)) {
            List<List<Entity>> auxiliaryEntityList =
                    Arrays.asList(
                            profilingPointRunEntityList,
                            coverageEntityList,
                            branchEntityList,
                            buildTargetEntityList);
            int indexCount = 0;
            for (List<Entity> entityList : auxiliaryEntityList) {
                switch (indexCount) {
                    case 0:
                    case 1:
                        if (entityList.size() > MAX_ENTITY_SIZE_PER_TRANSACTION) {
                            List<List<Entity>> partitionedList =
                                    Lists.partition(entityList, MAX_ENTITY_SIZE_PER_TRANSACTION);
                            partitionedList.forEach(
                                    subEntityList -> {
                                        datastoreTransactionalRetry(
                                                new Entity(NULL_ENTITY_KIND), subEntityList);
                                    });
                        } else {
                            datastoreTransactionalRetry(new Entity(NULL_ENTITY_KIND), entityList);
                        }
                        break;
                    case 2:
                    case 3:
                        datastoreTransactionalRetryWithXG(
                                new Entity(NULL_ENTITY_KIND), entityList, true);
                        break;
                    default:
                        break;
                }
                indexCount++;
            }

            if (testRunEntity.type == TestRunType.POSTSUBMIT) {
                VtsAlertJobServlet.addTask(testRunKey);
                if (testRunEntity.hasCoverage) {
                    VtsCoverageAlertJobServlet.addTask(testRunKey);
                }
                if (profilingPointKeys.size() > 0) {
                    VtsProfilingStatsJobServlet.addTasks(profilingPointKeys);
                }
            } else {
                logger.log(
                        Level.WARNING,
                        "The alert email was not sent as testRunEntity type is not POSTSUBMIT!" +
                           " \n " + " testRunEntity type => " + testRunEntity.type);
            }
        }
    }

    /**
     * Upload data from a test plan report message
     *
     * @param report The test plan report containing data to upload.
     */
    public static void insertTestPlanReport(TestPlanReportMessage report) {
        List<Entity> testEntityList = new ArrayList<>();

        List<String> testModules = report.getTestModuleNameList();
        List<Long> testTimes = report.getTestModuleStartTimestampList();
        if (testModules.size() != testTimes.size() || !report.hasTestPlanName()) {
            logger.log(Level.WARNING, "TestPlanReportMessage is missing information.");
            return;
        }

        String testPlanName = report.getTestPlanName();
        Entity testPlanEntity = new TestPlanEntity(testPlanName).toEntity();
        List<Key> testRunKeys = new ArrayList<>();
        for (int i = 0; i < testModules.size(); i++) {
            String test = testModules.get(i);
            long time = testTimes.get(i);
            Key parentKey = KeyFactory.createKey(TestEntity.KIND, test);
            Key testRunKey = KeyFactory.createKey(parentKey, TestRunEntity.KIND, time);
            testRunKeys.add(testRunKey);
        }
        Map<Key, Entity> testRuns = datastore.get(testRunKeys);
        long passCount = 0;
        long failCount = 0;
        long startTimestamp = -1;
        long endTimestamp = -1;
        String testBuildId = null;
        TestRunType type = null;
        Set<DeviceInfoEntity> deviceInfoEntitySet = new HashSet<>();
        for (Key testRunKey : testRuns.keySet()) {
            TestRunEntity testRun = TestRunEntity.fromEntity(testRuns.get(testRunKey));
            if (testRun == null) {
                continue; // not a valid test run
            }
            passCount += testRun.passCount;
            failCount += testRun.failCount;
            if (startTimestamp < 0 || testRunKey.getId() < startTimestamp) {
                startTimestamp = testRunKey.getId();
            }
            if (endTimestamp < 0 || testRun.endTimestamp > endTimestamp) {
                endTimestamp = testRun.endTimestamp;
            }
            if (type == null) {
                type = testRun.type;
            } else if (type != testRun.type) {
                type = TestRunType.OTHER;
            }
            testBuildId = testRun.testBuildId;
            Query deviceInfoQuery = new Query(DeviceInfoEntity.KIND).setAncestor(testRunKey);
            for (Entity deviceInfoEntity : datastore.prepare(deviceInfoQuery).asIterable()) {
                DeviceInfoEntity device = DeviceInfoEntity.fromEntity(deviceInfoEntity);
                if (device == null) {
                    continue; // invalid entity
                }
                deviceInfoEntitySet.add(device);
            }
        }
        if (startTimestamp < 0 || testBuildId == null || type == null) {
            logger.log(Level.WARNING, "Couldn't infer test run information from runs.");
            return;
        }
        TestPlanRunEntity testPlanRun =
                new TestPlanRunEntity(
                        testPlanEntity.getKey(),
                        testPlanName,
                        type,
                        startTimestamp,
                        endTimestamp,
                        testBuildId,
                        passCount,
                        failCount,
                        testRunKeys);

        // Create the device infos.
        for (DeviceInfoEntity device : deviceInfoEntitySet) {
            testEntityList.add(device.copyWithParent(testPlanRun.key).toEntity());
        }
        testEntityList.add(testPlanRun.toEntity());

        datastoreTransactionalRetry(testPlanEntity, testEntityList);
    }

    /**
     * Datastore Transactional process for data insertion with MAX_WRITE_RETRIES times and withXG of
     * false value
     *
     * @param entity The entity that you want to insert to datastore.
     * @param entityList The list of entity for using datastore put method.
     */
    private static boolean datastoreTransactionalRetry(Entity entity, List<Entity> entityList) {
        return datastoreTransactionalRetryWithXG(entity, entityList, false);
    }

    /**
     * Datastore Transactional process for data insertion with MAX_WRITE_RETRIES times
     *
     * @param entity The entity that you want to insert to datastore.
     * @param entityList The list of entity for using datastore put method.
     */
    private static boolean datastoreTransactionalRetryWithXG(
            Entity entity, List<Entity> entityList, boolean withXG) {
        int retries = 0;
        while (true) {
            Transaction txn;
            if (withXG) {
                TransactionOptions options = TransactionOptions.Builder.withXG(withXG);
                txn = datastore.beginTransaction(options);
            } else {
                txn = datastore.beginTransaction();
            }

            try {
                // Check if test already exists in the database
                if (!entity.getKind().equalsIgnoreCase(NULL_ENTITY_KIND)) {
                    try {
                        if (entity.getKind().equalsIgnoreCase("Test")) {
                            Entity datastoreEntity = datastore.get(entity.getKey());
                            TestEntity datastoreTestEntity = TestEntity.fromEntity(datastoreEntity);
                            if (datastoreTestEntity == null
                                    || !datastoreTestEntity.equals(entity)) {
                                entityList.add(entity);
                            }
                        } else if (entity.getKind().equalsIgnoreCase("TestPlan")) {
                            datastore.get(entity.getKey());
                        } else {
                            datastore.get(entity.getKey());
                        }
                    } catch (EntityNotFoundException e) {
                        entityList.add(entity);
                    }
                }
                datastore.put(txn, entityList);
                txn.commit();
                break;
            } catch (ConcurrentModificationException
                    | DatastoreFailureException
                    | DatastoreTimeoutException e) {
                entityList.remove(entity);
                logger.log(
                        Level.WARNING,
                        "Retrying insert kind: " + entity.getKind() + " key: " + entity.getKey());
                if (retries++ >= MAX_WRITE_RETRIES) {
                    logger.log(
                            Level.SEVERE,
                            "Exceeded maximum retries kind: "
                                    + entity.getKind()
                                    + " key: "
                                    + entity.getKey());
                    return false;
                }
            } finally {
                if (txn.isActive()) {
                    logger.log(
                            Level.WARNING, "Transaction rollback forced for : " + entity.getKind());
                    txn.rollback();
                }
            }
        }
        return true;
    }
}
