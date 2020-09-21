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

package android.server.am;

import static android.app.ActivityManager.SPLIT_SCREEN_CREATE_MODE_BOTTOM_OR_RIGHT;
import static android.app.ActivityManager.SPLIT_SCREEN_CREATE_MODE_TOP_OR_LEFT;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_HOME;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_RECENTS;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN_OR_SPLIT_SCREEN_SECONDARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_SECONDARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.server.am.Components.ALT_LAUNCHING_ACTIVITY;
import static android.server.am.Components.DOCKED_ACTIVITY;
import static android.server.am.Components.LAUNCHING_ACTIVITY;
import static android.server.am.Components.NON_RESIZEABLE_ACTIVITY;
import static android.server.am.Components.NO_RELAUNCH_ACTIVITY;
import static android.server.am.Components.RESIZEABLE_ACTIVITY;
import static android.server.am.Components.SINGLE_INSTANCE_ACTIVITY;
import static android.server.am.Components.SINGLE_TASK_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.Components.TestActivity.TEST_ACTIVITY_ACTION_FINISH_SELF;
import static android.server.am.UiDeviceUtils.pressHomeButton;
import static android.server.am.WindowManagerState.TRANSIT_WALLPAPER_OPEN;
import static android.view.Surface.ROTATION_0;
import static android.view.Surface.ROTATION_180;
import static android.view.Surface.ROTATION_270;
import static android.view.Surface.ROTATION_90;

import static org.hamcrest.Matchers.lessThan;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.content.ComponentName;
import android.graphics.Rect;
import android.platform.test.annotations.Presubmit;
import android.support.test.filters.FlakyTest;

import org.junit.Before;
import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityManagerSplitScreenTests
 */
public class ActivityManagerSplitScreenTests extends ActivityManagerTestBase {

    private static final int TASK_SIZE = 600;
    private static final int STACK_SIZE = 300;

    private boolean mIsHomeRecentsComponent;

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();

        mIsHomeRecentsComponent = mAmWmState.getAmState().isHomeRecentsComponent();

