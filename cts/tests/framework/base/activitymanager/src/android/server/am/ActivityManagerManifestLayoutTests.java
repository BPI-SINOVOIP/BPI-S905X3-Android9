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

package android.server.am;

import static android.app.WindowConfiguration.WINDOWING_MODE_FREEFORM;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.server.am.ActivityAndWindowManagersState.dpToPx;
import static android.server.am.ComponentNameUtils.getWindowName;
import static android.server.am.Components.BOTTOM_LEFT_LAYOUT_ACTIVITY;
import static android.server.am.Components.BOTTOM_RIGHT_LAYOUT_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.Components.TOP_LEFT_LAYOUT_ACTIVITY;
import static android.server.am.Components.TOP_RIGHT_LAYOUT_ACTIVITY;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNotNull;

import static org.junit.Assume.assumeTrue;

import android.content.ComponentName;
import android.graphics.Rect;
import android.server.am.WindowManagerState.Display;
import android.server.am.WindowManagerState.WindowState;

import org.junit.Test;

import java.util.List;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityManagerManifestLayoutTests
 */
public class ActivityManagerManifestLayoutTests extends ActivityManagerTestBase {

    // Test parameters
    private static final int DEFAULT_WIDTH_DP = 240;
    private static final int DEFAULT_HEIGHT_DP = 160;
    private static final float DEFAULT_WIDTH_FRACTION = 0.25f;
    private static final float DEFAULT_HEIGHT_FRACTION = 0.35f;
    private static final int MIN_WIDTH_DP = 100;
    private static final int MIN_HEIGHT_DP = 80;

    private static final int GRAVITY_VER_CENTER = 0x01;
    private static final int GRAVITY_VER_TOP    = 0x02;
    private static final int GRAVITY_VER_BOTTOM = 0x04;
    private static final int GRAVITY_HOR_CENTER = 0x10;
    private static final int GRAVITY_HOR_LEFT   = 0x20;
    private static final int GRAVITY_HOR_RIGHT  = 0x40;

    private Display mDisplay;
    private WindowState mWindowState;

    @Test
    public void testGravityAndDefaultSizeTopLeft() throws Exception {
        testLayout(GRAVITY_VER_TOP, GRAVITY_HOR_LEFT, false /*fraction*/);
    }

    @Test
    public void testGravityAndDefaultSizeTopRight() throws Exception {
        testLayout(GRAVITY_VER_TOP, GRAVITY_HOR_RIGHT, true /*fraction*/);
    }

    @Test
    public void testGravityAndDefaultSizeBottomLeft() throws Exception {
        testLayout(GRAVITY_VER_BOTTOM, GRAVITY_HOR_LEFT, true /*fraction*/);
    }

    @Test
    public void testGravityAndDefaultSizeBottomRight() throws Exception {
        testLayout(GRAVITY_VER_BOTTOM, GRAVITY_HOR_RIGHT, false /*fraction*/);
    }

    @Test
    public void testMinimalSizeFreeform() throws Exception {
        assumeTrue("Skipping test: no freeform support", supportsFreeform());

        testMinimalSize(WINDOWING_MODE_FREEFORM);
    }

    @Test
    public void testMinimalSizeDocked() throws Exception {
        assumeTrue("Skipping test: no multi-window support", supportsSplitScreenMultiWindow());

        testMinimalSize(WINDOWING_MODE_SPLIT_SCREEN_PRIMARY);
    }

