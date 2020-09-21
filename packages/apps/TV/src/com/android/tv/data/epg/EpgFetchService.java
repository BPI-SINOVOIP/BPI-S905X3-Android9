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

package com.android.tv.data.epg;

import android.app.job.JobParameters;
import android.app.job.JobService;
import com.android.tv.Starter;
import com.android.tv.TvSingletons;
import com.android.tv.data.ChannelDataManager;

/** JobService to Fetch EPG data. */
public class EpgFetchService extends JobService {
    private EpgFetcher mEpgFetcher;
    private ChannelDataManager mChannelDataManager;

    @Override
    public void onCreate() {
        super.onCreate();
        Starter.start(this);
        TvSingletons tvSingletons = TvSingletons.getSingletons(getApplicationContext());
        mEpgFetcher = tvSingletons.getEpgFetcher();
        mChannelDataManager = tvSingletons.getChannelDataManager();
    }

    @Override
    public boolean onStartJob(JobParameters params) {
        if (!mChannelDataManager.isDbLoadFinished()) {
            mChannelDataManager.addListener(
                    new ChannelDataManager.Listener() {
                        @Override
                        public void onLoadFinished() {
                            mChannelDataManager.removeListener(this);
                            if (!mEpgFetcher.executeFetchTaskIfPossible(
                                    EpgFetchService.this, params)) {
                                jobFinished(params, false);
                            }
                        }

                        @Override
                        public void onChannelListUpdated() {}

                        @Override
                        public void onChannelBrowsableChanged() {}
                    });
            return true;
        } else {
            return mEpgFetcher.executeFetchTaskIfPossible(this, params);
        }
    }

    @Override
    public boolean onStopJob(JobParameters params) {
        mEpgFetcher.stopFetchingJob();
        return false;
    }
}
