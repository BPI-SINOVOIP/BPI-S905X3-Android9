/*
 * Copyright (C) 2016 The Android Open Source Project
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

import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.content.ComponentName;
import android.content.Context;
import android.database.Cursor;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.net.TrafficStats;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.AnyThread;
import android.support.annotation.MainThread;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.support.annotation.WorkerThread;
import android.text.TextUtils;
import android.util.Log;
import com.android.tv.TvFeatures;
import com.android.tv.TvSingletons;
import com.android.tv.common.BuildConfig;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.config.api.RemoteConfigValue;
import com.android.tv.common.util.Clock;
import com.android.tv.common.util.CommonUtils;
import com.android.tv.common.util.LocationUtils;
import com.android.tv.common.util.NetworkTrafficTags;
import com.android.tv.common.util.PermissionUtils;
import com.android.tv.common.util.PostalCodeUtils;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.ChannelImpl;
import com.android.tv.data.ChannelLogoFetcher;
import com.android.tv.data.Lineup;
import com.android.tv.data.Program;
import com.android.tv.data.api.Channel;
import com.android.tv.perf.EventNames;
import com.android.tv.perf.PerformanceMonitor;
import com.android.tv.perf.TimerEvent;
import com.android.tv.util.Utils;
import com.google.android.tv.partner.support.EpgInput;
import com.google.android.tv.partner.support.EpgInputs;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * The service class to fetch EPG routinely or on-demand during channel scanning
 *
 * <p>Since the default executor of {@link AsyncTask} is {@link AsyncTask#SERIAL_EXECUTOR}, only one
 * task can run at a time. Because fetching EPG takes long time, the fetching task shouldn't run on
 * the serial executor. Instead, it should run on the {@link AsyncTask#THREAD_POOL_EXECUTOR}.
 */
public class EpgFetcherImpl implements EpgFetcher {
    private static final String TAG = "EpgFetcherImpl";
    private static final boolean DEBUG = false;

    private static final int EPG_ROUTINELY_FETCHING_JOB_ID = 101;

    private static final long INITIAL_BACKOFF_MS = TimeUnit.SECONDS.toMillis(10);

    @VisibleForTesting static final int REASON_EPG_READER_NOT_READY = 1;
    @VisibleForTesting static final int REASON_LOCATION_INFO_UNAVAILABLE = 2;
    @VisibleForTesting static final int REASON_LOCATION_PERMISSION_NOT_GRANTED = 3;
    @VisibleForTesting static final int REASON_NO_EPG_DATA_RETURNED = 4;
    @VisibleForTesting static final int REASON_NO_NEW_EPG = 5;
    @VisibleForTesting static final int REASON_ERROR = 6;
    @VisibleForTesting static final int REASON_CLOUD_EPG_FAILURE = 7;
    @VisibleForTesting static final int REASON_NO_BUILT_IN_CHANNELS = 8;

    private static final long FETCH_DURING_SCAN_WAIT_TIME_MS = TimeUnit.SECONDS.toMillis(10);

    private static final long FETCH_DURING_SCAN_DURATION_SEC = TimeUnit.HOURS.toSeconds(3);
    private static final long FAST_FETCH_DURATION_SEC = TimeUnit.DAYS.toSeconds(2);

    private static final RemoteConfigValue<Long> ROUTINE_INTERVAL_HOUR =
            RemoteConfigValue.create("live_channels_epg_fetcher_interval_hour", 4);

    private static final int MSG_PREPARE_FETCH_DURING_SCAN = 1;
    private static final int MSG_CHANNEL_UPDATED_DURING_SCAN = 2;
    private static final int MSG_FINISH_FETCH_DURING_SCAN = 3;
    private static final int MSG_RETRY_PREPARE_FETCH_DURING_SCAN = 4;

    private static final int QUERY_CHANNEL_COUNT = 50;
    private static final int MINIMUM_CHANNELS_TO_DECIDE_LINEUP = 3;

    private final Context mContext;
    private final ChannelDataManager mChannelDataManager;
    private final EpgReader mEpgReader;
    private final PerformanceMonitor mPerformanceMonitor;
    private FetchAsyncTask mFetchTask;
    private FetchDuringScanHandler mFetchDuringScanHandler;
    private long mEpgTimeStamp;
    private List<Lineup> mPossibleLineups;
    private final Object mPossibleLineupsLock = new Object();
    private final Object mFetchDuringScanHandlerLock = new Object();
    // A flag to block the re-entrance of onChannelScanStarted and onChannelScanFinished.
    private boolean mScanStarted;

