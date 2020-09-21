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

package com.android.tv.dvr;

import android.annotation.TargetApi;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.OperationApplicationException;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Handler;
import android.os.RemoteException;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.support.annotation.WorkerThread;
import android.util.Log;
import android.util.Range;
import android.database.Cursor;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;

import com.android.tv.TvSingletons;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.util.CommonUtils;
import com.android.tv.data.Program;
import com.android.tv.data.api.Channel;
import com.android.tv.dvr.DvrDataManager.OnRecordedProgramLoadFinishedListener;
import com.android.tv.dvr.DvrDataManager.RecordedProgramListener;
import com.android.tv.dvr.DvrScheduleManager.OnInitializeListener;
import com.android.tv.dvr.data.RecordedProgram;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.dvr.data.SeriesRecording;
import com.android.tv.util.AsyncDbTask;
import com.android.tv.util.Utils;
import com.android.tv.util.TvClock;
import com.android.tv.common.util.SystemProperties;

import com.droidlogic.app.tv.DroidLogicTvUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.Executor;

/**
 * DVR manager class to add and remove recordings. UI can modify recording list through this class,
 * instead of modifying them directly through {@link DvrDataManager}.
 */
@MainThread
@TargetApi(Build.VERSION_CODES.N)
public class DvrManager {
    private static final String TAG = "DvrManager";
    private static final boolean DEBUG = true || SystemProperties.USE_DEBUG_PVR.getValue();

    private final WritableDvrDataManager mDataManager;
    private final DvrScheduleManager mScheduleManager;
    // @GuardedBy("mListener")
    private final Map<Listener, Handler> mListener = new HashMap<>();
    private final Context mAppContext;
    private final Executor mDbExecutor;
    private final TvClock mTvClock;

    private final String ACTION_REMOVE_ALL_DVR_RECORDS = DroidLogicTvUtils.ACTION_REMOVE_ALL_DVR_RECORDS;
    private final String ACTION_DVR_RESPONSE = DroidLogicTvUtils.ACTION_DVR_RESPONSE;
    private final String KEY_DVR_DELETE_RESULT_LIST = DroidLogicTvUtils.KEY_DVR_DELETE_RESULT_LIST;
    private final String KEY_DTVKIT_SEARCH_TYPE = DroidLogicTvUtils.KEY_DTVKIT_SEARCH_TYPE;
    private final String KEY_DTVKIT_SEARCH_TYPE_AUTO = DroidLogicTvUtils.KEY_DTVKIT_SEARCH_TYPE_AUTO;
    private final String KEY_DTVKIT_SEARCH_TYPE_MANUAL = DroidLogicTvUtils.KEY_DTVKIT_SEARCH_TYPE_MANUAL;

