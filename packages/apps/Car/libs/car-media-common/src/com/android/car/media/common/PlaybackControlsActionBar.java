/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.car.media.common;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.ProgressBar;

import androidx.car.widget.ActionBar;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

/**
 * Implementation of {@link PlaybackControls} that uses the support library {@link ActionBar}
 */
public class PlaybackControlsActionBar extends ActionBar implements PlaybackControls {
    private static final String TAG = "PlaybackView";

    private PlayPauseStopImageView mPlayPauseStopImageView;
    private View mPlayPauseStopImageContainer;
    private ProgressBar mSpinner;
    private Context mContext;
    private ImageButton mSkipPrevButton;
    private ImageButton mSkipNextButton;
    private ImageButton mTrackListButton;
    private Listener mListener;
    private PlaybackModel mModel;
    private PlaybackModel.PlaybackObserver mObserver = new PlaybackModel.PlaybackObserver() {
        @Override
        public void onPlaybackStateChanged() {
            updateState();
            updateCustomActions();
        }

        @Override
        public void onSourceChanged() {
            updateState();
            updateCustomActions();
            updateAccentColor();
        }

        @Override
        public void onMetadataChanged() {
            updateCustomActions();  // rating might have changed
        }
    };
    private ColorStateList mIconsColor;

    /** Creates a {@link PlaybackControlsActionBar} view */
    public PlaybackControlsActionBar(Context context) {
        this(context, null, 0, 0);
    }

    /** Creates a {@link PlaybackControlsActionBar} view */
    public PlaybackControlsActionBar(Context context, AttributeSet attrs) {
        this(context, attrs, 0, 0);
    }

