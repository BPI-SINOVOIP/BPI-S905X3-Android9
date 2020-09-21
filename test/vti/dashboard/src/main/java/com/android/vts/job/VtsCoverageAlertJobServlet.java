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

package com.android.vts.job;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.TestCoverageStatusEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.EmailHelper;
import com.google.appengine.api.datastore.DatastoreFailureException;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.DatastoreTimeoutException;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.EntityNotFoundException;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Transaction;
import com.google.appengine.api.taskqueue.Queue;
import com.google.appengine.api.taskqueue.QueueFactory;
import com.google.appengine.api.taskqueue.TaskOptions;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.ConcurrentModificationException;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.commons.lang.StringUtils;

/** Coverage notification job. */
public class VtsCoverageAlertJobServlet extends HttpServlet {
    private static final String COVERAGE_ALERT_URL = "/task/vts_coverage_job";
    protected static final Logger logger =
            Logger.getLogger(VtsCoverageAlertJobServlet.class.getName());
    protected static final double CHANGE_ALERT_THRESHOLD = 0.05;
    protected static final double GOOD_THRESHOLD = 0.7;
    protected static final double BAD_THRESHOLD = 0.3;

    protected static final DecimalFormat FORMATTER;

    /** Initialize the decimal formatter. */
    static {
        FORMATTER = new DecimalFormat("#.#");
        FORMATTER.setRoundingMode(RoundingMode.HALF_UP);
    }

    /**
     * Gets a new coverage status and adds notification emails to the messages list.
     *
     * Send an email to notify subscribers in the event that a test goes up or down by more than
     * 5%, becomes higher or lower than 70%, or becomes higher or lower than 30%.
     *
     * @param status The TestCoverageStatusEntity object for the test.
     * @param testRunKey The key for TestRunEntity whose data to process and reflect in the state.
     * @param link The string URL linking to the test's status table.
     * @param emailAddresses The list of email addresses to send notifications to.
     * @param messages The email Message queue.
     * @returns TestCoverageStatusEntity or null if no update is available.
     * @throws IOException
     */
    public static TestCoverageStatusEntity getTestCoverageStatus(
            TestCoverageStatusEntity status,
            Key testRunKey,
            String link,
            List<String> emailAddresses,
            List<Message> messages)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

        String testName = status.testName;

        double previousPct;
        double coveragePct;
        if (status == null || status.totalLineCount <= 0 || status.coveredLineCount < 0) {
            previousPct = 0;
        } else {
            previousPct = ((double) status.coveredLineCount) / status.totalLineCount;
        }

        Entity testRun;
        try {
            testRun = datastore.get(testRunKey);
        } catch (EntityNotFoundException e) {
            logger.log(Level.WARNING, "Test run not found: " + testRunKey);
            return null;
        }

        TestRunEntity testRunEntity = TestRunEntity.fromEntity(testRun);
        if (testRunEntity == null || !testRunEntity.hasCoverage) {
            return null;
        }
        if (testRunEntity.totalLineCount <= 0 || testRunEntity.coveredLineCount < 0) {
            coveragePct = 0;
        } else {
            coveragePct = ((double) testRunEntity.coveredLineCount) / testRunEntity.totalLineCount;
        }

        Set<String> buildIdList = new HashSet<>();
        Query deviceQuery = new Query(DeviceInfoEntity.KIND).setAncestor(testRun.getKey());
        List<DeviceInfoEntity> devices = new ArrayList<>();
        for (Entity device : datastore.prepare(deviceQuery).asIterable()) {
            DeviceInfoEntity deviceEntity = DeviceInfoEntity.fromEntity(device);
            if (deviceEntity == null) {
                continue;
            }
            devices.add(deviceEntity);
            buildIdList.add(deviceEntity.buildId);
        }
        String deviceBuild = StringUtils.join(buildIdList, ", ");
        String footer = EmailHelper.getEmailFooter(testRunEntity, devices, link);

