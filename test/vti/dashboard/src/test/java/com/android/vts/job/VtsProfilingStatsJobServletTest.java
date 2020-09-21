/*
 * Copyright (C) 2017 The Android Open Source Project
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.ProfilingPointEntity;
import com.android.vts.entity.ProfilingPointRunEntity;
import com.android.vts.entity.ProfilingPointSummaryEntity;
import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.proto.VtsReportMessage;
import com.android.vts.util.StatSummary;
import com.android.vts.util.TimeUtil;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.EntityNotFoundException;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.taskqueue.dev.LocalTaskQueue;
import com.google.appengine.api.taskqueue.dev.QueueStateInfo;
import com.google.appengine.tools.development.testing.LocalDatastoreServiceTestConfig;
import com.google.appengine.tools.development.testing.LocalServiceTestHelper;
import com.google.appengine.tools.development.testing.LocalTaskQueueTestConfig;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.Month;
import java.time.ZonedDateTime;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import org.apache.commons.math3.stat.descriptive.moment.Mean;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class VtsProfilingStatsJobServletTest {
    private final LocalServiceTestHelper helper =
            new LocalServiceTestHelper(
                    new LocalDatastoreServiceTestConfig(),
                    new LocalTaskQueueTestConfig()
                            .setQueueXmlPath("src/main/webapp/WEB-INF/queue.xml"));
    private static final double THRESHOLD = 1e-10;

    @Before
    public void setUp() {
        helper.setUp();
    }

    @After
    public void tearDown() {
        helper.tearDown();
    }

    private static void createProfilingRun() {
        Date d = new Date();
        long time = TimeUnit.MILLISECONDS.toMicros(d.getTime());
        long canonicalTime = VtsProfilingStatsJobServlet.getCanonicalTime(time);
        String test = "test";
        String profilingPointName = "profilingPoint";
        String xLabel = "xLabel";
        String yLabel = "yLabel";
        VtsReportMessage.VtsProfilingType type =
                VtsReportMessage.VtsProfilingType.VTS_PROFILING_TYPE_UNLABELED_VECTOR;
        VtsReportMessage.VtsProfilingRegressionMode mode =
                VtsReportMessage.VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;

        Key testKey = KeyFactory.createKey(TestEntity.KIND, test);
        Key testRunKey = KeyFactory.createKey(testKey, TestRunEntity.KIND, time);
        Long[] valueArray = new Long[] {1l, 2l, 3l, 4l, 5l};
        StatSummary stats =
                new StatSummary(
                        "expected",
                        VtsReportMessage.VtsProfilingRegressionMode.UNKNOWN_REGRESSION_MODE);
        for (long value : valueArray) {
            stats.updateStats(value);
        }
        Mean mean = new Mean();
        List<Long> values = Arrays.asList(valueArray);
        ProfilingPointRunEntity profilingPointRunEntity =
                new ProfilingPointRunEntity(
                        testRunKey,
                        profilingPointName,
                        type.getNumber(),
                        mode.getNumber(),
                        null,
                        values,
                        xLabel,
                        yLabel,
                        null);

        String branch = "master";
        String product = "product";
        String flavor = "flavor";
        String id = "12345";
        String bitness = "64";
        String abiName = "abi";
        DeviceInfoEntity device =
                new DeviceInfoEntity(testRunKey, branch, product, flavor, id, bitness, abiName);
    }

    /**
     * Test that tasks are correctly scheduled on the queue.
     *
     * @throws InterruptedException
     */
    @Test
    public void testTasksScheduled() throws InterruptedException {
        String[] testNames = new String[] {"test1", "test2", "test3"};
        List<Key> testKeys = new ArrayList();
        Set<Key> testKeySet = new HashSet<>();
        String kind = "TEST";
        for (String testName : testNames) {
            Key key = KeyFactory.createKey(kind, testName);
            testKeys.add(key);
            testKeySet.add(key);
        }
        VtsProfilingStatsJobServlet.addTasks(testKeys);
        Thread.sleep(1000); // wait one second (tasks are scheduled asychronously), must wait.
        LocalTaskQueue taskQueue = LocalTaskQueueTestConfig.getLocalTaskQueue();
        QueueStateInfo qsi = taskQueue.getQueueStateInfo().get(VtsProfilingStatsJobServlet.QUEUE);
        assertNotNull(qsi);
        assertEquals(testNames.length, qsi.getTaskInfo().size());

        int i = 0;
        for (QueueStateInfo.TaskStateInfo taskStateInfo : qsi.getTaskInfo()) {
            assertEquals(
                    VtsProfilingStatsJobServlet.PROFILING_STATS_JOB_URL, taskStateInfo.getUrl());
            assertEquals("POST", taskStateInfo.getMethod());
            String body = taskStateInfo.getBody();
            String[] parts = body.split("=");
            assertEquals(2, parts.length);
            assertEquals(VtsProfilingStatsJobServlet.PROFILING_POINT_KEY, parts[0]);
            String keyString = parts[1];
            Key profilingPointRunKey;
            try {
                profilingPointRunKey = KeyFactory.stringToKey(keyString);
            } catch (IllegalArgumentException e) {
                fail();
                return;
            }
            assertTrue(testKeys.contains(profilingPointRunKey));
        }
    }

    /** Test that canonical time is correctly derived from a timestamp in the middle of the day. */
    @Test
    public void testCanonicalTimeMidday() {
        int year = 2017;
        Month month = Month.MAY;
        int day = 28;
        int hour = 14;
        int minute = 30;
        LocalDateTime now = LocalDateTime.of(year, month.getValue(), day, hour, minute);
        ZonedDateTime zdt = ZonedDateTime.of(now, TimeUtil.PT_ZONE);
        long time = TimeUnit.SECONDS.toMicros(zdt.toEpochSecond());
        long canonicalTime = VtsProfilingStatsJobServlet.getCanonicalTime(time);
        long canonicalTimeSec = TimeUnit.MICROSECONDS.toSeconds(canonicalTime);
        ZonedDateTime canonical =
                ZonedDateTime.ofInstant(Instant.ofEpochSecond(canonicalTimeSec), TimeUtil.PT_ZONE);
        assertEquals(month, canonical.getMonth());
        assertEquals(day, canonical.getDayOfMonth());
        assertEquals(0, canonical.getHour());
        assertEquals(0, canonical.getMinute());
    }

    /** Test that canonical time is correctly derived at the boundary of two days (midnight). */
    @Test
    public void testCanonicalTimeMidnight() {
        int year = 2017;
        Month month = Month.MAY;
        int day = 28;
        int hour = 0;
        int minute = 0;
        LocalDateTime now = LocalDateTime.of(year, month.getValue(), day, hour, minute);
        ZonedDateTime zdt = ZonedDateTime.of(now, TimeUtil.PT_ZONE);
        long time = TimeUnit.SECONDS.toMicros(zdt.toEpochSecond());
        long canonicalTime = VtsProfilingStatsJobServlet.getCanonicalTime(time);
        long canonicalTimeSec = TimeUnit.MICROSECONDS.toSeconds(canonicalTime);
        ZonedDateTime canonical =
                ZonedDateTime.ofInstant(Instant.ofEpochSecond(canonicalTimeSec), TimeUtil.PT_ZONE);
        assertEquals(zdt, canonical);
    }

    /** Test that new summaries are created with a clean database. */
    @Test
    public void testNewSummary() {
        Date d = new Date();
        long time = TimeUnit.MILLISECONDS.toMicros(d.getTime());
        String test = "test";

        Key testKey = KeyFactory.createKey(TestEntity.KIND, test);
        Key testRunKey = KeyFactory.createKey(testKey, TestRunEntity.KIND, time);
        Long[] valueArray = new Long[] {1l, 2l, 3l, 4l, 5l};
        StatSummary expected =
                new StatSummary(
                        "expected",
                        VtsReportMessage.VtsProfilingRegressionMode.UNKNOWN_REGRESSION_MODE);
        for (long value : valueArray) {
            expected.updateStats(value);
        }
        Mean mean = new Mean();
        List<Long> values = Arrays.asList(valueArray);
        ProfilingPointRunEntity profilingPointRunEntity =
                new ProfilingPointRunEntity(
                        testRunKey,
                        "profilingPoint",
                        VtsReportMessage.VtsProfilingType.VTS_PROFILING_TYPE_UNLABELED_VECTOR_VALUE,
                        VtsReportMessage.VtsProfilingRegressionMode
                                .VTS_REGRESSION_MODE_INCREASING_VALUE,
                        null,
                        values,
                        "xLabel",
                        "yLabel",
                        null);

        DeviceInfoEntity device =
                new DeviceInfoEntity(
                        testRunKey, "master", "product", "flavor", "12345", "64", "abi");

        List<DeviceInfoEntity> devices = new ArrayList<>();
        devices.add(device);

        boolean result =
                VtsProfilingStatsJobServlet.updateSummaries(
                        testKey, profilingPointRunEntity, devices, time);
        assertTrue(result);

        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

        // Check profiling point entity
        Key profilingPointKey = ProfilingPointEntity.createKey(test, profilingPointRunEntity.name);
        ProfilingPointEntity profilingPointEntity = null;
        try {
            Entity profilingPoint = datastore.get(profilingPointKey);
            profilingPointEntity = ProfilingPointEntity.fromEntity(profilingPoint);
        } catch (EntityNotFoundException exception) {
            fail();
        }
        assertNotNull(profilingPointEntity);
        assertEquals(profilingPointRunEntity.name, profilingPointEntity.profilingPointName);
        assertEquals(profilingPointRunEntity.xLabel, profilingPointEntity.xLabel);
        assertEquals(profilingPointRunEntity.yLabel, profilingPointEntity.yLabel);
        assertEquals(profilingPointRunEntity.type, profilingPointEntity.type);
        assertEquals(profilingPointRunEntity.regressionMode, profilingPointEntity.regressionMode);

        // Check all summary entities
        Query q = new Query(ProfilingPointSummaryEntity.KIND).setAncestor(profilingPointKey);
        for (Entity e : datastore.prepare(q).asIterable()) {
            ProfilingPointSummaryEntity pps = ProfilingPointSummaryEntity.fromEntity(e);
            assertNotNull(pps);
            assertTrue(
                    pps.branch.equals(device.branch)
                            || pps.branch.equals(ProfilingPointSummaryEntity.ALL));
            assertTrue(
                    pps.buildFlavor.equals(ProfilingPointSummaryEntity.ALL)
                            || pps.buildFlavor.equals(device.buildFlavor));
            assertEquals(expected.getCount(), pps.globalStats.getCount());
            assertEquals(expected.getMax(), pps.globalStats.getMax(), THRESHOLD);
            assertEquals(expected.getMin(), pps.globalStats.getMin(), THRESHOLD);
            assertEquals(expected.getMean(), pps.globalStats.getMean(), THRESHOLD);
            assertEquals(expected.getSumSq(), pps.globalStats.getSumSq(), THRESHOLD);
        }
    }

    /** Test that existing summaries are updated correctly when a job pushes new profiling data. */
    @Test
    public void testUpdateSummary() {
        Date d = new Date();
        long time = TimeUnit.MILLISECONDS.toMicros(d.getTime());
        String test = "test2";

        Key testKey = KeyFactory.createKey(TestEntity.KIND, test);
        Key testRunKey = KeyFactory.createKey(testKey, TestRunEntity.KIND, time);
        Long[] valueArray = new Long[] {0l};
        List<Long> values = Arrays.asList(valueArray);

        // Create a new profiling point run
        ProfilingPointRunEntity profilingPointRunEntity =
                new ProfilingPointRunEntity(
                        testRunKey,
                        "profilingPoint2",
                        VtsReportMessage.VtsProfilingType.VTS_PROFILING_TYPE_UNLABELED_VECTOR_VALUE,
                        VtsReportMessage.VtsProfilingRegressionMode
                                .VTS_REGRESSION_MODE_INCREASING_VALUE,
                        null,
                        values,
                        "xLabel",
                        "yLabel",
                        null);

        // Create a device for the run
        String series = "";
        DeviceInfoEntity device =
                new DeviceInfoEntity(
                        testRunKey, "master", "product", "flavor", "12345", "64", "abi");

        List<DeviceInfoEntity> devices = new ArrayList<>();
        devices.add(device);

        // Create the existing stats
        Key profilingPointKey = ProfilingPointEntity.createKey(test, profilingPointRunEntity.name);
        StatSummary expected =
                new StatSummary(
                        "label",
                        0,
                        10,
                        5,
                        100,
                        10,
                        VtsReportMessage.VtsProfilingRegressionMode.UNKNOWN_REGRESSION_MODE);
        ProfilingPointSummaryEntity summary =
                new ProfilingPointSummaryEntity(
                        profilingPointKey,
                        expected,
                        new ArrayList<>(),
                        new HashMap<>(),
                        device.branch,
                        device.buildFlavor,
                        series,
                        time);

        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        datastore.put(summary.toEntity());

        // Update the summaries in the database
        boolean result =
                VtsProfilingStatsJobServlet.updateSummaries(
                        testKey, profilingPointRunEntity, devices, time);
        assertTrue(result);

        // Calculate the expected stats with the values from the new run
        for (long value : values) expected.updateStats(value);

        // Get the summary and check the values match what is expected
        Key summaryKey =
                ProfilingPointSummaryEntity.createKey(
                        profilingPointKey, device.branch, device.buildFlavor, series, time);
        ProfilingPointSummaryEntity pps = null;
        try {
            Entity e = datastore.get(summaryKey);
            pps = ProfilingPointSummaryEntity.fromEntity(e);
        } catch (EntityNotFoundException e) {
            fail();
        }
        assertNotNull(pps);
        assertTrue(pps.branch.equals(device.branch));
        assertTrue(pps.buildFlavor.equals(device.buildFlavor));
        assertEquals(expected.getCount(), pps.globalStats.getCount());
        assertEquals(expected.getMax(), pps.globalStats.getMax(), THRESHOLD);
        assertEquals(expected.getMin(), pps.globalStats.getMin(), THRESHOLD);
        assertEquals(expected.getMean(), pps.globalStats.getMean(), THRESHOLD);
        assertEquals(expected.getSumSq(), pps.globalStats.getSumSq(), THRESHOLD);
    }
}