    /** Creates a {@link PlaybackControlsActionBar} view */
    public PlaybackControlsActionBar(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    /** Creates a {@link PlaybackControlsActionBar} view */
    public PlaybackControlsActionBar(Context context, AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init(context);
    }

    @Override
    public void setModel(PlaybackModel model) {
        if (mModel != null) {
            mModel.unregisterObserver(mObserver);
        }
        mModel = model;
        if (mModel != null) {
            mModel.registerObserver(mObserver);
        }
        updateAccentColor();
    }

    private void init(Context context) {
        mContext = context;

        mPlayPauseStopImageContainer = inflate(context, R.layout.car_play_pause_stop_button_layout,
                null);
        mPlayPauseStopImageContainer.setOnClickListener(this::onPlayPauseStopClicked);
        mPlayPauseStopImageView = mPlayPauseStopImageContainer.findViewById(R.id.play_pause_stop);
        mSpinner = mPlayPauseStopImageContainer.findViewById(R.id.spinner);
        mPlayPauseStopImageView.setAction(PlayPauseStopImageView.ACTION_DISABLED);
        mPlayPauseStopImageView.setOnClickListener(this::onPlayPauseStopClicked);

        mIconsColor = context.getResources().getColorStateList(R.color.playback_control_color,
                null);

        mSkipPrevButton = createIconButton(mContext, mIconsColor,
                context.getDrawable(R.drawable.ic_skip_previous));
        mSkipPrevButton.setVisibility(INVISIBLE);
        mSkipPrevButton.setOnClickListener(v -> {
            if (mModel != null) {
                mModel.onSkipPreviews();
            }
        });
        mSkipNextButton = createIconButton(mContext, mIconsColor,
                context.getDrawable(R.drawable.ic_skip_next));
        mSkipNextButton.setVisibility(INVISIBLE);
        mSkipNextButton.setOnClickListener(v -> {
            if (mModel != null) {
                mModel.onSkipNext();
            }
        });
        mTrackListButton = createIconButton(mContext, mIconsColor,
                context.getDrawable(R.drawable.ic_tracklist));
        mTrackListButton.setOnClickListener(v -> {
            if (mListener != null) {
                mListener.onToggleQueue();
            }
        });

        ImageButton overflowButton = createIconButton(context, mIconsColor,
                context.getDrawable(androidx.car.R.drawable.ic_overflow));

        setView(mPlayPauseStopImageContainer, ActionBar.SLOT_MAIN);
        setView(mSkipPrevButton, ActionBar.SLOT_LEFT);
        setView(mSkipNextButton, ActionBar.SLOT_RIGHT);
        setExpandCollapseView(overflowButton);
    }

    private ImageButton createIconButton(Context context, ColorStateList csl, Drawable icon) {
        ImageButton button = new ImageButton(context, null, 0, R.style.PlaybackControl);
        button.setImageDrawable(icon);
        return button;
    }

    private void updateState() {
        if (mModel != null) {
            mPlayPauseStopImageView.setVisibility(View.VISIBLE);
            mPlayPauseStopImageView.setAction(convertMainAction(mModel.getMainAction()));
        } else {
            mPlayPauseStopImageView.setVisibility(View.INVISIBLE);
        }
        mSpinner.setVisibility(mModel != null && mModel.isBuffering()
                ? View.VISIBLE : View.INVISIBLE);
        mSkipPrevButton.setVisibility(mModel != null && mModel.isSkipPreviewsEnabled()
                ? View.VISIBLE : View.INVISIBLE);
        mSkipNextButton.setVisibility(mModel != null && mModel.isSkipNextEnabled()
                ? View.VISIBLE : View.INVISIBLE);
    }

    @PlayPauseStopImageView.Action
    private int convertMainAction(@PlaybackModel.Action int action) {
        switch (action) {
            case PlaybackModel.ACTION_DISABLED:
                return PlayPauseStopImageView.ACTION_DISABLED;
            case PlaybackModel.ACTION_PLAY:
                return PlayPauseStopImageView.ACTION_PLAY;
            case PlaybackModel.ACTION_PAUSE:
                return PlayPauseStopImageView.ACTION_PAUSE;
            case PlaybackModel.ACTION_STOP:
                return PlayPauseStopImageView.ACTION_STOP;
        }
        Log.w(TAG, "Unknown action: " + action);
        return PlayPauseStopImageView.ACTION_DISABLED;
    }

    private void updateAccentColor() {
        int color = getMediaSourceColor();
        int tintColor = ColorChecker.getTintColor(mContext, color);
        mPlayPauseStopImageView.setPrimaryActionColor(color, tintColor);
        mSpinner.setIndeterminateTintList(ColorStateList.valueOf(color));
    }

    private int getMediaSourceColor() {
        int defaultColor = mContext.getResources().getColor(android.R.color.background_dark, null);
        MediaSource mediaSource = mModel != null ? mModel.getMediaSource() : null;
        return mediaSource != null ? mediaSource.getAccentColor(defaultColor) : defaultColor;
    }

    private List<ImageButton> getExtraActions() {
        List<ImageButton> extraActions = new ArrayList<>();
        if (mModel != null && mModel.hasQueue()) {
            extraActions.add(mTrackListButton);
        }
        return extraActions;
    }

    private void updateCustomActions() {
        if (mModel == null) {
            setViews(new ImageButton[0]);
            return;
        }
        List<ImageButton> combinedActions = new ArrayList<>();
        combinedActions.addAll(getExtraActions());
        combinedActions.addAll(mModel.getCustomActions()
                .stream()
                .map(action -> {
                    ImageButton button = createIconButton(getContext(), mIconsColor, action.mIcon);
                    button.setOnClickListener(view ->
                            mModel.onCustomAction(action.mAction, action.mExtras));
                    return button;
                })
                .collect(Collectors.toList()));
        setViews(combinedActions.toArray(new ImageButton[combinedActions.size()]));
    }

    private void onPlayPauseStopClicked(View view) {
        if (mModel == null) {
            return;
        }
        switch (mPlayPauseStopImageView.getAction()) {
            case PlayPauseStopImageView.ACTION_PLAY:
                mModel.onPlay();
                break;
            case PlayPauseStopImageView.ACTION_PAUSE:
                mModel.onPause();
                break;
            case PlayPauseStopImageView.ACTION_STOP:
                mModel.onStop();
                break;
            default:
                Log.i(TAG, "Play/Pause/Stop clicked on invalid state");
                break;
        }
    }

    @Override
    public void setQueueVisible(boolean visible) {
        mTrackListButton.setActivated(visible);
    }

    @Override
    public void setListener(Listener listener) {
        mListener = listener;
    }

    @Override
    public void close() {
        // TODO(b/77242566): This will be implemented once the corresponding change is published in
        // Car Support Library.
    }

    @Override
    public void setAnimationViewGroup(ViewGroup animationViewGroup) {
        // TODO(b/77242566): This will be implemented once the corresponding change is published in
        // Car Support Library.
    }
}
