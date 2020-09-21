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
 * limitations under the License
 */

package com.android.tv.testing;

import android.app.job.JobParameters;
import android.app.job.JobService;
import com.android.tv.data.epg.EpgFetcher;

/** Fake {@link EpgFetcher} for testing. */
public class FakeEpgFetcher implements EpgFetcher {
    public boolean fetchStarted = false;

    @Override
    public void startRoutineService() {}

    @Override
    public void fetchImmediatelyIfNeeded() {}

    @Override
    public void fetchImmediately() {
        fetchStarted = true;
    }

    @Override
    public void onChannelScanStarted() {}

    @Override
    public void onChannelScanFinished() {}

    @Override
    public boolean executeFetchTaskIfPossible(JobService jobService, JobParameters params) {
        return false;
    }

    @Override
    public void stopFetchingJob() {
        fetchStarted = false;
    }

    public void reset() {
        fetchStarted = false;
    }
}
