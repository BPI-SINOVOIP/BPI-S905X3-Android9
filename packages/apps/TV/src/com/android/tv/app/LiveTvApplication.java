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

package com.android.tv.app;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.media.tv.TvContract;
import com.android.tv.TvApplication;
import com.android.tv.analytics.Analytics;
import com.android.tv.analytics.StubAnalytics;
import com.android.tv.analytics.Tracker;
import com.android.tv.common.CommonConstants;
import com.android.tv.common.actions.InputSetupActionUtils;
import com.android.tv.common.config.DefaultConfigManager;
import com.android.tv.common.config.api.RemoteConfig;
import com.android.tv.common.experiments.ExperimentLoader;
import com.android.tv.common.util.CommonUtils;
import com.android.tv.data.epg.EpgReader;
import com.android.tv.data.epg.StubEpgReader;
import com.android.tv.perf.PerformanceMonitor;
import com.android.tv.perf.StubPerformanceMonitor;
import com.android.tv.tuner.livetuner.LiveTvTunerTvInputService;
import com.android.tv.tuner.setup.LiveTvTunerSetupActivity;
import com.android.tv.util.account.AccountHelper;
import com.android.tv.util.account.AccountHelperImpl;
import javax.inject.Provider;

/** The top level application for Live TV. */
public class LiveTvApplication extends TvApplication {
    protected static final String TV_ACTIVITY_CLASS_NAME =
            CommonConstants.BASE_PACKAGE + ".TvActivity";

    private final StubPerformanceMonitor performanceMonitor = new StubPerformanceMonitor();
    private final Provider<EpgReader> mEpgReaderProvider =
            new Provider<EpgReader>() {

                @Override
                public EpgReader get() {
                    return new StubEpgReader(LiveTvApplication.this);
                }
            };

    private AccountHelper mAccountHelper;
    private Analytics mAnalytics;
    private Tracker mTracker;
    private String mEmbeddedInputId;
    private RemoteConfig mRemoteConfig;
    private ExperimentLoader mExperimentLoader;

    /** Returns the {@link AccountHelperImpl}. */
    @Override
    public AccountHelper getAccountHelper() {
        if (mAccountHelper == null) {
            mAccountHelper = new AccountHelperImpl(getApplicationContext());
        }
        return mAccountHelper;
    }

    @Override
    public synchronized PerformanceMonitor getPerformanceMonitor() {
        return performanceMonitor;
    }

    @Override
    public Provider<EpgReader> providesEpgReader() {
        return mEpgReaderProvider;
    }

    @Override
    public ExperimentLoader getExperimentLoader() {
        mExperimentLoader = new ExperimentLoader();
        return mExperimentLoader;
    }

    /** Returns the {@link Analytics}. */
    @Override
    public synchronized Analytics getAnalytics() {
        if (mAnalytics == null) {
            mAnalytics = StubAnalytics.getInstance(this);
        }
        return mAnalytics;
    }

    /** Returns the default tracker. */
    @Override
    public synchronized Tracker getTracker() {
        if (mTracker == null) {
            mTracker = getAnalytics().getDefaultTracker();
        }
        return mTracker;
    }

    @Override
    public Intent getTunerSetupIntent(Context context) {
        // Make an intent to launch the setup activity of TV tuner input.
        Intent intent =
                CommonUtils.createSetupIntent(
                        new Intent(context, LiveTvTunerSetupActivity.class), mEmbeddedInputId);
        intent.putExtra(InputSetupActionUtils.EXTRA_INPUT_ID, mEmbeddedInputId);
        Intent tvActivityIntent = new Intent();
        tvActivityIntent.setComponent(new ComponentName(context, TV_ACTIVITY_CLASS_NAME));
        intent.putExtra(InputSetupActionUtils.EXTRA_ACTIVITY_AFTER_COMPLETION, tvActivityIntent);
        return intent;
    }

    @Override
    public synchronized String getEmbeddedTunerInputId() {
        if (mEmbeddedInputId == null) {
            mEmbeddedInputId =
                    TvContract.buildInputId(
                            new ComponentName(this, LiveTvTunerTvInputService.class));
        }
        return mEmbeddedInputId;
    }

    @Override
    public RemoteConfig getRemoteConfig() {
        if (mRemoteConfig == null) {
            // No need to synchronize this, it does not hurt to create two and throw one away.
            mRemoteConfig = DefaultConfigManager.createInstance(this).getRemoteConfig();
        }
        return mRemoteConfig;
    }
}