    private void testMinimalSize(int windowingMode) throws Exception {
        // Issue command to resize to <0,0,1,1>. We expect the size to be floored at
        // MIN_WIDTH_DPxMIN_HEIGHT_DP.
        if (windowingMode == WINDOWING_MODE_FREEFORM) {
            launchActivity(BOTTOM_RIGHT_LAYOUT_ACTIVITY, WINDOWING_MODE_FREEFORM);
            resizeActivityTask(BOTTOM_RIGHT_LAYOUT_ACTIVITY, 0, 0, 1, 1);
        } else { // stackId == DOCKED_STACK_ID
            launchActivitiesInSplitScreen(
                    getLaunchActivityBuilder().setTargetActivity(BOTTOM_RIGHT_LAYOUT_ACTIVITY),
                    getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
            resizeDockedStack(1, 1, 1, 1);
        }
        getDisplayAndWindowState(BOTTOM_RIGHT_LAYOUT_ACTIVITY, false);

        final int minWidth = dpToPx(MIN_WIDTH_DP, mDisplay.getDpi());
        final int minHeight = dpToPx(MIN_HEIGHT_DP, mDisplay.getDpi());
        final Rect containingRect = mWindowState.getContainingFrame();

        assertEquals("Min width is incorrect", minWidth, containingRect.width());
        assertEquals("Min height is incorrect", minHeight, containingRect.height());
    }

    private void testLayout(
            int vGravity, int hGravity, boolean fraction) throws Exception {
        assumeTrue("Skipping test: no freeform support", supportsFreeform());

        final ComponentName activityName;
        if (vGravity == GRAVITY_VER_TOP) {
            activityName = (hGravity == GRAVITY_HOR_LEFT) ? TOP_LEFT_LAYOUT_ACTIVITY
                    : TOP_RIGHT_LAYOUT_ACTIVITY;
        } else {
            activityName = (hGravity == GRAVITY_HOR_LEFT) ? BOTTOM_LEFT_LAYOUT_ACTIVITY
                    : BOTTOM_RIGHT_LAYOUT_ACTIVITY;
        }

        // Launch in freeform stack
        launchActivity(activityName, WINDOWING_MODE_FREEFORM);

        getDisplayAndWindowState(activityName, true);

        final Rect containingRect = mWindowState.getContainingFrame();
        final Rect appRect = mDisplay.getAppRect();
        final int expectedWidthPx, expectedHeightPx;
        // Evaluate the expected window size in px. If we're using fraction dimensions,
        // calculate the size based on the app rect size. Otherwise, convert the expected
        // size in dp to px.
        if (fraction) {
            expectedWidthPx = (int) (appRect.width() * DEFAULT_WIDTH_FRACTION);
            expectedHeightPx = (int) (appRect.height() * DEFAULT_HEIGHT_FRACTION);
        } else {
            final int densityDpi = mDisplay.getDpi();
            expectedWidthPx = dpToPx(DEFAULT_WIDTH_DP, densityDpi);
            expectedHeightPx = dpToPx(DEFAULT_HEIGHT_DP, densityDpi);
        }

        verifyFrameSizeAndPosition(
                vGravity, hGravity, expectedWidthPx, expectedHeightPx, containingRect, appRect);
    }

    private void getDisplayAndWindowState(ComponentName activityName, boolean checkFocus)
            throws Exception {
        final String windowName = getWindowName(activityName);

        mAmWmState.computeState(activityName);

        if (checkFocus) {
            mAmWmState.assertFocusedWindow("Test window must be the front window.", windowName);
        } else {
            mAmWmState.assertVisibility(activityName, true);
        }

        final List<WindowState> windowList =
                mAmWmState.getWmState().getMatchingVisibleWindowState(windowName);

        assertEquals("Should have exactly one window state for the activity.",
                1, windowList.size());

        mWindowState = windowList.get(0);
        assertNotNull("Should have a valid window", mWindowState);

        mDisplay = mAmWmState.getWmState().getDisplay(mWindowState.getDisplayId());
        assertNotNull("Should be on a display", mDisplay);
    }

    private void verifyFrameSizeAndPosition(
            int vGravity, int hGravity, int expectedWidthPx, int expectedHeightPx,
            Rect containingFrame, Rect parentFrame) {
        assertEquals("Width is incorrect", expectedWidthPx, containingFrame.width());
        assertEquals("Height is incorrect", expectedHeightPx, containingFrame.height());

        if (vGravity == GRAVITY_VER_TOP) {
            assertEquals("Should be on the top", parentFrame.top, containingFrame.top);
        } else if (vGravity == GRAVITY_VER_BOTTOM) {
            assertEquals("Should be on the bottom", parentFrame.bottom, containingFrame.bottom);
        }

        if (hGravity == GRAVITY_HOR_LEFT) {
            assertEquals("Should be on the left", parentFrame.left, containingFrame.left);
        } else if (hGravity == GRAVITY_HOR_RIGHT){
            assertEquals("Should be on the right", parentFrame.right, containingFrame.right);
        }
    }
}
