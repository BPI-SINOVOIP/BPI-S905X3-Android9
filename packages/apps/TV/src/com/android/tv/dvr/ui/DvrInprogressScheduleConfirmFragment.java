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

package com.android.tv.dvr.ui;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;
import android.util.Log;

import com.android.tv.R;
import com.android.tv.TvSingletons;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.data.api.Channel;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.dvr.ui.DvrConflictFragment.DvrChannelRecordConflictFragment;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

public class DvrInprogressScheduleConfirmFragment extends DvrGuidedStepFragment {
    private static final String TAG = "DvrInprogressScheduleConfirmFragment";
    private DvrManager mDvrManager;
    private DvrDataManager mDvrDataManager;
    private static final int ACTION_STOP_ALL = -1;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mDvrManager =
                TvSingletons.getSingletons(getContext())
                        .getDvrManager();
        mDvrDataManager =
                TvSingletons.getSingletons(getContext())
                .getDvrDataManager();
        SoftPreconditions.checkArgument(mDvrManager != null && mDvrDataManager != null);
        super.onCreate(savedInstanceState);
    }

    @Override
    public Guidance onCreateGuidance(Bundle savedInstanceState) {
        String title = getResources().getString(R.string.pvr_confirm_stop_in_progress_schedule);
        Drawable icon = getResources().getDrawable(R.drawable.ic_dvr, null);
        return new Guidance(title, null, null, icon);
    }

    @Override
    public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
        List<ScheduledRecording> startedScheduled = new ArrayList<ScheduledRecording>();
        startedScheduled = mDvrDataManager.getStartedRecordings();
        if (startedScheduled != null && startedScheduled.size() > 0) {
            actions.add(
                    new GuidedAction.Builder(getContext())
                            .id(ACTION_STOP_ALL)
                            .title(getResources().getString(R.string.pvr_confirm_stop_in_progress_schedule_stop_all))
                            .build());
            for (ScheduledRecording one : startedScheduled) {
                actions.add(
                        new GuidedAction.Builder(getContext())
                                .id((int)one.getId())
                                .title(one.getProgramDisplayTitle(getContext()))
                                .build());
            }
        }
    }

    @Override
    public void onTrackedGuidedActionClicked(GuidedAction action) {
        if (action != null) {
            int actionId = (int)action.getId();
            if (actionId == ACTION_STOP_ALL) {
                Log.d(TAG, "onTrackedGuidedActionClicked ACTION_STOP_ALL");
                mDvrManager.stopInprogressRecording();
            } else {
                ScheduledRecording schedule = getScheduledRecordingById(actionId);
                if (schedule != null) {
                    Log.d(TAG, "onTrackedGuidedActionClicked schedule " + schedule.getProgramDisplayTitle(getContext()));
                    mDvrManager.stopRecording(schedule);
                } else {
                    Log.d(TAG, "onTrackedGuidedActionClicked schedule " + actionId + " not found");
                }
            }
        }
        dismissDialog();
    }

    @Override
    public String getTrackerPrefix() {
        return "DvrInprogressScheduleConfirmFragment";
    }

    @Override
    public String getTrackerLabelForGuidedAction(GuidedAction action) {
        return super.getTrackerLabelForGuidedAction(action);
    }

    private ScheduledRecording getScheduledRecordingById(int actionId) {
        ScheduledRecording result = null;
        List<ScheduledRecording> startedScheduled = new ArrayList<ScheduledRecording>();
        startedScheduled = mDvrDataManager.getStartedRecordings();
        if (startedScheduled != null && startedScheduled.size() > 0) {
            for (ScheduledRecording one : startedScheduled) {
                if (actionId == one.getId()) {
                    result = one;
                }
            }
        }
        return result;
    }
}
