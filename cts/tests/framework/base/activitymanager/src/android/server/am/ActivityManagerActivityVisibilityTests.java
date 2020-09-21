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

import static android.app.ActivityManager.StackId.INVALID_STACK_ID;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN_OR_SPLIT_SCREEN_SECONDARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_SECONDARY;
import static android.server.am.ActivityManagerState.STATE_PAUSED;
import static android.server.am.ActivityManagerState.STATE_RESUMED;
import static android.server.am.ActivityManagerState.STATE_STOPPED;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.Components.ALT_LAUNCHING_ACTIVITY;
import static android.server.am.Components.ALWAYS_FOCUSABLE_PIP_ACTIVITY;
import static android.server.am.Components.BROADCAST_RECEIVER_ACTIVITY;
import static android.server.am.Components.BroadcastReceiverActivity.ACTION_TRIGGER_BROADCAST;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_FINISH_BROADCAST;
import static android.server.am.Components.DOCKED_ACTIVITY;
import static android.server.am.Components.LAUNCHING_ACTIVITY;
import static android.server.am.Components.LAUNCH_PIP_ON_PIP_ACTIVITY;
import static android.server.am.Components.MOVE_TASK_TO_BACK_ACTIVITY;
import static android.server.am.Components.MoveTaskToBackActivity.EXTRA_FINISH_POINT;
import static android.server.am.Components.MoveTaskToBackActivity.FINISH_POINT_ON_PAUSE;
import static android.server.am.Components.MoveTaskToBackActivity.FINISH_POINT_ON_STOP;
import static android.server.am.Components.NO_HISTORY_ACTIVITY;
import static android.server.am.Components.SWIPE_REFRESH_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.Components.TRANSLUCENT_ACTIVITY;
import static android.server.am.Components.TURN_SCREEN_ON_ACTIVITY;
import static android.server.am.Components.TURN_SCREEN_ON_ATTR_ACTIVITY;
import static android.server.am.Components.TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY;
import static android.server.am.Components.TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY;
import static android.server.am.Components.TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY;
import static android.server.am.Components.TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY;
import static android.server.am.UiDeviceUtils.pressBackButton;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.platform.test.annotations.Presubmit;

import org.junit.Rule;
import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityManagerActivityVisibilityTests
 */
public class ActivityManagerActivityVisibilityTests extends ActivityManagerTestBase {

    // TODO(b/70247058): Use {@link Context#sendBroadcast(Intent).
    // Shell command to finish {@link #BROADCAST_RECEIVER_ACTIVITY}.
    private static final String FINISH_ACTIVITY_BROADCAST = "am broadcast -a "
            + ACTION_TRIGGER_BROADCAST + " --ez " + EXTRA_FINISH_BROADCAST + " true";

    @Rule
    public final DisableScreenDozeRule mDisableScreenDozeRule = new DisableScreenDozeRule();

