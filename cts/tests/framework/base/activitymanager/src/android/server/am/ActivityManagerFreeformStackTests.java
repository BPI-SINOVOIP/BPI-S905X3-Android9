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
 * limitations under the License
 */

package android.server.am;

import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.WINDOWING_MODE_FREEFORM;
import static android.server.am.Components.FREEFORM_ACTIVITY;
import static android.server.am.Components.NON_RESIZEABLE_ACTIVITY;
import static android.server.am.Components.NO_RELAUNCH_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;

import static org.junit.Assert.assertEquals;

import android.graphics.Rect;
import android.server.am.ActivityManagerState.ActivityStack;
import android.server.am.ActivityManagerState.ActivityTask;

import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityManagerFreeformStackTests
 */
public class ActivityManagerFreeformStackTests extends ActivityManagerTestBase {

    private static final int TEST_TASK_OFFSET = 20;
    private static final int TEST_TASK_OFFSET_2 = 100;
    private static final int TEST_TASK_SIZE_1 = 900;
    private static final int TEST_TASK_SIZE_2 = TEST_TASK_SIZE_1 * 2;
    private static final int TEST_TASK_SIZE_DP_1 = 220;
    private static final int TEST_TASK_SIZE_DP_2 = TEST_TASK_SIZE_DP_1 * 2;

    // NOTE: Launching the FreeformActivity will automatically launch the TestActivity
    // with bounds (0, 0, 900, 900)

    @Test
    public void testFreeformWindowManagementSupport() throws Exception {

        launchActivity(FREEFORM_ACTIVITY, WINDOWING_MODE_FREEFORM);

        mAmWmState.computeState(FREEFORM_ACTIVITY, TEST_ACTIVITY);

        if (!supportsFreeform()) {
            mAmWmState.assertDoesNotContainStack("Must not contain freeform stack.",
                    WINDOWING_MODE_FREEFORM, ACTIVITY_TYPE_STANDARD);
            return;
        }

        mAmWmState.assertFrontStack("Freeform stack must be the front stack.",
                WINDOWING_MODE_FREEFORM, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertVisibility(FREEFORM_ACTIVITY, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY, true);
        mAmWmState.assertFocusedActivity(
                TEST_ACTIVITY + " must be focused Activity", TEST_ACTIVITY);
        assertEquals(new Rect(0, 0, TEST_TASK_SIZE_1, TEST_TASK_SIZE_1),
                mAmWmState.getAmState().getTaskByActivity(TEST_ACTIVITY).getBounds());
    }

    @Test
    public void testNonResizeableActivityHasFullDisplayBounds() throws Exception {
        launchActivity(NON_RESIZEABLE_ACTIVITY, WINDOWING_MODE_FREEFORM);

        mAmWmState.computeState(NON_RESIZEABLE_ACTIVITY);

        final ActivityTask task =
                mAmWmState.getAmState().getTaskByActivity(NON_RESIZEABLE_ACTIVITY);
        final ActivityStack stack = mAmWmState.getAmState().getStackById(task.mStackId);

        if (task.isFullscreen()) {
            // If the task is on the fullscreen stack, then we know that it will have bounds that
            // fill the entire display.
            return;
        }

        // If the task is not on the fullscreen stack, then compare the task bounds to the display
        // bounds.
        assertEquals(mAmWmState.getWmState().getDisplay(stack.mDisplayId).getDisplayRect(),
                task.getBounds());
    }

    @Test
    public void testActivityLifeCycleOnResizeFreeformTask() throws Exception {
        launchActivity(TEST_ACTIVITY, WINDOWING_MODE_FREEFORM);
        launchActivity(NO_RELAUNCH_ACTIVITY, WINDOWING_MODE_FREEFORM);

        mAmWmState.computeState(TEST_ACTIVITY, NO_RELAUNCH_ACTIVITY);

        if (!supportsFreeform()) {
            mAmWmState.assertDoesNotContainStack("Must not contain freeform stack.",
                    WINDOWING_MODE_FREEFORM, ACTIVITY_TYPE_STANDARD);
            return;
        }

        final int displayId = mAmWmState.getAmState().getStandardStackByWindowingMode(
                WINDOWING_MODE_FREEFORM).mDisplayId;
        final int densityDpi =
                mAmWmState.getWmState().getDisplay(displayId).getDpi();
        final int testTaskSize1 =
                ActivityAndWindowManagersState.dpToPx(TEST_TASK_SIZE_DP_1, densityDpi);
        final int testTaskSize2 =
                ActivityAndWindowManagersState.dpToPx(TEST_TASK_SIZE_DP_2, densityDpi);

        resizeActivityTask(TEST_ACTIVITY,
                TEST_TASK_OFFSET, TEST_TASK_OFFSET, testTaskSize1, testTaskSize2);
        resizeActivityTask(NO_RELAUNCH_ACTIVITY,
                TEST_TASK_OFFSET_2, TEST_TASK_OFFSET_2, testTaskSize1, testTaskSize2);

        mAmWmState.computeState(new WaitForValidActivityState.Builder(TEST_ACTIVITY).build(),
                new WaitForValidActivityState.Builder(NO_RELAUNCH_ACTIVITY).build());

        final LogSeparator logSeparator = separateLogs();
        resizeActivityTask(TEST_ACTIVITY,
                TEST_TASK_OFFSET, TEST_TASK_OFFSET, testTaskSize2, testTaskSize1);
        resizeActivityTask(NO_RELAUNCH_ACTIVITY,
                TEST_TASK_OFFSET_2, TEST_TASK_OFFSET_2, testTaskSize2, testTaskSize1);
        mAmWmState.computeState(TEST_ACTIVITY, NO_RELAUNCH_ACTIVITY);

        assertActivityLifecycle(TEST_ACTIVITY, true /* relaunched */, logSeparator);
        assertActivityLifecycle(NO_RELAUNCH_ACTIVITY, false /* relaunched */, logSeparator);
    }
}
