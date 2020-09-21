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

package com.android.tv.dvr.recorder;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager.TvInputCallback;
import android.os.Build;
import android.os.HandlerThread;
import android.os.Looper;
import android.support.annotation.MainThread;
import android.support.annotation.RequiresApi;
import android.support.annotation.VisibleForTesting;
import android.util.ArrayMap;
import android.util.Log;
import android.util.Range;
import android.media.tv.TvContract;
import android.content.ContentValues;

import com.android.tv.InputSessionManager;
import com.android.tv.TvSingletons;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.util.Clock;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.ChannelDataManager.Listener;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.DvrDataManager.OnDvrScheduleLoadFinishedListener;
import com.android.tv.dvr.DvrDataManager.ScheduledRecordingListener;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.WritableDvrDataManager;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.Utils;

import java.util.Arrays;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * The core class to manage DVR schedule and run recording task. *
 *
 * <p>This class is responsible for:
 *
 * <ul>
 *   <li>Sending record commands to TV inputs
 *   <li>Resolving conflicting schedules, handling overlapping recording time durations, etc.
 * </ul>
 *
 * <p>This should be a singleton associated with application's main process.
 */
@RequiresApi(Build.VERSION_CODES.N)
@MainThread
public class RecordingScheduler extends TvInputCallback implements ScheduledRecordingListener {
    private static final String TAG = "RecordingScheduler";
    private static final boolean DEBUG = true;
    private static final boolean DISABLE_STREAM_TIME_FOR_DIRECT_RECORD = true;

    private static final String HANDLER_THREAD_NAME = "RecordingScheduler";
    private static final long SOON_DURATION_IN_MS = TimeUnit.MINUTES.toMillis(1);
    @VisibleForTesting static final long MS_TO_WAKE_BEFORE_START = TimeUnit.SECONDS.toMillis(5/*30*/);

    private final Looper mLooper;
    private final InputSessionManager mSessionManager;
    private final WritableDvrDataManager mDataManager;
    private final DvrManager mDvrManager;
    private final ChannelDataManager mChannelDataManager;
    private final TvInputManagerHelper mInputManager;
    private final Context mContext;
    private final Clock mClock;
    private final AlarmManager mAlarmManager;

    private final Map<String, InputTaskScheduler> mInputSchedulerMap = new ArrayMap<>();
    private long mLastStartTimePendingMs;

    private OnDvrScheduleLoadFinishedListener mDvrScheduleLoadListener =
            new OnDvrScheduleLoadFinishedListener() {
                @Override
                public void onDvrScheduleLoadFinished() {
                    mDataManager.removeDvrScheduleLoadFinishedListener(this);
                    if (isDbLoaded()) {
                        updateInternal();
                    }
                }
            };

    private Listener mChannelDataLoadListener =
            new Listener() {
                @Override
                public void onLoadFinished() {
                    mChannelDataManager.removeListener(this);
                    if (isDbLoaded()) {
                        updateInternal();
                    }
                }

                @Override
                public void onChannelListUpdated() {}

                @Override
                public void onChannelBrowsableChanged() {}
            };

    /**
     * Creates a scheduler to schedule alarms for scheduled recordings and create recording tasks.
     * This method should be only called once in the life-cycle of the application.
     */
    public static RecordingScheduler createScheduler(Context context) {
        SoftPreconditions.checkState(
                TvSingletons.getSingletons(context).getRecordingScheduler() == null);
        HandlerThread handlerThread = new HandlerThread(HANDLER_THREAD_NAME);
        handlerThread.start();
        TvSingletons singletons = TvSingletons.getSingletons(context);
        return new RecordingScheduler(
                handlerThread.getLooper(),
                singletons.getDvrManager(),
                singletons.getInputSessionManager(),
                (WritableDvrDataManager) singletons.getDvrDataManager(),
                singletons.getChannelDataManager(),
                singletons.getTvInputManagerHelper(),
                context,
                singletons.getTvClock()/*Clock.SYSTEM*/,
                (AlarmManager) context.getSystemService(Context.ALARM_SERVICE));
    }