    private final long mRoutineIntervalMs;
    private final long mEpgDataExpiredTimeLimitMs;
    private final long mFastFetchDurationSec;
    private Clock mClock;

    public static EpgFetcher create(Context context) {
        context = context.getApplicationContext();
        TvSingletons tvSingletons = TvSingletons.getSingletons(context);
        ChannelDataManager channelDataManager = tvSingletons.getChannelDataManager();
        PerformanceMonitor performanceMonitor = tvSingletons.getPerformanceMonitor();
        EpgReader epgReader = tvSingletons.providesEpgReader().get();
        Clock clock = tvSingletons.getClock();
        long routineIntervalMs = ROUTINE_INTERVAL_HOUR.get(tvSingletons.getRemoteConfig());

        return new EpgFetcherImpl(
                context,
                channelDataManager,
                epgReader,
                performanceMonitor,
                clock,
                routineIntervalMs);
    }

    @VisibleForTesting
    EpgFetcherImpl(
            Context context,
            ChannelDataManager channelDataManager,
            EpgReader epgReader,
            PerformanceMonitor performanceMonitor,
            Clock clock,
            long routineIntervalMs) {
        mContext = context;
        mChannelDataManager = channelDataManager;
        mEpgReader = epgReader;
        mPerformanceMonitor = performanceMonitor;
        mClock = clock;
        mRoutineIntervalMs =
                routineIntervalMs <= 0
                        ? TimeUnit.HOURS.toMillis(ROUTINE_INTERVAL_HOUR.getDefaultValue())
                        : TimeUnit.HOURS.toMillis(routineIntervalMs);
        mEpgDataExpiredTimeLimitMs = routineIntervalMs * 2;
        mFastFetchDurationSec = FAST_FETCH_DURATION_SEC + routineIntervalMs / 1000;
    }

    private static Set<Channel> getExistingChannelsForMyPackage(Context context) {
        HashSet<Channel> channels = new HashSet<>();
        String selection = null;
        String[] selectionArgs = null;
        String myPackageName = context.getPackageName();
        if (PermissionUtils.hasAccessAllEpg(context)) {
            selection = "package_name=?";
            selectionArgs = new String[] {myPackageName};
        }
        try (Cursor c =
                context.getContentResolver()
                        .query(
                                TvContract.Channels.CONTENT_URI,
                                ChannelImpl.PROJECTION,
                                selection,
                                selectionArgs,
                                null)) {
            if (c != null) {
                while (c.moveToNext()) {
                    Channel channel = ChannelImpl.fromCursor(c);
                    if (DEBUG) Log.d(TAG, "Found " + channel);
                    if (myPackageName.equals(channel.getPackageName())) {
                        channels.add(channel);
                    }
                }
            }
        }
        if (DEBUG)
            Log.d(TAG, "Found " + channels.size() + " channels for package " + myPackageName);
        return channels;
    }

    @Override
    @MainThread
    public void startRoutineService() {
        JobScheduler jobScheduler =
                (JobScheduler) mContext.getSystemService(Context.JOB_SCHEDULER_SERVICE);
        for (JobInfo job : jobScheduler.getAllPendingJobs()) {
            if (job.getId() == EPG_ROUTINELY_FETCHING_JOB_ID) {
                return;
            }
        }
        JobInfo job =
                new JobInfo.Builder(
                                EPG_ROUTINELY_FETCHING_JOB_ID,
                                new ComponentName(mContext, EpgFetchService.class))
                        .setPeriodic(mRoutineIntervalMs)
                        .setBackoffCriteria(INITIAL_BACKOFF_MS, JobInfo.BACKOFF_POLICY_EXPONENTIAL)
                        .setPersisted(true)
                        .build();
        jobScheduler.schedule(job);
        Log.i(TAG, "EPG fetching routine service started.");
    }

