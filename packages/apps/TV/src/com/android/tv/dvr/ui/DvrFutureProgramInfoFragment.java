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
 * limitations under the License.
 */

package com.android.tv.dvr.ui;

import android.app.Activity;
import android.os.Bundle;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;
import com.android.tv.TvSingletons;
import com.android.tv.data.Program;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.util.Utils;
import java.util.List;

/**
 * A fragment which shows the formation of a program.
 */
public class DvrFutureProgramInfoFragment extends DvrGuidedStepFragment {
    private static final long ACTION_ID_VIEW_SCHEDULE = 1;
    private ScheduledRecording mScheduledRecording;
    private Program mProgram;

    @Override
    public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
        long startTime = mProgram.getStartTimeUtcMillis();
        // TODO(b/71717923): use R.string when the strings are finalized
        StringBuilder description = new StringBuilder()
                .append("This program will start at ")
                .append(Utils.getDurationString(getContext(), startTime, startTime, false));
        if (mScheduledRecording != null) {
            description.append("\nThis program has been scheduled for recording.");
        }
        return new GuidanceStylist.Guidance(
                mProgram.getTitle(), description.toString(), null, null);
    }

    @Override
    public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
        Activity activity = getActivity();
        mProgram = getArguments().getParcelable(DvrHalfSizedDialogFragment.KEY_PROGRAM);
        mScheduledRecording =
                TvSingletons.getSingletons(getContext())
                        .getDvrDataManager()
                        .getScheduledRecordingForProgramId(mProgram.getId());
        actions.add(
                new GuidedAction.Builder(activity)
                        .id(GuidedAction.ACTION_ID_OK)
                        .title(android.R.string.ok)
                        .build());
        if (mScheduledRecording != null) {
            actions.add(
                    new GuidedAction.Builder(activity)
                            .id(ACTION_ID_VIEW_SCHEDULE)
                            .title("View schedules")
                            .build());
        }

    }

    @Override
    public void onTrackedGuidedActionClicked(GuidedAction action) {
        if (action.getId() == ACTION_ID_VIEW_SCHEDULE) {
            DvrUiHelper.startSchedulesActivity(getContext(), mScheduledRecording);
            return;
        }
        dismissDialog();
    }

    @Override
    public String getTrackerPrefix() {
        return "DvrFutureProgramInfoFragment";
    }
}
