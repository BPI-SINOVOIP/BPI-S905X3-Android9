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
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.widget.ImageView;

/**
 * An {@link ImageView} with a fixed aspect ratio. When using this view, either width or height
 * must be set to MATCH_PARENT.
 */
public class FixedRatioImageView extends ImageView {
    private float mAspectRatio;

    private static final float DEFAULT_ASPECT_RATIO = 1f;

    public FixedRatioImageView(Context context) {
        this(context, null);
    }

    public FixedRatioImageView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public FixedRatioImageView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.FixedRatioImageView);
        mAspectRatio = a.getFloat(R.styleable.FixedRatioImageView_aspect_ratio,
                DEFAULT_ASPECT_RATIO);
    }

    /**
     * Modify the aspect ratio of this view.
     *
     * @param aspectRatio a ratio between width and height. For example: 2 means this view's width
     *                    would be double the height, while 0.5 means that this view's width will
     *                    be half the height.
     */
    public void setAspectRatio(float aspectRatio) {
        mAspectRatio = aspectRatio;
        requestLayout();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        if (getLayoutParams().height == ViewGroup.LayoutParams.MATCH_PARENT) {
            int height = getDefaultSize(getSuggestedMinimumHeight(), heightMeasureSpec);
            setMeasuredDimension((int) (height * mAspectRatio), height);
        } else if (getLayoutParams().width == ViewGroup.LayoutParams.MATCH_PARENT) {
            int width = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
            setMeasuredDimension(width, (int) (width / mAspectRatio));
        } else {
            throw new IllegalArgumentException(
                    "Either width or height should be set to MATCH_PARENT");
        }
    }
}
