/*
 * Copyright (C) 2014 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.server.job.controllers;

import android.content.Context;
import android.util.proto.ProtoOutputStream;

import com.android.internal.util.IndentingPrintWriter;
import com.android.server.job.JobSchedulerService;
import com.android.server.job.JobSchedulerService.Constants;
import com.android.server.job.StateChangedListener;

import java.util.function.Predicate;

/**
 * Incorporates shared controller logic between the various controllers of the JobManager.
 * These are solely responsible for tracking a list of jobs, and notifying the JM when these
 * are ready to run, or whether they must be stopped.
 */
public abstract class StateController {
    protected final JobSchedulerService mService;
    protected final StateChangedListener mStateChangedListener;
    protected final Context mContext;
    protected final Object mLock;
    protected final Constants mConstants;

    StateController(JobSchedulerService service) {
        mService = service;
        mStateChangedListener = service;
        mContext = service.getTestableContext();
        mLock = service.getLock();
        mConstants = service.getConstants();
    }

    /**
     * Implement the logic here to decide whether a job should be tracked by this controller.
     * This logic is put here so the JobManager can be completely agnostic of Controller logic.
     * Also called when updating a task, so implementing controllers have to be aware of
     * preexisting tasks.
     */
    public abstract void maybeStartTrackingJobLocked(JobStatus jobStatus, JobStatus lastJob);
    /**
     * Optionally implement logic here to prepare the job to be executed.
     */
    public void prepareForExecutionLocked(JobStatus jobStatus) {
    }
    /**
     * Remove task - this will happen if the task is cancelled, completed, etc.
     */
    public abstract void maybeStopTrackingJobLocked(JobStatus jobStatus, JobStatus incomingJob,
            boolean forUpdate);
    /**
     * Called when a new job is being created to reschedule an old failed job.
     */
    public void rescheduleForFailureLocked(JobStatus newJob, JobStatus failureToReschedule) {
    }

    public abstract void dumpControllerStateLocked(IndentingPrintWriter pw,
            Predicate<JobStatus> predicate);
    public abstract void dumpControllerStateLocked(ProtoOutputStream proto, long fieldId,
            Predicate<JobStatus> predicate);
}
