/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tv.data.epg;

import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.support.annotation.MainThread;

/** Fetch EPG routinely or on-demand during channel scanning */
public interface EpgFetcher {

    /**
     * Starts the routine service of EPG fetching. It use {@link JobScheduler} to schedule the EPG
     * fetching routine. The EPG fetching routine will be started roughly every 4 hours, unless the
     * channel scanning of tuner input is started.
     */
    @MainThread
    void startRoutineService();

    /**
     * Fetches EPG immediately if current EPG data are out-dated, i.e., not successfully updated by
     * routine fetching service due to various reasons.
     */
    @MainThread
    void fetchImmediatelyIfNeeded();

    /** Fetches EPG immediately. */
    @MainThread
    void fetchImmediately();

    /** Notifies EPG fetch service that channel scanning is started. */
    @MainThread
    void onChannelScanStarted();

    /** Notifies EPG fetch service that channel scanning is finished. */
    @MainThread
    void onChannelScanFinished();

    @MainThread
    boolean executeFetchTaskIfPossible(JobService jobService, JobParameters params);

    @MainThread
    void stopFetchingJob();
}
