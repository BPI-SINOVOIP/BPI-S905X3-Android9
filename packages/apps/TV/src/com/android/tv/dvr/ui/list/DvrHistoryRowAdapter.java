/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tv.dvr.ui.list;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build.VERSION_CODES;
import android.support.annotation.Nullable;
import android.support.v17.leanback.widget.ArrayObjectAdapter;
import android.support.v17.leanback.widget.ClassPresenterSelector;
import android.text.format.DateUtils;
import android.util.Log;
import com.android.tv.R;
import com.android.tv.TvSingletons;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.util.Clock;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.data.RecordedProgram;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.dvr.recorder.ScheduledProgramReaper;
import com.android.tv.dvr.ui.list.SchedulesHeaderRow.DateHeaderRow;
import com.android.tv.util.Utils;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/** An adapter for DVR history. */
@TargetApi(VERSION_CODES.N)
@SuppressWarnings("AndroidApiChecker") // TODO(b/32513850) remove when error prone is updated
class DvrHistoryRowAdapter extends ArrayObjectAdapter {
    private static final String TAG = "DvrHistoryRowAdapter";
    private static final boolean DEBUG = false;

    private static final long ONE_DAY_MS = TimeUnit.DAYS.toMillis(1);
    private static final int MAX_HISTORY_DAYS = ScheduledProgramReaper.DAYS;

    private final Context mContext;
    private final Clock mClock;
    private final DvrDataManager mDvrDataManager;
    private final List<String> mTitles = new ArrayList<>();
    private final Map<Long, ScheduledRecording> mRecordedProgramScheduleMap = new HashMap<>();

    public DvrHistoryRowAdapter(
            Context context, ClassPresenterSelector classPresenterSelector, Clock clock) {
        super(classPresenterSelector);
        mContext = context;
        mClock = clock;
        mDvrDataManager = TvSingletons.getSingletons(mContext).getDvrDataManager();
        mTitles.add(mContext.getString(R.string.dvr_date_today));
        mTitles.add(mContext.getString(R.string.dvr_date_yesterday));
    }

    /** Returns context. */
    protected Context getContext() {
        return mContext;
    }

    /** Starts row adapter. */
    public void start() {
        clear();
        List<ScheduledRecording> recordingList = mDvrDataManager.getFailedScheduledRecordings();
        List<RecordedProgram> recordedProgramList = mDvrDataManager.getRecordedPrograms();

        recordingList.addAll(
                recordedProgramsToScheduledRecordings(recordedProgramList, MAX_HISTORY_DAYS));
        recordingList
                .sort(ScheduledRecording.START_TIME_THEN_PRIORITY_THEN_ID_COMPARATOR.reversed());
        long deadLine = Utils.getFirstMillisecondOfDay(mClock.currentTimeMillis());
        for (int i = 0; i < recordingList.size(); ) {
            ArrayList<ScheduledRecording> section = new ArrayList<>();
            while (i < recordingList.size() && recordingList.get(i).getStartTimeMs() >= deadLine) {
                section.add(recordingList.get(i++));
            }
            if (!section.isEmpty()) {
                SchedulesHeaderRow headerRow =
                        new DateHeaderRow(
                                calculateHeaderDate(deadLine),
                                mContext.getResources()
                                        .getQuantityString(
                                                R.plurals.dvr_schedules_section_subtitle,
                                                section.size(),
                                                section.size()),
                                section.size(),
                                deadLine);
                add(headerRow);
                for (ScheduledRecording recording : section) {
                    add(new ScheduleRow(recording, headerRow, mContext));
                }
            }
            deadLine -= ONE_DAY_MS;
        }
    }

    private String calculateHeaderDate(long timeMs) {
        int titleIndex =
                (int)
                        ((Utils.getFirstMillisecondOfDay(mClock.currentTimeMillis()) - timeMs)
                                / ONE_DAY_MS);
        String headerDate;
        if (titleIndex < mTitles.size()) {
            headerDate = mTitles.get(titleIndex);
        } else {
            headerDate =
                    DateUtils.formatDateTime(
                            getContext(),
                            timeMs,
                            DateUtils.FORMAT_SHOW_WEEKDAY
                                    | DateUtils.FORMAT_SHOW_DATE
                                    | DateUtils.FORMAT_ABBREV_MONTH);
        }
        return headerDate;
    }

