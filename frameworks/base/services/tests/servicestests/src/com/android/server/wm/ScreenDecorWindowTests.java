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
 * limitations under the License
 */

package com.android.server.wm;

import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;
import static android.graphics.Color.RED;
import static android.hardware.display.DisplayManager.VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY;
import static android.hardware.display.DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION;
import static android.view.Display.DEFAULT_DISPLAY;
import static android.view.Gravity.BOTTOM;
import static android.view.Gravity.LEFT;
import static android.view.Gravity.RIGHT;
import static android.view.Gravity.TOP;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
import static android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
import static android.view.WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
import static android.view.WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH;
import static android.view.WindowManager.LayoutParams.PRIVATE_FLAG_IS_SCREEN_DECOR;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
import static org.junit.Assert.assertEquals;

import android.app.Activity;
import android.app.ActivityOptions;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.ImageReader;
import android.os.Handler;
import android.platform.test.annotations.Presubmit;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Pair;
import android.view.Display;
import android.view.DisplayInfo;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.widget.TextView;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.function.BooleanSupplier;

/**
 * Tests for the {@link android.view.WindowManager.LayoutParams#PRIVATE_FLAG_IS_SCREEN_DECOR} flag.
 *
 * Build/Install/Run:
 *  atest FrameworksServicesTests:com.android.server.wm.ScreenDecorWindowTests
 */
// TODO: Add test for FLAG_FULLSCREEN which hides the status bar and also other flags.
// TODO: Test non-Activity windows.
@SmallTest
@Presubmit
@RunWith(AndroidJUnit4.class)
public class ScreenDecorWindowTests {

    private final Context mContext = InstrumentationRegistry.getTargetContext();
    private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();

    private WindowManager mWm;
    private ArrayList<View> mWindows = new ArrayList<>();

    private Activity mTestActivity;
    private VirtualDisplay mDisplay;
    private ImageReader mImageReader;

    private int mDecorThickness;
    private int mHalfDecorThickness;

    @Before
    public void setUp() {
        final Pair<VirtualDisplay, ImageReader> result = createDisplay();
        mDisplay = result.first;
        mImageReader = result.second;
        final Display display = mDisplay.getDisplay();
        final Context dContext = mContext.createDisplayContext(display);
        mWm = dContext.getSystemService(WindowManager.class);
        mTestActivity = startActivityOnDisplay(TestActivity.class, display.getDisplayId());
        final Point size = new Point();
        mDisplay.getDisplay().getRealSize(size);
        mDecorThickness = Math.min(size.x, size.y) / 3;
        mHalfDecorThickness = mDecorThickness / 2;
    }

    @After
    public void tearDown() {
        while (!mWindows.isEmpty()) {
            removeWindow(mWindows.get(0));
        }
        finishActivity(mTestActivity);
        mDisplay.release();
        mImageReader.close();
    }

    @Test
    public void testScreenSides() throws Exception {
        // Decor on top
        final View decorWindow = createDecorWindow(TOP, MATCH_PARENT, mDecorThickness);
        assertInsetGreaterOrEqual(mTestActivity, TOP, mDecorThickness);

        // Decor at the bottom
        updateWindow(decorWindow, BOTTOM, MATCH_PARENT, mDecorThickness, 0, 0);
        assertInsetGreaterOrEqual(mTestActivity, BOTTOM, mDecorThickness);

        // Decor to the left
        updateWindow(decorWindow, LEFT, mDecorThickness, MATCH_PARENT, 0, 0);
        assertInsetGreaterOrEqual(mTestActivity, LEFT, mDecorThickness);

        // Decor to the right
        updateWindow(decorWindow, RIGHT, mDecorThickness, MATCH_PARENT, 0, 0);
        assertInsetGreaterOrEqual(mTestActivity, RIGHT, mDecorThickness);
    }

