/*
 * Copyright (c) 2016 Google Inc. All Rights Reserved.
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

package com.android.vts.job;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.TestAcknowledgmentEntity;
import com.android.vts.entity.TestCaseRunEntity;
import com.android.vts.entity.TestCaseRunEntity.TestCase;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.entity.TestStatusEntity;
import com.android.vts.entity.TestStatusEntity.TestCaseReference;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.EmailHelper;
import com.android.vts.util.FilterUtil;
import com.android.vts.util.TimeUtil;
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
import com.google.appengine.api.datastore.Query.SortDirection;
import com.google.appengine.api.datastore.Transaction;
import com.google.appengine.api.taskqueue.Queue;
import com.google.appengine.api.taskqueue.QueueFactory;
import com.google.appengine.api.taskqueue.TaskOptions;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.ConcurrentModificationException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.commons.lang.StringUtils;

/** Represents the notifications service which is automatically called on a fixed schedule. */
public class VtsAlertJobServlet extends HttpServlet {
    private static final String ALERT_JOB_URL = "/task/vts_alert_job";
    protected static final Logger logger = Logger.getLogger(VtsAlertJobServlet.class.getName());
    protected static final int MAX_RUN_COUNT = 1000; // maximum number of runs to query for

    /**
     * Process the current test case failures for a test.
     *
     * @param status The TestStatusEntity object for the test.
     * @returns a map from test case name to the test case run ID for which the test case failed.
     */
    private static Map<String, TestCase> getCurrentFailures(TestStatusEntity status) {
        if (status.failingTestCases == null || status.failingTestCases.size() == 0) {
            return new HashMap<>();
        }
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Map<String, TestCase> failingTestcases = new HashMap<>();
        Set<Key> gets = new HashSet<>();
        for (TestCaseReference testCaseRef : status.failingTestCases) {
            gets.add(KeyFactory.createKey(TestCaseRunEntity.KIND, testCaseRef.parentId));
        }
        if (gets.size() == 0) {
            return failingTestcases;
        }
        Map<Key, Entity> testCaseMap = datastore.get(gets);

        for (TestCaseReference testCaseRef : status.failingTestCases) {
            Key key = KeyFactory.createKey(TestCaseRunEntity.KIND, testCaseRef.parentId);
            if (!testCaseMap.containsKey(key)) {
                continue;
            }
            Entity testCaseRun = testCaseMap.get(key);
            TestCaseRunEntity testCaseRunEntity = TestCaseRunEntity.fromEntity(testCaseRun);
            if (testCaseRunEntity.testCases.size() <= testCaseRef.offset) {
                continue;
            }
            TestCase testCase = testCaseRunEntity.testCases.get(testCaseRef.offset);
            failingTestcases.put(testCase.name, testCase);
        }
        return failingTestcases;
    }

    /**
     * Get the test acknowledgments for a test key.
     *
     * @param testKey The key to the test whose acknowledgments to fetch.
     * @return A list of test acknowledgments.
     */
    private static List<TestAcknowledgmentEntity> getTestCaseAcknowledgments(Key testKey) {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

        List<TestAcknowledgmentEntity> acks = new ArrayList<>();
        Filter testFilter =
                new Query.FilterPredicate(
                    TestAcknowledgmentEntity.TEST_KEY, Query.FilterOperator.EQUAL, testKey);
        Query q = new Query(TestAcknowledgmentEntity.KIND).setFilter(testFilter);

        for (Entity ackEntity : datastore.prepare(q).asIterable()) {
            TestAcknowledgmentEntity ack = TestAcknowledgmentEntity.fromEntity(ackEntity);
            if (ack == null) continue;
            acks.add(ack);
        }
        return acks;
    }