    @VisibleForTesting
    RecordingScheduler(
            Looper looper,
            DvrManager dvrManager,
            InputSessionManager sessionManager,
            WritableDvrDataManager dataManager,
            ChannelDataManager channelDataManager,
            TvInputManagerHelper inputManager,
            Context context,
            Clock clock,
            AlarmManager alarmManager) {
        mLooper = looper;
        mDvrManager = dvrManager;
        mSessionManager = sessionManager;
        mDataManager = dataManager;
        mChannelDataManager = channelDataManager;
        mInputManager = inputManager;
        mContext = context;
        mClock = clock;
        mAlarmManager = alarmManager;
        mDataManager.addScheduledRecordingListener(this);
        mInputManager.addCallback(this);
        if (isDbLoaded()) {
            updateInternal();
        } else {
            if (!mDataManager.isDvrScheduleLoadFinished()) {
                mDataManager.addDvrScheduleLoadFinishedListener(mDvrScheduleLoadListener);
            }
            if (!mChannelDataManager.isDbLoadFinished()) {
                mChannelDataManager.addListener(mChannelDataLoadListener);
            }
        }
    }

    /** Start recording that will happen soon, and set the next alarm time. */
    public void updateAndStartServiceIfNeeded() {
        if (DEBUG) Log.d(TAG, "update and start service if needed");
        if (isDbLoaded()) {
            updateInternal();
        } else {
            // updateInternal will be called when DB is loaded. Start DvrRecordingService to
            // prevent process being killed before that.
            DvrRecordingService.startForegroundService(mContext, false);
        }
    }

    private void updateInternal() {
        boolean recordingSoon = updatePendingRecordings();
        updateNextAlarm(DvrDataManager.NEXT_START_TIME_NOT_FOUND);
        if (recordingSoon) {
            // Start DvrRecordingService to protect upcoming recording task from being killed.
            DvrRecordingService.startForegroundService(mContext, true);
        } else {
            DvrRecordingService.stopForegroundIfNotRecording();
        }
    }

    private boolean updatePendingRecordings() {
        if (DEBUG) Log.d(TAG, "updatePendingRecordings");
        List<ScheduledRecording> scheduledRecordings = new ArrayList<ScheduledRecording>();
        long currentStreamTime = mClock.currentTimeMillis();
        if (mLastStartTimePendingMs <= currentStreamTime + SOON_DURATION_IN_MS) {
            scheduledRecordings =  mDataManager.getScheduledRecordings(
                        new Range<>(
                                mLastStartTimePendingMs,
                                mClock.currentTimeMillis() + SOON_DURATION_IN_MS),
                        ScheduledRecording.STATE_RECORDING_NOT_STARTED);
        }
        for (ScheduledRecording r : scheduledRecordings) {
            scheduleRecordingSoon(r);
        }
        // update() may be called multiple times, under this situation, pending recordings may be
        // already updated thus scheduledRecordings may have a size of 0. Therefore we also have to
        // check mLastStartTimePendingMs to check if we have upcoming recordings and prevent the
        // recording service being wrongly pushed back to background in updateInternal().
        return scheduledRecordings.size() > 0
                || (mLastStartTimePendingMs > mClock.currentTimeMillis()
                        && mLastStartTimePendingMs
                                < mClock.currentTimeMillis() + SOON_DURATION_IN_MS);
    }

    private boolean isDbLoaded() {
        return mDataManager.isDvrScheduleLoadFinished() && mChannelDataManager.isDbLoadFinished();
    }

    @Override
    public void onScheduledRecordingAdded(ScheduledRecording... schedules) {
        if (DEBUG) Log.d(TAG, "added " + Arrays.asList(schedules));
        if (!isDbLoaded()) {
            return;
        }
        handleScheduleChange(schedules);
    }

    @Override
    public void onScheduledRecordingRemoved(ScheduledRecording... schedules) {
        if (DEBUG) Log.d(TAG, "removed " + Arrays.asList(schedules));
        if (!isDbLoaded()) {
            return;
        }
        boolean needToUpdateAlarm = false;
        for (ScheduledRecording schedule : schedules) {
            if (schedule != null) {
                long programId = schedule.getProgramId();
                if (programId != ScheduledRecording.ID_NOT_SET) {
                    updateProgramRecordStatusToDb(programId, com.droidlogic.app.tv.Program.RECORD_STATUS_NOT_STARTED);
                }
            }
            InputTaskScheduler inputTaskScheduler = mInputSchedulerMap.get(schedule.getInputId());
            if (inputTaskScheduler != null) {
                inputTaskScheduler.removeSchedule(schedule);
                needToUpdateAlarm = true;
            }
        }
        if (needToUpdateAlarm) {
            updateNextAlarm(DvrDataManager.NEXT_START_TIME_NOT_FOUND);
        }
    }