    @Test
    public void testMultipleDecors() throws Exception {
        // Test 2 decor windows on-top.
        createDecorWindow(TOP, MATCH_PARENT, mHalfDecorThickness);
        assertInsetGreaterOrEqual(mTestActivity, TOP, mHalfDecorThickness);
        createDecorWindow(TOP, MATCH_PARENT, mDecorThickness);
        assertInsetGreaterOrEqual(mTestActivity, TOP, mDecorThickness);

        // And one at the bottom.
        createDecorWindow(BOTTOM, MATCH_PARENT, mHalfDecorThickness);
        assertInsetGreaterOrEqual(mTestActivity, TOP, mDecorThickness);
        assertInsetGreaterOrEqual(mTestActivity, BOTTOM, mHalfDecorThickness);
    }

    @Test
    public void testFlagChange() throws Exception {
        WindowInsets initialInsets = getInsets(mTestActivity);

        final View decorWindow = createDecorWindow(TOP, MATCH_PARENT, mDecorThickness);
        assertTopInsetEquals(mTestActivity, mDecorThickness);

        updateWindow(decorWindow, TOP, MATCH_PARENT, mDecorThickness,
                0, PRIVATE_FLAG_IS_SCREEN_DECOR);

        // TODO: fix test and re-enable assertion.
        // initialInsets was not actually immutable and just updated to the current insets,
        // meaning this assertion never actually tested anything. Now that WindowInsets actually is
        // immutable, it turns out the test was broken.
        // assertTopInsetEquals(mTestActivity, initialInsets.getSystemWindowInsetTop());

        updateWindow(decorWindow, TOP, MATCH_PARENT, mDecorThickness,
                PRIVATE_FLAG_IS_SCREEN_DECOR, PRIVATE_FLAG_IS_SCREEN_DECOR);
        assertTopInsetEquals(mTestActivity, mDecorThickness);
    }

    @Test
    public void testRemoval() throws Exception {
        WindowInsets initialInsets = getInsets(mTestActivity);

        final View decorWindow = createDecorWindow(TOP, MATCH_PARENT, mDecorThickness);
        assertInsetGreaterOrEqual(mTestActivity, TOP, mDecorThickness);

        removeWindow(decorWindow);
        assertTopInsetEquals(mTestActivity, initialInsets.getSystemWindowInsetTop());
    }

    private View createDecorWindow(int gravity, int width, int height) {
        return createWindow("decorWindow", gravity, width, height, RED,
                FLAG_LAYOUT_IN_SCREEN, PRIVATE_FLAG_IS_SCREEN_DECOR);
    }

    private View createWindow(String name, int gravity, int width, int height, int color, int flags,
            int privateFlags) {

        final View[] viewHolder = new View[1];
        final int finalFlag = flags
                | FLAG_NOT_FOCUSABLE | FLAG_WATCH_OUTSIDE_TOUCH | FLAG_NOT_TOUCHABLE;

        // Needs to run on the UI thread.
        Handler.getMain().runWithScissors(() -> {
            final WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                    width, height, TYPE_APPLICATION_OVERLAY, finalFlag, PixelFormat.OPAQUE);
            lp.gravity = gravity;
            lp.privateFlags |= privateFlags;

            final TextView view = new TextView(mContext);
            view.setText("ScreenDecorWindowTests - " + name);
            view.setBackgroundColor(color);
            mWm.addView(view, lp);
            mWindows.add(view);
            viewHolder[0] = view;
        }, 0);

