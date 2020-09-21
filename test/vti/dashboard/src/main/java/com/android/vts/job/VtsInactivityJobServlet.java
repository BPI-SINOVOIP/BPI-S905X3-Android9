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

import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.entity.TestStatusEntity;
import com.android.vts.util.EmailHelper;
import com.android.vts.util.FilterUtil;
import com.android.vts.util.TaskQueueHelper;
import com.android.vts.util.TimeUtil;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.EntityNotFoundException;
import com.google.appengine.api.datastore.FetchOptions;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.appengine.api.datastore.Query.SortDirection;
import com.google.appengine.api.taskqueue.Queue;
import com.google.appengine.api.taskqueue.QueueFactory;
import com.google.appengine.api.taskqueue.TaskOptions;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Test inactivity notification job. */
public class VtsInactivityJobServlet extends HttpServlet {
    private static final String INACTIVITY_ALERT_URL = "/cron/vts_inactivity_job";
    protected static final Logger logger =
            Logger.getLogger(VtsInactivityJobServlet.class.getName());

    /**
     * Compose an email if the test is inactive.
     *
     * @param test The TestStatusEntity document storing the test status.
     * @param lastRunTime The timestamp in microseconds of the last test run for this test.
     * @param link Fully specified link to the test's status page.
     * @param emails The list of email addresses to send the email.
     * @param messages The message list in which to insert the inactivity notification email.
     * @return True if the test is inactive, false otherwise.
     */
    private static boolean notifyIfInactive(
            TestStatusEntity test,
            long lastRunTime,
            String link,
            List<String> emails,
            List<Message> messages) {
        long now = TimeUnit.MILLISECONDS.toMicros(System.currentTimeMillis());
        long diff = now - lastRunTime;
        // Send an email daily to notify that the test hasn't been running.
        // After 7 full days have passed, notifications will no longer be sent (i.e. the
        // test is assumed to be deprecated).
        if (diff >= TimeUnit.DAYS.toMicros(1) && diff < TimeUnit.DAYS.toMicros(8)) {
            String uploadTimeString = TimeUtil.getDateTimeString(lastRunTime);
            String subject = "Warning! Inactive test: " + test.testName;
            String body =
                    "Hello,<br><br>Test \""
                            + test.testName
                            + "\" is inactive. "
                            + "No new data has been uploaded since "
                            + uploadTimeString
                            + "."
                            + EmailHelper.getEmailFooter(null, null, link);
            try {
                messages.add(EmailHelper.composeEmail(emails, subject, body));
                return true;
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        }
        return false;
    }

    /**
     * Get the timestamp for the last test run for the specified test.
     *
     * @param testKey The parent key of the test runs to query for.
     * @return The timestamp in microseconds of the last test run for the test, or -1 if none.
     */
    private static long getLastRunTime(Key testKey) {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Filter testTypeFilter = FilterUtil.getTestTypeFilter(false, true, false);
        Query q =
                new Query(TestRunEntity.KIND)
                        .setAncestor(testKey)
                        .setFilter(testTypeFilter)
                        .addSort(Entity.KEY_RESERVED_PROPERTY, SortDirection.DESCENDING)
                        .setKeysOnly();

        long lastTestRun = -1;
        for (Entity testRun : datastore.prepare(q).asIterable(FetchOptions.Builder.withLimit(1))) {
            lastTestRun = testRun.getKey().getId();
        }
        return lastTestRun;
    }

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Queue queue = QueueFactory.getDefaultQueue();
        Query q = new Query(TestStatusEntity.KIND).setKeysOnly();
        List<TaskOptions> tasks = new ArrayList<>();
        for (Entity status : datastore.prepare(q).asIterable()) {
            TaskOptions task =
                    TaskOptions.Builder.withUrl(INACTIVITY_ALERT_URL)
                            .param("statusKey", KeyFactory.keyToString(status.getKey()))
                            .method(TaskOptions.Method.POST);
            tasks.add(task);
        }
        TaskQueueHelper.addToQueue(queue, tasks);
    }

    @Override
    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        String statusKeyString = request.getParameter("statusKey");

        Key statusKey;
        try {
            statusKey = KeyFactory.stringToKey(statusKeyString);
        } catch (IllegalArgumentException e) {
            logger.log(Level.WARNING, "Invalid key specified: " + statusKeyString);
            return;
        }

        TestStatusEntity status = null;
        try {
            status = TestStatusEntity.fromEntity(datastore.get(statusKey));
        } catch (EntityNotFoundException e) {
            // no existing status
        }
        if (status == null) {
            return;
        }
        Key testKey = KeyFactory.createKey(TestEntity.KIND, status.testName);
        long lastRunTime = getLastRunTime(testKey);

        StringBuffer fullUrl = request.getRequestURL();
        String baseUrl = fullUrl.substring(0, fullUrl.indexOf(request.getRequestURI()));
        String link = baseUrl + "/show_tree?testName=" + status.testName;

        List<Message> messageQueue = new ArrayList<>();
        List<String> emails;
        try {
            emails = EmailHelper.getSubscriberEmails(testKey);
        } catch (IOException e) {
            logger.log(Level.SEVERE, e.toString());
            return;
        }
        notifyIfInactive(status, lastRunTime, link, emails, messageQueue);
        if (messageQueue.size() > 0) {
            EmailHelper.sendAll(messageQueue);
        }
    }
}
