/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv;

import android.content.Context;
import com.android.tv.analytics.Analytics;
import com.android.tv.analytics.Tracker;
import com.android.tv.common.BaseApplication;
import com.android.tv.common.BaseSingletons;
import com.android.tv.common.experiments.ExperimentLoader;
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
import com.android.tv.tuner.TunerInputController;
import com.android.tv.util.SetupUtils;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.account.AccountHelper;
import com.android.tv.util.TvClock;
import java.util.concurrent.Executor;
import javax.inject.Provider;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.TvTime;
import com.droidlogic.app.tv.TvControlDataManager;

/** Interface with getters for application scoped singletons. */
public interface TvSingletons extends BaseSingletons {

    /** Returns the @{@link TvSingletons} using the application context. */
    static TvSingletons getSingletons(Context context) {
        return (TvSingletons) BaseApplication.getSingletons(context);
    }

    Analytics getAnalytics();

    void handleInputCountChanged();

    ChannelDataManager getChannelDataManager();

    /**
     * Checks if the {@link ChannelDataManager} instance has been created and all the channels has
     * been loaded.
     */
    boolean isChannelDataManagerLoadFinished();

    ProgramDataManager getProgramDataManager();

    /**
     * Checks if the {@link ProgramDataManager} instance has been created and the current programs
     * for all the channels has been loaded.
     */
    boolean isProgramDataManagerCurrentProgramsLoadFinished();

    PreviewDataManager getPreviewDataManager();

    DvrDataManager getDvrDataManager();

    DvrScheduleManager getDvrScheduleManager();

    DvrManager getDvrManager();

    RecordingScheduler getRecordingScheduler();

    DvrWatchedPositionManager getDvrWatchedPositionManager();

    InputSessionManager getInputSessionManager();

    Tracker getTracker();

    MainActivityWrapper getMainActivityWrapper();

    AccountHelper getAccountHelper();

    boolean isRunningInMainProcess();

    PerformanceMonitor getPerformanceMonitor();

    TvInputManagerHelper getTvInputManagerHelper();

    Provider<EpgReader> providesEpgReader();

    EpgFetcher getEpgFetcher();

    SetupUtils getSetupUtils();

    TunerInputController getTunerInputController();

    ExperimentLoader getExperimentLoader();

    Executor getDbExecutor();
    SystemControlManager getSystemControlManager();

    TvTime getTvTime();

    TvControlDataManager getTvControlDataManager();

    TvClock getTvClock();
}
