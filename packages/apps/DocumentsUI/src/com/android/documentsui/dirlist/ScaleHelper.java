/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.documentsui.dirlist;

import static android.support.v4.util.Preconditions.checkState;
import static com.android.documentsui.base.SharedMinimal.VERBOSE;

import android.content.Context;
import android.os.Build;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.OnItemTouchListener;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import com.android.documentsui.base.Features;

import java.util.function.Consumer;

/**
 * Class providing support for gluing gesture scaling of the ui into RecyclerView + DocsUI.
 */
final class ScaleHelper {

    private static final String TAG = "ScaleHelper";

    private final Context mContext;
    private final Features mFeatures;
    private final Consumer<Float> mScaleCallback;

    private @Nullable ScaleGestureDetector mScaleDetector;

    public ScaleHelper(Context context, Features features, Consumer<Float> scaleCallback) {
        mContext = context;
        mFeatures = features;
        mScaleCallback = scaleCallback;
    }

    private boolean onInterceptTouchEvent(RecyclerView rv, MotionEvent e) {
        // Checking feature is deferred to runtime so the feature can be enabled
        // after this class has been setup.
        if (mFeatures.isGestureScaleEnabled() && mScaleDetector != null) {
            mScaleDetector.onTouchEvent(e);
        }

        return false;
    }

    void attach(RecyclerView view) {
        checkState(Build.IS_DEBUGGABLE);
        checkState(mScaleDetector == null);

        mScaleDetector = new ScaleGestureDetector(
            mContext,
            new ScaleGestureDetector.SimpleOnScaleGestureListener() {
                @Override
                public boolean onScale(ScaleGestureDetector detector) {
                    if (VERBOSE) Log.v(TAG,
                            "Received scale event: " + detector.getScaleFactor());
                    mScaleCallback.accept(detector.getScaleFactor());
                    return true;
                }
            });

        view.addOnItemTouchListener(
                new OnItemTouchListener() {
                    @Override
                    public boolean onInterceptTouchEvent(RecyclerView rv, MotionEvent e) {
                        return ScaleHelper.this.onInterceptTouchEvent(rv, e);
                    }

                    @Override
                    public void onTouchEvent(RecyclerView rv, MotionEvent e) {}

                    @Override
                    public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept) {}
        });
    }
}