    @Override
    public void onScheduledRecordingStatusChanged(ScheduledRecording... schedules) {
        if (DEBUG) Log.d(TAG, "state changed " + Arrays.asList(schedules));
        if (!isDbLoaded()) {
            return;
        }
        // Update the recordings.
        for (ScheduledRecording schedule : schedules) {
            InputTaskScheduler inputTaskScheduler = mInputSchedulerMap.get(schedule.getInputId());
            if (inputTaskScheduler != null) {
                inputTaskScheduler.updateSchedule(schedule);
            }
        }
        handleScheduleChange(schedules);
    }

    private void handleScheduleChange(ScheduledRecording... schedules) {
        if (DEBUG) Log.d(TAG, "handleScheduleChange " + Arrays.asList(schedules));
        boolean needToUpdateAlarm = false;
        long latestStartRecordTimePeriod = DvrDataManager.NEXT_START_TIME_NOT_FOUND;
        for (ScheduledRecording schedule : schedules) {
            if (schedule.getState() == ScheduledRecording.STATE_RECORDING_NOT_STARTED) {
                if (DISABLE_STREAM_TIME_FOR_DIRECT_RECORD && schedule.getType() == ScheduledRecording.TYPE_TIMED) {
                    Log.d(TAG, "handleScheduleChange record directly");
                    scheduleRecordingSoon(schedule);
                } else if (startsWithin(schedule, SOON_DURATION_IN_MS)) {
                    long exactRecordTimePeriod = schedule.getStartTimeMs() - mClock.currentTimeMillis();
                    if (exactRecordTimePeriod > 0) {
                        if (latestStartRecordTimePeriod != DvrDataManager.NEXT_START_TIME_NOT_FOUND) {
                            latestStartRecordTimePeriod = Math.min(latestStartRecordTimePeriod, exactRecordTimePeriod);
                        } else {
                            latestStartRecordTimePeriod = (exactRecordTimePeriod - MS_TO_WAKE_BEFORE_START) > 0 ? (exactRecordTimePeriod - MS_TO_WAKE_BEFORE_START) : 0;
                        }
                    }
                    scheduleRecordingSoon(schedule);
                } else {
                    needToUpdateAlarm = true;
                }
            }
            boolean hasSetNewStatus = false;
            int newStatus = -1;
            if (schedule.getState() == ScheduledRecording.STATE_RECORDING_NOT_STARTED) {
                hasSetNewStatus = true;
                newStatus = com.droidlogic.app.tv.Program.RECORD_STATUS_APPOINTED;
            } else if (schedule.getState() == ScheduledRecording.STATE_RECORDING_IN_PROGRESS) {
                hasSetNewStatus = true;
                newStatus = com.droidlogic.app.tv.Program.RECORD_STATUS_IN_PROGRESS;
            } else if (schedule.getState() == ScheduledRecording.STATE_RECORDING_FINISHED ||
                    schedule.getState() == ScheduledRecording.STATE_RECORDING_FAILED ||
                    schedule.getState() == ScheduledRecording.STATE_RECORDING_DELETED ||
                    schedule.getState() == ScheduledRecording.STATE_RECORDING_CANCELED) {
                hasSetNewStatus = true;
                newStatus = com.droidlogic.app.tv.Program.RECORD_STATUS_NOT_STARTED;
            }
            if (hasSetNewStatus && schedule != null) {
                long programId = schedule.getProgramId();
                if (programId != ScheduledRecording.ID_NOT_SET) {
                    updateProgramRecordStatusToDb(programId, newStatus);
                }
            }
        }
        if (needToUpdateAlarm || latestStartRecordTimePeriod != DvrDataManager.NEXT_START_TIME_NOT_FOUND) {
            updateNextAlarm(latestStartRecordTimePeriod);
         }
    }

    private void scheduleRecordingSoon(ScheduledRecording schedule) {
        if (DEBUG) Log.d(TAG, "scheduleRecordingSoon " + Arrays.asList(schedule));
        TvInputInfo input = Utils.getTvInputInfoForInputId(mContext, schedule.getInputId());
        if (input == null) {
            Log.e(TAG, "Can't find input for " + schedule);
            mDataManager.changeState(
                    schedule,
                    ScheduledRecording.STATE_RECORDING_FAILED,
                    ScheduledRecording.FAILED_REASON_INPUT_UNAVAILABLE);
            return;
        }
        if (!input.canRecord() || input.getTunerCount() <= 0) {
            Log.e(TAG, "TV input doesn't support recording: " + input);
            mDataManager.changeState(
                    schedule,
                    ScheduledRecording.STATE_RECORDING_FAILED,
                    ScheduledRecording.FAILED_REASON_INPUT_DVR_UNSUPPORTED);
            return;
        }
        InputTaskScheduler inputTaskScheduler = mInputSchedulerMap.get(input.getId());
        if (inputTaskScheduler == null) {
            inputTaskScheduler =
                    new InputTaskScheduler(
                            mContext,
                            input,
                            mLooper,
                            mChannelDataManager,
                            mDvrManager,
                            mDataManager,
                            mSessionManager,
                            mClock);
            mInputSchedulerMap.put(input.getId(), inputTaskScheduler);
        }
        inputTaskScheduler.addSchedule(schedule);
        //if (mLastStartTimePendingMs < schedule.getStartTimeMs()) {
            mLastStartTimePendingMs = schedule.getStartTimeMs();//update mLastStartTimePendingMs as stream card plays with a loop
        //}
        if (DEBUG) Log.d(TAG, "scheduleRecordingSoon mLastStartTimePendingMs = " + Utils.toTimeString(mLastStartTimePendingMs));
    }