    /**
     * Get the test runs for the test in the specified time window.
     *
     * If the start and end time delta is greater than one day, the query will be truncated.
     *
     * @param testKey The key to the test whose runs to query.
     * @param startTime The start time for the query.
     * @param endTime The end time for the query.
     * @return A list of test runs in the specified time window.
     */
    private static List<TestRunEntity> getTestRuns(Key testKey, long startTime, long endTime) {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Filter testTypeFilter = FilterUtil.getTestTypeFilter(false, true, false);
        long delta = endTime - startTime;
        delta = Math.min(delta, TimeUnit.DAYS.toMicros(1));
        Filter runFilter =
                FilterUtil.getTimeFilter(
                        testKey, TestRunEntity.KIND, endTime - delta + 1, endTime, testTypeFilter);

        Query q =
                new Query(TestRunEntity.KIND)
                        .setAncestor(testKey)
                        .setFilter(runFilter)
                        .addSort(Entity.KEY_RESERVED_PROPERTY, SortDirection.DESCENDING);

        List<TestRunEntity> testRuns = new ArrayList<>();
        for (Entity testRunEntity :
                datastore.prepare(q).asIterable(FetchOptions.Builder.withLimit(MAX_RUN_COUNT))) {
            TestRunEntity testRun = TestRunEntity.fromEntity(testRunEntity);
            if (testRun == null) continue;
            testRuns.add(testRun);
        }
        return testRuns;
    }

    /**
     * Separate the test cases which are acknowledged by the provided acknowledgments.
     *
     * @param testCases The list of test case names.
     * @param devices The list of devices for a test run.
     * @param acks The list of acknowledgments for the test.
     * @return A list of acknowledged test case names that have been removed from the input test
     *     cases.
     */
    public static Set<String> separateAcknowledged(
            Set<String> testCases,
            List<DeviceInfoEntity> devices,
            List<TestAcknowledgmentEntity> acks) {
        Set<String> acknowledged = new HashSet<>();
        for (TestAcknowledgmentEntity ack : acks) {
            boolean allDevices = ack.devices == null || ack.devices.size() == 0;
            boolean allBranches = ack.branches == null || ack.branches.size() == 0;
            boolean isRelevant = allDevices && allBranches;

            // Determine if the acknowledgment is relevant to the devices.
            if (!isRelevant) {
                for (DeviceInfoEntity device : devices) {
                    boolean deviceAcknowledged =
                            allDevices || ack.devices.contains(device.buildFlavor);
                    boolean branchAcknowledged =
                            allBranches || ack.branches.contains(device.branch);
                    if (deviceAcknowledged && branchAcknowledged) isRelevant = true;
                }
            }

            if (isRelevant) {
                // Separate the test cases
                boolean allTestCases = ack.testCaseNames == null || ack.testCaseNames.size() == 0;
                if (allTestCases) {
                    acknowledged.addAll(testCases);
                    testCases.removeAll(acknowledged);
                } else {
                    for (String testCase : ack.testCaseNames) {
                        if (testCases.contains(testCase)) {
                            acknowledged.add(testCase);
                            testCases.remove(testCase);
                        }
                    }
                }
            }
        }
        return acknowledged;
    }

