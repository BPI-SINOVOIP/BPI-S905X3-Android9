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

package com.android.tv.dvr.ui.browse;

import android.content.res.Resources;
import android.os.Bundle;
import android.content.Intent;
import android.content.Context;
import android.support.v17.leanback.widget.Action;
import android.support.v17.leanback.widget.OnActionClickedListener;
import android.support.v17.leanback.widget.SparseArrayObjectAdapter;

import com.android.tv.R;
import com.android.tv.TvSingletons;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.ui.DvrUiHelper;
import com.android.tv.dvr.ui.browse.DvrDetailsActivity;
import com.android.tv.data.Program;
import com.android.tv.data.ProgramDataManager;

/** {@link AppointedWatchDetailsFragment} for appoint watch in epg. */
public class AppointedWatchDetailsFragment extends DvrDetailsFragment implements DvrDataManager.AppointedProgramListener {
    private static final int ACTION_CANCEL = 1;

    private Program mProgram;
    private boolean mPaused;
    private DvrDataManager mDvrDataManager;
    DvrManager mDvrManager;

    @Override
    public void onCreate(Bundle savedInstance) {
        super.onCreate(savedInstance);
        mDvrDataManager = TvSingletons.getSingletons(getContext()).getDvrDataManager();
        mDvrDataManager.addAppointedProgramListener(this);
        mDvrManager = TvSingletons.getSingletons(getContext()).getDvrManager();
    }

    @Override
    protected void onCreateInternal() {
        setDetailsOverviewRow(
                DetailsContent.createFromProgram(getContext(), mProgram));
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mPaused) {
            updateActions();
            mPaused = false;
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        mPaused = true;
    }

    @Override
    public void onDestroy() {
        mDvrDataManager.removeAppointedProgramListener(this);
        super.onDestroy();
    }

    @Override
    protected boolean onLoadRecordingDetails(Bundle args) {
        long programId = args.getLong(DvrDetailsActivity.RECORDING_ID);
        mProgram = ProgramDataManager.getProgram(getContext(), programId);
        return mProgram != null;
    }

    public Program getProgram() {
        return mProgram;
    }

    @Override
    protected SparseArrayObjectAdapter onCreateActionsAdapter() {
        SparseArrayObjectAdapter adapter =
                new SparseArrayObjectAdapter(new ActionPresenterSelector());
        Resources res = getResources();
        adapter.set(
                ACTION_CANCEL,
                new Action(
                        ACTION_CANCEL,
                        res.getString(R.string.dvr_detail_cancel_appoint),
                        null,
                        res.getDrawable(R.drawable.ic_dvr_cancel_32dp)));
        return adapter;
    }

    @Override
    protected OnActionClickedListener onCreateOnActionClickedListener() {
        return new OnActionClickedListener() {
            @Override
            public void onActionClicked(Action action) {
                long actionId = action.getId();
                if (actionId == ACTION_CANCEL) {
                    if (mProgram != null) {
                        sendUpdateAppointedProgramIntent(mProgram.getId(), false);
                    }
                }
                getActivity().finish();
            }
        };
    }

    private void sendUpdateAppointedProgramIntent(long programId, boolean status) {
        Context context = getContext();
        if (context != null && programId != -1L) {
            //use dvrmanager to set it if action is excuted in app
            if (mDvrManager != null) {
                if (status) {
                    mDvrManager.addAppointedWatchProgram(mProgram);
                } else {
                    mDvrManager.removeAppointedWatchProgram(mProgram);
                }
            }
        }
    }

    @Override
    public void onAppointedProgramsAdded(Program... programs) {}

    @Override
    public void onAppointedProgramsRemoved(Program... programs) {
        for (Program program : programs) {
            if (mProgram != null && program.getId() == mProgram.getId()) {
                getActivity().finish();
            }
        }
    }
}
