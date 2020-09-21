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

package android.accessibilityservice.cts;

import static android.accessibilityservice.cts.utils.AsyncUtils.await;
import static android.accessibilityservice.cts.utils.AsyncUtils.waitOn;
import static android.accessibilityservice.cts.utils.GestureUtils.add;
import static android.accessibilityservice.cts.utils.GestureUtils.click;
import static android.accessibilityservice.cts.utils.GestureUtils.dispatchGesture;
import static android.accessibilityservice.cts.utils.GestureUtils.distance;
import static android.accessibilityservice.cts.utils.GestureUtils.drag;
import static android.accessibilityservice.cts.utils.GestureUtils.endTimeOf;
import static android.accessibilityservice.cts.utils.GestureUtils.lastPointOf;
import static android.accessibilityservice.cts.utils.GestureUtils.longClick;
import static android.accessibilityservice.cts.utils.GestureUtils.pointerDown;
import static android.accessibilityservice.cts.utils.GestureUtils.pointerUp;
import static android.accessibilityservice.cts.utils.GestureUtils.startingAt;
import static android.accessibilityservice.cts.utils.GestureUtils.swipe;
import static android.view.MotionEvent.ACTION_DOWN;
import static android.view.MotionEvent.ACTION_MOVE;
import static android.view.MotionEvent.ACTION_UP;

import static org.hamcrest.Matchers.empty;
import static org.hamcrest.Matchers.is;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import static java.util.concurrent.TimeUnit.SECONDS;

import android.accessibilityservice.GestureDescription;
import android.accessibilityservice.GestureDescription.StrokeDescription;
import android.accessibilityservice.cts.AccessibilityGestureDispatchTest.GestureDispatchActivity;
import android.accessibilityservice.cts.utils.EventCapturingTouchListener;
import android.app.Instrumentation;
import android.content.pm.PackageManager;
import android.graphics.PointF;
import android.os.SystemClock;
import android.platform.test.annotations.AppModeFull;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.MotionEvent;
import android.widget.TextView;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;

/**
 * Class for testing magnification.
 */
@RunWith(AndroidJUnit4.class)
@AppModeFull
public class MagnificationGestureHandlerTest {

    private static final double MIN_SCALE = 1.2;

    private InstrumentedAccessibilityService mService;
    private Instrumentation mInstrumentation;
    private EventCapturingTouchListener mTouchListener = new EventCapturingTouchListener();
    float mCurrentScale = 1f;
    PointF mCurrentZoomCenter = null;
    PointF mTapLocation;
    PointF mTapLocation2;
    float mPan;
    private boolean mHasTouchscreen;
    private boolean mOriginalIsMagnificationEnabled;

    private final Object mZoomLock = new Object();

    @Rule
    public ActivityTestRule<GestureDispatchActivity> mActivityRule =
            new ActivityTestRule<>(GestureDispatchActivity.class);

    @Before
    public void setUp() throws Exception {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();

        PackageManager pm = mInstrumentation.getContext().getPackageManager();
        mHasTouchscreen = pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN)
                || pm.hasSystemFeature(PackageManager.FEATURE_FAKETOUCH);
        if (!mHasTouchscreen) return;

        mOriginalIsMagnificationEnabled =
                Settings.Secure.getInt(mInstrumentation.getContext().getContentResolver(),
                        Settings.Secure.ACCESSIBILITY_DISPLAY_MAGNIFICATION_ENABLED, 0) == 1;
        setMagnificationEnabled(true);

        mService = StubMagnificationAccessibilityService.enableSelf(mInstrumentation);
        mService.getMagnificationController().addListener(
                (controller, region, scale, centerX, centerY) -> {
                    mCurrentScale = scale;
                    mCurrentZoomCenter = isZoomed() ? new PointF(centerX, centerY) : null;

                    synchronized (mZoomLock) {
                        mZoomLock.notifyAll();
                    }
                });

