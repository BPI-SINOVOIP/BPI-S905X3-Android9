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

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.ProfilingPointEntity;
import com.android.vts.entity.ProfilingPointRunEntity;
import com.android.vts.entity.ProfilingPointSummaryEntity;
import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.PerformanceUtil;
import com.android.vts.util.TaskQueueHelper;
import com.android.vts.util.TimeUtil;
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
import java.time.Instant;
import java.time.ZonedDateTime;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.ConcurrentModificationException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Represents the notifications service which is automatically called on a fixed schedule. */
public class VtsProfilingStatsJobServlet extends HttpServlet {
    protected static final Logger logger =
            Logger.getLogger(VtsProfilingStatsJobServlet.class.getName());
    private static final String HIDL_HAL_OPTION = "hidl_hal_mode";
    private static final String[] splitKeysArray = new String[] {HIDL_HAL_OPTION};
    private static final Set<String> splitKeySet = new HashSet<>(Arrays.asList(splitKeysArray));

    public static final String PROFILING_STATS_JOB_URL = "/task/vts_profiling_stats_job";
    public static final String PROFILING_POINT_KEY = "profilingPointKey";
    public static final String QUEUE = "profilingStatsQueue";

    /**
     * Round the date down to the start of the day (PST).
     *
     * @param time The time in microseconds.
     * @return
     */
    public static long getCanonicalTime(long time) {
        long timeMillis = TimeUnit.MICROSECONDS.toMillis(time);
        ZonedDateTime zdt =
                ZonedDateTime.ofInstant(Instant.ofEpochMilli(timeMillis), TimeUtil.PT_ZONE);
        return TimeUnit.SECONDS.toMicros(
                zdt.withHour(0).withMinute(0).withSecond(0).toEpochSecond());
    }

    /**
     * Add tasks to process profiling run data
     *
     * @param profilingPointKeys The list of keys of the profiling point runs whose data process.
     */
    public static void addTasks(List<Key> profilingPointKeys) {
        Queue queue = QueueFactory.getQueue(QUEUE);
        List<TaskOptions> tasks = new ArrayList<>();
        for (Key key : profilingPointKeys) {
            String keyString = KeyFactory.keyToString(key);
            tasks.add(
                    TaskOptions.Builder.withUrl(PROFILING_STATS_JOB_URL)
                            .param(PROFILING_POINT_KEY, keyString)
                            .method(TaskOptions.Method.POST));
        }
        TaskQueueHelper.addToQueue(queue, tasks);
    }

