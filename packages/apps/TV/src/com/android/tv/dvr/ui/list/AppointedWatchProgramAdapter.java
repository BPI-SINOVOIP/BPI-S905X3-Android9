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
import android.text.format.DateUtils;
import android.util.Log;
import android.support.v17.leanback.widget.ArrayObjectAdapter;
import android.support.v17.leanback.widget.ClassPresenterSelector;

import com.android.tv.R;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.util.Clock;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.data.RecordedProgram;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.dvr.ui.list.SchedulesHeaderRow.DateHeaderRow;
import com.android.tv.util.Utils;
import com.android.tv.data.Program;
import com.android.tv.data.ProgramDataManager;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/** An adapter for DVR history. */
@TargetApi(VERSION_CODES.N)
@SuppressWarnings("AndroidApiChecker") // TODO(b/32513850) remove when error prone is updated
class AppointedWatchProgramAdapter extends ArrayObjectAdapter {
    private static final String TAG = "AppointedWatchProgramAdapter";
    private static final boolean DEBUG = true;

    private static final long ONE_DAY_MS = TimeUnit.DAYS.toMillis(1);

    private final long mMaxHistoryDays;
    private final Context mContext;
    private final Clock mClock;
    private final DvrDataManager mDvrDataManager;
    private final List<String> mTitles = new ArrayList<>();
    private final Map<Long, ScheduledRecording> mProgramScheduleMap = new HashMap<>();

    public AppointedWatchProgramAdapter(
            Context context,
            ClassPresenterSelector classPresenterSelector,
            Clock clock,
            DvrDataManager dvrDataManager) {
        super(classPresenterSelector);
        mContext = context;
        mClock = clock;
        mDvrDataManager = dvrDataManager;
        mMaxHistoryDays = 0;
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
        List<ScheduledRecording> recordingList = new ArrayList<ScheduledRecording>();
		List<Program> appointProgramList = ProgramDataManager.getAppointedPrograms(getContext());
        recordingList.addAll(
                programsToScheduledRecordings(appointProgramList, mMaxHistoryDays));
        recordingList.sort(
                ScheduledRecording.START_TIME_THEN_PRIORITY_THEN_ID_COMPARATOR.reversed());
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
                                                R.plurals.dvr_appointed_watch_programs_section_subtitle,
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

    private List<ScheduledRecording> programsToScheduledRecordings(
            List<Program> programs, long maxDays) {
        List<ScheduledRecording> result = new ArrayList<>();
        for (Program program : programs) {
            ScheduledRecording scheduledRecording =
                    programsToScheduledRecordings(program, maxDays);
            if (scheduledRecording != null) {
                result.add(scheduledRecording);
            }
        }
        return result;
    }

    @Nullable
    private ScheduledRecording programsToScheduledRecordings(
            Program program, long maxDays) {
        long firstMillisecondToday = Utils.getFirstMillisecondOfDay(mClock.currentTimeMillis());
        if (maxDays != 0
                && maxDays
                        < Utils.computeDateDifference(
                                program.getStartTimeUtcMillis(), firstMillisecondToday)) {
            return null;
        }
        ScheduledRecording scheduledRecording = ScheduledRecording.builder(program).build();
        mProgramScheduleMap.put(program.getId(), scheduledRecording);
        return scheduledRecording;
    }

    public void onScheduledRecordingAdded(Program program) {
        if (DEBUG) {
            Log.d(TAG, "onScheduledRecordingAdded: " + program);
        }
        if (mProgramScheduleMap.get(program.getId()) != null) {
            return;
        }
        ScheduledRecording schedule =
                programsToScheduledRecordings(program, mMaxHistoryDays);
        if (schedule == null) {
            return;
        }
        addScheduleRow(schedule);
    }

    public void onScheduledRecordingRemoved(Program program) {
        if (DEBUG) {
            Log.d(TAG, "onScheduledRecordingRemoved: " + program);
        }
        ScheduledRecording scheduledRecording = mProgramScheduleMap.get(program.getId());
        if (scheduledRecording != null) {
            mProgramScheduleMap.remove(program.getId());
            ScheduleRow row = findRowByProgram(program);
            if (row != null) {
                removeScheduleRow(row);
                notifyArrayItemRangeChanged(indexOf(row), 1);
            }
        }
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
                    if (ScheduledRecording.START_TIME_THEN_PRIORITY_THEN_ID_COMPARATOR
                                    .reversed()
                                    .compare(scheduleRow.getSchedule(), recording)
                            > 0) {
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

    private ScheduleRow findRowByProgram(Program program) {
        if (program == null) {
            return null;
        }
        for (int i = 0; i < size(); i++) {
            Object item = get(i);
            if (item instanceof ScheduleRow) {
                ScheduleRow row = (ScheduleRow) item;
                if (row.isAppointedWatchProgram()) {
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