    private final BroadcastReceiver mDvrCommandReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (DEBUG) Log.d(TAG, "intent = " + (intent != null ? intent.toString() : "null"));
            if (intent != null) {
                if (ACTION_REMOVE_ALL_DVR_RECORDS.equals(intent.getAction())) {
                    String searchType = intent.getStringExtra(KEY_DTVKIT_SEARCH_TYPE);
                    boolean autoSearch = KEY_DTVKIT_SEARCH_TYPE_AUTO.equals(searchType);
                    removeSchedulerInThread(autoSearch);
                }
            }
        }
    };

    public DvrManager(Context context) {
        SoftPreconditions.checkFeatureEnabled(context, CommonFeatures.DVR, TAG);
        mAppContext = context.getApplicationContext();
        TvSingletons tvSingletons = TvSingletons.getSingletons(context);
        mDbExecutor = tvSingletons.getDbExecutor();
        mDataManager = (WritableDvrDataManager) tvSingletons.getDvrDataManager();
        mScheduleManager = tvSingletons.getDvrScheduleManager();
        mTvClock = tvSingletons.getTvClock();
        if (mDataManager.isInitialized() && mScheduleManager.isInitialized()) {
            createSeriesRecordingsForRecordedProgramsIfNeeded(mDataManager.getRecordedPrograms());
        } else {
            // No need to handle DVR schedule load finished because schedule manager is initialized
            // after the all the schedules are loaded.
            if (!mDataManager.isRecordedProgramLoadFinished()) {
                mDataManager.addRecordedProgramLoadFinishedListener(
                        new OnRecordedProgramLoadFinishedListener() {
                            @Override
                            public void onRecordedProgramLoadFinished() {
                                mDataManager.removeRecordedProgramLoadFinishedListener(this);
                                if (mDataManager.isInitialized()
                                        && mScheduleManager.isInitialized()) {
                                    createSeriesRecordingsForRecordedProgramsIfNeeded(
                                            mDataManager.getRecordedPrograms());
                                }
                            }
                        });
            }
            if (!mScheduleManager.isInitialized()) {
                mScheduleManager.addOnInitializeListener(
                        new OnInitializeListener() {
                            @Override
                            public void onInitialize() {
                                mScheduleManager.removeOnInitializeListener(this);
                                if (mDataManager.isInitialized()
                                        && mScheduleManager.isInitialized()) {
                                    createSeriesRecordingsForRecordedProgramsIfNeeded(
                                            mDataManager.getRecordedPrograms());
                                }
                            }
                        });
            }
        }
        mDataManager.addRecordedProgramListener(
                new RecordedProgramListener() {
                    @Override
                    public void onRecordedProgramsAdded(RecordedProgram... recordedPrograms) {
                        if (!mDataManager.isInitialized() || !mScheduleManager.isInitialized()) {
                            return;
                        }
                        for (RecordedProgram recordedProgram : recordedPrograms) {
                            createSeriesRecordingForRecordedProgramIfNeeded(recordedProgram);
                        }
                    }

                    @Override
                    public void onRecordedProgramsChanged(RecordedProgram... recordedPrograms) {}

                    @Override
                    public void onRecordedProgramsRemoved(RecordedProgram... recordedPrograms) {
                        // Removing series recording is handled in the
                        // SeriesRecordingDetailsFragment.
                    }
                });
    }

    public void registerCommandReceiver() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ACTION_REMOVE_ALL_DVR_RECORDS);
        mAppContext.registerReceiver(mDvrCommandReceiver, intentFilter);
    }

    public void unRegisterCommandReceiver() {
        mAppContext.unregisterReceiver(mDvrCommandReceiver);
    }

    public void sendDvrCommandReaponse(Context context, String value) {
        Log.d(TAG, "sendDvrCommandReaponse " + value);
        Intent intent = new Intent(ACTION_DVR_RESPONSE);
        intent.putExtra(ACTION_DVR_RESPONSE, value);
        if (context != null) {
            context.sendBroadcast(intent);
        } else {
            Log.d(TAG, "sendDvrCommandReaponse null context");
        }
    }

    private void removeSchedulerInThread(final boolean autoSearch) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                removeScheduledRecordingAccordingToSearchType(autoSearch);
            }
        }).start();
    }

    private synchronized void removeScheduledRecordingAccordingToSearchType(boolean autoSearch) {
        Log.d(TAG, "removeScheduledRecordingAccordingToSearchType " + autoSearch);
        String status = "";
        List<ScheduledRecording> notStartedScheduled = mDataManager.getNonStartedScheduledRecordings();
        if (notStartedScheduled != null && notStartedScheduled.size() > 0) {
            for (ScheduledRecording one : notStartedScheduled) {
                removeScheduledRecording(one);
            }
            status = ",notstarted";
        }
        List<ScheduledRecording> startedScheduled = mDataManager.getStartedRecordings();
        if (startedScheduled != null && startedScheduled.size() > 0) {
            for (ScheduledRecording one : startedScheduled) {
                cancelRecording(one);
            }
            status += ",started";
        }
        //delete all before auto search
        if (autoSearch) {
            List<ScheduledRecording> failedScheduled = mDataManager.getFailedScheduledRecordings();
            if (failedScheduled != null && failedScheduled.size() > 0) {
                for (ScheduledRecording one : failedScheduled) {
                    removeScheduledRecording(one);
                }
                status += ",failed";
            }
            List<RecordedProgram> availableProgram = mDataManager.getRecordedPrograms();
            if (availableProgram != null && availableProgram.size() > 0) {
                for (RecordedProgram one : availableProgram) {
                    removeRecordedProgram(one);
                }
                status += ",recorded";
            }
        }
        sendDvrCommandReaponse(mAppContext, status);
    }

    public synchronized void removeInprogressRecording() {
        Log.d(TAG, "removeInprogressRecording ");
        List<ScheduledRecording> startedScheduled = mDataManager.getStartedRecordings();
        if (startedScheduled != null && startedScheduled.size() > 0) {
            for (ScheduledRecording one : startedScheduled) {
                cancelRecording(one);
            }
        }
    }

    public synchronized void stopInprogressRecording() {
        Log.d(TAG, "stopInprogressRecording ");
        List<ScheduledRecording> startedScheduled = mDataManager.getStartedRecordings();
        if (startedScheduled != null && startedScheduled.size() > 0) {
            for (ScheduledRecording one : startedScheduled) {
                stopRecording(one);
            }
        }
    }

    public boolean hasScheduleRecordingAvailable() {
        boolean result = false;
        List<ScheduledRecording> availableScheduled = mDataManager.getAvailableScheduledRecordings();
        if (availableScheduled != null && availableScheduled.size() > 0) {
            result = true;
        }
        return result;
    }

    public boolean hasInProgressScheduleRecordingAvailable() {
        boolean result = false;
        List<ScheduledRecording> startedScheduled = mDataManager.getStartedRecordings();
        if (startedScheduled != null && startedScheduled.size() > 0) {
            result = true;
        }
        return result;
    }

    public boolean hasRecordProgramAvailable() {
        boolean result = false;
        result = hasScheduleRecordingAvailable();
        if (!result) {
            List<ScheduledRecording> failedScheduled = mDataManager.getFailedScheduledRecordings();
            if (failedScheduled != null && failedScheduled.size() > 0) {
                result = true;
                return result;
            }
            List<RecordedProgram> availableScheduled = mDataManager.getRecordedPrograms();
            if (availableScheduled != null && availableScheduled.size() > 0) {
                result = true;
                return result;
            }
        }
        return result;
    }

    private void createSeriesRecordingsForRecordedProgramsIfNeeded(
            List<RecordedProgram> recordedPrograms) {
        for (RecordedProgram recordedProgram : recordedPrograms) {
            createSeriesRecordingForRecordedProgramIfNeeded(recordedProgram);
        }
    }

    private void createSeriesRecordingForRecordedProgramIfNeeded(RecordedProgram recordedProgram) {
        if (recordedProgram.isEpisodic()) {
            SeriesRecording seriesRecording =
                    mDataManager.getSeriesRecording(recordedProgram.getSeriesId());
            if (seriesRecording == null) {
                addSeriesRecording(recordedProgram);
            }
        }
    }

    /** Schedules a recording for {@code program}. */
    public ScheduledRecording addSchedule(Program program) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return null;
        }
        SeriesRecording seriesRecording = getSeriesRecording(program);
        return addSchedule(
                program,
                seriesRecording == null
                        ? mScheduleManager.suggestNewPriority()
                        : seriesRecording.getPriority());
    }

    /**
     * Schedules a recording for {@code program} with the highest priority so that the schedule can
     * be recorded.
     */
    public ScheduledRecording addScheduleWithHighestPriority(Program program) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return null;
        }
        SeriesRecording seriesRecording = getSeriesRecording(program);
        return addSchedule(
                program,
                seriesRecording == null
                        ? mScheduleManager.suggestNewPriority()
                        : mScheduleManager.suggestHighestPriority(
                                seriesRecording.getInputId(),
                                new Range(
                                        program.getStartTimeUtcMillis(),
                                        program.getEndTimeUtcMillis()),
                                seriesRecording.getPriority()));
    }

    private ScheduledRecording addSchedule(Program program, long priority) {
        TvInputInfo input = Utils.getTvInputInfoForProgram(mAppContext, program);
        if (input == null) {
            Log.e(TAG, "Can't find input for program: " + program);
            return null;
        }
        ScheduledRecording schedule;
        SeriesRecording seriesRecording = getSeriesRecording(program);
        schedule =
                createScheduledRecordingBuilder(input.getId(), program)
                        .setPriority(priority)
                        .setSeriesRecordingId(
                                seriesRecording == null
                                        ? SeriesRecording.ID_NOT_SET
                                        : seriesRecording.getId())
                        .build();
        mDataManager.addScheduledRecording(schedule);
        return schedule;
    }

    /** Adds a recording schedule with a time range. */
    public void addSchedule(Channel channel, long startTime, long endTime) {
        Log.i(
                TAG,
                "Adding scheduled recording of channel "
                        + channel
                        + " starting at "
                        + Utils.toTimeString(startTime)
                        + " and ending at "
                        + Utils.toTimeString(endTime));
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return;
        }
        TvInputInfo input = Utils.getTvInputInfoForChannelId(mAppContext, channel.getId());
        if (input == null) {
            Log.e(TAG, "Can't find input for channel: " + channel);
            return;
        }
        addScheduleInternal(input.getId(), channel.getId(), startTime, endTime);
    }

    /** Adds the schedule. */
    public void addSchedule(ScheduledRecording schedule) {
        if (mDataManager.isDvrScheduleLoadFinished()) {
            mDataManager.addScheduledRecording(schedule);
        }
    }

    private void addScheduleInternal(String inputId, long channelId, long startTime, long endTime) {
        Channel channel =
                TvSingletons.getSingletons(mAppContext)
                        .getChannelDataManager()
                        .getChannel(channelId);
        mDataManager.addScheduledRecording(
                ScheduledRecording.builder(inputId, channelId, startTime, endTime)
                        .setPriority(mScheduleManager.suggestNewPriority())
                        .setProgramTitle(channel != null ? channel.getDisplayName() : null)//use channel name to replace schedule title
                        .build());
    }

    /** Adds a new series recording and schedules for the programs with the initial state. */
    public SeriesRecording addSeriesRecording(
            Program selectedProgram,
            List<Program> programsToSchedule,
            @SeriesRecording.SeriesState int initialState) {
        Log.i(
                TAG,
                "Adding series recording for program "
                        + selectedProgram
                        + ", and schedules: "
                        + programsToSchedule);
        if (!SoftPreconditions.checkState(mDataManager.isInitialized())) {
            return null;
        }
        TvInputInfo input = Utils.getTvInputInfoForProgram(mAppContext, selectedProgram);
        if (input == null) {
            Log.e(TAG, "Can't find input for program: " + selectedProgram);
            return null;
        }
        SeriesRecording seriesRecording =
                SeriesRecording.builder(input.getId(), selectedProgram)
                        .setPriority(mScheduleManager.suggestNewSeriesPriority())
                        .setState(initialState)
                        .build();
        mDataManager.addSeriesRecording(seriesRecording);
        // The schedules for the recorded programs should be added not to create the schedule the
        // duplicate episodes.
        addRecordedProgramToSeriesRecording(seriesRecording);
        addScheduleToSeriesRecording(seriesRecording, programsToSchedule);
        return seriesRecording;
    }

    private void addSeriesRecording(RecordedProgram recordedProgram) {
        SeriesRecording seriesRecording =
                SeriesRecording.builder(recordedProgram.getInputId(), recordedProgram)
                        .setPriority(mScheduleManager.suggestNewSeriesPriority())
                        .setState(SeriesRecording.STATE_SERIES_STOPPED)
                        .build();
        mDataManager.addSeriesRecording(seriesRecording);
        // The schedules for the recorded programs should be added not to create the schedule the
        // duplicate episodes.
        addRecordedProgramToSeriesRecording(seriesRecording);
    }

    private void addRecordedProgramToSeriesRecording(SeriesRecording series) {
        List<ScheduledRecording> toAdd = new ArrayList<>();
        for (RecordedProgram recordedProgram : mDataManager.getRecordedPrograms()) {
            if (series.getSeriesId().equals(recordedProgram.getSeriesId())
                    && !recordedProgram.isClipped()) {
                // Duplicate schedules can exist, but they will be deleted in a few days. And it's
                // also guaranteed that the schedules don't belong to any series recordings because
                // there are no more than one series recordings which have the same program title.
                toAdd.add(
                        ScheduledRecording.builder(recordedProgram)
                                .setPriority(series.getPriority())
                                .setSeriesRecordingId(series.getId())
                                .build());
            }
        }
        if (!toAdd.isEmpty()) {
            mDataManager.addScheduledRecording(ScheduledRecording.toArray(toAdd));
        }
    }

    /**
     * Adds {@link ScheduledRecording}s for the series recording.
     *
     * <p>This method doesn't add the series recording.
     */
    public void addScheduleToSeriesRecording(
            SeriesRecording series, List<Program> programsToSchedule) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return;
        }
        TvInputInfo input = Utils.getTvInputInfoForInputId(mAppContext, series.getInputId());
        if (input == null) {
            Log.e(TAG, "Can't find input with ID: " + series.getInputId());
            return;
        }
        List<ScheduledRecording> toAdd = new ArrayList<>();
        List<ScheduledRecording> toUpdate = new ArrayList<>();
        for (Program program : programsToSchedule) {
            ScheduledRecording scheduleWithSameProgram =
                    mDataManager.getScheduledRecordingForProgramId(program.getId());
            if (scheduleWithSameProgram != null) {
                if (scheduleWithSameProgram.isNotStarted()) {
                    ScheduledRecording r =
                            ScheduledRecording.buildFrom(scheduleWithSameProgram)
                                    .setSeriesRecordingId(series.getId())
                                    .build();
                    if (!r.equals(scheduleWithSameProgram)) {
                        toUpdate.add(r);
                    }
                }
            } else {
                toAdd.add(
                        createScheduledRecordingBuilder(input.getId(), program)
                                .setPriority(series.getPriority())
                                .setSeriesRecordingId(series.getId())
                                .build());
            }
        }
        if (!toAdd.isEmpty()) {
            mDataManager.addScheduledRecording(ScheduledRecording.toArray(toAdd));
        }
        if (!toUpdate.isEmpty()) {
            mDataManager.updateScheduledRecording(ScheduledRecording.toArray(toUpdate));
        }
    }

    /** Updates the series recording. */
    public void updateSeriesRecording(SeriesRecording series) {
        if (SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            SeriesRecording previousSeries = mDataManager.getSeriesRecording(series.getId());
            if (previousSeries != null) {
                // If the channel option of series changed, remove the existing schedules. The new
                // schedules will be added by SeriesRecordingScheduler or by SeriesSettingsFragment.
                if (previousSeries.getChannelOption() != series.getChannelOption()
                        || (previousSeries.getChannelOption() == SeriesRecording.OPTION_CHANNEL_ONE
                                && previousSeries.getChannelId() != series.getChannelId())) {
                    List<ScheduledRecording> schedules =
                            mDataManager.getScheduledRecordings(series.getId());
                    List<ScheduledRecording> schedulesToRemove = new ArrayList<>();
                    for (ScheduledRecording schedule : schedules) {
                        if (schedule.isNotStarted()) {
                            schedulesToRemove.add(schedule);
                        } else if (schedule.isInProgress()
                                && series.getChannelOption() == SeriesRecording.OPTION_CHANNEL_ONE
                                && schedule.getChannelId() != series.getChannelId()) {
                            stopRecording(schedule);
                        }
                    }
                    List<ScheduledRecording> deletedSchedules =
                            new ArrayList<>(mDataManager.getDeletedSchedules());
                    for (ScheduledRecording deletedSchedule : deletedSchedules) {
                        if (deletedSchedule.getSeriesRecordingId() == series.getId()
                                && deletedSchedule.getEndTimeMs() > mTvClock.currentTimeMillis()/*System.currentTimeMillis()*/) {
                            schedulesToRemove.add(deletedSchedule);
                        }
                    }
                    mDataManager.removeScheduledRecording(
                            true, ScheduledRecording.toArray(schedulesToRemove));
                }
            }
            mDataManager.updateSeriesRecording(series);
            if (previousSeries == null || previousSeries.getPriority() != series.getPriority()) {
                long priority = series.getPriority();
                List<ScheduledRecording> schedulesToUpdate = new ArrayList<>();
                for (ScheduledRecording schedule :
                        mDataManager.getScheduledRecordings(series.getId())) {
                    if (schedule.isNotStarted() || schedule.isInProgress()) {
                        schedulesToUpdate.add(
                                ScheduledRecording.buildFrom(schedule)
                                        .setPriority(priority)
                                        .build());
                    }
                }
                if (!schedulesToUpdate.isEmpty()) {
                    mDataManager.updateScheduledRecording(
                            ScheduledRecording.toArray(schedulesToUpdate));
                }
            }
        }
    }

    /**
     * Removes the series recording and all the corresponding schedules which are not started yet.
     */
    public void removeSeriesRecording(long seriesRecordingId) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return;
        }
        SeriesRecording series = mDataManager.getSeriesRecording(seriesRecordingId);
        if (series == null) {
            return;
        }
        for (ScheduledRecording schedule : mDataManager.getAllScheduledRecordings()) {
            if (schedule.getSeriesRecordingId() == seriesRecordingId) {
                if (schedule.getState() == ScheduledRecording.STATE_RECORDING_IN_PROGRESS) {
                    stopRecording(schedule);
                    break;
                }
            }
        }
        mDataManager.removeSeriesRecording(series);
    }

    /** Stops the currently recorded program */
    public void stopRecording(final ScheduledRecording recording) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return;
        }
        Log.d(TAG, "stopRecording " + recording);
        synchronized (mListener) {
            for (final Entry<Listener, Handler> entry : mListener.entrySet()) {
                if (DEBUG) {
                    Log.d(TAG, "stopRecording " + entry.getKey());
                }
                entry.getValue()
                        .post(
                                new Runnable() {
                                    @Override
                                    public void run() {
                                        entry.getKey().onStopRecordingRequested(recording);
                                    }
                                });
            }
        }
    }

    /** Stops the currently recorded program */
    public void cancelRecording(final ScheduledRecording recording) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return;
        }
        Log.d(TAG, "cancelRecording " + recording);
        synchronized (mListener) {
            for (final Entry<Listener, Handler> entry : mListener.entrySet()) {
                if (DEBUG) {
                    Log.d(TAG, "cancelRecording " + entry.getKey());
                }
                entry.getValue()
                        .post(new Runnable() {
                        @Override
                        public void run() {
                            entry.getKey().onCancelRecording(recording);
                        }
                });
            }
        }
    }

    /** Removes scheduled recordings or an existing recordings. */
    public void removeScheduledRecording(ScheduledRecording... schedules) {
        Log.i(TAG, "Removing " + Arrays.asList(schedules));
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return;
        }
        for (ScheduledRecording r : schedules) {
            if (r.getState() == ScheduledRecording.STATE_RECORDING_IN_PROGRESS) {
                stopRecording(r);
            } else {
                mDataManager.removeScheduledRecording(r);
            }
        }
    }

    /** Removes scheduled recordings without changing to the DELETED state. */
    public void forceRemoveScheduledRecording(ScheduledRecording... schedules) {
        Log.i(TAG, "Force removing " + Arrays.asList(schedules));
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return;
        }
        for (ScheduledRecording r : schedules) {
            if (r.getState() == ScheduledRecording.STATE_RECORDING_IN_PROGRESS) {
                stopRecording(r);
            } else {
                mDataManager.removeScheduledRecording(true, r);
            }
        }
    }

    /** Removes the recorded program. It deletes the file if possible. */
    public void removeRecordedProgram(Uri recordedProgramUri) {
        if (!SoftPreconditions.checkState(mDataManager.isInitialized())) {
            return;
        }
        removeRecordedProgram(ContentUris.parseId(recordedProgramUri));
    }

    /** Removes the recorded program. It deletes the file if possible. */
    public void removeRecordedProgram(long recordedProgramId) {
        if (!SoftPreconditions.checkState(mDataManager.isInitialized())) {
            return;
        }
        RecordedProgram recordedProgram = mDataManager.getRecordedProgram(recordedProgramId);
        if (recordedProgram != null) {
            removeRecordedProgram(recordedProgram);
        } else {
            //livetv can't update on time, delete it in db
            Log.d(TAG, "removeRecordedProgram from db directly");
            removeRecordedProgramDirectlyByUri(TvContract.buildRecordedProgramUri(recordedProgramId));
        }
    }

    /** Removes the recorded program. It deletes the file if possible. */
    public void removeRecordedProgram(final RecordedProgram recordedProgram) {
        if (!SoftPreconditions.checkState(mDataManager.isInitialized())) {
            return;
        }
        new AsyncDbTask<Void, Void, Integer>(mDbExecutor) {
            @Override
            protected Integer doInBackground(Void... params) {
                ContentResolver resolver = mAppContext.getContentResolver();
                return resolver.delete(recordedProgram.getUri(), null, null);
            }

            @Override
            protected void onPostExecute(Integer deletedCounts) {
                if (deletedCounts > 0) {
                    new AsyncTask<Void, Void, Void>() {
                        @Override
                        protected Void doInBackground(Void... params) {
                            removeRecordedData(recordedProgram.getDataUri());
                            return null;
                        }
                    }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                }
            }
        }.executeOnDbThread();
    }

    public RecordedProgram getRecordedProgramFromDb(ContentResolver resolver, Uri recordedProgramUri) {
        if (recordedProgramUri == null) {
            return null;
        }
        Cursor cursor = null;
        RecordedProgram recordedProgram = null;
        try {
            cursor = resolver.query(recordedProgramUri, RecordedProgram.PROJECTION, null, null, null);
            if (cursor != null) {
                cursor.moveToNext();
                recordedProgram = RecordedProgram.fromCursor(cursor);
            }
        } catch (Exception e) {
            Log.e(TAG, "getRecordedProgramFromDb failed from TvProvider.", e);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return recordedProgram;
    }

    /** Removes the recorded program. It deletes the file if possible. */
    public void removeRecordedProgramDirectlyByUri(final Uri recordedProgramUri) {
        if (!SoftPreconditions.checkState(mDataManager.isInitialized())) {
            return;
        }
        new AsyncDbTask<Void, Void, HashMap<String, Object>>(mDbExecutor) {

            @Override
            protected HashMap<String, Object> doInBackground(Void... params) {
                ContentResolver resolver = mAppContext.getContentResolver();
                HashMap<String, Object> multiResult = new HashMap<String, Object>();
                RecordedProgram recordedProgram = getRecordedProgramFromDb(resolver, recordedProgramUri);
                multiResult.put("recorded_program", recordedProgram);
                multiResult.put("number", resolver.delete(recordedProgramUri, null, null));
                return multiResult;
            }

            @Override
            protected void onPostExecute(HashMap<String, Object> content) {
                int deletedCounts = (int)content.get("number");
                RecordedProgram recordedProgram = (RecordedProgram)content.get("recorded_program");
                if (deletedCounts > 0) {
                    new AsyncTask<Void, Void, Void>() {
                        @Override
                        protected Void doInBackground(Void... params) {
                            if (recordedProgram != null) {
                                removeRecordedData(recordedProgram.getDataUri());
                            }
                            return null;
                        }
                    }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                }
            }
        }.executeOnDbThread();
    }

    public void removeRecordedPrograms(List<Long> recordedProgramIds) {
        final ArrayList<ContentProviderOperation> dbOperations = new ArrayList<>();
        final List<Uri> dataUris = new ArrayList<>();
        for (Long rId : recordedProgramIds) {
            RecordedProgram r = mDataManager.getRecordedProgram(rId);
            if (r != null) {
                dataUris.add(r.getDataUri());
                dbOperations.add(ContentProviderOperation.newDelete(r.getUri()).build());
            }
        }
        new AsyncDbTask<Void, Void, Boolean>(mDbExecutor) {
            @Override
            protected Boolean doInBackground(Void... params) {
                ContentResolver resolver = mAppContext.getContentResolver();
                try {
                    resolver.applyBatch(TvContract.AUTHORITY, dbOperations);
                } catch (RemoteException | OperationApplicationException e) {
                    Log.w(TAG, "Remove recorded programs from DB failed.", e);
                    return false;
                }
                return true;
            }

            @Override
            protected void onPostExecute(Boolean success) {
                if (success) {
                    new AsyncTask<Void, Void, Void>() {
                        @Override
                        protected Void doInBackground(Void... params) {
                            for (Uri dataUri : dataUris) {
                                removeRecordedData(dataUri);
                            }
                            return null;
                        }
                    }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                }
            }
        }.executeOnDbThread();
    }

    /** Updates the scheduled recording. */
    public void updateScheduledRecording(ScheduledRecording recording) {
        if (SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            mDataManager.updateScheduledRecording(recording);
        }
    }

    /**
     * Returns priority ordered list of all scheduled recordings that will not be recorded if this
     * program is.
     *
     * @see DvrScheduleManager#getConflictingSchedules(Program)
     */
    public List<ScheduledRecording> getConflictingSchedules(Program program) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return Collections.emptyList();
        }
        return mScheduleManager.getConflictingSchedules(program);
    }

    /**
     * Returns priority ordered list of all scheduled recordings that will not be recorded if this
     * channel is.
     *
     * @see DvrScheduleManager#getConflictingSchedules(long, long, long)
     */
    public List<ScheduledRecording> getConflictingSchedules(
            long channelId, long startTimeMs, long endTimeMs) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return Collections.emptyList();
        }
        return mScheduleManager.getConflictingSchedules(channelId, startTimeMs, endTimeMs);
    }

    /**
     * Checks if the schedule is conflicting.
     *
     * <p>Note that the {@code schedule} should be the existing one. If not, this returns {@code
     * false}.
     */
    public boolean isConflicting(ScheduledRecording schedule) {
        return schedule != null
                && SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())
                && mScheduleManager.isConflicting(schedule);
    }

    /**
     * Returns priority ordered list of all scheduled recording that will not be recorded if this
     * channel is tuned to.
     *
     * @see DvrScheduleManager#getConflictingSchedulesForTune
     */
    public List<ScheduledRecording> getConflictingSchedulesForTune(long channelId) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return Collections.emptyList();
        }
        return mScheduleManager.getConflictingSchedulesForTune(channelId);
    }

    /** Sets the highest priority to the schedule. */
    public void setHighestPriority(ScheduledRecording schedule) {
        if (SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            long newPriority = mScheduleManager.suggestHighestPriority(schedule);
            if (newPriority != schedule.getPriority()) {
                mDataManager.updateScheduledRecording(
                        ScheduledRecording.buildFrom(schedule).setPriority(newPriority).build());
            }
        }
    }

    /** Suggests the higher priority than the schedules which overlap with {@code schedule}. */
    public long suggestHighestPriority(ScheduledRecording schedule) {
        if (SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return mScheduleManager.suggestHighestPriority(schedule);
        }
        return DvrScheduleManager.DEFAULT_PRIORITY;
    }

    /**
     * Returns {@code true} if the channel can be recorded.
     *
     * <p>Note that this method doesn't check the conflict of the schedule or available tuners. This
     * can be called from the UI before the schedules are loaded.
     */
    public boolean isChannelRecordable(Channel channel) {
        if (!mDataManager.isDvrScheduleLoadFinished() || channel == null) {
            return false;
        }
        if (channel.isRecordingProhibited()) {
            return false;
        }
        TvInputInfo info = Utils.getTvInputInfoForChannelId(mAppContext, channel.getId());
        if (info == null) {
            Log.w(TAG, "Could not find TvInputInfo for " + channel);
            return false;
        }
        if (!info.canRecord()) {
            return false;
        }
        Program program =
                TvSingletons.getSingletons(mAppContext)
                        .getProgramDataManager()
                        .getCurrentProgram(channel.getId());
        return program == null || !program.isRecordingProhibited();
    }

    /**
     * Returns {@code true} if the program can be recorded.
     *
     * <p>Note that this method doesn't check the conflict of the schedule or available tuners. This
     * can be called from the UI before the schedules are loaded.
     */
    public boolean isProgramRecordable(Program program) {
        if (!mDataManager.isInitialized()) {
            return false;
        }
        Channel channel =
                TvSingletons.getSingletons(mAppContext)
                        .getChannelDataManager()
                        .getChannel(program.getChannelId());
        if (channel == null || channel.isRecordingProhibited()) {
            return false;
        }
        TvInputInfo info = Utils.getTvInputInfoForChannelId(mAppContext, channel.getId());
        if (info == null) {
            Log.w(TAG, "Could not find TvInputInfo for " + program);
            return false;
        }
        return info.canRecord() && !program.isRecordingProhibited();
    }

    /**
     * Returns the current recording for the channel.
     *
     * <p>This can be called from the UI before the schedules are loaded.
     */
    public ScheduledRecording getCurrentRecording(long channelId) {
        if (!mDataManager.isDvrScheduleLoadFinished()) {
            return null;
        }
        for (ScheduledRecording recording : mDataManager.getStartedRecordings()) {
            if (recording.getChannelId() == channelId) {
                return recording;
            }
        }
        return null;
    }

    /**
     * Returns schedules which is available (i.e., isNotStarted or isInProgress) and belongs to the
     * series recording {@code seriesRecordingId}.
     */
    public List<ScheduledRecording> getAvailableScheduledRecording(long seriesRecordingId) {
        if (!mDataManager.isDvrScheduleLoadFinished()) {
            return Collections.emptyList();
        }
        List<ScheduledRecording> schedules = new ArrayList<>();
        for (ScheduledRecording schedule : mDataManager.getScheduledRecordings(seriesRecordingId)) {
            if (schedule.isInProgress() || schedule.isNotStarted()) {
                schedules.add(schedule);
            }
        }
        return schedules;
    }

    /** Returns the series recording related to the program. */
    @Nullable
    public SeriesRecording getSeriesRecording(Program program) {
        if (!SoftPreconditions.checkState(mDataManager.isDvrScheduleLoadFinished())) {
            return null;
        }
        return mDataManager.getSeriesRecording(program.getSeriesId());
    }

    /**
     * Returns if there are valid items. Valid item contains {@link RecordedProgram}, available
     * {@link ScheduledRecording} and {@link SeriesRecording}.
     */
    public boolean hasValidItems() {
        return !(mDataManager.getRecordedPrograms().isEmpty()
                && mDataManager.getStartedRecordings().isEmpty()
                && mDataManager.getNonStartedScheduledRecordings().isEmpty()
                && mDataManager.getSeriesRecordings().isEmpty());
    }

    @WorkerThread
    @VisibleForTesting
    // Should be public to use mock DvrManager object.
    public void addListener(Listener listener, @NonNull Handler handler) {
        SoftPreconditions.checkNotNull(handler);
        synchronized (mListener) {
            mListener.put(listener, handler);
        }
    }

    @WorkerThread
    @VisibleForTesting
    // Should be public to use mock DvrManager object.
    public void removeListener(Listener listener) {
        synchronized (mListener) {
            mListener.remove(listener);
        }
    }

    /**
     * Returns ScheduledRecording.builder based on {@code program}. If program is already started,
     * recording started time is clipped to the current time.
     */
    private ScheduledRecording.Builder createScheduledRecordingBuilder(
            String inputId, Program program) {
        ScheduledRecording.Builder builder = ScheduledRecording.builder(inputId, program);
        long time = mTvClock.currentTimeMillis()/*System.currentTimeMillis()*/;
        if (program.getStartTimeUtcMillis() < time && time < program.getEndTimeUtcMillis()) {
            builder.setStartTimeMs(time);
        }
        return builder;
    }

    /** Returns a schedule which matches to the given episode. */
    public ScheduledRecording getScheduledRecording(
            String title, String seasonNumber, String episodeNumber) {
        if (!SoftPreconditions.checkState(mDataManager.isInitialized())
                || title == null
                || seasonNumber == null
                || episodeNumber == null) {
            return null;
        }
        for (ScheduledRecording r : mDataManager.getAllScheduledRecordings()) {
            if (title.equals(r.getProgramTitle())
                    && seasonNumber.equals(r.getSeasonNumber())
                    && episodeNumber.equals(r.getEpisodeNumber())) {
                return r;
            }
        }
        return null;
    }

    /** Returns a recorded program which is the same episode as the given {@code program}. */
    public RecordedProgram getRecordedProgram(
            String title, String seasonNumber, String episodeNumber) {
        if (!SoftPreconditions.checkState(mDataManager.isInitialized())
                || title == null
                || seasonNumber == null
                || episodeNumber == null) {
            return null;
        }
        for (RecordedProgram r : mDataManager.getRecordedPrograms()) {
            if (title.equals(r.getTitle())
                    && seasonNumber.equals(r.getSeasonNumber())
                    && episodeNumber.equals(r.getEpisodeNumber())
                    && !r.isClipped()) {
                return r;
            }
        }
        return null;
    }

    @WorkerThread
    private void removeRecordedData(Uri dataUri) {
        try {
            if (dataUri != null
                    && ContentResolver.SCHEME_FILE.equals(dataUri.getScheme())
                    && dataUri.getPath() != null) {
                File recordedProgramPath = new File(dataUri.getPath());
                if (!recordedProgramPath.exists()) {
                    if (DEBUG) Log.d(TAG, "File to delete not exist: " + recordedProgramPath);
                } else {
                    CommonUtils.deleteDirOrFile(recordedProgramPath);
                    if (DEBUG) {
                        Log.d(TAG, "Sucessfully deleted files of the recorded program: " + dataUri);
                    }
                }
            }
        } catch (SecurityException e) {
            if (DEBUG) {
                Log.d(
                        TAG,
                        "To delete this recorded program, please manually delete video data at"
                                + "\nadb shell rm -rf "
                                + dataUri);
            }
        }
    }

    /**
     * Remove all the records related to the input.
     *
     * <p>Note that this should be called after the input was removed.
     */
    public void forgetStorage(String inputId) {
        if (mDataManager.isInitialized()) {
            mDataManager.forgetStorage(inputId);
        }
    }

    /**
     * hide or unhide all the records related.
     *
     * <p>Note that this should be called after the related storage disk is added or removed.
     */
    public void updateDisplayforAddedOrRemovedStorage() {
        if (mDataManager.isInitialized()) {
            mDataManager.updateDisplayforAddedOrRemovedStorage();
        }
    }

    /**
     * Listener to stop recording request. Should only be internally used inside dvr and its
     * sub-package.
     */
    public interface Listener {
        void onStopRecordingRequested(ScheduledRecording scheduledRecording);
        void onCancelRecording(ScheduledRecording scheduledRecording);
    }

     /** Adds new appointed watch program. */
    public void addAppointedWatchProgram(Program program) {
        mDataManager.addAppointedWatchProgram(program);
    }

    /** Removes appointed watch program. */
    public void removeAppointedWatchProgram(Program program) {
        mDataManager.removeAppointedWatchProgram(program);
    }
}
