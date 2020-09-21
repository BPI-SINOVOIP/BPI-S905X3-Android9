/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.ui;

import android.content.Context;
import android.media.tv.TvInputInfo;
import android.media.tv.TvContract;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.data.api.Channel;

import com.droidlogic.app.tv.DroidLogicTvUtils;

public class InputBannerView extends LinearLayout implements TvTransitionManager.TransitionLayout {
    private final long mShowDurationMillis;

    private final Runnable mHideRunnable =
            new Runnable() {
                @Override
                public void run() {
                    ((MainActivity) getContext())
                            .getOverlayManager()
                            .hideOverlays(
                                    TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_DIALOG
                                            | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_SIDE_PANELS
                                            | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_PROGRAM_GUIDE
                                            | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_MENU
                                            | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_FRAGMENT);
                }
            };

    private TextView mInputLabelTextView;
    private TextView mSecondaryInputLabelTextView;
    private TextView mResolutionTextView;
    private TextView mCaptionTextView;
    private TextView mVchipTextView;

    public InputBannerView(Context context) {
        this(context, null, 0);
    }

    public InputBannerView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public InputBannerView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mShowDurationMillis =
                context.getResources().getInteger(R.integer.select_input_show_duration);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mInputLabelTextView = (TextView) findViewById(R.id.input_label);
        mSecondaryInputLabelTextView = (TextView) findViewById(R.id.secondary_input_label);
        mResolutionTextView = (TextView) findViewById(R.id.resolution);
        mCaptionTextView = (TextView) findViewById(R.id.av_caption);
        mVchipTextView = (TextView) findViewById(R.id.av_vchip);
    }

    public void updateLabel() {
        MainActivity mainActivity = (MainActivity) getContext();
        Channel channel = mainActivity.getCurrentChannel();
        if (channel == null || !channel.isPassthrough()) {
            return;
        }
        TvInputInfo input =
                mainActivity.getTvInputManagerHelper().getTvInputInfo(channel.getInputId());
        CharSequence customLabel = input.loadCustomLabel(getContext());
        CharSequence label = input.loadLabel(getContext());
        if (TextUtils.isEmpty(customLabel) || customLabel.equals(label)) {
            mInputLabelTextView.setText(label);
            mSecondaryInputLabelTextView.setVisibility(View.GONE);
        } else {
            mInputLabelTextView.setText(customLabel);
            mSecondaryInputLabelTextView.setText(label);
            mSecondaryInputLabelTextView.setVisibility(View.VISIBLE);
        }
        mResolutionTextView.setText(mainActivity.mQuickKeyInfo.getResolution());
        boolean hascaption = mainActivity.getTvView().hasClosedCaption();
        String vchip = mainActivity.mQuickKeyInfo.getCurrentRatingsString();
        final String DELIMITER = "  ";
        final String EMPTY = "";
        int inputType = input.getType();
        if (inputType == TvInputInfo.TYPE_COMPOSITE) {//TYPE_COMPOSITE is AV type.
            mCaptionTextView.setText(hascaption ? (DELIMITER + mainActivity.getString(R.string.closed_caption)) : EMPTY);
            if (DroidLogicTvUtils.getCurrentSignalType(getContext()).contains("ATSC")) {
                mVchipTextView.setText(TextUtils.isEmpty(vchip) ? EMPTY : (DELIMITER + vchip));
                mCaptionTextView.setVisibility(View.VISIBLE);
                mVchipTextView.setVisibility(View.VISIBLE);
            } else {
                mCaptionTextView.setVisibility(View.GONE);
                mVchipTextView.setVisibility(View.GONE);
            }
        } else if (inputType == TvInputInfo.TYPE_HDMI) {//TYPE_HDMI is HDMI type.
            mCaptionTextView.setVisibility(View.GONE);
            mVchipTextView.setVisibility(View.GONE);
        }
    }

    @Override
    public void onEnterAction(boolean fromEmptyScene) {
        removeCallbacks(mHideRunnable);
        postDelayed(mHideRunnable, mShowDurationMillis);
        updateLabel();
    }

    @Override
    public void onExitAction() {
        removeCallbacks(mHideRunnable);
    }
}