    private List<ScheduledRecording> recordedProgramsToScheduledRecordings(
            List<RecordedProgram> programs, int maxDays) {
        List<ScheduledRecording> result = new ArrayList<>();
        for (RecordedProgram recordedProgram : programs) {
            ScheduledRecording scheduledRecording =
                    recordedProgramsToScheduledRecordings(recordedProgram, maxDays);
            if (scheduledRecording != null) {
                result.add(scheduledRecording);
            }
        }
        return result;
    }

    @Nullable
    private ScheduledRecording recordedProgramsToScheduledRecordings(
            RecordedProgram program, int maxDays) {
        long firstMillisecondToday = Utils.getFirstMillisecondOfDay(mClock.currentTimeMillis());
        if (maxDays
                < Utils.computeDateDifference(
                        program.getStartTimeUtcMillis(),
                        firstMillisecondToday)) {
            return null;
        }
        ScheduledRecording scheduledRecording = ScheduledRecording.builder(program).build();
        mRecordedProgramScheduleMap.put(program.getId(), scheduledRecording);
        return scheduledRecording;
    }

    public void onScheduledRecordingAdded(ScheduledRecording schedule) {
        if (DEBUG) {
            Log.d(TAG, "onScheduledRecordingAdded: " + schedule);
        }
        if (findRowByScheduledRecording(schedule) == null
                && (schedule.getState() == ScheduledRecording.STATE_RECORDING_FINISHED
                        || schedule.getState() == ScheduledRecording.STATE_RECORDING_CLIPPED
                        || schedule.getState() == ScheduledRecording.STATE_RECORDING_FAILED)) {
            addScheduleRow(schedule);
        }
    }

    public void onScheduledRecordingAdded(RecordedProgram program) {
        if (DEBUG) {
            Log.d(TAG, "onScheduledRecordingAdded: " + program);
        }
        if (mRecordedProgramScheduleMap.get(program.getId()) != null) {
            return;
        }
        ScheduledRecording schedule =
                recordedProgramsToScheduledRecordings(program, MAX_HISTORY_DAYS);
        if (schedule == null) {
            return;
        }
        addScheduleRow(schedule);
    }

    public void onScheduledRecordingRemoved(ScheduledRecording schedule) {
        if (DEBUG) {
            Log.d(TAG, "onScheduledRecordingRemoved: " + schedule);
        }
        ScheduleRow row = findRowByScheduledRecording(schedule);
        if (row != null) {
            removeScheduleRow(row);
            notifyArrayItemRangeChanged(indexOf(row), 1);
        }
    }

    public void onScheduledRecordingRemoved(RecordedProgram program) {
        if (DEBUG) {
            Log.d(TAG, "onScheduledRecordingRemoved: " + program);
        }
        ScheduledRecording scheduledRecording = mRecordedProgramScheduleMap.get(program.getId());
        if (scheduledRecording != null) {
            mRecordedProgramScheduleMap.remove(program.getId());
            ScheduleRow row = findRowByRecordedProgram(program);
            if (row != null) {
                removeScheduleRow(row);
                notifyArrayItemRangeChanged(indexOf(row), 1);
            }
        }
    }

    public void onScheduledRecordingUpdated(ScheduledRecording schedule) {
        if (DEBUG) {
            Log.d(TAG, "onScheduledRecordingUpdated: " + schedule);
        }
        ScheduleRow row = findRowByScheduledRecording(schedule);
        if (row != null) {
            row.setSchedule(schedule);
            if (schedule.getState() != ScheduledRecording.STATE_RECORDING_FAILED) {
                // Only handle failed schedules. Finished schedules are handled as recorded programs
                removeScheduleRow(row);
            }
            notifyArrayItemRangeChanged(indexOf(row), 1);
        }
    }

    public void onScheduledRecordingUpdated(RecordedProgram program) {
        if (DEBUG) {
            Log.d(TAG, "onScheduledRecordingUpdated: " + program);
        }
        ScheduleRow row = findRowByRecordedProgram(program);
        if (row != null) {
            removeScheduleRow(row);
            notifyArrayItemRangeChanged(indexOf(row), 1);
            ScheduledRecording schedule = mRecordedProgramScheduleMap.get(program.getId());
            if (schedule != null) {
                mRecordedProgramScheduleMap.remove(program.getId());
            }
        }
        onScheduledRecordingAdded(program);
    }

