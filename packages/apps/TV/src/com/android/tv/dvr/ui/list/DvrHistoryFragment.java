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

import android.os.Bundle;
import android.support.v17.leanback.app.DetailsFragment;
import android.support.v17.leanback.widget.ClassPresenterSelector;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import com.android.tv.R;
import com.android.tv.TvSingletons;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.data.RecordedProgram;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.dvr.ui.list.SchedulesHeaderRowPresenter.DateHeaderRowPresenter;

/** A fragment to show the DVR history. */
public class DvrHistoryFragment extends DetailsFragment
        implements DvrDataManager.ScheduledRecordingListener,
        DvrDataManager.RecordedProgramListener {

    private DvrHistoryRowAdapter mRowsAdapter;
    private TextView mEmptyInfoScreenView;
    private DvrDataManager mDvrDataManager;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ClassPresenterSelector presenterSelector = new ClassPresenterSelector();
        presenterSelector.addClassPresenter(
                SchedulesHeaderRow.class, new DateHeaderRowPresenter(getContext()));
        presenterSelector.addClassPresenter(
                ScheduleRow.class, new ScheduleRowPresenter(getContext()));
        TvSingletons singletons = TvSingletons.getSingletons(getContext());
        mRowsAdapter = new DvrHistoryRowAdapter(
                getContext(), presenterSelector, singletons.getClock());
        setAdapter(mRowsAdapter);
        mRowsAdapter.start();
        mDvrDataManager = singletons.getDvrDataManager();
        mDvrDataManager.addScheduledRecordingListener(this);
        mDvrDataManager.addRecordedProgramListener(this);
        mEmptyInfoScreenView = (TextView) getActivity().findViewById(R.id.empty_info_screen);
    }

    @Override
    public void onDestroy() {
        mDvrDataManager.removeScheduledRecordingListener(this);
        mDvrDataManager.removeRecordedProgramListener(this);
        super.onDestroy();
    }

    /** Shows the empty message. */
    void showEmptyMessage() {
        mEmptyInfoScreenView.setText(R.string.dvr_history_empty_state);
        if (mEmptyInfoScreenView.getVisibility() != View.VISIBLE) {
            mEmptyInfoScreenView.setVisibility(View.VISIBLE);
        }
    }

    /** Hides the empty message. */
    void hideEmptyMessage() {
        if (mEmptyInfoScreenView.getVisibility() == View.VISIBLE) {
            mEmptyInfoScreenView.setVisibility(View.GONE);
        }
    }

    @Override
    public View onInflateTitleView(
            LayoutInflater inflater, ViewGroup parent, Bundle savedInstanceState) {
        // Workaround of b/31046014
        return null;
    }

    @Override
    public void onScheduledRecordingAdded(ScheduledRecording... scheduledRecordings) {
        if (mRowsAdapter != null) {
            for (ScheduledRecording recording : scheduledRecordings) {
                mRowsAdapter.onScheduledRecordingAdded(recording);
            }
            if (mRowsAdapter.size() > 0) {
                hideEmptyMessage();
            }
        }
    }

    @Override
    public void onScheduledRecordingRemoved(ScheduledRecording... scheduledRecordings) {
        if (mRowsAdapter != null) {
            for (ScheduledRecording recording : scheduledRecordings) {
                mRowsAdapter.onScheduledRecordingRemoved(recording);
            }
            if (mRowsAdapter.size() == 0) {
                showEmptyMessage();
            }
        }
    }

    @Override
    public void onScheduledRecordingStatusChanged(ScheduledRecording... scheduledRecordings) {
        if (mRowsAdapter != null) {
            for (ScheduledRecording recording : scheduledRecordings) {
                mRowsAdapter.onScheduledRecordingUpdated(recording);
            }
            if (mRowsAdapter.size() == 0) {
                showEmptyMessage();
            } else {
                hideEmptyMessage();
            }
        }
    }

    @Override
    public void onRecordedProgramsAdded(RecordedProgram... recordedPrograms) {
        if (mRowsAdapter != null) {
            for (RecordedProgram p : recordedPrograms) {
                mRowsAdapter.onScheduledRecordingAdded(p);
            }
            if (mRowsAdapter.size() > 0) {
                hideEmptyMessage();
            }
        }

    }

    @Override
    public void onRecordedProgramsChanged(RecordedProgram... recordedPrograms) {
        if (mRowsAdapter != null) {
            for (RecordedProgram program : recordedPrograms) {
                mRowsAdapter.onScheduledRecordingUpdated(program);
            }
            if (mRowsAdapter.size() == 0) {
                showEmptyMessage();
            } else {
                hideEmptyMessage();
            }
        }
    }

    @Override
    public void onRecordedProgramsRemoved(RecordedProgram... recordedPrograms) {
        if (mRowsAdapter != null) {
            for (RecordedProgram p : recordedPrograms) {
                mRowsAdapter.onScheduledRecordingRemoved(p);
            }
            if (mRowsAdapter.size() == 0) {
                showEmptyMessage();
            }
        }
    }
}
