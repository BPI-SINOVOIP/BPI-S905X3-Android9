/*
 * Copyright (C) 2009 The Android Open Source Project
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import android.app.Instrumentation;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.InputDevice;
import android.view.MotionEvent;


import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class TouchDelegateTest {
    private Instrumentation mInstrumentation;
    private TouchDelegateTestActivity mActivity;

    @Rule
    public ActivityTestRule<TouchDelegateTestActivity> mActivityRule =
            new ActivityTestRule<>(TouchDelegateTestActivity.class);

    @Before
    public void setup() throws Throwable {
        mActivity = mActivityRule.getActivity();
        mActivity.resetCounters();
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mInstrumentation.waitForIdleSync();
    }

    @Test
    public void testParentClick() {
        // If only clicking parent, button should not receive click
        clickParent();
        assertEquals(0, mActivity.getButtonClickCount());
        assertEquals(1, mActivity.getParentClickCount());

        // When clicking TouchDelegate area, both parent and button
        // should receive DOWN and UP events. However, click will only be generated for the button
        mActivity.resetCounters();
        clickTouchDelegateArea();
        assertEquals(1, mActivity.getButtonClickCount());
        assertEquals(0, mActivity.getParentClickCount());

        // Ensure parent can still receive clicks after TouchDelegate has been activated once
        mActivity.resetCounters();
        clickParent();
        assertEquals(0, mActivity.getButtonClickCount());
        assertEquals(1, mActivity.getParentClickCount());
    }

    @Test
    public void testCancelEvent() {
        // Ensure events with ACTION_CANCEL are received by the TouchDelegate
        final long downTime = SystemClock.uptimeMillis();
        dispatchMotionEventToActivity(MotionEvent.ACTION_DOWN, mActivity.touchDelegateY,
                downTime);
        dispatchMotionEventToActivity(MotionEvent.ACTION_CANCEL, mActivity.touchDelegateY,
                downTime);
        mInstrumentation.waitForIdleSync();

        ensureOldestActionEquals(MotionEvent.ACTION_DOWN);
        ensureOldestActionEquals(MotionEvent.ACTION_CANCEL);

        assertNull(mActivity.removeOldestButtonEvent());
        assertEquals(0, mActivity.getButtonClickCount());
        assertEquals(0, mActivity.getParentClickCount());
    }

    @Test
    public void testTwoPointers() {
        // Ensure ACTION_POINTER_DOWN and ACTION_POINTER_UP are forwarded to the target view
        // by the TouchDelegate
        final long downTime = SystemClock.uptimeMillis();
        dispatchMotionEventToActivity(MotionEvent.ACTION_DOWN, mActivity.touchDelegateY, downTime);
        dispatchMotionEventToActivity(MotionEvent.ACTION_MOVE, mActivity.touchDelegateY, downTime);
        int actionPointer1Down =
                (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT) + MotionEvent.ACTION_POINTER_DOWN;
        dispatchMultiTouchMotionEventToActivity(actionPointer1Down, 2,
                mActivity.touchDelegateY, downTime);
        dispatchMultiTouchMotionEventToActivity(MotionEvent.ACTION_MOVE, 2,
                mActivity.touchDelegateY, downTime);
        int actionPointer1Up =
                (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT) + MotionEvent.ACTION_POINTER_UP;
        dispatchMultiTouchMotionEventToActivity(actionPointer1Up, 2,
                mActivity.touchDelegateY, downTime);
        dispatchMotionEventToActivity(MotionEvent.ACTION_UP, mActivity.touchDelegateY, downTime);
        mInstrumentation.waitForIdleSync();

        ensureOldestActionEquals(MotionEvent.ACTION_DOWN);
        ensureOldestActionEquals(MotionEvent.ACTION_MOVE);
        ensureOldestActionEquals(MotionEvent.ACTION_POINTER_DOWN);
        ensureOldestActionEquals(MotionEvent.ACTION_MOVE);
        ensureOldestActionEquals(MotionEvent.ACTION_POINTER_UP);
        ensureOldestActionEquals(MotionEvent.ACTION_UP);
    }

    private void ensureOldestActionEquals(int action) {
        MotionEvent event = mActivity.removeOldestButtonEvent();
        assertNotNull(event);
        assertEquals(action, event.getActionMasked());
        event.recycle();
    }

    private void clickParent() {
        click(mActivity.parentViewY);
    }

    private void clickTouchDelegateArea() {
        click(mActivity.touchDelegateY);
    }

    // Low-level input-handling functions for the activity

    private void click(int y) {
        final long downTime = SystemClock.uptimeMillis();
        dispatchMotionEventToActivity(MotionEvent.ACTION_DOWN, y, downTime);
        dispatchMotionEventToActivity(MotionEvent.ACTION_UP, y, downTime);
        mInstrumentation.waitForIdleSync();
    }

    private void dispatchMotionEventToActivity(int action, int y, long downTime) {
        mActivity.runOnUiThread(() -> {
            final long eventTime = SystemClock.uptimeMillis();
            final MotionEvent event = MotionEvent.obtain(downTime, eventTime, action,
                    mActivity.x, y, 0);
            event.setSource(InputDevice.SOURCE_TOUCHSCREEN);
            mActivity.dispatchTouchEvent(event);
            event.recycle();
        });
    }

    private void dispatchMultiTouchMotionEventToActivity(int action, int pointerCount,
            int y, long downTime) {
        mActivity.runOnUiThread(() -> {
            final long eventTime = SystemClock.uptimeMillis();
            MotionEvent.PointerProperties[] properties =
                    new MotionEvent.PointerProperties[pointerCount];
            MotionEvent.PointerCoords[] coords = new MotionEvent.PointerCoords[pointerCount];

            for (int i = 0; i < pointerCount; i++) {
                properties[i] = new MotionEvent.PointerProperties();
                properties[i].id = i;
                properties[i].toolType = MotionEvent.TOOL_TYPE_FINGER;
                coords[i] = new MotionEvent.PointerCoords();
                coords[i].x = mActivity.x + i * 10; // small offset so that pointers do not overlap
                coords[i].y = y;
            }

            final MotionEvent event = MotionEvent.obtain(downTime, eventTime, action, pointerCount,
                    properties, coords, 0, 0, 0, 0,
                    0, 0, InputDevice.SOURCE_TOUCHSCREEN, 0);

            mActivity.dispatchTouchEvent(event);
            event.recycle();
        });
    }
}
