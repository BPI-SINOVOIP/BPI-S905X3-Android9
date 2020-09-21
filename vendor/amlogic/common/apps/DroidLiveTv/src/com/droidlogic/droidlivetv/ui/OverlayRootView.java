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

package com.droidlogic.droidlivetv.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.widget.FrameLayout;

import com.droidlogic.droidlivetv.DroidLiveTvActivity;

public class OverlayRootView extends FrameLayout {

    private final DroidLiveTvActivity mMainActivity;

    public OverlayRootView(Context context) {
        this(context, null, 0, 0);
    }

    public OverlayRootView(Context context, AttributeSet attrs) {
        this(context, attrs, 0, 0);
    }

    public OverlayRootView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public OverlayRootView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        mMainActivity = (DroidLiveTvActivity) context;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        return mMainActivity.dispatchKeyEvent(event) || super.dispatchKeyEvent(event);
    }
}