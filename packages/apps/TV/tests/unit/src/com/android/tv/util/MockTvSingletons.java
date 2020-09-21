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

package com.android.tv.util;

import android.content.Context;
import android.content.Intent;
import com.android.tv.InputSessionManager;
import com.android.tv.MainActivityWrapper;
import com.android.tv.TvApplication;
import com.android.tv.TvSingletons;
import com.android.tv.analytics.Analytics;
import com.android.tv.analytics.Tracker;
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
import com.android.tv.testing.FakeClock;
import com.android.tv.tuner.TunerInputController;
import java.util.concurrent.Executor;
import javax.inject.Provider;

/** Mock {@link TvSingletons} class. */
public class MockTvSingletons implements TvSingletons {
    public final FakeClock fakeClock = FakeClock.createWithCurrentTime();

    private final TvApplication mApp;
    private PerformanceMonitor mPerformanceMonitor;

    public MockTvSingletons(Context context) {
        mApp = (TvApplication) context.getApplicationContext();
    }

    @Override
    public Analytics getAnalytics() {
        return mApp.getAnalytics();
    }

    @Override
    public void handleInputCountChanged() {}

    @Override
    public ChannelDataManager getChannelDataManager() {
        return mApp.getChannelDataManager();
    }

    @Override
    public boolean isChannelDataManagerLoadFinished() {
        return mApp.isChannelDataManagerLoadFinished();
    }

    @Override
    public ProgramDataManager getProgramDataManager() {
        return mApp.getProgramDataManager();
    }

    @Override
    public boolean isProgramDataManagerCurrentProgramsLoadFinished() {
        return mApp.isProgramDataManagerCurrentProgramsLoadFinished();
    }

    @Override
    public PreviewDataManager getPreviewDataManager() {
        return mApp.getPreviewDataManager();
    }

    @Override
    public DvrDataManager getDvrDataManager() {
        return mApp.getDvrDataManager();
    }

    @Override
    public Clock getClock() {
        return fakeClock;
    }

    @Override
    public RecordingStorageStatusManager getRecordingStorageStatusManager() {
        return mApp.getRecordingStorageStatusManager();
    }

    @Override
    public DvrScheduleManager getDvrScheduleManager() {
        return mApp.getDvrScheduleManager();
    }

    @Override
    public DvrManager getDvrManager() {
        return mApp.getDvrManager();
    }

    @Override
    public RecordingScheduler getRecordingScheduler() {
        return mApp.getRecordingScheduler();
    }

    @Override
    public DvrWatchedPositionManager getDvrWatchedPositionManager() {
        return mApp.getDvrWatchedPositionManager();
    }

    @Override
    public InputSessionManager getInputSessionManager() {
        return mApp.getInputSessionManager();
    }

    @Override
    public Tracker getTracker() {
        return mApp.getTracker();
    }

    @Override
    public TvInputManagerHelper getTvInputManagerHelper() {
        return mApp.getTvInputManagerHelper();
    }

    @Override
    public Provider<EpgReader> providesEpgReader() {
        return mApp.providesEpgReader();
    }

    @Override
    public EpgFetcher getEpgFetcher() {
        return mApp.getEpgFetcher();
    }

    @Override
    public SetupUtils getSetupUtils() {
        return mApp.getSetupUtils();
    }

    @Override
    public TunerInputController getTunerInputController() {
        return mApp.getTunerInputController();
    }

    @Override
    public ExperimentLoader getExperimentLoader() {
        return mApp.getExperimentLoader();
    }

    @Override
    public MainActivityWrapper getMainActivityWrapper() {
        return mApp.getMainActivityWrapper();
    }

    @Override
    public com.android.tv.util.account.AccountHelper getAccountHelper() {
        return mApp.getAccountHelper();
    }

    @Override
    public RemoteConfig getRemoteConfig() {
        return mApp.getRemoteConfig();
    }

    @Override
    public Intent getTunerSetupIntent(Context context) {
        return mApp.getTunerSetupIntent(context);
    }

    @Override
    public boolean isRunningInMainProcess() {
        return mApp.isRunningInMainProcess();
    }

    @Override
    public PerformanceMonitor getPerformanceMonitor() {
        return mPerformanceMonitor != null ? mPerformanceMonitor : mApp.getPerformanceMonitor();
    }

    public void setPerformanceMonitor(PerformanceMonitor performanceMonitor) {
        mPerformanceMonitor = performanceMonitor;
    }

    @Override
    public String getEmbeddedTunerInputId() {
        return "com.android.tv/.tuner.tvinput.LiveTvTunerTvInputService";
    }

    @Override
    public Executor getDbExecutor() {
        return mApp.getDbExecutor();
    }
}
