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

package com.android.server.cts.device.statsd;

import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.content.Context;
import android.os.Handler;
import android.util.Log;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Handles callback from the framework {@link android.app.job.JobScheduler}.
 * Runs a job for 0.5 seconds. Provides a countdown latch to wait on, by the test that schedules it.
 */
@TargetApi(21)
public class StatsdJobService extends JobService {
  private static final String TAG = "AtomTestsJobService";

  JobInfo mRunningJobInfo;
  JobParameters mRunningParams;
  private static CountDownLatch sLatch;

  final Handler mHandler = new Handler();
  final Runnable mWorker = new Runnable() {
    @Override public void run() {
      try {
        Thread.sleep(500);
      } catch (InterruptedException e) {
      }
      jobFinished(mRunningParams, false);
      if (sLatch != null) {
        sLatch.countDown();
      }
    }
  };

  public static synchronized CountDownLatch resetCountDownLatch() {
    sLatch = new CountDownLatch(1);
    return sLatch;
  }

  @Override
  public void onCreate() {
    super.onCreate();
  }

  @Override
  public boolean onStartJob(JobParameters params) {
    mRunningParams = params;
    mHandler.post(mWorker);
    return true;
  }

  @Override
  public boolean onStopJob(JobParameters params) {
    return false;
  }
}