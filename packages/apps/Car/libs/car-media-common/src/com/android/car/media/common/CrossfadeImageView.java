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
package com.android.car.media.common;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;

import java.util.Objects;

/**
 * A view where updating the image will show certain animations. Current animations include fading
 * in and scaling down the new image.
 */
public class CrossfadeImageView extends FrameLayout {
    private final TopCropImageView mImageView1;
    private final TopCropImageView mImageView2;

    private TopCropImageView mActiveImageView;
    private TopCropImageView mInactiveImageView;

    private Bitmap mCurrentBitmap = null;
    private Integer mCurrentColor = null;
    private Animation mImageInAnimation;
    private Animation mImageOutAnimation;

    public CrossfadeImageView(Context context) {
        this(context, null);
    }

    public CrossfadeImageView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public CrossfadeImageView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public CrossfadeImageView(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        LayoutParams lp = new LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        ImageView imageViewBackground = new ImageView(context, attrs, defStyleAttr, defStyleRes);
        imageViewBackground.setLayoutParams(lp);
        imageViewBackground.setBackgroundColor(Color.BLACK);
        addView(imageViewBackground);
        mImageView1 = new TopCropImageView(context, attrs, defStyleAttr, defStyleRes);
        mImageView1.setLayoutParams(lp);
        addView(mImageView1);
        mImageView2 = new TopCropImageView(context, attrs, defStyleAttr, defStyleRes);
        mImageView2.setLayoutParams(lp);
        addView(mImageView2);

        mActiveImageView = mImageView1;
        mInactiveImageView = mImageView2;

        mImageInAnimation = AnimationUtils.loadAnimation(context, R.anim.image_in);
        mImageInAnimation.setInterpolator(new DecelerateInterpolator());
        mImageOutAnimation = AnimationUtils.loadAnimation(context, R.anim.image_out);
    }

    /**
     * Sets the image to show on this view
     *
     * @param bitmap image to show
     * @param showAnimation whether the transition should be animated or not.
     */
    public void setImageBitmap(Bitmap bitmap, boolean showAnimation) {
        if (Objects.equals(mCurrentBitmap, bitmap)) {
            return;
        }

        mCurrentBitmap = bitmap;
        mCurrentColor = null;
        mInactiveImageView.setImageBitmap(bitmap);
        if (showAnimation) {
            animateViews();
        } else {
            mActiveImageView.setImageBitmap(bitmap);
        }
    }

    /**
     * Sets a plain color as background, with an animated transition
     */
    @Override
    public void setBackgroundColor(int color) {
        if (mCurrentColor != null && mCurrentColor == color) {
            return;
        }
        mInactiveImageView.setImageBitmap(null);
        mCurrentBitmap = null;
        mCurrentColor = color;
        mInactiveImageView.setBackgroundColor(color);
        animateViews();
    }

    private final Animation.AnimationListener mAnimationListener =
            new Animation.AnimationListener() {
        @Override
        public void onAnimationEnd(Animation animation) {
            if (mInactiveImageView != null) {
                mInactiveImageView.setVisibility(View.GONE);
            }
        }

        @Override
        public void onAnimationStart(Animation animation) { }

        @Override
        public void onAnimationRepeat(Animation animation) { }
    };

    private void animateViews() {
        mInactiveImageView.setVisibility(View.VISIBLE);
        mInactiveImageView.startAnimation(mImageInAnimation);
        mInactiveImageView.bringToFront();
        mActiveImageView.startAnimation(mImageOutAnimation);
        mImageOutAnimation.setAnimationListener(mAnimationListener);
        if (mActiveImageView == mImageView1) {
            mActiveImageView = mImageView2;
            mInactiveImageView = mImageView1;
        } else {
            mActiveImageView = mImageView1;
            mInactiveImageView = mImageView2;
        }
    }
}
