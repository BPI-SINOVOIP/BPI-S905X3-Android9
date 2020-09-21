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

import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.media.tv.TvInputManager;
import android.os.AsyncTask;
import com.android.tv.InputSessionManager;
import com.android.tv.MainActivityWrapper;
import com.android.tv.TvSingletons;
import com.android.tv.analytics.Analytics;
import com.android.tv.analytics.Tracker;
import com.android.tv.common.BaseApplication;
import com.android.tv.common.config.api.RemoteConfig;
import com.android.tv.common.experiments.ExperimentLoader;
import com.android.tv.common.recording.RecordingStorageStatusManager;
import com.android.tv.common.util.Clock;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.PreviewDataManager;
import com.android.tv.data.ProgramDataManager;
import com.android.tv.data.epg.EpgFetcher;
import com.android.tv.data.epg.EpgReader;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.DvrScheduleManager;
import com.android.tv.dvr.DvrWatchedPositionManager;
import com.android.tv.dvr.recorder.RecordingScheduler;
import com.android.tv.perf.PerformanceMonitor;
import com.android.tv.perf.StubPerformanceMonitor;
import com.android.tv.testing.dvr.DvrDataManagerInMemoryImpl;
import com.android.tv.testing.testdata.TestData;
import com.android.tv.tuner.TunerInputController;
import com.android.tv.util.SetupUtils;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.account.AccountHelper;
import java.util.concurrent.Executor;
import javax.inject.Provider;

/** Test application for Live TV. */
public class TestSingletonApp extends Application implements TvSingletons {
    public final FakeClock fakeClock = FakeClock.createWithCurrentTime();
    public final FakeEpgReader epgReader = new FakeEpgReader(fakeClock);
    public final FakeRemoteConfig remoteConfig = new FakeRemoteConfig();
    public final FakeEpgFetcher epgFetcher = new FakeEpgFetcher();

    public FakeTvInputManagerHelper tvInputManagerHelper;
    public SetupUtils setupUtils;
    public DvrManager dvrManager;
    public DvrDataManager mDvrDataManager;

    private final Provider<EpgReader> mEpgReaderProvider = SingletonProvider.create(epgReader);
    private TunerInputController mTunerInputController;
    private PerformanceMonitor mPerformanceMonitor;
    private ChannelDataManager mChannelDataManager;

    @Override
    public void onCreate() {
        super.onCreate();
        mTunerInputController =
                new TunerInputController(
                        ComponentName.unflattenFromString(getEmbeddedTunerInputId()));

        tvInputManagerHelper = new FakeTvInputManagerHelper(this);
        setupUtils = SetupUtils.createForTvSingletons(this);
        tvInputManagerHelper.start();
        mChannelDataManager = new ChannelDataManager(this, tvInputManagerHelper);
        mChannelDataManager.start();
        mDvrDataManager = new DvrDataManagerInMemoryImpl(this, fakeClock);
        // HACK reset the singleton for tests
        BaseApplication.sSingletons = this;
    }

    public void loadTestData(TestData testData, long durationMs) {
        tvInputManagerHelper
                .getFakeTvInputManager()
                .add(testData.getTvInputInfo(), TvInputManager.INPUT_STATE_CONNECTED);
        testData.init(this, fakeClock, durationMs);
    }

    @Override
    public Analytics getAnalytics() {
        return null;
    }

    @Override
    public void handleInputCountChanged() {}

    @Override
    public ChannelDataManager getChannelDataManager() {
        return mChannelDataManager;
    }

    @Override
    public boolean isChannelDataManagerLoadFinished() {
        return false;
    }

    @Override
    public ProgramDataManager getProgramDataManager() {
        return null;
    }

    @Override
    public boolean isProgramDataManagerCurrentProgramsLoadFinished() {
        return false;
    }

    @Override
    public PreviewDataManager getPreviewDataManager() {
        return null;
    }

    @Override
    public DvrDataManager getDvrDataManager() {
        return mDvrDataManager;
    }

    @Override
    public DvrScheduleManager getDvrScheduleManager() {
        return null;
    }

    @Override
    public DvrManager getDvrManager() {
        return dvrManager;
    }

    @Override
    public RecordingScheduler getRecordingScheduler() {
        return null;
    }

    @Override
    public DvrWatchedPositionManager getDvrWatchedPositionManager() {
        return null;
    }

    @Override
    public InputSessionManager getInputSessionManager() {
        return null;
    }

    @Override
    public Tracker getTracker() {
        return null;
    }

    @Override
    public TvInputManagerHelper getTvInputManagerHelper() {
        return tvInputManagerHelper;
    }

    @Override
    public Provider<EpgReader> providesEpgReader() {
        return mEpgReaderProvider;
    }

    @Override
    public EpgFetcher getEpgFetcher() {
        return epgFetcher;
    }

    @Override
    public SetupUtils getSetupUtils() {
        return setupUtils;
    }

    @Override
    public TunerInputController getTunerInputController() {
        return mTunerInputController;
    }

    @Override
    public ExperimentLoader getExperimentLoader() {
        return new ExperimentLoader();
    }

    @Override
    public MainActivityWrapper getMainActivityWrapper() {
        return null;
    }

    @Override
    public AccountHelper getAccountHelper() {
        return null;
    }

    @Override
    public Clock getClock() {
        return fakeClock;
    }

    @Override
    public RecordingStorageStatusManager getRecordingStorageStatusManager() {
        return null;
    }

    @Override
    public RemoteConfig getRemoteConfig() {
        return remoteConfig;
    }

    @Override
    public Intent getTunerSetupIntent(Context context) {
        return null;
    }

    @Override
    public boolean isRunningInMainProcess() {
        return false;
    }

    @Override
    public PerformanceMonitor getPerformanceMonitor() {
        if (mPerformanceMonitor == null) {
            mPerformanceMonitor = new StubPerformanceMonitor();
        }
        return mPerformanceMonitor;
    }

    @Override
    public String getEmbeddedTunerInputId() {
        return "com.android.tv/.tuner.tvinput.TunerTvInputService";
    }

    @Override
    public Executor getDbExecutor() {
        return AsyncTask.SERIAL_EXECUTOR;
    }
}