    /**
     * Update the profiling summaries with the information from a profiling point run.
     *
     * @param testKey The key to the TestEntity whose profiling data to analyze.
     * @param profilingPointRun The profiling data to analyze.
     * @param devices The list of devices used in the profiling run.
     * @param time The canonical timestamp of the summary to update.
     * @return true if the update succeeds, false otherwise.
     */
    public static boolean updateSummaries(
            Key testKey,
            ProfilingPointRunEntity profilingPointRun,
            List<DeviceInfoEntity> devices,
            long time) {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Transaction tx = datastore.beginTransaction();
        try {
            List<Entity> puts = new ArrayList<>();

            ProfilingPointEntity profilingPoint =
                    new ProfilingPointEntity(
                            testKey.getName(),
                            profilingPointRun.name,
                            profilingPointRun.type.getNumber(),
                            profilingPointRun.regressionMode.getNumber(),
                            profilingPointRun.xLabel,
                            profilingPointRun.yLabel);
            puts.add(profilingPoint.toEntity());

            String option = PerformanceUtil.getOptionAlias(profilingPointRun, splitKeySet);

            Set<String> branches = new HashSet<>();
            Set<String> deviceNames = new HashSet<>();

            branches.add(ProfilingPointSummaryEntity.ALL);
            deviceNames.add(ProfilingPointSummaryEntity.ALL);

            for (DeviceInfoEntity d : devices) {
                branches.add(d.branch);
                deviceNames.add(d.buildFlavor);
            }

            List<Key> summaryGets = new ArrayList<>();
            for (String branch : branches) {
                for (String device : deviceNames) {
                    summaryGets.add(
                            ProfilingPointSummaryEntity.createKey(
                                    profilingPoint.key, branch, device, option, time));
                }
            }

            Map<Key, Entity> summaries = datastore.get(tx, summaryGets);
            Map<String, Map<String, ProfilingPointSummaryEntity>> summaryMap = new HashMap<>();
            for (Key key : summaries.keySet()) {
                Entity e = summaries.get(key);
                ProfilingPointSummaryEntity profilingPointSummary =
                        ProfilingPointSummaryEntity.fromEntity(e);
                if (profilingPointSummary == null) {
                    logger.log(Level.WARNING, "Invalid profiling point summary: " + e.getKey());
                    continue;
                }
                if (!summaryMap.containsKey(profilingPointSummary.branch)) {
                    summaryMap.put(profilingPointSummary.branch, new HashMap<>());
                }
                Map<String, ProfilingPointSummaryEntity> deviceMap =
                        summaryMap.get(profilingPointSummary.branch);
                deviceMap.put(profilingPointSummary.buildFlavor, profilingPointSummary);
            }

            Set<ProfilingPointSummaryEntity> modifiedEntities = new HashSet<>();

            for (String branch : branches) {
                if (!summaryMap.containsKey(branch)) {
                    summaryMap.put(branch, new HashMap<>());
                }
                Map<String, ProfilingPointSummaryEntity> deviceMap = summaryMap.get(branch);

                for (String device : deviceNames) {
                    ProfilingPointSummaryEntity summary;
                    if (deviceMap.containsKey(device)) {
                        summary = deviceMap.get(device);
                    } else {
                        summary =
                                new ProfilingPointSummaryEntity(
                                        profilingPoint.key, branch, device, option, time);
                        deviceMap.put(device, summary);
                    }
                    summary.update(profilingPointRun);
                    modifiedEntities.add(summary);
                }
            }

            for (ProfilingPointSummaryEntity profilingPointSummary : modifiedEntities) {
                puts.add(profilingPointSummary.toEntity());
            }
            datastore.put(tx, puts);
            tx.commit();
        } catch (ConcurrentModificationException
                | DatastoreFailureException
                | DatastoreTimeoutException e) {
            return false;
        } finally {
            if (tx.isActive()) {
                tx.rollback();
                logger.log(
                        Level.WARNING,
                        "Profiling stats job transaction still active: " + profilingPointRun.key);
                return false;
            }
        }
        return true;
    }

    @Override
    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        String profilingPointKeyString = request.getParameter(PROFILING_POINT_KEY);

        Key profilingPointRunKey;
        try {
            profilingPointRunKey = KeyFactory.stringToKey(profilingPointKeyString);
        } catch (IllegalArgumentException e) {
            logger.log(Level.WARNING, "Invalid key specified: " + profilingPointKeyString);
            return;
        }
        Key testKey = profilingPointRunKey.getParent().getParent();

        ProfilingPointRunEntity profilingPointRun = null;
        try {
            Entity profilingPointRunEntity = datastore.get(profilingPointRunKey);
            profilingPointRun = ProfilingPointRunEntity.fromEntity(profilingPointRunEntity);
        } catch (EntityNotFoundException e) {
            // no run found
        }
        if (profilingPointRun == null) {
            return;
        }

        Query deviceQuery =
                new Query(DeviceInfoEntity.KIND).setAncestor(profilingPointRunKey.getParent());

        List<DeviceInfoEntity> devices = new ArrayList<>();
        for (Entity e : datastore.prepare(deviceQuery).asIterable()) {
            DeviceInfoEntity deviceInfoEntity = DeviceInfoEntity.fromEntity(e);
            if (e == null) continue;
            devices.add(deviceInfoEntity);
        }

        long canonicalTime = getCanonicalTime(profilingPointRunKey.getParent().getId());
        int retryCount = 0;
        while (retryCount++ <= DatastoreHelper.MAX_WRITE_RETRIES) {
            boolean result = updateSummaries(testKey, profilingPointRun, devices, canonicalTime);
            if (!result) {
                logger.log(
                        Level.WARNING, "Retrying profiling stats update: " + profilingPointRunKey);
                continue;
            }
            break;
        }
        if (retryCount > DatastoreHelper.MAX_WRITE_RETRIES) {
            logger.log(Level.SEVERE, "Could not update profiling stats: " + profilingPointRunKey);
        }
    }
}
