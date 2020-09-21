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

import android.os.Bundle;
import android.support.v17.leanback.app.DetailsFragment;
import com.android.tv.TvSingletons;
import com.android.tv.dvr.data.ScheduledRecording;

/** {@link DetailsFragment} for recordings in DVR. */
abstract class RecordingDetailsFragment extends DvrDetailsFragment {
    private ScheduledRecording mRecording;

    @Override
    protected void onCreateInternal() {
        setDetailsOverviewRow(
                DetailsContent.createFromScheduledRecording(getContext(), mRecording));
    }

    @Override
    protected boolean onLoadRecordingDetails(Bundle args) {
        long scheduledRecordingId = args.getLong(DvrDetailsActivity.RECORDING_ID);
        mRecording =
                TvSingletons.getSingletons(getContext())
                        .getDvrDataManager()
                        .getScheduledRecording(scheduledRecordingId);
        return mRecording != null;
    }

    protected ScheduledRecording getScheduledRecording() {
        return mRecording;
    }

    /** Returns {@link ScheduledRecording} for the current fragment. */
    public ScheduledRecording getRecording() {
        return mRecording;
    }
}
