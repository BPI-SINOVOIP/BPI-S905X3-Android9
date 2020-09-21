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
 * limitations under the License
 */

package com.android.tv.dvr.ui.list;

import android.os.Bundle;
import android.support.v17.leanback.widget.ClassPresenterSelector;
import com.android.tv.R;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.dvr.ui.list.SchedulesHeaderRowPresenter.DateHeaderRowPresenter;

/** A fragment to show the list of schedule recordings. */
public class DvrSchedulesFragment extends BaseDvrSchedulesFragment {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getRowsAdapter().size() == 0) {
            showEmptyMessage(R.string.dvr_schedules_empty_state);
        }
    }

    @Override
    public SchedulesHeaderRowPresenter onCreateHeaderRowPresenter() {
        return new DateHeaderRowPresenter(getContext());
    }

    @Override
    public ScheduleRowPresenter onCreateRowPresenter() {
        return new ScheduleRowPresenter(getContext());
    }

    @Override
    public ScheduleRowAdapter onCreateRowsAdapter(ClassPresenterSelector presenterSelector) {
        return new ScheduleRowAdapter(getContext(), presenterSelector);
    }

    @Override
    public void onScheduledRecordingAdded(ScheduledRecording... scheduledRecordings) {
        super.onScheduledRecordingAdded(scheduledRecordings);
        if (getRowsAdapter().size() > 0) {
            hideEmptyMessage();
        }
    }

    @Override
    public void onScheduledRecordingRemoved(ScheduledRecording... scheduledRecordings) {
        super.onScheduledRecordingRemoved(scheduledRecordings);
        if (getRowsAdapter().size() == 0) {
            showEmptyMessage(R.string.dvr_schedules_empty_state);
        }
    }

    @Override
    protected int getFirstItemPosition() {
        Bundle args = getArguments();
        ScheduledRecording recording = null;
        if (args != null) {
            recording = args.getParcelable(SCHEDULES_KEY_SCHEDULED_RECORDING);
        }
        final int selectedPostion =
                getRowsAdapter().indexOf(getRowsAdapter().findRowByScheduledRecording(recording));
        if (selectedPostion != -1) {
            return selectedPostion;
        }
        return super.getFirstItemPosition();
    }
}
