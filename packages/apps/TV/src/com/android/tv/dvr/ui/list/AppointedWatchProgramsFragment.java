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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.support.v17.leanback.app.DetailsFragment;
import android.support.v17.leanback.widget.ClassPresenterSelector;
import com.android.tv.R;
import com.android.tv.TvSingletons;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.data.RecordedProgram;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.dvr.ui.list.SchedulesHeaderRowPresenter.DateHeaderRowPresenter;
import com.android.tv.data.Program;

/** A fragment to show the DVR history. */
public class AppointedWatchProgramsFragment extends DetailsFragment
        implements DvrDataManager.AppointedProgramListener {

    private AppointedWatchProgramAdapter mRowsAdapter;
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
        mDvrDataManager = singletons.getDvrDataManager();
        mRowsAdapter =
                new AppointedWatchProgramAdapter(
                        getContext(),
                        presenterSelector,
                        singletons.getClock(),
                        mDvrDataManager);
        setAdapter(mRowsAdapter);
        mRowsAdapter.start();
        mDvrDataManager.addAppointedProgramListener(this);
        mEmptyInfoScreenView = (TextView) getActivity().findViewById(R.id.empty_info_screen);
        if (mRowsAdapter.size() == 0) {
            showEmptyMessage();
        }
    }

    @Override
    public void onDestroy() {
        mDvrDataManager.removeAppointedProgramListener(this);
        super.onDestroy();
    }

    /** Shows the empty message. */
    void showEmptyMessage() {
        mEmptyInfoScreenView.setText(R.string.dvr_appointed_watch_empty_state);
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
    public void onAppointedProgramsAdded(Program... programs) {
        if (mRowsAdapter != null) {
            for (Program p : programs) {
                mRowsAdapter.onScheduledRecordingAdded(p);
            }
            if (mRowsAdapter.size() > 0) {
                hideEmptyMessage();
            }
        }
    }

    @Override
    public void onAppointedProgramsRemoved(Program... programs) {
        if (mRowsAdapter != null) {
            for (Program p : programs) {
                mRowsAdapter.onScheduledRecordingRemoved(p);
            }
            if (mRowsAdapter.size() == 0) {
                showEmptyMessage();
            }
        }
    }
}