    private void addScheduleRow(ScheduledRecording recording) {
        // This method must not be called from inherited class.
        SoftPreconditions.checkState(getClass().equals(DvrHistoryRowAdapter.class));
        if (recording != null) {
            int pre = -1;
            int index = 0;
            for (; index < size(); index++) {
                if (get(index) instanceof ScheduleRow) {
                    ScheduleRow scheduleRow = (ScheduleRow) get(index);
                    if (ScheduledRecording.START_TIME_THEN_PRIORITY_THEN_ID_COMPARATOR.reversed()
                            .compare(scheduleRow.getSchedule(), recording) > 0) {
                        break;
                    }
                    pre = index;
                }
            }
            long deadLine = Utils.getFirstMillisecondOfDay(recording.getStartTimeMs());
            if (pre >= 0 && getHeaderRow(pre).getDeadLineMs() == deadLine) {
                SchedulesHeaderRow headerRow = ((ScheduleRow) get(pre)).getHeaderRow();
                headerRow.setItemCount(headerRow.getItemCount() + 1);
                ScheduleRow addedRow = new ScheduleRow(recording, headerRow, mContext);
                add(++pre, addedRow);
                updateHeaderDescription(headerRow);
            } else if (index < size() && getHeaderRow(index).getDeadLineMs() == deadLine) {
                SchedulesHeaderRow headerRow = ((ScheduleRow) get(index)).getHeaderRow();
                headerRow.setItemCount(headerRow.getItemCount() + 1);
                ScheduleRow addedRow = new ScheduleRow(recording, headerRow, mContext);
                add(index, addedRow);
                updateHeaderDescription(headerRow);
            } else {
                SchedulesHeaderRow headerRow =
                        new DateHeaderRow(
                                calculateHeaderDate(deadLine),
                                mContext.getResources()
                                        .getQuantityString(
                                                R.plurals.dvr_schedules_section_subtitle, 1, 1),
                                1,
                                deadLine);
                add(++pre, headerRow);
                ScheduleRow addedRow = new ScheduleRow(recording, headerRow, mContext);
                add(pre, addedRow);
            }
        }
    }

    private DateHeaderRow getHeaderRow(int index) {
        return ((DateHeaderRow) ((ScheduleRow) get(index)).getHeaderRow());
    }

    /** Gets which {@link ScheduleRow} the {@link ScheduledRecording} belongs to. */
    private ScheduleRow findRowByScheduledRecording(ScheduledRecording recording) {
        if (recording == null) {
            return null;
        }
        for (int i = 0; i < size(); i++) {
            Object item = get(i);
            if (item instanceof ScheduleRow && ((ScheduleRow) item).getSchedule() != null) {
                if (((ScheduleRow) item).getSchedule().getId() == recording.getId()) {
                    return (ScheduleRow) item;
                }
            }
        }
        return null;
    }

    private ScheduleRow findRowByRecordedProgram(RecordedProgram program) {
        if (program == null) {
            return null;
        }
        for (int i = 0; i < size(); i++) {
            Object item = get(i);
            if (item instanceof ScheduleRow) {
                ScheduleRow row = (ScheduleRow) item;
                if (row.hasRecordedProgram()
                        && row.getSchedule().getRecordedProgramId() == program.getId()) {
                    return (ScheduleRow) item;
                }
            }
        }
        return null;
    }

    private void removeScheduleRow(ScheduleRow scheduleRow) {
        // This method must not be called from inherited class.
        SoftPreconditions.checkState(getClass().equals(DvrHistoryRowAdapter.class));
        if (scheduleRow != null) {
            scheduleRow.setSchedule(null);
            SchedulesHeaderRow headerRow = scheduleRow.getHeaderRow();
            remove(scheduleRow);
            // Changes the count information of header which the removed row belongs to.
            if (headerRow != null) {
                int currentCount = headerRow.getItemCount();
                headerRow.setItemCount(--currentCount);
                if (headerRow.getItemCount() == 0) {
                    remove(headerRow);
                } else {
                    replace(indexOf(headerRow), headerRow);
                    updateHeaderDescription(headerRow);
                }
            }
        }
    }

    private void updateHeaderDescription(SchedulesHeaderRow headerRow) {
        headerRow.setDescription(
                mContext.getResources()
                        .getQuantityString(
                                R.plurals.dvr_schedules_section_subtitle,
                                headerRow.getItemCount(),
                                headerRow.getItemCount()));
    }
}