    /**
     * Checks whether any new failures have occurred beginning since (and including) startTime.
     *
     * @param testRuns The list of test runs for which to update the status.
     * @param link The string URL linking to the test's status table.
     * @param failedTestCaseMap The map of test case names to TestCase for those failing in the last
     *     status update.
     * @param emailAddresses The list of email addresses to send notifications to.
     * @param messages The email Message queue.
     * @returns latest TestStatusMessage or null if no update is available.
     * @throws IOException
     */
    public TestStatusEntity getTestStatus(
            List<TestRunEntity> testRuns,
            String link,
            Map<String, TestCase> failedTestCaseMap,
            List<TestAcknowledgmentEntity> testAcks,
            List<String> emailAddresses,
            List<Message> messages)
            throws IOException {
        if (testRuns.size() == 0) return null;
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

        TestRunEntity mostRecentRun = null;
        Map<String, TestCaseResult> mostRecentTestCaseResults = new HashMap<>();
        Map<String, TestCase> testCaseBreakageMap = new HashMap<>();
        int passingTestcaseCount = 0;
        List<TestCaseReference> failingTestCases = new ArrayList<>();
        Set<String> fixedTestcases = new HashSet<>();
        Set<String> newTestcaseFailures = new HashSet<>();
        Set<String> continuedTestcaseFailures = new HashSet<>();
        Set<String> skippedTestcaseFailures = new HashSet<>();
        Set<String> transientTestcaseFailures = new HashSet<>();

        for (TestRunEntity testRun : testRuns) {
            if (mostRecentRun == null) {
                mostRecentRun = testRun;
            }
            List<Key> testCaseKeys = new ArrayList<>();
            for (long testCaseId : testRun.testCaseIds) {
                testCaseKeys.add(KeyFactory.createKey(TestCaseRunEntity.KIND, testCaseId));
            }
            Map<Key, Entity> entityMap = datastore.get(testCaseKeys);
            for (Key testCaseKey : testCaseKeys) {
                if (!entityMap.containsKey(testCaseKey)) {
                    logger.log(Level.WARNING, "Test case entity missing: " + testCaseKey);
                    continue;
                }
                Entity testCaseRun = entityMap.get(testCaseKey);
                TestCaseRunEntity testCaseRunEntity = TestCaseRunEntity.fromEntity(testCaseRun);
                if (testCaseRunEntity == null) {
                    logger.log(Level.WARNING, "Invalid test case run: " + testCaseRun.getKey());
                    continue;
                }
                for (TestCase testCase : testCaseRunEntity.testCases) {
                    String testCaseName = testCase.name;
                    TestCaseResult result = TestCaseResult.valueOf(testCase.result);

                    if (mostRecentRun == testRun) {
                        mostRecentTestCaseResults.put(testCaseName, result);
                    } else {
                        if (!mostRecentTestCaseResults.containsKey(testCaseName)) {
                            // Deprecate notifications for tests that are not present on newer runs
                            continue;
                        }
                        TestCaseResult mostRecentRes = mostRecentTestCaseResults.get(testCaseName);
                        if (mostRecentRes == TestCaseResult.TEST_CASE_RESULT_SKIP) {
                            mostRecentTestCaseResults.put(testCaseName, result);
                        } else if (mostRecentRes == TestCaseResult.TEST_CASE_RESULT_PASS) {
                            // Test is passing now, witnessed a transient failure
                            if (result != TestCaseResult.TEST_CASE_RESULT_PASS
                                    && result != TestCaseResult.TEST_CASE_RESULT_SKIP) {
                                transientTestcaseFailures.add(testCaseName);
                            }
                        }
                    }

                    // Record test case breakages
                    if (result != TestCaseResult.TEST_CASE_RESULT_PASS
                            && result != TestCaseResult.TEST_CASE_RESULT_SKIP) {
                        testCaseBreakageMap.put(testCaseName, testCase);
                    }
                }
            }
        }

        Set<String> buildIdList = new HashSet<>();
        List<DeviceInfoEntity> devices = new ArrayList<>();
        Query deviceQuery = new Query(DeviceInfoEntity.KIND).setAncestor(mostRecentRun.key);
        for (Entity device : datastore.prepare(deviceQuery).asIterable()) {
            DeviceInfoEntity deviceEntity = DeviceInfoEntity.fromEntity(device);
            if (deviceEntity == null) {
                continue;
            }
            buildIdList.add(deviceEntity.buildId);
            devices.add(deviceEntity);
        }
        String footer = EmailHelper.getEmailFooter(mostRecentRun, devices, link);
        String buildId = StringUtils.join(buildIdList, ",");

        for (String testCaseName : mostRecentTestCaseResults.keySet()) {
            TestCaseResult mostRecentResult = mostRecentTestCaseResults.get(testCaseName);
            boolean previouslyFailed = failedTestCaseMap.containsKey(testCaseName);
            if (mostRecentResult == TestCaseResult.TEST_CASE_RESULT_SKIP) {
                // persist previous status
                if (previouslyFailed) {
                    skippedTestcaseFailures.add(testCaseName);
                    failingTestCases.add(
                            new TestCaseReference(failedTestCaseMap.get(testCaseName)));
                } else {
                    ++passingTestcaseCount;
                }
            } else if (mostRecentResult == TestCaseResult.TEST_CASE_RESULT_PASS) {
                ++passingTestcaseCount;
                if (previouslyFailed && !transientTestcaseFailures.contains(testCaseName)) {
                    fixedTestcases.add(testCaseName);
                }
            } else {
                if (!previouslyFailed) {
                    newTestcaseFailures.add(testCaseName);
                    failingTestCases.add(
                            new TestCaseReference(testCaseBreakageMap.get(testCaseName)));
                } else {
                    continuedTestcaseFailures.add(testCaseName);
                    failingTestCases.add(
                            new TestCaseReference(failedTestCaseMap.get(testCaseName)));
                }
            }
        }

        Set<String> acknowledgedFailures =
                separateAcknowledged(newTestcaseFailures, devices, testAcks);
        acknowledgedFailures.addAll(
                separateAcknowledged(transientTestcaseFailures, devices, testAcks));
        acknowledgedFailures.addAll(
                separateAcknowledged(continuedTestcaseFailures, devices, testAcks));

        String summary = new String();
        if (newTestcaseFailures.size() + continuedTestcaseFailures.size() > 0) {
            summary += "The following test cases failed in the latest test run:<br>";

            // Add new test case failures to top of summary in bold font.
            List<String> sortedNewTestcaseFailures = new ArrayList<>(newTestcaseFailures);
            sortedNewTestcaseFailures.sort(Comparator.naturalOrder());
            for (String testcaseName : sortedNewTestcaseFailures) {
                summary += "- " + "<b>" + testcaseName + "</b><br>";
            }

            // Add continued test case failures to summary.
            List<String> sortedContinuedTestcaseFailures =
                    new ArrayList<>(continuedTestcaseFailures);
            sortedContinuedTestcaseFailures.sort(Comparator.naturalOrder());
            for (String testcaseName : sortedContinuedTestcaseFailures) {
                summary += "- " + testcaseName + "<br>";
            }
        }
        if (fixedTestcases.size() > 0) {
            // Add fixed test cases to summary.
            summary += "<br><br>The following test cases were fixed in the latest test run:<br>";
            List<String> sortedFixedTestcases = new ArrayList<>(fixedTestcases);
            sortedFixedTestcases.sort(Comparator.naturalOrder());
            for (String testcaseName : sortedFixedTestcases) {
                summary += "- <i>" + testcaseName + "</i><br>";
            }
        }
        if (transientTestcaseFailures.size() > 0) {
            // Add transient test case failures to summary.
            summary += "<br><br>The following transient test case failures occured:<br>";
            List<String> sortedTransientTestcaseFailures =
                    new ArrayList<>(transientTestcaseFailures);
            sortedTransientTestcaseFailures.sort(Comparator.naturalOrder());
            for (String testcaseName : sortedTransientTestcaseFailures) {
                summary += "- " + testcaseName + "<br>";
            }
        }
        if (skippedTestcaseFailures.size() > 0) {
            // Add skipped test case failures to summary.
            summary += "<br><br>The following test cases have not been run since failing:<br>";
            List<String> sortedSkippedTestcaseFailures = new ArrayList<>(skippedTestcaseFailures);
            sortedSkippedTestcaseFailures.sort(Comparator.naturalOrder());
            for (String testcaseName : sortedSkippedTestcaseFailures) {
                summary += "- " + testcaseName + "<br>";
            }
        }
        if (acknowledgedFailures.size() > 0) {
            // Add acknowledged test case failures to summary.
            List<String> sortedAcknowledgedFailures = new ArrayList<>(acknowledgedFailures);
            sortedAcknowledgedFailures.sort(Comparator.naturalOrder());
            if (acknowledgedFailures.size() > 0) {
                summary +=
                        "<br><br>The following acknowledged test case failures continued to fail:<br>";
                for (String testcaseName : sortedAcknowledgedFailures) {
                    summary += "- " + testcaseName + "<br>";
                }
            }
        }

        String testName = mostRecentRun.key.getParent().getName();
        String uploadDateString = TimeUtil.getDateString(mostRecentRun.startTimestamp);
        String subject = "VTS Test Alert: " + testName + " @ " + uploadDateString;
        if (newTestcaseFailures.size() > 0) {
            String body =
                    "Hello,<br><br>New test case failure(s) in "
                            + testName
                            + " for device build ID(s): "
                            + buildId
                            + ".<br><br>"
                            + summary
                            + footer;
            try {
                messages.add(EmailHelper.composeEmail(emailAddresses, subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        } else if (continuedTestcaseFailures.size() > 0) {
            String body =
                    "Hello,<br><br>Continuous test case failure(s) in "
                            + testName
                            + " for device build ID(s): "
                            + buildId
                            + ".<br><br>"
                            + summary
                            + footer;
            try {
                messages.add(EmailHelper.composeEmail(emailAddresses, subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        } else if (transientTestcaseFailures.size() > 0) {
            String body =
                    "Hello,<br><br>Transient test case failure(s) in "
                            + testName
                            + " but tests all "
                            + "are passing in the latest device build(s): "
                            + buildId
                            + ".<br><br>"
                            + summary
                            + footer;
            try {
                messages.add(EmailHelper.composeEmail(emailAddresses, subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        } else if (fixedTestcases.size() > 0) {
            String body =
                    "Hello,<br><br>All test cases passed in "
                            + testName
                            + " for device build ID(s): "
                            + buildId
                            + "!<br><br>"
                            + summary
                            + footer;
            try {
                messages.add(EmailHelper.composeEmail(emailAddresses, subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        }
        return new TestStatusEntity(
                testName,
                mostRecentRun.startTimestamp,
                passingTestcaseCount,
                failingTestCases.size(),
                failingTestCases);
    }

    /**
     * Add a task to process test run data
     *
     * @param testRunKey The key of the test run whose data process.
     */
    public static void addTask(Key testRunKey) {
        Queue queue = QueueFactory.getDefaultQueue();
        String keyString = KeyFactory.keyToString(testRunKey);
        queue.add(
                TaskOptions.Builder.withUrl(ALERT_JOB_URL)
                        .param("runKey", keyString)
                        .method(TaskOptions.Method.POST));
    }

    @Override
    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        String runKeyString = request.getParameter("runKey");

        Key testRunKey;
        try {
            testRunKey = KeyFactory.stringToKey(runKeyString);
        } catch (IllegalArgumentException e) {
            logger.log(Level.WARNING, "Invalid key specified: " + runKeyString);
            return;
        }
        String testName = testRunKey.getParent().getName();

        TestStatusEntity status = null;
        Key statusKey = KeyFactory.createKey(TestStatusEntity.KIND, testName);
        try {
            status = TestStatusEntity.fromEntity(datastore.get(statusKey));
        } catch (EntityNotFoundException e) {
            // no existing status
        }
        if (status == null) {
            status = new TestStatusEntity(testName);
        }
        if (status.timestamp >= testRunKey.getId()) {
            // Another job has already updated the status first
            return;
        }
        List<String> emails = EmailHelper.getSubscriberEmails(testRunKey.getParent());

        StringBuffer fullUrl = request.getRequestURL();
        String baseUrl = fullUrl.substring(0, fullUrl.indexOf(request.getRequestURI()));
        String link =
                baseUrl + "/show_tree?testName=" + testName + "&endTime=" + testRunKey.getId();

        List<Message> messageQueue = new ArrayList<>();
        Map<String, TestCase> failedTestcaseMap = getCurrentFailures(status);
        List<TestAcknowledgmentEntity> testAcks =
                getTestCaseAcknowledgments(testRunKey.getParent());
        List<TestRunEntity> testRuns =
                getTestRuns(testRunKey.getParent(), status.timestamp, testRunKey.getId());
        if (testRuns.size() == 0) return;

        TestStatusEntity newStatus =
                getTestStatus(testRuns, link, failedTestcaseMap, testAcks, emails, messageQueue);
        if (newStatus == null) {
            // No changes to status
            return;
        }

        int retries = 0;
        while (true) {
            Transaction txn = datastore.beginTransaction();
            try {
                try {
                    status = TestStatusEntity.fromEntity(datastore.get(statusKey));
                } catch (EntityNotFoundException e) {
                    // no status left
                }
                if (status == null || status.timestamp >= newStatus.timestamp) {
                    txn.rollback();
                } else { // This update is most recent.
                    datastore.put(newStatus.toEntity());
                    txn.commit();
                    EmailHelper.sendAll(messageQueue);
                }
                break;
            } catch (ConcurrentModificationException
                    | DatastoreFailureException
                    | DatastoreTimeoutException e) {
                logger.log(Level.WARNING, "Retrying alert job insert: " + statusKey);
                if (retries++ >= DatastoreHelper.MAX_WRITE_RETRIES) {
                    logger.log(Level.SEVERE, "Exceeded alert job retries: " + statusKey);
                    throw e;
                }
            } finally {
                if (txn.isActive()) {
                    txn.rollback();
                }
            }
        }
    }
}
