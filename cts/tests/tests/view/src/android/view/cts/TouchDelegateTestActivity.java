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

package android.view.cts;

import android.app.Activity;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.TouchDelegate;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.Button;

import java.util.ArrayDeque;
import java.util.Queue;

/**
 * The layout is:
 *                                    |       <-- top edge of RelativeLayout (parent)
 *                                    |
 *                                    |
 *                                    |       (touches in this region should go to parent only)
 *                                    |
 *                                    |
 *                                    |       <-- TouchDelegate boundary
 *                                    |
 *                                    |
 *                                    |       (touches in this region should go to button + parent)
 *                                    |
 *                                    |
 *                                    |       <-- Button top boundary
 *                                    |
 */
public class TouchDelegateTestActivity extends Activity {
    // Counters for click events. Must use waitForIdleSync() before accessing these
    private volatile int mParentClickCount = 0;
    private volatile int mButtonClickCount = 0;
    // Storage for MotionEvents received by the TouchDelegate
    private Queue<MotionEvent> mButtonEvents = new ArrayDeque<>();

    // Coordinates for injecting input events from the test
    public int x; // common X coordinate for all input events - center of the screen
    public int touchDelegateY; // Y coordinate for touches inside TouchDelegate area
    public int parentViewY; // Y coordinate for touches inside parent area only

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.touch_delegate_test_activity_layout);

        final Button button = findViewById(R.id.button);
        final View parent = findViewById(R.id.layout);

        parent.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        final int[] parentLocation = new int[2];
                        parent.getLocationOnScreen(parentLocation);
                        final int[] buttonLocation = new int[2];
                        button.getLocationOnScreen(buttonLocation);
                        x = parentLocation[0] + parent.getWidth() / 2;
                        final int gap = buttonLocation[1] - parentLocation[1];

                        Rect rect = new Rect();
                        button.getHitRect(rect);
                        rect.top -= gap / 2; // TouchDelegate is halfway between button and parent
                        parent.setTouchDelegate(new TouchDelegate(rect, button));
                        touchDelegateY = buttonLocation[1] - gap / 4;
                        parentViewY = parentLocation[1] + gap / 4;
                    }
                });

        parent.setOnClickListener(v -> mParentClickCount++);
        button.setOnClickListener(v -> mButtonClickCount++);
        button.setOnTouchListener((v, event) -> {
            mButtonEvents.add(MotionEvent.obtain(event));
            return TouchDelegateTestActivity.super.onTouchEvent(event);
        });
    }

    void resetCounters() {
        mParentClickCount = 0;
        mButtonClickCount = 0;
        mButtonEvents.clear();
    }

    int getButtonClickCount() {
        return mButtonClickCount;
    }

    int getParentClickCount() {
        return mParentClickCount;
    }

    /**
     * Remove and return the oldest MotionEvent. Caller must recycle the returned object.
     * @return the oldest MotionEvent that the Button has received
     */
    MotionEvent removeOldestButtonEvent() {
        return mButtonEvents.poll();
    }
}