        assumeTrue("Skipping test: no split multi-window support",
                supportsSplitScreenMultiWindow());
    }

    @Test
    public void testMinimumDeviceSize() throws Exception {
        mAmWmState.assertDeviceDefaultDisplaySize(
                "Devices supporting multi-window must be larger than the default minimum"
                        + " task size");
    }

    @Test
    @Presubmit
    @FlakyTest(bugId = 71792393)
    public void testStackList() throws Exception {
        launchActivity(TEST_ACTIVITY);
        mAmWmState.computeState(TEST_ACTIVITY);
        mAmWmState.assertContainsStack("Must contain home stack.",
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME);
        mAmWmState.assertContainsStack("Must contain fullscreen stack.",
                WINDOWING_MODE_FULLSCREEN, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertDoesNotContainStack("Must not contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);
    }

    @Test
    @Presubmit
    public void testDockActivity() throws Exception {
        launchActivityInSplitScreenWithRecents(TEST_ACTIVITY);
        mAmWmState.assertContainsStack("Must contain home stack.",
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME);
        mAmWmState.assertContainsStack("Must contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);
    }

    @Test
    @Presubmit
    public void testNonResizeableNotDocked() throws Exception {
        launchActivityInSplitScreenWithRecents(NON_RESIZEABLE_ACTIVITY);

        mAmWmState.assertContainsStack("Must contain home stack.",
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME);
        mAmWmState.assertDoesNotContainStack("Must not contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertFrontStack("Fullscreen stack must be front stack.",
                WINDOWING_MODE_FULLSCREEN, ACTIVITY_TYPE_STANDARD);
    }

    @Test
    @Presubmit
    public void testLaunchToSide() throws Exception {
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        mAmWmState.assertContainsStack("Must contain fullscreen stack.",
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertContainsStack("Must contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);
    }

    @Test
    @Presubmit
    public void testLaunchToSideMultiWindowCallbacks() throws Exception {
        // Launch two activities in split-screen mode.
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        mAmWmState.assertContainsStack("Must contain fullscreen stack.",
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertContainsStack("Must contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);

        // Exit split-screen mode and ensure we only get 1 multi-window mode changed callback.
        final LogSeparator logSeparator = separateLogs();
        removeStacksInWindowingModes(WINDOWING_MODE_SPLIT_SCREEN_PRIMARY);
        final ActivityLifecycleCounts lifecycleCounts = waitForOnMultiWindowModeChanged(
                TEST_ACTIVITY, logSeparator);
        assertEquals(1, lifecycleCounts.mMultiWindowModeChangedCount);
    }

    @Test
    @Presubmit
    @FlakyTest(bugId = 72956284)
    public void testNoUserLeaveHintOnMultiWindowModeChanged() throws Exception {
        launchActivity(TEST_ACTIVITY, WINDOWING_MODE_FULLSCREEN);

        // Move to docked stack.
        LogSeparator logSeparator = separateLogs();
        setActivityTaskWindowingMode(TEST_ACTIVITY, WINDOWING_MODE_SPLIT_SCREEN_PRIMARY);
        ActivityLifecycleCounts lifecycleCounts = waitForOnMultiWindowModeChanged(
                TEST_ACTIVITY, logSeparator);
        assertEquals("mMultiWindowModeChangedCount",
                1, lifecycleCounts.mMultiWindowModeChangedCount);
        assertEquals("mUserLeaveHintCount", 0, lifecycleCounts.mUserLeaveHintCount);

        // Make sure docked stack is focused. This way when we dismiss it later fullscreen stack
        // will come up.
        launchActivity(TEST_ACTIVITY, WINDOWING_MODE_SPLIT_SCREEN_PRIMARY);

        // Move activity back to fullscreen stack.
        logSeparator = separateLogs();
        setActivityTaskWindowingMode(TEST_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        lifecycleCounts = waitForOnMultiWindowModeChanged(TEST_ACTIVITY, logSeparator);
        assertEquals("mMultiWindowModeChangedCount",
                1, lifecycleCounts.mMultiWindowModeChangedCount);
        assertEquals("mUserLeaveHintCount", 0, lifecycleCounts.mUserLeaveHintCount);
    }

    @Test
    @Presubmit
    public void testLaunchToSideAndBringToFront() throws Exception {
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));

        int taskNumberInitial = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        mAmWmState.assertFocusedActivity("Launched to side activity must be in front.",
                TEST_ACTIVITY);

        // Launch another activity to side to cover first one.
        launchActivity(NO_RELAUNCH_ACTIVITY, WINDOWING_MODE_FULLSCREEN_OR_SPLIT_SCREEN_SECONDARY);
        int taskNumberCovered = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        assertEquals("Fullscreen stack must have one task added.",
                taskNumberInitial + 1, taskNumberCovered);
        mAmWmState.assertFocusedActivity("Launched to side covering activity must be in front.",
                NO_RELAUNCH_ACTIVITY);

        // Launch activity that was first launched to side. It should be brought to front.
        getLaunchActivityBuilder()
                .setTargetActivity(TEST_ACTIVITY)
                .setToSide(true)
                .setWaitForLaunched(true)
                .execute();
        int taskNumberFinal = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        assertEquals("Task number in fullscreen stack must remain the same.",
                taskNumberCovered, taskNumberFinal);
        mAmWmState.assertFocusedActivity("Launched to side covering activity must be in front.",
                TEST_ACTIVITY);
    }

    @Test
    @Presubmit
    @FlakyTest(bugId = 71792393)
    public void testLaunchToSideMultiple() throws Exception {
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));

        int taskNumberInitial = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        assertNotNull("Launched to side activity must be in fullscreen stack.",
                mAmWmState.getAmState().getTaskByActivity(
                        TEST_ACTIVITY, WINDOWING_MODE_SPLIT_SCREEN_SECONDARY));

        // Try to launch to side same activity again.
        getLaunchActivityBuilder().setToSide(true).execute();
        mAmWmState.computeState(TEST_ACTIVITY, LAUNCHING_ACTIVITY);
        int taskNumberFinal = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        assertEquals("Task number mustn't change.", taskNumberInitial, taskNumberFinal);
        mAmWmState.assertFocusedActivity("Launched to side activity must remain in front.",
                TEST_ACTIVITY);
        assertNotNull("Launched to side activity must remain in fullscreen stack.",
                mAmWmState.getAmState().getTaskByActivity(
                        TEST_ACTIVITY, WINDOWING_MODE_SPLIT_SCREEN_SECONDARY));
    }

    @Test
    @Presubmit
    @FlakyTest(bugId = 73808815)
    public void testLaunchToSideSingleInstance() throws Exception {
        launchTargetToSide(SINGLE_INSTANCE_ACTIVITY, false);
    }

    @Test
    public void testLaunchToSideSingleTask() throws Exception {
        launchTargetToSide(SINGLE_TASK_ACTIVITY, false);
    }

    @Presubmit
    @FlakyTest(bugId = 71792393)
    @Test
    public void testLaunchToSideMultipleWithDifferentIntent() throws Exception {
        launchTargetToSide(TEST_ACTIVITY, true);
    }

    private void launchTargetToSide(ComponentName targetActivityName,
            boolean taskCountMustIncrement) throws Exception {
        // Launch in fullscreen first
        getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY)
                .setUseInstrumentation()
                .setWaitForLaunched(true)
                .execute();

        // Move to split-screen primary
        final int taskId = mAmWmState.getAmState().getTaskByActivity(LAUNCHING_ACTIVITY).mTaskId;
        moveTaskToPrimarySplitScreen(taskId, true /* launchSideActivityIfNeeded */);

        // Launch target to side
        final LaunchActivityBuilder targetActivityLauncher = getLaunchActivityBuilder()
                .setTargetActivity(targetActivityName)
                .setToSide(true)
                .setRandomData(true)
                .setMultipleTask(false);
        targetActivityLauncher.execute();

        mAmWmState.computeState(targetActivityName, LAUNCHING_ACTIVITY);
        mAmWmState.assertContainsStack("Must contain secondary stack.",
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD);
        int taskNumberInitial = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        assertNotNull("Launched to side activity must be in fullscreen stack.",
                mAmWmState.getAmState().getTaskByActivity(
                        targetActivityName, WINDOWING_MODE_SPLIT_SCREEN_SECONDARY));

        // Try to launch to side same activity again with different data.
        targetActivityLauncher.execute();
        mAmWmState.computeState(targetActivityName, LAUNCHING_ACTIVITY);
        int taskNumberSecondLaunch = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        if (taskCountMustIncrement) {
            assertEquals("Task number must be incremented.", taskNumberInitial + 1,
                    taskNumberSecondLaunch);
        } else {
            assertEquals("Task number must not change.", taskNumberInitial,
                    taskNumberSecondLaunch);
        }
        mAmWmState.assertFocusedActivity("Launched to side activity must be in front.",
                targetActivityName);
        assertNotNull("Launched to side activity must be launched in fullscreen stack.",
                mAmWmState.getAmState().getTaskByActivity(
                        targetActivityName, WINDOWING_MODE_SPLIT_SCREEN_SECONDARY));

        // Try to launch to side same activity again with different random data. Note that null
        // cannot be used here, since the first instance of TestActivity is launched with no data
        // in order to launch into split screen.
        targetActivityLauncher.execute();
        mAmWmState.computeState(targetActivityName, LAUNCHING_ACTIVITY);
        int taskNumberFinal = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        if (taskCountMustIncrement) {
            assertEquals("Task number must be incremented.", taskNumberSecondLaunch + 1,
                    taskNumberFinal);
        } else {
            assertEquals("Task number must not change.", taskNumberSecondLaunch,
                    taskNumberFinal);
        }
        mAmWmState.assertFocusedActivity("Launched to side activity must be in front.",
                targetActivityName);
        assertNotNull("Launched to side activity must be launched in fullscreen stack.",
                mAmWmState.getAmState().getTaskByActivity(
                        targetActivityName, WINDOWING_MODE_SPLIT_SCREEN_SECONDARY));
    }

    @Presubmit
    @Test
    public void testLaunchToSideMultipleWithFlag() throws Exception {
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        int taskNumberInitial = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        assertNotNull("Launched to side activity must be in fullscreen stack.",
                mAmWmState.getAmState().getTaskByActivity(
                        TEST_ACTIVITY, WINDOWING_MODE_SPLIT_SCREEN_SECONDARY));

        // Try to launch to side same activity again, but with Intent#FLAG_ACTIVITY_MULTIPLE_TASK.
        getLaunchActivityBuilder().setToSide(true).setMultipleTask(true).execute();
        mAmWmState.computeState(LAUNCHING_ACTIVITY, TEST_ACTIVITY);
        int taskNumberFinal = mAmWmState.getAmState().getStandardTaskCountByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
        assertEquals("Task number must be incremented.", taskNumberInitial + 1,
                taskNumberFinal);
        mAmWmState.assertFocusedActivity("Launched to side activity must be in front.",
                TEST_ACTIVITY);
        assertNotNull("Launched to side activity must remain in fullscreen stack.",
                mAmWmState.getAmState().getTaskByActivity(
                        TEST_ACTIVITY, WINDOWING_MODE_SPLIT_SCREEN_SECONDARY));
    }

    @Test
    public void testRotationWhenDocked() throws Exception {
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        mAmWmState.assertContainsStack("Must contain fullscreen stack.",
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertContainsStack("Must contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);

        // Rotate device single steps (90°) 0-1-2-3.
        // Each time we compute the state we implicitly assert valid bounds.
        try (final RotationSession rotationSession = new RotationSession()) {
            for (int i = 0; i < 4; i++) {
                rotationSession.set(i);
                mAmWmState.computeState(LAUNCHING_ACTIVITY, TEST_ACTIVITY);
            }
            // Double steps (180°) We ended the single step at 3. So, we jump directly to 1 for
            // double step. So, we are testing 3-1-3 for one side and 0-2-0 for the other side.
            rotationSession.set(ROTATION_90);
            mAmWmState.computeState(LAUNCHING_ACTIVITY, TEST_ACTIVITY);
            rotationSession.set(ROTATION_270);
            mAmWmState.computeState(LAUNCHING_ACTIVITY, TEST_ACTIVITY);
            rotationSession.set(ROTATION_0);
            mAmWmState.computeState(LAUNCHING_ACTIVITY, TEST_ACTIVITY);
            rotationSession.set(ROTATION_180);
            mAmWmState.computeState(LAUNCHING_ACTIVITY, TEST_ACTIVITY);
            rotationSession.set(ROTATION_0);
            mAmWmState.computeState(LAUNCHING_ACTIVITY, TEST_ACTIVITY);
        }
    }

    @Test
    @Presubmit
    public void testRotationWhenDockedWhileLocked() throws Exception {
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        mAmWmState.assertSanity();
        mAmWmState.assertContainsStack("Must contain fullscreen stack.",
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertContainsStack("Must contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);

        try (final RotationSession rotationSession = new RotationSession();
             final LockScreenSession lockScreenSession = new LockScreenSession()) {
            for (int i = 0; i < 4; i++) {
                lockScreenSession.sleepDevice();
                rotationSession.set(i);
                lockScreenSession.wakeUpDevice()
                        .unlockDevice();
                mAmWmState.computeState(LAUNCHING_ACTIVITY, TEST_ACTIVITY);
            }
        }
    }

    @Test
    public void testMinimizedFromEachDockedSide() throws Exception {
        try (final RotationSession rotationSession = new RotationSession()) {
            for (int i = 0; i < 2; i++) {
                rotationSession.set(i);
                launchActivityInDockStackAndMinimize(TEST_ACTIVITY);
                if (!mAmWmState.isScreenPortrait() && isTablet()) {
                    // Test minimize to the right only on tablets in landscape
                    removeStacksWithActivityTypes(ALL_ACTIVITY_TYPE_BUT_HOME);
                    launchActivityInDockStackAndMinimize(TEST_ACTIVITY,
                            SPLIT_SCREEN_CREATE_MODE_BOTTOM_OR_RIGHT);
                }
                removeStacksWithActivityTypes(ALL_ACTIVITY_TYPE_BUT_HOME);
            }
        }
    }

    @Test
    @Presubmit
    public void testRotationWhileDockMinimized() throws Exception {
        launchActivityInDockStackAndMinimize(TEST_ACTIVITY);

        // Rotate device single steps (90°) 0-1-2-3.
        // Each time we compute the state we implicitly assert valid bounds in minimized mode.
        try (final RotationSession rotationSession = new RotationSession()) {
            for (int i = 0; i < 4; i++) {
                rotationSession.set(i);
                mAmWmState.computeState(TEST_ACTIVITY);
            }

            // Double steps (180°) We ended the single step at 3. So, we jump directly to 1 for
            // double step. So, we are testing 3-1-3 for one side and 0-2-0 for the other side in
            // minimized mode.
            rotationSession.set(ROTATION_90);
            mAmWmState.computeState(TEST_ACTIVITY);
            rotationSession.set(ROTATION_270);
            mAmWmState.computeState(TEST_ACTIVITY);
            rotationSession.set(ROTATION_0);
            mAmWmState.computeState(TEST_ACTIVITY);
            rotationSession.set(ROTATION_180);
            mAmWmState.computeState(TEST_ACTIVITY);
            rotationSession.set(ROTATION_0);
            mAmWmState.computeState(TEST_ACTIVITY);
        }
    }

    @Test
    public void testMinimizeAndUnminimizeThenGoingHome() throws Exception {
        // Rotate the screen to check that minimize, unminimize, dismiss the docked stack and then
        // going home has the correct app transition
        try (final RotationSession rotationSession = new RotationSession()) {
            for (int rotation = ROTATION_0; rotation <= ROTATION_270; rotation++) {
                rotationSession.set(rotation);
                launchActivityInDockStackAndMinimize(DOCKED_ACTIVITY);

                if (mIsHomeRecentsComponent) {
                    launchActivity(TEST_ACTIVITY,
                            WINDOWING_MODE_FULLSCREEN_OR_SPLIT_SCREEN_SECONDARY);
                } else {
                    // Unminimize the docked stack
                    pressAppSwitchButtonAndWaitForRecents();
                    waitForDockNotMinimized();
                    assertDockNotMinimized();
                }

                // Dismiss the dock stack
                setActivityTaskWindowingMode(DOCKED_ACTIVITY,
                        WINDOWING_MODE_FULLSCREEN_OR_SPLIT_SCREEN_SECONDARY);
                mAmWmState.computeState(DOCKED_ACTIVITY);

                // Go home and check the app transition
                assertNotEquals(
                        TRANSIT_WALLPAPER_OPEN, mAmWmState.getWmState().getLastTransition());
                pressHomeButton();
                mAmWmState.waitForHomeActivityVisible();

                assertEquals(TRANSIT_WALLPAPER_OPEN, mAmWmState.getWmState().getLastTransition());
            }
        }
    }

    @FlakyTest(bugId = 73813034)
    @Test
    @Presubmit
    public void testFinishDockActivityWhileMinimized() throws Exception {
        launchActivityInDockStackAndMinimize(TEST_ACTIVITY);

        executeShellCommand("am broadcast -a " + TEST_ACTIVITY_ACTION_FINISH_SELF);
        waitForDockNotMinimized();
        mAmWmState.assertVisibility(TEST_ACTIVITY, false);
        assertDockNotMinimized();
    }

    @Test
    @Presubmit
    public void testDockedStackToMinimizeWhenUnlocked() throws Exception {
        launchActivityInSplitScreenWithRecents(TEST_ACTIVITY);
        mAmWmState.computeState(TEST_ACTIVITY);
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.sleepDevice()
                    .wakeUpDevice()
                    .unlockDevice();
            mAmWmState.computeState(TEST_ACTIVITY);
            assertDockMinimized();
        }
    }

    @Test
    public void testMinimizedStateWhenUnlockedAndUnMinimized() throws Exception {
        launchActivityInDockStackAndMinimize(TEST_ACTIVITY);

        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.sleepDevice()
                    .wakeUpDevice()
                    .unlockDevice();
            mAmWmState.computeState(TEST_ACTIVITY);

            // Unminimized back to splitscreen
            if (mIsHomeRecentsComponent) {
                launchActivity(RESIZEABLE_ACTIVITY,
                        WINDOWING_MODE_FULLSCREEN_OR_SPLIT_SCREEN_SECONDARY);
            } else {
                pressAppSwitchButtonAndWaitForRecents();
            }
            mAmWmState.computeState(TEST_ACTIVITY);
        }
    }

    @Test
    @Presubmit
    public void testResizeDockedStack() throws Exception {
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        resizeDockedStack(STACK_SIZE, STACK_SIZE, TASK_SIZE, TASK_SIZE);
        mAmWmState.computeState(false /* compareTaskAndStackBounds */,
                new WaitForValidActivityState(TEST_ACTIVITY),
                new WaitForValidActivityState(DOCKED_ACTIVITY));
        mAmWmState.assertContainsStack("Must contain secondary split-screen stack.",
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertContainsStack("Must contain primary split-screen stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);
        assertEquals(new Rect(0, 0, STACK_SIZE, STACK_SIZE),
                mAmWmState.getAmState().getStandardStackByWindowingMode(
                        WINDOWING_MODE_SPLIT_SCREEN_PRIMARY).getBounds());
        mAmWmState.assertDockedTaskBounds(TASK_SIZE, TASK_SIZE, DOCKED_ACTIVITY);
        mAmWmState.assertVisibility(DOCKED_ACTIVITY, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY, true);
    }

    @Test
    public void testActivityLifeCycleOnResizeDockedStack() throws Exception {
        launchActivity(TEST_ACTIVITY);
        mAmWmState.computeState(TEST_ACTIVITY);
        final Rect fullScreenBounds = mAmWmState.getWmState().getStandardStackByWindowingMode(
                WINDOWING_MODE_FULLSCREEN).getBounds();

        setActivityTaskWindowingMode(TEST_ACTIVITY, WINDOWING_MODE_SPLIT_SCREEN_PRIMARY);
        mAmWmState.computeState(TEST_ACTIVITY);
        launchActivity(NO_RELAUNCH_ACTIVITY, WINDOWING_MODE_FULLSCREEN_OR_SPLIT_SCREEN_SECONDARY);

        mAmWmState.computeState(TEST_ACTIVITY, NO_RELAUNCH_ACTIVITY);
        final Rect initialDockBounds = mAmWmState.getWmState().getStandardStackByWindowingMode(
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY) .getBounds();

        final LogSeparator logSeparator = separateLogs();

        Rect newBounds = computeNewDockBounds(fullScreenBounds, initialDockBounds, true);
        resizeDockedStack(
                newBounds.width(), newBounds.height(), newBounds.width(), newBounds.height());
        mAmWmState.computeState(TEST_ACTIVITY, NO_RELAUNCH_ACTIVITY);

        // We resize twice to make sure we cross an orientation change threshold for both
        // activities.
        newBounds = computeNewDockBounds(fullScreenBounds, initialDockBounds, false);
        resizeDockedStack(
                newBounds.width(), newBounds.height(), newBounds.width(), newBounds.height());
        mAmWmState.computeState(TEST_ACTIVITY, NO_RELAUNCH_ACTIVITY);
        assertActivityLifecycle(TEST_ACTIVITY, true /* relaunched */, logSeparator);
        assertActivityLifecycle(NO_RELAUNCH_ACTIVITY, false /* relaunched */, logSeparator);
    }

    private Rect computeNewDockBounds(
            Rect fullscreenBounds, Rect dockBounds, boolean reduceSize) {
        final boolean inLandscape = fullscreenBounds.width() > dockBounds.width();
        // We are either increasing size or reducing it.
        final float sizeChangeFactor = reduceSize ? 0.5f : 1.5f;
        final Rect newBounds = new Rect(dockBounds);
        if (inLandscape) {
            // In landscape we change the width.
            newBounds.right = (int) (newBounds.left + (newBounds.width() * sizeChangeFactor));
        } else {
            // In portrait we change the height
            newBounds.bottom = (int) (newBounds.top + (newBounds.height() * sizeChangeFactor));
        }

        return newBounds;
    }

    @Test
    @Presubmit
    public void testStackListOrderLaunchDockedActivity() throws Exception {
        assumeTrue(!mIsHomeRecentsComponent);

        launchActivityInSplitScreenWithRecents(TEST_ACTIVITY);

        final int homeStackIndex = mAmWmState.getStackIndexByActivityType(ACTIVITY_TYPE_HOME);
        final int recentsStackIndex = mAmWmState.getStackIndexByActivityType(ACTIVITY_TYPE_RECENTS);
        assertThat("Recents stack should be on top of home stack",
                recentsStackIndex, lessThan(homeStackIndex));
    }

    @Test
    @Presubmit
    public void testStackListOrderOnSplitScreenDismissed() throws Exception {
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));

        setActivityTaskWindowingMode(DOCKED_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        mAmWmState.computeState(new WaitForValidActivityState.Builder(DOCKED_ACTIVITY)
                .setWindowingMode(WINDOWING_MODE_FULLSCREEN)
                .build());

        final int homeStackIndex = mAmWmState.getStackIndexByActivityType(ACTIVITY_TYPE_HOME);
        final int prevSplitScreenPrimaryIndex =
                mAmWmState.getAmState().getStackIndexByActivity(DOCKED_ACTIVITY);
        final int prevSplitScreenSecondaryIndex =
                mAmWmState.getAmState().getStackIndexByActivity(TEST_ACTIVITY);

        final int expectedHomeStackIndex =
                (prevSplitScreenPrimaryIndex > prevSplitScreenSecondaryIndex
                        ? prevSplitScreenPrimaryIndex : prevSplitScreenSecondaryIndex) - 1;
        assertEquals("Home stack needs to be directly behind the top stack",
                expectedHomeStackIndex, homeStackIndex);
    }

    private void launchActivityInDockStackAndMinimize(ComponentName activityName) throws Exception {
        launchActivityInDockStackAndMinimize(activityName, SPLIT_SCREEN_CREATE_MODE_TOP_OR_LEFT);
    }

    private void launchActivityInDockStackAndMinimize(ComponentName activityName, int createMode)
            throws Exception {
        launchActivityInSplitScreenWithRecents(activityName, createMode);
        pressHomeButton();
        waitForAndAssertDockMinimized();
    }

    private void assertDockMinimized() {
        assertTrue(mAmWmState.getWmState().isDockedStackMinimized());
    }

    private void waitForAndAssertDockMinimized() throws Exception {
        waitForDockMinimized();
        assertDockMinimized();
        mAmWmState.computeState(TEST_ACTIVITY);
        mAmWmState.assertContainsStack("Must contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertFocusedStack("Home activity should be focused in minimized mode",
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME);
    }

    private void assertDockNotMinimized() {
        assertFalse(mAmWmState.getWmState().isDockedStackMinimized());
    }

    private void waitForDockMinimized() throws Exception {
        mAmWmState.waitForWithWmState(state -> state.isDockedStackMinimized(),
                "***Waiting for Dock stack to be minimized");
    }

    private void waitForDockNotMinimized() throws Exception {
        mAmWmState.waitForWithWmState(state -> !state.isDockedStackMinimized(),
                "***Waiting for Dock stack to not be minimized");
    }
}