        String subject = null;
        String body = null;
        String subjectSuffix = " @ " + deviceBuild;
        if (coveragePct >= GOOD_THRESHOLD && previousPct < GOOD_THRESHOLD) {
            // Coverage entered the good zone
            subject =
                    "Congratulations! "
                            + testName
                            + " has exceeded "
                            + FORMATTER.format(GOOD_THRESHOLD * 100)
                            + "% coverage"
                            + subjectSuffix;
            body =
                    "Hello,<br><br>The "
                            + testName
                            + " has achieved "
                            + FORMATTER.format(coveragePct * 100)
                            + "% code coverage on device build ID(s): "
                            + deviceBuild
                            + "."
                            + footer;
        } else if (coveragePct < GOOD_THRESHOLD && previousPct >= GOOD_THRESHOLD) {
            // Coverage dropped out of the good zone
            subject =
                    "Warning! "
                            + testName
                            + " has dropped below "
                            + FORMATTER.format(GOOD_THRESHOLD * 100)
                            + "% coverage"
                            + subjectSuffix;
            ;
            body =
                    "Hello,<br><br>The test "
                            + testName
                            + " has dropped to "
                            + FORMATTER.format(coveragePct * 100)
                            + "% code coverage on device build ID(s): "
                            + deviceBuild
                            + "."
                            + footer;
        } else if (coveragePct <= BAD_THRESHOLD && previousPct > BAD_THRESHOLD) {
            // Coverage entered into the bad zone
            subject =
                    "Warning! "
                            + testName
                            + " has dropped below "
                            + FORMATTER.format(BAD_THRESHOLD * 100)
                            + "% coverage"
                            + subjectSuffix;
            body =
                    "Hello,<br><br>The test "
                            + testName
                            + " has dropped to "
                            + FORMATTER.format(coveragePct * 100)
                            + "% code coverage on device build ID(s): "
                            + deviceBuild
                            + "."
                            + footer;
        } else if (coveragePct > BAD_THRESHOLD && previousPct <= BAD_THRESHOLD) {
            // Coverage emerged from the bad zone
            subject =
                    "Congratulations! "
                            + testName
                            + " has exceeded "
                            + FORMATTER.format(BAD_THRESHOLD * 100)
                            + "% coverage"
                            + subjectSuffix;
            body =
                    "Hello,<br><br>The test "
                            + testName
                            + " has achived "
                            + FORMATTER.format(coveragePct * 100)
                            + "% code coverage on device build ID(s): "
                            + deviceBuild
                            + "."
                            + footer;
        } else if (coveragePct - previousPct < -CHANGE_ALERT_THRESHOLD) {
            // Send a coverage drop alert
            subject =
                    "Warning! "
                            + testName
                            + "'s code coverage has decreased by more than "
                            + FORMATTER.format(CHANGE_ALERT_THRESHOLD * 100)
                            + "%"
                            + subjectSuffix;
            body =
                    "Hello,<br><br>The test "
                            + testName
                            + " has dropped from "
                            + FORMATTER.format(previousPct * 100)
                            + "% code coverage to "
                            + FORMATTER.format(coveragePct * 100)
                            + "% code coverage on device build ID(s): "
                            + deviceBuild
                            + "."
                            + footer;
        } else if (coveragePct - previousPct > CHANGE_ALERT_THRESHOLD) {
            // Send a coverage improvement alert
            subject =
                    testName
                            + "'s code coverage has increased by more than "
                            + FORMATTER.format(CHANGE_ALERT_THRESHOLD * 100)
                            + "%"
                            + subjectSuffix;
            body =
                    "Hello,<br><br>The test "
                            + testName
                            + " has increased from "
                            + FORMATTER.format(previousPct * 100)
                            + "% code coverage to "
                            + FORMATTER.format(coveragePct * 100)
                            + "% code coverage on device build ID(s): "
                            + deviceBuild
                            + "."
                            + footer;
        }
        if (subject != null && body != null) {
            try {
                messages.add(EmailHelper.composeEmail(emailAddresses, subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        }
        return new TestCoverageStatusEntity(
                testName,
                testRunEntity.startTimestamp,
                testRunEntity.coveredLineCount,
                testRunEntity.totalLineCount);
    }

    /**
     * Add a task to process coverage data
     *
     * @param testRunKey The key of the test run whose data process.
     */
    public static void addTask(Key testRunKey) {
        Queue queue = QueueFactory.getDefaultQueue();
        String keyString = KeyFactory.keyToString(testRunKey);
        queue.add(
                TaskOptions.Builder.withUrl(COVERAGE_ALERT_URL)
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

        TestCoverageStatusEntity status = null;
        Key statusKey = KeyFactory.createKey(TestCoverageStatusEntity.KIND, testName);
        try {
            status = TestCoverageStatusEntity.fromEntity(datastore.get(statusKey));
        } catch (EntityNotFoundException e) {
            // no existing status
        }
        if (status == null) {
            status = new TestCoverageStatusEntity(testName, 0, -1, -1);
        }

        StringBuffer fullUrl = request.getRequestURL();
        String baseUrl = fullUrl.substring(0, fullUrl.indexOf(request.getRequestURI()));
        String link = baseUrl + "/show_tree?testName=" + testName;
        TestCoverageStatusEntity newStatus;
        List<Message> messageQueue = new ArrayList<>();
        try {
            List<String> emails = EmailHelper.getSubscriberEmails(testRunKey.getParent());
            newStatus = getTestCoverageStatus(status, testRunKey, link, emails, messageQueue);
        } catch (IOException e) {
            logger.log(Level.SEVERE, e.toString());
            return;
        }

        if (newStatus == null) {
            return;
        }
        int retries = 0;
        while (true) {
            Transaction txn = datastore.beginTransaction();
            try {
                try {
                    status = TestCoverageStatusEntity.fromEntity(datastore.get(statusKey));
                } catch (EntityNotFoundException e) {
                    // no status left
                }
                if (status == null || status.timestamp < newStatus.timestamp) {
                    datastore.put(newStatus.toEntity());
                    txn.commit();
                    EmailHelper.sendAll(messageQueue);
                } else {
                    txn.rollback();
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