    @Override
    @MainThread
    public void fetchImmediatelyIfNeeded() {
        if (CommonUtils.isRunningInTest()) {
            // Do not run EpgFetcher in test.
            return;
        }
        new AsyncTask<Void, Void, Long>() {
            @Override
            protected Long doInBackground(Void... args) {
                return EpgFetchHelper.getLastEpgUpdatedTimestamp(mContext);
            }

            @Override
            protected void onPostExecute(Long result) {
                if (mClock.currentTimeMillis() - EpgFetchHelper.getLastEpgUpdatedTimestamp(mContext)
                        > mEpgDataExpiredTimeLimitMs) {
                    Log.i(TAG, "EPG data expired. Start fetching immediately.");
                    fetchImmediately();
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    @Override
    @MainThread
    public void fetchImmediately() {
        if (DEBUG) Log.d(TAG, "fetchImmediately");
        if (!mChannelDataManager.isDbLoadFinished()) {
            mChannelDataManager.addListener(
                    new ChannelDataManager.Listener() {
                        @Override
                        public void onLoadFinished() {
                            mChannelDataManager.removeListener(this);
                            executeFetchTaskIfPossible(null, null);
                        }

                        @Override
                        public void onChannelListUpdated() {}

                        @Override
                        public void onChannelBrowsableChanged() {}
                    });
        } else {
            executeFetchTaskIfPossible(null, null);
        }
    }

    @Override
    @MainThread
    public void onChannelScanStarted() {
        if (mScanStarted || !TvFeatures.ENABLE_CLOUD_EPG_REGION.isEnabled(mContext)) {
            return;
        }
        mScanStarted = true;
        stopFetchingJob();
        synchronized (mFetchDuringScanHandlerLock) {
            if (mFetchDuringScanHandler == null) {
                HandlerThread thread = new HandlerThread("EpgFetchDuringScan");
                thread.start();
                mFetchDuringScanHandler = new FetchDuringScanHandler(thread.getLooper());
            }
            mFetchDuringScanHandler.sendEmptyMessage(MSG_PREPARE_FETCH_DURING_SCAN);
        }
        Log.i(TAG, "EPG fetching on channel scanning started.");
    }

    @Override
    @MainThread
    public void onChannelScanFinished() {
        if (!mScanStarted) {
            return;
        }
        mScanStarted = false;
        mFetchDuringScanHandler.sendEmptyMessage(MSG_FINISH_FETCH_DURING_SCAN);
    }

    @MainThread
    @Override
    public void stopFetchingJob() {
        if (DEBUG) Log.d(TAG, "Try to stop routinely fetching job...");
        if (mFetchTask != null) {
            mFetchTask.cancel(true);
            mFetchTask = null;
            Log.i(TAG, "EPG routinely fetching job stopped.");
        }
    }

    @MainThread
    @Override
    public boolean executeFetchTaskIfPossible(JobService service, JobParameters params) {
        if (DEBUG) Log.d(TAG, "executeFetchTaskIfPossible");
        SoftPreconditions.checkState(mChannelDataManager.isDbLoadFinished());
        if (!CommonUtils.isRunningInTest() && checkFetchPrerequisite()) {
            mFetchTask = createFetchTask(service, params);
            mFetchTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            return true;
        }
        return false;
    }

    @VisibleForTesting
    FetchAsyncTask createFetchTask(JobService service, JobParameters params) {
        return new FetchAsyncTask(service, params);
    }

    @MainThread
    private boolean checkFetchPrerequisite() {
        if (DEBUG) Log.d(TAG, "Check prerequisite of routinely fetching job.");
        if (!TvFeatures.ENABLE_CLOUD_EPG_REGION.isEnabled(mContext)) {
            Log.i(
                    TAG,
                    "Cannot start routine service: country not supported: "
                            + LocationUtils.getCurrentCountry(mContext));
            return false;
        }
        if (mFetchTask != null) {
            // Fetching job is already running or ready to run, no need to start again.
            return false;
        }
        if (mFetchDuringScanHandler != null) {
            if (DEBUG) Log.d(TAG, "Cannot start routine service: scanning channels.");
            return false;
        }
        return true;
    }

    @MainThread
    private int getTunerChannelCount() {
        for (TvInputInfo input :
                TvSingletons.getSingletons(mContext)
                        .getTvInputManagerHelper()
                        .getTvInputInfos(true, true)) {
            String inputId = input.getId();
            if (Utils.isInternalTvInput(mContext, inputId)) {
                return mChannelDataManager.getChannelCountForInput(inputId);
            }
        }
        return 0;
    }

    @AnyThread
    private void clearUnusedLineups(@Nullable String lineupId) {
        synchronized (mPossibleLineupsLock) {
            if (mPossibleLineups == null) {
                return;
            }
            for (Lineup lineup : mPossibleLineups) {
                if (!TextUtils.equals(lineupId, lineup.getId())) {
                    mEpgReader.clearCachedChannels(lineup.getId());
                }
            }
            mPossibleLineups = null;
        }
    }

    @WorkerThread
    private Integer prepareFetchEpg(boolean forceUpdatePossibleLineups) {
        if (!mEpgReader.isAvailable()) {
            Log.i(TAG, "EPG reader is temporarily unavailable.");
            return REASON_EPG_READER_NOT_READY;
        }
        // Checks the EPG Timestamp.
        mEpgTimeStamp = mEpgReader.getEpgTimestamp();
        if (mEpgTimeStamp <= EpgFetchHelper.getLastEpgUpdatedTimestamp(mContext)) {
            if (DEBUG) Log.d(TAG, "No new EPG.");
            return REASON_NO_NEW_EPG;
        }
        // Updates postal code.
        boolean postalCodeChanged = false;
        try {
            postalCodeChanged = PostalCodeUtils.updatePostalCode(mContext);
        } catch (IOException e) {
            if (DEBUG) Log.d(TAG, "Couldn't get the current location.", e);
            if (TextUtils.isEmpty(PostalCodeUtils.getLastPostalCode(mContext))) {
                return REASON_LOCATION_INFO_UNAVAILABLE;
            }
        } catch (SecurityException e) {
            Log.w(TAG, "No permission to get the current location.");
            if (TextUtils.isEmpty(PostalCodeUtils.getLastPostalCode(mContext))) {
                return REASON_LOCATION_PERMISSION_NOT_GRANTED;
            }
        } catch (PostalCodeUtils.NoPostalCodeException e) {
            Log.i(TAG, "Cannot get address or postal code.");
            return REASON_LOCATION_INFO_UNAVAILABLE;
        }
        // Updates possible lineups if necessary.
        SoftPreconditions.checkState(mPossibleLineups == null, TAG, "Possible lineups not reset.");
        if (postalCodeChanged
                || forceUpdatePossibleLineups
                || EpgFetchHelper.getLastLineupId(mContext) == null) {
            // To prevent main thread being blocked, though theoretically it should not happen.
            String lastPostalCode = PostalCodeUtils.getLastPostalCode(mContext);
            List<Lineup> possibleLineups = mEpgReader.getLineups(lastPostalCode);
            if (possibleLineups.isEmpty()) {
                Log.i(TAG, "No lineups found for " + lastPostalCode);
                return REASON_NO_EPG_DATA_RETURNED;
            }
            for (Lineup lineup : possibleLineups) {
                mEpgReader.preloadChannels(lineup.getId());
            }
            synchronized (mPossibleLineupsLock) {
                mPossibleLineups = possibleLineups;
            }
            EpgFetchHelper.setLastLineupId(mContext, null);
        }
        return null;
    }

    @WorkerThread
    private void batchFetchEpg(Set<EpgReader.EpgChannel> epgChannels, long durationSec) {
        Log.i(TAG, "Start batch fetching (" + durationSec + ")...." + epgChannels.size());
        if (epgChannels.size() == 0) {
            return;
        }
        Set<EpgReader.EpgChannel> batch = new HashSet<>(QUERY_CHANNEL_COUNT);
        for (EpgReader.EpgChannel epgChannel : epgChannels) {
            batch.add(epgChannel);
            if (batch.size() >= QUERY_CHANNEL_COUNT) {
                batchUpdateEpg(mEpgReader.getPrograms(batch, durationSec));
                batch.clear();
            }
        }
        if (!batch.isEmpty()) {
            batchUpdateEpg(mEpgReader.getPrograms(batch, durationSec));
        }
    }

    @WorkerThread
    private void batchUpdateEpg(Map<EpgReader.EpgChannel, Collection<Program>> allPrograms) {
        for (Map.Entry<EpgReader.EpgChannel, Collection<Program>> entry : allPrograms.entrySet()) {
            List<Program> programs = new ArrayList(entry.getValue());
            if (programs == null) {
                continue;
            }
            Collections.sort(programs);
            Log.i(
                    TAG,
                    "Batch fetched " + programs.size() + " programs for channel " + entry.getKey());
            EpgFetchHelper.updateEpgData(
                    mContext, mClock, entry.getKey().getChannel().getId(), programs);
        }
    }

    @Nullable
    @WorkerThread
    private String pickBestLineupId(Set<Channel> currentChannels) {
        String maxLineupId = null;
        synchronized (mPossibleLineupsLock) {
            if (mPossibleLineups == null) {
                return null;
            }
            int maxCount = 0;
            for (Lineup lineup : mPossibleLineups) {
                int count = getMatchedChannelCount(lineup.getId(), currentChannels);
                Log.i(TAG, lineup.getName() + " (" + lineup.getId() + ") - " + count + " matches");
                if (count > maxCount) {
                    maxCount = count;
                    maxLineupId = lineup.getId();
                }
            }
        }
        return maxLineupId;
    }

    @WorkerThread
    private int getMatchedChannelCount(String lineupId, Set<Channel> currentChannels) {
        // Construct a list of display numbers for existing channels.
        if (currentChannels.isEmpty()) {
            if (DEBUG) Log.d(TAG, "No existing channel to compare");
            return 0;
        }
        List<String> numbers = new ArrayList<>(currentChannels.size());
        for (Channel channel : currentChannels) {
            // We only support channels from internal tuner inputs.
            if (Utils.isInternalTvInput(mContext, channel.getInputId())) {
                numbers.add(channel.getDisplayNumber());
            }
        }
        numbers.retainAll(mEpgReader.getChannelNumbers(lineupId));
        return numbers.size();
    }

    @VisibleForTesting
    class FetchAsyncTask extends AsyncTask<Void, Void, Integer> {
        private final JobService mService;
        private final JobParameters mParams;
        private Set<Channel> mCurrentChannels;
        private TimerEvent mTimerEvent;

        private FetchAsyncTask(JobService service, JobParameters params) {
            mService = service;
            mParams = params;
        }

        @Override
        protected void onPreExecute() {
            mTimerEvent = mPerformanceMonitor.startTimer();
            mCurrentChannels = new HashSet<>(mChannelDataManager.getChannelList());
        }

        @Override
        protected Integer doInBackground(Void... args) {
            final int oldTag = TrafficStats.getThreadStatsTag();
            TrafficStats.setThreadStatsTag(NetworkTrafficTags.EPG_FETCH);
            try {
                if (DEBUG) Log.d(TAG, "Start EPG routinely fetching.");
                Integer builtInResult = fetchEpgForBuiltInTuner();
                boolean anyCloudEpgFailure = false;
                boolean anyCloudEpgSuccess = false;
                return builtInResult;
            } finally {
                TrafficStats.setThreadStatsTag(oldTag);
            }
        }

        private Set<Channel> getExistingChannelsFor(String inputId) {
            Set<Channel> result = new HashSet<>();
            try (Cursor cursor =
                    mContext.getContentResolver()
                            .query(
                                    TvContract.buildChannelsUriForInput(inputId),
                                    ChannelImpl.PROJECTION,
                                    null,
                                    null,
                                    null)) {
                while (cursor.moveToNext()) {
                    result.add(ChannelImpl.fromCursor(cursor));
                }
                return result;
            }
        }

        private Integer fetchEpgForBuiltInTuner() {
            try {
                Integer failureReason = prepareFetchEpg(false);
                // InterruptedException might be caught by RPC, we should check it here.
                if (failureReason != null || this.isCancelled()) {
                    return failureReason;
                }
                String lineupId = EpgFetchHelper.getLastLineupId(mContext);
                lineupId = lineupId == null ? pickBestLineupId(mCurrentChannels) : lineupId;
                if (lineupId != null) {
                    Log.i(TAG, "Selecting the lineup " + lineupId);
                    // During normal fetching process, the lineup ID should be confirmed since all
                    // channels are known, clear up possible lineups to save resources.
                    EpgFetchHelper.setLastLineupId(mContext, lineupId);
                    clearUnusedLineups(lineupId);
                } else {
                    Log.i(TAG, "Failed to get lineup id");
                    return REASON_NO_EPG_DATA_RETURNED;
                }
                Set<Channel> existingChannelsForMyPackage =
                        getExistingChannelsForMyPackage(mContext);
                if (existingChannelsForMyPackage.isEmpty()) {
                    return REASON_NO_BUILT_IN_CHANNELS;
                }
                return fetchEpgFor(lineupId, existingChannelsForMyPackage);
            } catch (Exception e) {
                Log.w(TAG, "Failed to update EPG for builtin tuner", e);
                return REASON_ERROR;
            }
        }

        @Nullable
        private Integer fetchEpgFor(String lineupId, Set<Channel> existingChannels) {
            if (DEBUG) {
                Log.d(
                        TAG,
                        "Starting Fetching EPG is for "
                                + lineupId
                                + " with  channelCount "
                                + existingChannels.size());
            }
            final Set<EpgReader.EpgChannel> channels =
                    mEpgReader.getChannels(existingChannels, lineupId);
            // InterruptedException might be caught by RPC, we should check it here.
            if (this.isCancelled()) {
                return null;
            }
            if (channels.isEmpty()) {
                Log.i(TAG, "Failed to get EPG channels for " + lineupId);
                return REASON_NO_EPG_DATA_RETURNED;
            }
            if (mClock.currentTimeMillis() - EpgFetchHelper.getLastEpgUpdatedTimestamp(mContext)
                    > mEpgDataExpiredTimeLimitMs) {
                batchFetchEpg(channels, mFastFetchDurationSec);
            }
            new Handler(mContext.getMainLooper())
                    .post(
                            new Runnable() {
                                @Override
                                public void run() {
                                    ChannelLogoFetcher.startFetchingChannelLogos(
                                            mContext, asChannelList(channels));
                                }
                            });
            for (EpgReader.EpgChannel epgChannel : channels) {
                if (this.isCancelled()) {
                    return null;
                }
                List<Program> programs = new ArrayList<>(mEpgReader.getPrograms(epgChannel));
                // InterruptedException might be caught by RPC, we should check it here.
                Collections.sort(programs);
                Log.i(
                        TAG,
                        "Fetched "
                                + programs.size()
                                + " programs for channel "
                                + epgChannel.getChannel());
                EpgFetchHelper.updateEpgData(
                        mContext, mClock, epgChannel.getChannel().getId(), programs);
            }
            EpgFetchHelper.setLastEpgUpdatedTimestamp(mContext, mEpgTimeStamp);
            if (DEBUG) Log.d(TAG, "Fetching EPG is for " + lineupId);
            return null;
        }

        @Override
        protected void onPostExecute(Integer failureReason) {
            mFetchTask = null;
            if (failureReason == null
                    || failureReason == REASON_LOCATION_PERMISSION_NOT_GRANTED
                    || failureReason == REASON_NO_NEW_EPG) {
                jobFinished(false);
            } else {
                // Applies back-off policy
                jobFinished(true);
            }
            mPerformanceMonitor.stopTimer(mTimerEvent, EventNames.FETCH_EPG_TASK);
            mPerformanceMonitor.recordMemory(EventNames.FETCH_EPG_TASK);
        }

        @Override
        protected void onCancelled(Integer failureReason) {
            clearUnusedLineups(null);
            jobFinished(false);
        }

        private void jobFinished(boolean reschedule) {
            if (mService != null && mParams != null) {
                // Task is executed from JobService, need to report jobFinished.
                mService.jobFinished(mParams, reschedule);
            }
        }
    }

    private List<Channel> asChannelList(Set<EpgReader.EpgChannel> epgChannels) {
        List<Channel> result = new ArrayList<>(epgChannels.size());
        for (EpgReader.EpgChannel epgChannel : epgChannels) {
            result.add(epgChannel.getChannel());
        }
        return result;
    }

    @WorkerThread
    private class FetchDuringScanHandler extends Handler {
        private final Set<Long> mFetchedChannelIdsDuringScan = new HashSet<>();
        private String mPossibleLineupId;

        private final ChannelDataManager.Listener mDuringScanChannelListener =
                new ChannelDataManager.Listener() {
                    @Override
                    public void onLoadFinished() {
                        if (DEBUG) Log.d(TAG, "ChannelDataManager.onLoadFinished()");
                        if (getTunerChannelCount() >= MINIMUM_CHANNELS_TO_DECIDE_LINEUP
                                && !hasMessages(MSG_CHANNEL_UPDATED_DURING_SCAN)) {
                            Message.obtain(
                                            FetchDuringScanHandler.this,
                                            MSG_CHANNEL_UPDATED_DURING_SCAN,
                                            getExistingChannelsForMyPackage(mContext))
                                    .sendToTarget();
                        }
                    }

                    @Override
                    public void onChannelListUpdated() {
                        if (DEBUG) Log.d(TAG, "ChannelDataManager.onChannelListUpdated()");
                        if (getTunerChannelCount() >= MINIMUM_CHANNELS_TO_DECIDE_LINEUP
                                && !hasMessages(MSG_CHANNEL_UPDATED_DURING_SCAN)) {
                            Message.obtain(
                                            FetchDuringScanHandler.this,
                                            MSG_CHANNEL_UPDATED_DURING_SCAN,
                                            getExistingChannelsForMyPackage(mContext))
                                    .sendToTarget();
                        }
                    }

                    @Override
                    public void onChannelBrowsableChanged() {
                        // Do nothing
                    }
                };

        @AnyThread
        private FetchDuringScanHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_PREPARE_FETCH_DURING_SCAN:
                case MSG_RETRY_PREPARE_FETCH_DURING_SCAN:
                    onPrepareFetchDuringScan();
                    break;
                case MSG_CHANNEL_UPDATED_DURING_SCAN:
                    if (!hasMessages(MSG_CHANNEL_UPDATED_DURING_SCAN)) {
                        onChannelUpdatedDuringScan((Set<Channel>) msg.obj);
                    }
                    break;
                case MSG_FINISH_FETCH_DURING_SCAN:
                    removeMessages(MSG_RETRY_PREPARE_FETCH_DURING_SCAN);
                    if (hasMessages(MSG_CHANNEL_UPDATED_DURING_SCAN)) {
                        sendEmptyMessage(MSG_FINISH_FETCH_DURING_SCAN);
                    } else {
                        onFinishFetchDuringScan();
                    }
                    break;
                default:
                    // do nothing
            }
        }

        private void onPrepareFetchDuringScan() {
            Integer failureReason = prepareFetchEpg(true);
            if (failureReason != null) {
                sendEmptyMessageDelayed(
                        MSG_RETRY_PREPARE_FETCH_DURING_SCAN, FETCH_DURING_SCAN_WAIT_TIME_MS);
                return;
            }
            mChannelDataManager.addListener(mDuringScanChannelListener);
        }

        private void onChannelUpdatedDuringScan(Set<Channel> currentChannels) {
            String lineupId = pickBestLineupId(currentChannels);
            Log.i(TAG, "Fast fetch channels for lineup ID: " + lineupId);
            if (TextUtils.isEmpty(lineupId)) {
                if (TextUtils.isEmpty(mPossibleLineupId)) {
                    return;
                }
            } else if (!TextUtils.equals(lineupId, mPossibleLineupId)) {
                mFetchedChannelIdsDuringScan.clear();
                mPossibleLineupId = lineupId;
            }
            List<Long> currentChannelIds = new ArrayList<>();
            for (Channel channel : currentChannels) {
                currentChannelIds.add(channel.getId());
            }
            mFetchedChannelIdsDuringScan.retainAll(currentChannelIds);
            Set<EpgReader.EpgChannel> newChannels = new HashSet<>();
            for (EpgReader.EpgChannel epgChannel :
                    mEpgReader.getChannels(currentChannels, mPossibleLineupId)) {
                if (!mFetchedChannelIdsDuringScan.contains(epgChannel.getChannel().getId())) {
                    newChannels.add(epgChannel);
                    mFetchedChannelIdsDuringScan.add(epgChannel.getChannel().getId());
                }
            }
            batchFetchEpg(newChannels, FETCH_DURING_SCAN_DURATION_SEC);
        }

        private void onFinishFetchDuringScan() {
            mChannelDataManager.removeListener(mDuringScanChannelListener);
            EpgFetchHelper.setLastLineupId(mContext, mPossibleLineupId);
            clearUnusedLineups(null);
            mFetchedChannelIdsDuringScan.clear();
            synchronized (mFetchDuringScanHandlerLock) {
                if (!hasMessages(MSG_PREPARE_FETCH_DURING_SCAN)) {
                    removeCallbacksAndMessages(null);
                    getLooper().quit();
                    mFetchDuringScanHandler = null;
                }
            }
            // Clear timestamp to make routine service start right away.
            EpgFetchHelper.setLastEpgUpdatedTimestamp(mContext, 0);
            Log.i(TAG, "EPG Fetching during channel scanning finished.");
            new Handler(Looper.getMainLooper())
                    .post(
                            new Runnable() {
                                @Override
                                public void run() {
                                    fetchImmediately();
                                }
                            });
        }
    }
}
