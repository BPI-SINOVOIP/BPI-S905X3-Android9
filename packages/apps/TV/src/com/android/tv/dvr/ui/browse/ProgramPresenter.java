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
 * limitations under the License.
 */

package com.android.tv.dvr.ui.browse;

import android.content.Context;
import android.media.tv.TvInputManager;
import android.util.Log;

import com.android.tv.R;
import com.android.tv.TvSingletons;
import com.android.tv.dvr.DvrWatchedPositionManager;
import com.android.tv.dvr.DvrWatchedPositionManager.WatchedPositionChangedListener;
import com.android.tv.dvr.data.RecordedProgram;
import com.android.tv.util.Utils;
import com.android.tv.util.TvClock;
import com.android.tv.data.Program;

/** Presents a {@link RecordedProgram} in the {@link DvrBrowseFragment}. */
public class ProgramPresenter extends DvrItemPresenter<Program> {
    private static final String TAG = "ProgramPresenter";
    private static final boolean DEBUG = true;

    private final TvClock mTvClock;

    protected final class ProgramViewHolder extends DvrItemViewHolder {
        private Program mProgram;
        private boolean mShowProgress;

        public ProgramViewHolder(RecordingCardView view) {
            super(view);
        }

        @Override
        protected void onBound(Program program) {
            mProgram = program;
        }

        @Override
        protected void onUnbound() {
            getView().reset();
        }
    }

    ProgramPresenter(Context context) {
        super(context);
        mTvClock = TvSingletons.getSingletons(mContext).getTvClock();
    }

    @Override
    public DvrItemViewHolder onCreateDvrItemViewHolder() {
        return new ProgramViewHolder(
                new RecordingCardView(mContext));
    }

    @Override
    public void onBindDvrItemViewHolder(DvrItemViewHolder baseHolder, Program program) {
        final ProgramViewHolder viewHolder = (ProgramViewHolder) baseHolder;
        final RecordingCardView cardView = viewHolder.getView();
        DetailsContent details = DetailsContent.createFromProgram(mContext, program);
        cardView.setTitle(details.getTitle());
        cardView.setImageUri(details.getLogoImageUri(), details.isUsingChannelLogo());
        cardView.setContent(generateMajorContent(program), null);
        cardView.setDetailBackgroundImageUri(details.getBackgroundImageUri());
    }

    private String generateMajorContent(Program program) {
        int dateDifference =
                Utils.computeDateDifference(
                        program.getStartTimeUtcMillis(), mTvClock.currentTimeMillis()/*System.currentTimeMillis()*/);
        if (dateDifference <= 0) {
            return mContext.getString(
                    R.string.dvr_date_today_time,
                    Utils.getDurationString(
                            mContext,
                            program.getStartTimeUtcMillis(),
                            program.getEndTimeUtcMillis(),
                            false,
                            false,
                            true,
                            0));
        } else if (dateDifference == 1) {
            return mContext.getString(
                    R.string.dvr_date_tomorrow_time,
                    Utils.getDurationString(
                            mContext,
                            program.getStartTimeUtcMillis(),
                            program.getEndTimeUtcMillis(),
                            false,
                            false,
                            true,
                            0));
        } else {
            return Utils.getDurationString(
                    mContext,
                    program.getStartTimeUtcMillis(),
                    program.getEndTimeUtcMillis(),
                    false,
                    true,
                    true,
                    0);
        }
    }
}
