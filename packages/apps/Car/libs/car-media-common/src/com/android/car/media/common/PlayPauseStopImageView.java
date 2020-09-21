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
import android.graphics.PorterDuff;
import android.support.annotation.IntDef;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ImageView;

import com.android.car.apps.common.FabDrawable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Custom {@link android.widget.ImageButton} that has four custom states:
 * <ul>
 * <li>state_playing</li>
 * <li>state_paused</li>
 * <li>state_stopped</li>
 * <li>state_disabled</li>
 * </ul>
 */
public class PlayPauseStopImageView extends ImageView {
    private static final String TAG = "PlayPauseStopImageView";

    private static final int[] STATE_PAUSE = {R.attr.state_pause};
    private static final int[] STATE_STOP = {R.attr.state_stop};
    private static final int[] STATE_PLAY = {R.attr.state_play};
    private static final int[] STATE_DISABLED = {R.attr.state_disabled};

    /**
     * Possible states of this view
     */
    @IntDef({ACTION_PLAY, ACTION_STOP, ACTION_PAUSE, ACTION_DISABLED})
    @Retention(RetentionPolicy.SOURCE)
    public @interface Action {}

    /** Used when no action can be executed at this time */
    public static final int ACTION_DISABLED = 0;
    /** Used when the media source is ready to start playing */
    public static final int ACTION_PLAY = 1;
    /** Used when the media source is playing and it only support stop action */
    public static final int ACTION_STOP = 2;
    /** Used when the media source is playing and it supports pause action */
    public static final int ACTION_PAUSE = 3;

    private int mAction = ACTION_DISABLED;

    /**
     * Constructs an instance of this view
     */
    public PlayPauseStopImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setBackground(new FabDrawable(context));
    }

    /**
     * Sets the action to display on this view
     *
     * @param action one of {@link Action}
     */
    public void setAction(@Action int action) {
        mAction = action;
        refreshDrawableState();
    }

    /**
     * @return currently selected action
     */
    public int getAction() {
        return mAction;
    }

    @Override
    public int[] onCreateDrawableState(int extraSpace) {
        // + 1 so we can potentially add our custom PlayState
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);

        switch(mAction) {
            case ACTION_PLAY:
                mergeDrawableStates(drawableState, STATE_PLAY);
                break;
            case ACTION_STOP:
                mergeDrawableStates(drawableState, STATE_STOP);
                break;
            case ACTION_PAUSE:
                mergeDrawableStates(drawableState, STATE_PAUSE);
                break;
            case ACTION_DISABLED:
                mergeDrawableStates(drawableState, STATE_DISABLED);
                break;
            default:
                Log.e(TAG, "Unknown action: " + mAction);
        }
        if (getBackground() != null) {
            getBackground().setState(drawableState);
        }
        return drawableState;
    }

    /**
     * Updates the primary color of this view.
     * @param color fill or main color
     * @param tintColor contrast color
     */
    public void setPrimaryActionColor(int color, int tintColor) {
        ((FabDrawable) getBackground()).setFabAndStrokeColor(color);
        if (getDrawable() != null) {
            getDrawable().setColorFilter(tintColor, PorterDuff.Mode.SRC_IN);
        }
    }
}
