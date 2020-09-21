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

package com.android.tv.settings.suggestions;

import android.animation.ObjectAnimator;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import com.android.tv.settings.R;


/**
 * View for settings suggestion.
 * Handles dismiss button focus behavior.
 */
public class SuggestionItemView extends LinearLayout {
    private View mDissmissButton;
    private View mContainer;
    private View mItemContainer;
    private View mIcon;

    public SuggestionItemView(Context context) {
        super(context);
    }

    public SuggestionItemView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mDissmissButton = findViewById(R.id.dismiss_button);
        mContainer = findViewById(R.id.main_container);
        mIcon = findViewById(android.R.id.icon);
        mItemContainer = findViewById(R.id.item_container);

        int translateX = getResources().getDimensionPixelSize(
                R.dimen.suggestion_item_change_focus_translate_x);

        if (getResources().getConfiguration().getLayoutDirection() == View.LAYOUT_DIRECTION_RTL) {
            translateX = -translateX;
        }

        ObjectAnimator containerSlideOut = ObjectAnimator.ofFloat(mContainer,
                View.TRANSLATION_X, 0, translateX);

        ObjectAnimator containerSlideIn = ObjectAnimator.ofFloat(mContainer,
                View.TRANSLATION_X, translateX, 0);

        mDissmissButton.setOnFocusChangeListener(new OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (hasFocus) {
                    containerSlideOut.start();
                    mDissmissButton.setAlpha(1.0f);
                    mIcon.setAlpha(0.3f);
                    mItemContainer.setAlpha(0.3f);
                } else {
                    containerSlideIn.start();
                    mDissmissButton.setAlpha(0.4f);
                    mIcon.setAlpha(1.0f);
                    mItemContainer.setAlpha(1.0f);
                }
            }
        });
    }

    @Override
    public View focusSearch(View focused, int direction) {
        boolean isRTL =
                getResources().getConfiguration().getLayoutDirection() == View.LAYOUT_DIRECTION_RTL;
        if (focused.getId() == R.id.dismiss_button
                && ((isRTL && direction == ViewGroup.FOCUS_RIGHT)
                || (!isRTL && direction == ViewGroup.FOCUS_LEFT)))  {
            return mContainer;
        } else if (focused.getId() == R.id.main_container
                && ((isRTL && direction == ViewGroup.FOCUS_LEFT)
                || (!isRTL && direction == ViewGroup.FOCUS_RIGHT))) {
            return  mDissmissButton;
        }
        return super.focusSearch(focused, direction);
    }
}