    private void updateNextAlarm(long period) {
        long systemTime = Clock.SYSTEM.currentTimeMillis();
        long streamTime = mClock.currentTimeMillis();
        long diff = systemTime - streamTime;
        long nextStartTime =
                mDataManager.getNextScheduledStartTimeAfter(streamTime);
                        //Math.max(mLastStartTimePendingMs, streamTime/*mClock.currentTimeMillis()*/));
        if (DEBUG) Log.d(TAG, "updateNextAlarm mLastStartTimePendingMs = " + Utils.toTimeString(mLastStartTimePendingMs) + ", nextStartTime = " + Utils.toTimeString(nextStartTime) +
            "\n" + ", currentTimeMillis = " + Utils.toTimeString(systemTime) + ", currentStreamTimeMillis = " + Utils.toTimeString(streamTime));
        if (nextStartTime != DvrDataManager.NEXT_START_TIME_NOT_FOUND || period != DvrDataManager.NEXT_START_TIME_NOT_FOUND) {
            long wakeAtStreamTime = nextStartTime - MS_TO_WAKE_BEFORE_START;
            long wakeAtSystemtime = wakeAtStreamTime + diff;//add system time diff
            if (period != DvrDataManager.NEXT_START_TIME_NOT_FOUND) {
                Log.d(TAG, "updateNextAlarm recording is going to start and adjust wake up time");
                wakeAtStreamTime = Math.min(streamTime + period, wakeAtStreamTime);
                wakeAtSystemtime = Math.min(systemTime + period, wakeAtSystemtime);
            }
            if (DEBUG) Log.d(TAG, "Set alarm to record at " + wakeAtSystemtime + "\n"
                + ", asSystemTime = " + Utils.toTimeString(wakeAtSystemtime) + ", asStreamTime = " + Utils.toTimeString(wakeAtStreamTime));
            Intent intent = new Intent(mContext, DvrStartRecordingReceiver.class);
            PendingIntent alarmIntent = PendingIntent.getBroadcast(mContext, 0, intent, 0);
            // This will cancel the previous alarm.
            mAlarmManager.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, wakeAtSystemtime/*wakeAt*/, alarmIntent);
        } else {
            if (DEBUG) Log.d(TAG, "No future recording, alarm not set");
        }
    }

    @VisibleForTesting
    boolean startsWithin(ScheduledRecording scheduledRecording, long durationInMs) {
        return mClock.currentTimeMillis() >= scheduledRecording.getStartTimeMs() - durationInMs;
    }

    // No need to remove input task schedule worker when the input is removed. If the input is
    // removed temporarily, the scheduler should keep the non-started schedules.
    @Override
    public void onInputUpdated(String inputId) {
        InputTaskScheduler inputTaskScheduler = mInputSchedulerMap.get(inputId);
        if (inputTaskScheduler != null) {
            inputTaskScheduler.updateTvInputInfo(Utils.getTvInputInfoForInputId(mContext, inputId));
        }
    }

    @Override
    public void onTvInputInfoUpdated(TvInputInfo input) {
        InputTaskScheduler inputTaskScheduler = mInputSchedulerMap.get(input.getId());
        if (inputTaskScheduler != null) {
            inputTaskScheduler.updateTvInputInfo(input);
        }
    }

    public void updateProgramRecordStatusToDb(long programid, int status) {
        if (programid != -1) {
            try {
                ContentValues values = new ContentValues();
                values.put(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG4, status);
                mContext.getContentResolver().update(TvContract.buildProgramUri(programid), values, null, null);
                Log.d(TAG, "updateProgramRecordStatusToDb programid = " + programid + ", status = " + status);
            } catch (Exception e) {
                Log.e(TAG, "updateProgramRecordStatusToDb Exception = ", e);
            }
        }
    }
}