        waitForIdle();
        return viewHolder[0];
    }

    private void updateWindow(View v, int gravity, int width, int height,
            int privateFlags, int privateFlagsMask) {
        // Needs to run on the UI thread.
        Handler.getMain().runWithScissors(() -> {
            final WindowManager.LayoutParams lp = (WindowManager.LayoutParams) v.getLayoutParams();
            lp.gravity = gravity;
            lp.width = width;
            lp.height = height;
            setPrivateFlags(lp, privateFlags, privateFlagsMask);

            mWm.updateViewLayout(v, lp);
        }, 0);

        waitForIdle();
    }

    private void removeWindow(View v) {
        Handler.getMain().runWithScissors(() -> mWm.removeView(v), 0);
        mWindows.remove(v);
        waitForIdle();
    }

    private WindowInsets getInsets(Activity a) {
        return new WindowInsets(a.getWindow().getDecorView().getRootWindowInsets());
    }

    /**
     * Set the flags of the window, as per the
     * {@link WindowManager.LayoutParams WindowManager.LayoutParams}
     * flags.
     *
     * @param flags The new window flags (see WindowManager.LayoutParams).
     * @param mask Which of the window flag bits to modify.
     */
    public void setPrivateFlags(WindowManager.LayoutParams lp, int flags, int mask) {
        lp.flags = (lp.flags & ~mask) | (flags & mask);
    }

    /**
     * Asserts the top inset of {@param activity} is equal to {@param expected} waiting as needed.
     */
    private void assertTopInsetEquals(Activity activity, int expected) throws Exception {
        waitFor(() -> getInsets(activity).getSystemWindowInsetTop() == expected);
        assertEquals(expected, getInsets(activity).getSystemWindowInsetTop());
    }

    /**
     * Asserts the inset at {@param side} of {@param activity} is equal to {@param expected}
     * waiting as needed.
     */
    private void assertInsetGreaterOrEqual(Activity activity, int side, int expected)
            throws Exception {
        waitFor(() -> {
            final WindowInsets insets = getInsets(activity);
            switch (side) {
                case TOP: return insets.getSystemWindowInsetTop() >= expected;
                case BOTTOM: return insets.getSystemWindowInsetBottom() >= expected;
                case LEFT: return insets.getSystemWindowInsetLeft() >= expected;
                case RIGHT: return insets.getSystemWindowInsetRight() >= expected;
                default: return true;
            }
        });

        final WindowInsets insets = getInsets(activity);
        switch (side) {
            case TOP: assertGreaterOrEqual(insets.getSystemWindowInsetTop(), expected); break;
            case BOTTOM: assertGreaterOrEqual(insets.getSystemWindowInsetBottom(), expected); break;
            case LEFT: assertGreaterOrEqual(insets.getSystemWindowInsetLeft(), expected); break;
            case RIGHT: assertGreaterOrEqual(insets.getSystemWindowInsetRight(), expected); break;
        }
    }

    /** Asserts that the first entry is greater than or equal to the second entry. */
    private void assertGreaterOrEqual(int first, int second) throws Exception {
        Assert.assertTrue("Excepted " + first + " >= " + second, first >= second);
    }

    private void waitFor(BooleanSupplier waitCondition) {
        int retriesLeft = 5;
        do {
            if (waitCondition.getAsBoolean()) {
                break;
            }
            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                // Well I guess we are not waiting...
            }
        } while (retriesLeft-- > 0);
    }

    private void finishActivity(Activity a) {
        if (a == null) {
            return;
        }
        a.finish();
        waitForIdle();
    }

    private void waitForIdle() {
        mInstrumentation.waitForIdleSync();
    }

    private Activity startActivityOnDisplay(Class<?> cls, int displayId) {
        final Intent intent = new Intent(mContext, cls);
        intent.addFlags(FLAG_ACTIVITY_NEW_TASK);
        final ActivityOptions options = ActivityOptions.makeBasic();
        options.setLaunchDisplayId(displayId);
        final Activity activity = mInstrumentation.startActivitySync(intent, options.toBundle());
        waitForIdle();

        assertEquals(displayId, activity.getDisplay().getDisplayId());
        return activity;
    }

    private Pair<VirtualDisplay, ImageReader> createDisplay() {
        final DisplayManager dm = mContext.getSystemService(DisplayManager.class);
        final DisplayInfo displayInfo = new DisplayInfo();
        final Display defaultDisplay = dm.getDisplay(DEFAULT_DISPLAY);
        defaultDisplay.getDisplayInfo(displayInfo);
        final String name = "ScreenDecorWindowTests";
        int flags = VIRTUAL_DISPLAY_FLAG_PRESENTATION | VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY;

        final ImageReader imageReader = ImageReader.newInstance(
                displayInfo.logicalWidth, displayInfo.logicalHeight, PixelFormat.RGBA_8888, 2);

        final VirtualDisplay display = dm.createVirtualDisplay(name, displayInfo.logicalWidth,
                displayInfo.logicalHeight, displayInfo.logicalDensityDpi, imageReader.getSurface(),
                flags);

        return Pair.create(display, imageReader);
    }

    public static class TestActivity extends Activity {
    }
}