        TextView view = mActivityRule.getActivity().findViewById(R.id.full_screen_text_view);
        mInstrumentation.runOnMainSync(() -> {
            view.setOnTouchListener(mTouchListener);
            int[] xy = new int[2];
            view.getLocationOnScreen(xy);
            mTapLocation = new PointF(xy[0] + view.getWidth() / 2, xy[1] + view.getHeight() / 2);
            mTapLocation2 = add(mTapLocation, 31, 29);
            mPan = view.getWidth() / 4;
        });
    }

    @After
    public void tearDown() throws Exception {
        if (!mHasTouchscreen) return;

        setMagnificationEnabled(mOriginalIsMagnificationEnabled);

        if (mService != null) {
            mService.runOnServiceSync(() -> mService.disableSelfAndRemove());
            mService = null;
        }
    }

    @Test
    public void testZoomOnOff() {
        if (!mHasTouchscreen) return;

        assertFalse(isZoomed());

        assertGesturesPropagateToView();
        assertFalse(isZoomed());

        setZoomByTripleTapping(true);

        assertGesturesPropagateToView();
        assertTrue(isZoomed());

        setZoomByTripleTapping(false);
    }

    @Test
    public void testViewportDragging() {
        if (!mHasTouchscreen) return;

        assertFalse(isZoomed());
        tripleTapAndDragViewport();
        waitOn(mZoomLock, () -> !isZoomed());

        setZoomByTripleTapping(true);
        tripleTapAndDragViewport();
        assertTrue(isZoomed());

        setZoomByTripleTapping(false);
    }

    @Test
    public void testPanning() {
        if (!mHasTouchscreen) return;
        assertFalse(isZoomed());

        setZoomByTripleTapping(true);
        PointF oldCenter = mCurrentZoomCenter;

        dispatch(
                swipe(mTapLocation, add(mTapLocation, -mPan, 0)),
                swipe(mTapLocation2, add(mTapLocation2, -mPan, 0)));

        waitOn(mZoomLock,
                () -> (mCurrentZoomCenter.x - oldCenter.x >= mPan / mCurrentScale * 0.9));

        setZoomByTripleTapping(false);
    }

    private void setZoomByTripleTapping(boolean desiredZoomState) {
        if (isZoomed() == desiredZoomState) return;
        dispatch(tripleTap());
        waitOn(mZoomLock, () -> isZoomed() == desiredZoomState);
        assertNoTouchInputPropagated();
    }

    private void tripleTapAndDragViewport() {
        StrokeDescription down = tripleTapAndHold();

        PointF oldCenter = mCurrentZoomCenter;

        StrokeDescription drag = drag(down, add(lastPointOf(down), mPan, 0f));
        dispatch(drag);
        waitOn(mZoomLock, () -> distance(mCurrentZoomCenter, oldCenter) >= mPan / 5);
        assertTrue(isZoomed());
        assertNoTouchInputPropagated();

        dispatch(pointerUp(drag));
        assertNoTouchInputPropagated();
    }

    private StrokeDescription tripleTapAndHold() {
        StrokeDescription tap1 = click(mTapLocation);
        StrokeDescription tap2 = startingAt(endTimeOf(tap1) + 20, click(mTapLocation2));
        StrokeDescription down = startingAt(endTimeOf(tap2) + 20, pointerDown(mTapLocation));
        dispatch(tap1, tap2, down);
        waitOn(mZoomLock, () -> isZoomed());
        return down;
    }

    private void assertGesturesPropagateToView() {
        dispatch(click(mTapLocation));
        assertPropagated(ACTION_DOWN, ACTION_UP);

        dispatch(longClick(mTapLocation));
        assertPropagated(ACTION_DOWN, ACTION_UP);

        dispatch(doubleTap());
        assertPropagated(ACTION_DOWN, ACTION_UP, ACTION_DOWN, ACTION_UP);

        dispatch(swipe(
                mTapLocation,
                add(mTapLocation, 31, 29)));
        assertPropagated(ACTION_DOWN, ACTION_MOVE, ACTION_UP);
    }

    private void assertNoTouchInputPropagated() {
        assertThat(prettyPrintable(mTouchListener.events), is(empty()));
    }

    private void setMagnificationEnabled(boolean enabled) {
        Settings.Secure.putInt(mInstrumentation.getContext().getContentResolver(),
                Settings.Secure.ACCESSIBILITY_DISPLAY_MAGNIFICATION_ENABLED, enabled ? 1 : 0);
    }

    private boolean isZoomed() {
        return mCurrentScale >= MIN_SCALE;
    }

    private void assertPropagated(int... eventTypes) {
        MotionEvent ev;
        try {
            while (true) {
                if (eventTypes.length == 0) return;
                int expectedEventType = eventTypes[0];
                long startedPollingAt = SystemClock.uptimeMillis();
                ev = mTouchListener.events.poll(5, SECONDS);
                assertNotNull("Expected "
                        + MotionEvent.actionToString(expectedEventType)
                        + " but none present after "
                        + (SystemClock.uptimeMillis() - startedPollingAt) + "ms",
                        ev);
                int action = ev.getActionMasked();
                if (action == expectedEventType) {
                    eventTypes = Arrays.copyOfRange(eventTypes, 1, eventTypes.length);
                } else {
                    if (action != ACTION_MOVE) fail("Unexpected event: " + ev);
                }
            }
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    private GestureDescription doubleTap() {
        return multiTap(2);
    }

    private GestureDescription tripleTap() {
        return multiTap(3);
    }

    private GestureDescription multiTap(int taps) {
        GestureDescription.Builder builder = new GestureDescription.Builder();
        long time = 0;
        for (int i = 0; i < taps; i++) {
            StrokeDescription stroke = click(mTapLocation);
            builder.addStroke(startingAt(time, stroke));
            time += stroke.getDuration() + 20;
        }
        return builder.build();
    }

    public void dispatch(StrokeDescription firstStroke, StrokeDescription... rest) {
        GestureDescription.Builder builder =
                new GestureDescription.Builder().addStroke(firstStroke);
        for (StrokeDescription stroke : rest) {
            builder.addStroke(stroke);
        }
        dispatch(builder.build());
    }

    public void dispatch(GestureDescription gesture) {
        await(dispatchGesture(mService, gesture));
    }

    private static <T> Collection<T> prettyPrintable(Collection<T> c) {
        return new ArrayList<T>(c) {

            @Override
            public String toString() {
                return stream()
                        .map(t -> "\n" + t)
                        .reduce(String::concat)
                        .orElse("");
            }
        };
    }
}