    @Presubmit
    @Test
    public void testTranslucentActivityOnTopOfPinnedStack() throws Exception {
        if (!supportsPip()) {
            return;
        }

        executeShellCommand(getAmStartCmdOverHome(LAUNCH_PIP_ON_PIP_ACTIVITY));
        mAmWmState.waitForValidState(LAUNCH_PIP_ON_PIP_ACTIVITY);
        // NOTE: moving to pinned stack will trigger the pip-on-pip activity to launch the
        // translucent activity.
        final int stackId = mAmWmState.getAmState().getStackIdByActivity(
                LAUNCH_PIP_ON_PIP_ACTIVITY);

        assertNotEquals(stackId, INVALID_STACK_ID);
        executeShellCommand(getMoveToPinnedStackCommand(stackId));
        mAmWmState.waitForValidState(
                new WaitForValidActivityState.Builder(ALWAYS_FOCUSABLE_PIP_ACTIVITY)
                        .setWindowingMode(WINDOWING_MODE_PINNED)
                        .setActivityType(ACTIVITY_TYPE_STANDARD)
                        .build());

        mAmWmState.assertFrontStack("Pinned stack must be the front stack.",
                WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertVisibility(LAUNCH_PIP_ON_PIP_ACTIVITY, true);
        mAmWmState.assertVisibility(ALWAYS_FOCUSABLE_PIP_ACTIVITY, true);
    }

    /**
     * Asserts that the home activity is visible when a translucent activity is launched in the
     * fullscreen stack over the home activity.
     */
    @Test
    public void testTranslucentActivityOnTopOfHome() throws Exception {
        if (!hasHomeScreen()) {
            return;
        }

        launchHomeActivity();
        launchActivity(ALWAYS_FOCUSABLE_PIP_ACTIVITY);

        mAmWmState.assertFrontStack("Fullscreen stack must be the front stack.",
                WINDOWING_MODE_FULLSCREEN, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertVisibility(ALWAYS_FOCUSABLE_PIP_ACTIVITY, true);
        mAmWmState.assertHomeActivityVisible(true);
    }

    /**
     * Assert that the home activity is visible if a task that was launched from home is pinned
     * and also assert the next task in the fullscreen stack isn't visible.
     */
    @Presubmit
    @Test
    public void testHomeVisibleOnActivityTaskPinned() throws Exception {
        if (!supportsPip()) {
            return;
        }

        launchHomeActivity();
        launchActivity(TEST_ACTIVITY);
        launchHomeActivity();
        launchActivity(ALWAYS_FOCUSABLE_PIP_ACTIVITY);
        final int stackId = mAmWmState.getAmState().getStackIdByActivity(
                ALWAYS_FOCUSABLE_PIP_ACTIVITY);

        assertNotEquals(stackId, INVALID_STACK_ID);
        executeShellCommand(getMoveToPinnedStackCommand(stackId));
        mAmWmState.waitForValidState(
                new WaitForValidActivityState.Builder(ALWAYS_FOCUSABLE_PIP_ACTIVITY)
                        .setWindowingMode(WINDOWING_MODE_PINNED)
                        .setActivityType(ACTIVITY_TYPE_STANDARD)
                        .build());

        mAmWmState.assertVisibility(ALWAYS_FOCUSABLE_PIP_ACTIVITY, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY, false);
        mAmWmState.assertHomeActivityVisible(true);
    }

    @Presubmit
    @Test
    public void testTranslucentActivityOverDockedStack() throws Exception {
        if (!supportsSplitScreenMultiWindow()) {
            // Skipping test: no multi-window support
            return;
        }

        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        launchActivity(TRANSLUCENT_ACTIVITY, WINDOWING_MODE_SPLIT_SCREEN_PRIMARY);
        mAmWmState.computeState(false /* compareTaskAndStackBounds */,
                new WaitForValidActivityState(TEST_ACTIVITY),
                new WaitForValidActivityState(DOCKED_ACTIVITY),
                new WaitForValidActivityState(TRANSLUCENT_ACTIVITY));
        mAmWmState.assertContainsStack("Must contain fullscreen stack.",
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertContainsStack("Must contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertVisibility(DOCKED_ACTIVITY, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY, true);
        mAmWmState.assertVisibility(TRANSLUCENT_ACTIVITY, true);
    }

    @Presubmit
    @Test
    public void testTurnScreenOnActivity() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.sleepDevice();
            launchActivity(TURN_SCREEN_ON_ACTIVITY);

            mAmWmState.assertVisibility(TURN_SCREEN_ON_ACTIVITY, true);
            assertTrue("Display turns on", isDisplayOn());
        }
    }

    @Presubmit
    @Test
    public void testFinishActivityInNonFocusedStack() throws Exception {
        if (!supportsSplitScreenMultiWindow()) {
            // Skipping test: no multi-window support
            return;
        }

        // Launch two activities in docked stack.
        launchActivityInSplitScreenWithRecents(LAUNCHING_ACTIVITY);
        getLaunchActivityBuilder()
                .setTargetActivity(BROADCAST_RECEIVER_ACTIVITY)
                .setWaitForLaunched(true)
                .execute();
        mAmWmState.assertVisibility(BROADCAST_RECEIVER_ACTIVITY, true);
        // Launch something to fullscreen stack to make it focused.
        launchActivity(TEST_ACTIVITY, WINDOWING_MODE_FULLSCREEN_OR_SPLIT_SCREEN_SECONDARY);
        mAmWmState.assertVisibility(TEST_ACTIVITY, true);
        // Finish activity in non-focused (docked) stack.
        executeShellCommand(FINISH_ACTIVITY_BROADCAST);

        mAmWmState.waitForActivityState(LAUNCHING_ACTIVITY, STATE_PAUSED);
        mAmWmState.waitForAllExitingWindows();

        mAmWmState.computeState(LAUNCHING_ACTIVITY);
        mAmWmState.assertVisibility(LAUNCHING_ACTIVITY, true);
        mAmWmState.assertVisibility(BROADCAST_RECEIVER_ACTIVITY, false);
    }

    @Test
    public void testFinishActivityWithMoveTaskToBackAfterPause() throws Exception {
        performFinishActivityWithMoveTaskToBack(FINISH_POINT_ON_PAUSE);
    }

    @Test
    public void testFinishActivityWithMoveTaskToBackAfterStop() throws Exception {
        performFinishActivityWithMoveTaskToBack(FINISH_POINT_ON_STOP);
    }

    private void performFinishActivityWithMoveTaskToBack(String finishPoint) throws Exception {
        // Make sure home activity is visible.
        launchHomeActivity();
        if (hasHomeScreen()) {
            mAmWmState.assertHomeActivityVisible(true /* visible */);
        }

        // Launch an activity that calls "moveTaskToBack" to finish itself.
        launchActivity(MOVE_TASK_TO_BACK_ACTIVITY, EXTRA_FINISH_POINT, finishPoint);
        mAmWmState.assertVisibility(MOVE_TASK_TO_BACK_ACTIVITY, true);

        // Launch a different activity on top.
        launchActivity(BROADCAST_RECEIVER_ACTIVITY);
        mAmWmState.waitForActivityState(BROADCAST_RECEIVER_ACTIVITY, STATE_RESUMED);
        final boolean shouldBeVisible =
                !mAmWmState.getAmState().isBehindOpaqueActivities(MOVE_TASK_TO_BACK_ACTIVITY);
        mAmWmState.assertVisibility(MOVE_TASK_TO_BACK_ACTIVITY, shouldBeVisible);
        mAmWmState.assertVisibility(BROADCAST_RECEIVER_ACTIVITY, true);

        // Finish the top-most activity.
        executeShellCommand(FINISH_ACTIVITY_BROADCAST);
        //TODO: BUG: MoveTaskToBackActivity returns to the top of the stack when
        // BroadcastActivity finishes, so homeActivity is not visible afterwards

        // Home must be visible.
        if (hasHomeScreen()) {
            mAmWmState.waitForHomeActivityVisible();
            mAmWmState.assertHomeActivityVisible(true /* visible */);
        }
    }

    /**
     * Asserts that launching between reorder to front activities exhibits the correct backstack
     * behavior.
     */
    @Test
    public void testReorderToFrontBackstack() throws Exception {
        // Start with home on top
        launchHomeActivity();
        if (hasHomeScreen()) {
            mAmWmState.assertHomeActivityVisible(true /* visible */);
        }

        // Launch the launching activity to the foreground
        launchActivity(LAUNCHING_ACTIVITY);

        // Launch the alternate launching activity from launching activity with reorder to front.
        getLaunchActivityBuilder().setTargetActivity(ALT_LAUNCHING_ACTIVITY)
                .setReorderToFront(true).execute();

        // Launch the launching activity from the alternate launching activity with reorder to
        // front.
        getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY)
                .setLaunchingActivity(ALT_LAUNCHING_ACTIVITY)
                .setReorderToFront(true)
                .execute();

        // Press back
        pressBackButton();

        mAmWmState.waitForValidState(ALT_LAUNCHING_ACTIVITY);

        // Ensure the alternate launching activity is in focus
        mAmWmState.assertFocusedActivity("Alt Launching Activity must be focused",
                ALT_LAUNCHING_ACTIVITY);
    }

    /**
     * Asserts that the activity focus and history is preserved moving between the activity and
     * home stack.
     */
    @Test
    public void testReorderToFrontChangingStack() throws Exception {
        // Start with home on top
        launchHomeActivity();
        if (hasHomeScreen()) {
            mAmWmState.assertHomeActivityVisible(true /* visible */);
        }

        // Launch the launching activity to the foreground
        launchActivity(LAUNCHING_ACTIVITY);

        // Launch the alternate launching activity from launching activity with reorder to front.
        getLaunchActivityBuilder().setTargetActivity(ALT_LAUNCHING_ACTIVITY)
                .setReorderToFront(true)
                .execute();

        // Return home
        launchHomeActivity();
        if (hasHomeScreen()) {
            mAmWmState.assertHomeActivityVisible(true /* visible */);
        }
        // Launch the launching activity from the alternate launching activity with reorder to
        // front.

        // Bring launching activity back to the foreground
        launchActivityNoWait(LAUNCHING_ACTIVITY);
        // Wait for the most front activity of the task.
        mAmWmState.waitForValidState(ALT_LAUNCHING_ACTIVITY);

        // Ensure the alternate launching activity is still in focus.
        mAmWmState.assertFocusedActivity("Alt Launching Activity must be focused",
                ALT_LAUNCHING_ACTIVITY);

        pressBackButton();

        // Wait for the bottom activity back to the foreground.
        mAmWmState.waitForValidState(LAUNCHING_ACTIVITY);

        // Ensure launching activity was brought forward.
        mAmWmState.assertFocusedActivity("Launching Activity must be focused",
                LAUNCHING_ACTIVITY);
    }

    /**
     * Asserts that a nohistory activity is stopped and removed immediately after a resumed activity
     * above becomes visible and does not idle.
     */
    @Test
    public void testNoHistoryActivityFinishedResumedActivityNotIdle() throws Exception {
        if (!hasHomeScreen()) {
            return;
        }

        // Start with home on top
        launchHomeActivity();

        // Launch no history activity
        launchActivity(NO_HISTORY_ACTIVITY);

        // Launch an activity with a swipe refresh layout configured to prevent idle.
        launchActivity(SWIPE_REFRESH_ACTIVITY);

        pressBackButton();
        mAmWmState.waitForHomeActivityVisible();
        mAmWmState.assertHomeActivityVisible(true);
    }

    @Test
    public void testTurnScreenOnAttrNoLockScreen() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.disableLockScreen()
                    .sleepDevice();
            final LogSeparator logSeparator = separateLogs();
            launchActivity(TURN_SCREEN_ON_ATTR_ACTIVITY);
            mAmWmState.assertVisibility(TURN_SCREEN_ON_ATTR_ACTIVITY, true);
            assertTrue("Display turns on", isDisplayOn());
            assertSingleLaunch(TURN_SCREEN_ON_ATTR_ACTIVITY, logSeparator);
        }
    }

    @Test
    public void testTurnScreenOnAttrWithLockScreen() throws Exception {
        if (!supportsSecureLock()) {
            return;
        }

        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential()
                    .sleepDevice();
            final LogSeparator logSeparator = separateLogs();
            launchActivityNoWait(TURN_SCREEN_ON_ATTR_ACTIVITY);
            // Wait for the activity stopped because lock screen prevent showing the activity.
            mAmWmState.waitForActivityState(TURN_SCREEN_ON_ATTR_ACTIVITY, STATE_STOPPED);
            assertFalse("Display keeps off", isDisplayOn());
            assertSingleLaunchAndStop(TURN_SCREEN_ON_ATTR_ACTIVITY, logSeparator);
        }
    }

    @Test
    public void testTurnScreenOnShowOnLockAttr() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.sleepDevice();
            mAmWmState.waitForAllStoppedActivities();
            final LogSeparator logSeparator = separateLogs();
            launchActivity(TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY);
            mAmWmState.assertVisibility(TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY, true);
            assertTrue("Display turns on", isDisplayOn());
            assertSingleLaunch(TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY, logSeparator);
        }
    }

    @Test
    public void testTurnScreenOnAttrRemove() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.sleepDevice();
            mAmWmState.waitForAllStoppedActivities();
            LogSeparator logSeparator = separateLogs();
            launchActivity(TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY);
            assertTrue("Display turns on", isDisplayOn());
            assertSingleLaunch(TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY, logSeparator);

            lockScreenSession.sleepDevice();
            mAmWmState.waitForAllStoppedActivities();
            logSeparator = separateLogs();
            launchActivity(TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY);
            // Display should keep off, because setTurnScreenOn(false) has been called at
            // {@link TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY}'s onStop.
            assertFalse("Display keeps off", isDisplayOn());
            assertSingleStartAndStop(TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY, logSeparator);
        }
    }

    @Test
    @Presubmit
    public void testTurnScreenOnSingleTask() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.sleepDevice();
            LogSeparator logSeparator = separateLogs();
            launchActivity(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY);
            mAmWmState.assertVisibility(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY, true);
            assertTrue("Display turns on", isDisplayOn());
            assertSingleLaunch(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY, logSeparator);

            lockScreenSession.sleepDevice();
            logSeparator = separateLogs();
            launchActivity(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY);
            mAmWmState.assertVisibility(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY, true);
            assertTrue("Display turns on", isDisplayOn());
            assertSingleStart(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY, logSeparator);
        }
    }

    @Test
    public void testTurnScreenOnActivity_withRelayout() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.sleepDevice();
            launchActivity(TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY);
            mAmWmState.assertVisibility(TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY, true);

            lockScreenSession.sleepDevice();
            mAmWmState.waitForActivityState(TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY, STATE_STOPPED);

            // Ensure there was an actual stop if the waitFor timed out.
            assertTrue(getActivityName(TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY) + " stopped",
                    mAmWmState.getAmState().hasActivityState(
                            TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY, STATE_STOPPED));
            assertFalse("Display keeps off", isDisplayOn());
        }
    }
}
