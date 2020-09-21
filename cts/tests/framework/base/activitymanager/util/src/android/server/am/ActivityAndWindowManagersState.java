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
import static android.app.WindowConfiguration.ACTIVITY_TYPE_UNDEFINED;
import static android.app.WindowConfiguration.WINDOWING_MODE_FREEFORM;
import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.content.pm.ActivityInfo.RESIZE_MODE_RESIZEABLE;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.ComponentNameUtils.getWindowName;
import static android.server.am.StateLogger.log;
import static android.server.am.StateLogger.logAlways;
import static android.server.am.StateLogger.logE;
import static android.util.DisplayMetrics.DENSITY_DEFAULT;
import static android.view.Display.DEFAULT_DISPLAY;

import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.lessThan;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.ComponentName;
import android.graphics.Rect;
import android.os.SystemClock;
import android.server.am.ActivityManagerState.ActivityStack;
import android.server.am.ActivityManagerState.ActivityTask;
import android.server.am.WindowManagerState.Display;
import android.server.am.WindowManagerState.WindowStack;
import android.server.am.WindowManagerState.WindowState;
import android.server.am.WindowManagerState.WindowTask;

import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.function.BiPredicate;
import java.util.function.BooleanSupplier;
import java.util.function.Predicate;
import java.util.stream.Collectors;

/**
 * Combined state of the activity manager and window manager.
 */
public class ActivityAndWindowManagersState {

    // Default minimal size of resizable task, used if none is set explicitly.
    // Must be kept in sync with 'default_minimal_size_resizable_task' dimen from frameworks/base.
    private static final int DEFAULT_RESIZABLE_TASK_SIZE_DP = 220;

    // Default minimal size of a resizable PiP task, used if none is set explicitly.
    // Must be kept in sync with 'default_minimal_size_pip_resizable_task' dimen from
    // frameworks/base.
    private static final int DEFAULT_PIP_RESIZABLE_TASK_SIZE_DP = 108;

    private final ActivityManagerState mAmState = new ActivityManagerState();
    private final WindowManagerState mWmState = new WindowManagerState();

    /**
     * Compute AM and WM state of device, check sanity and bounds.
     * WM state will include only visible windows, stack and task bounds will be compared.
     *
     * @param componentNames array of activity names to wait for.
     */
    public void computeState(ComponentName... componentNames) {
        waitForValidState(true /* compareTaskAndStackBounds */,
                Arrays.stream(componentNames)
                        .map(WaitForValidActivityState::new)
                        .toArray(WaitForValidActivityState[]::new));
    }

    /**
     * Compute AM and WM state of device, check sanity and bounds.
     * WM state will include only visible windows, stack and task bounds will be compared.
     *
     * @param waitForActivitiesVisible array of activity names to wait for.
     */
    public void computeState(WaitForValidActivityState... waitForActivitiesVisible) {
        waitForValidState(true /* compareTaskAndStackBounds */, waitForActivitiesVisible);
    }

    /**
     * Compute AM and WM state of device, check sanity and bounds.
     *
     * @param compareTaskAndStackBounds pass 'true' if stack and task bounds should be compared,
     *                                  'false' otherwise.
     * @param waitForActivitiesVisible  array of activity states to wait for.
     */
    void computeState(boolean compareTaskAndStackBounds,
            WaitForValidActivityState... waitForActivitiesVisible) {
        waitForValidState(compareTaskAndStackBounds, waitForActivitiesVisible);
    }

    /**
     * Wait for the activities to appear and for valid state in AM and WM.
     *
     * @param activityNames name list of activities to wait for.
     */
    public void waitForValidState(ComponentName... activityNames) {
        waitForValidState(false /* compareTaskAndStackBounds */,
                Arrays.stream(activityNames)
                        .map(WaitForValidActivityState::new)
                        .toArray(WaitForValidActivityState[]::new));

    }

    /** Wait for the activity to appear and for valid state in AM and WM. */
    void waitForValidState(WaitForValidActivityState... waitForActivityVisible) {
        waitForValidState(false /* compareTaskAndStackBounds */, waitForActivityVisible);
    }

    /**
     * Wait for the activities to appear in proper stacks and for valid state in AM and WM.
     *
     * @param compareTaskAndStackBounds flag indicating if we should compare task and stack bounds
     *                                  for equality.
     * @param waitForActivitiesVisible  array of activity states to wait for.
     */
    private void waitForValidState(boolean compareTaskAndStackBounds,
            WaitForValidActivityState... waitForActivitiesVisible) {
        for (int retry = 1; retry <= 5; retry++) {
            // TODO: Get state of AM and WM at the same time to avoid mismatches caused by
            // requesting dump in some intermediate state.
            mAmState.computeState();
            mWmState.computeState();
            if (shouldWaitForSanityCheck(compareTaskAndStackBounds)
                    || shouldWaitForValidStacks(compareTaskAndStackBounds)
                    || shouldWaitForActivities(waitForActivitiesVisible)
                    || shouldWaitForWindows()) {
                logAlways("***Waiting for valid stacks and activities states... retry=" + retry);
                SystemClock.sleep(1000);
            } else {
                return;
            }
        }
        logE("***Waiting for states failed: " + Arrays.toString(waitForActivitiesVisible));
    }

    /**
     * Ensures all exiting windows have been removed.
     */
    void waitForAllExitingWindows() {
        List<WindowState> exitingWindows = null;
        for (int retry = 1; retry <= 5; retry++) {
            mWmState.computeState();
            exitingWindows = mWmState.getExitingWindows();
            if (exitingWindows.isEmpty()) {
                return;
            }
            logAlways("***Waiting for all exiting windows have been removed... retry=" + retry);
            SystemClock.sleep(1000);
        }
        fail("All exiting windows have been removed, actual=" + exitingWindows.stream()
                .map(WindowState::getName)
                .collect(Collectors.joining(",")));
    }

    void waitForAllStoppedActivities() {
        for (int retry = 1; retry <= 5; retry++) {
            mAmState.computeState();
            if (!mAmState.containsStartedActivities()) {
                return;
            }
            logAlways("***Waiting for all started activities have been removed... retry=" + retry);
            SystemClock.sleep(1500);
        }
        fail("All started activities have been removed");
    }

    /**
     * Compute AM and WM state of device, wait for the activity records to be added, and
     * wait for debugger window to show up.
     *
     * This should only be used when starting with -D (debugger) option, where we pop up the
     * waiting-for-debugger window, but real activity window won't show up since we're waiting
     * for debugger.
     */
    void waitForDebuggerWindowVisible(ComponentName activityName) {
        for (int retry = 1; retry <= 5; retry++) {
            mAmState.computeState();
            mWmState.computeState();
            if (shouldWaitForDebuggerWindow(activityName)
                    || shouldWaitForActivityRecords(activityName)) {
                logAlways("***Waiting for debugger window... retry=" + retry);
                SystemClock.sleep(1000);
            } else {
                return;
            }
        }
        logE("***Waiting for debugger window failed");
    }

    boolean waitForHomeActivityVisible() {
        ComponentName homeActivity = mAmState.getHomeActivityName();
        // Sometimes this function is called before we know what Home Activity is
        if (homeActivity == null) {
            logAlways("Computing state to determine Home Activity");
            computeState(true);
            homeActivity = mAmState.getHomeActivityName();
        }
        assertNotNull("homeActivity should not be null", homeActivity);
        waitForValidState(homeActivity);
        return mAmState.isHomeActivityVisible();
    }

    /**
     * @return true if recents activity is visible. Devices without recents will return false
     */
    boolean waitForRecentsActivityVisible() {
        if (mAmState.isHomeRecentsComponent()) {
            return waitForHomeActivityVisible();
        }

        waitForWithAmState(ActivityManagerState::isRecentsActivityVisible,
                "***Waiting for recents activity to be visible...");
        return mAmState.isRecentsActivityVisible();
    }

    void waitForKeyguardShowingAndNotOccluded() {
        waitForWithAmState(state -> state.getKeyguardControllerState().keyguardShowing
                        && !state.getKeyguardControllerState().keyguardOccluded,
                "***Waiting for Keyguard showing...");
    }

    void waitForKeyguardShowingAndOccluded() {
        waitForWithAmState(state -> state.getKeyguardControllerState().keyguardShowing
                        && state.getKeyguardControllerState().keyguardOccluded,
                "***Waiting for Keyguard showing and occluded...");
    }

    void waitForKeyguardGone() {
        waitForWithAmState(state -> !state.getKeyguardControllerState().keyguardShowing,
                "***Waiting for Keyguard gone...");
    }

    /** Wait for specific rotation for the default display. Values are Surface#Rotation */
    void waitForRotation(int rotation) {
        waitForWithWmState(state -> state.getRotation() == rotation,
                "***Waiting for Rotation: " + rotation);
    }

    /**
     * Wait for specific orientation for the default display.
     * Values are ActivityInfo.ScreenOrientation
     */
    void waitForLastOrientation(int orientation) {
        waitForWithWmState(state -> state.getLastOrientation() == orientation,
                "***Waiting for LastOrientation: " + orientation);
    }

    void waitForDisplayUnfrozen() {
        waitForWithWmState(state -> !state.isDisplayFrozen(),
                "***Waiting for Display unfrozen");
    }

    public void waitForActivityState(ComponentName activityName, String activityState) {
        waitForWithAmState(state -> state.hasActivityState(activityName, activityState),
                "***Waiting for Activity State: " + activityState);
    }

    @Deprecated
    void waitForFocusedStack(int stackId) {
        waitForWithAmState(state -> state.getFocusedStackId() == stackId,
                "***Waiting for focused stack...");
    }

    void waitForFocusedStack(int windowingMode, int activityType) {
        waitForWithAmState(state ->
                        (activityType == ACTIVITY_TYPE_UNDEFINED
                                || state.getFocusedStackActivityType() == activityType)
                        && (windowingMode == WINDOWING_MODE_UNDEFINED
                                || state.getFocusedStackWindowingMode() == windowingMode),
                "***Waiting for focused stack...");
    }

    void waitForAppTransitionIdle() {
        waitForWithWmState(
                state -> WindowManagerState.APP_STATE_IDLE.equals(state.getAppTransitionState()),
                "***Waiting for app transition idle...");
    }

    void waitForWithAmState(Predicate<ActivityManagerState> waitCondition, String message) {
        waitFor((amState, wmState) -> waitCondition.test(amState), message);
    }

    void waitForWithWmState(Predicate<WindowManagerState> waitCondition, String message) {
        waitFor((amState, wmState) -> waitCondition.test(wmState), message);
    }

    void waitFor(
            BiPredicate<ActivityManagerState, WindowManagerState> waitCondition, String message) {
        waitFor(message, () -> {
            mAmState.computeState();
            mWmState.computeState();
            return waitCondition.test(mAmState, mWmState);
        });
    }

    void waitFor(String message, BooleanSupplier waitCondition) {
        for (int retry = 1; retry <= 5; retry++) {
            if (waitCondition.getAsBoolean()) {
                return;
            }
            logAlways(message + " retry=" + retry);
            SystemClock.sleep(1000);
        }
        logE(message + " failed");
    }

    /**
     * @return true if should wait for valid stacks state.
     */
    private boolean shouldWaitForValidStacks(boolean compareTaskAndStackBounds) {
        if (!taskListsInAmAndWmAreEqual()) {
            // We want to wait for equal task lists in AM and WM in case we caught them in the
            // middle of some state change operations.
            logAlways("***taskListsInAmAndWmAreEqual=false");
            return true;
        }
        if (!stackBoundsInAMAndWMAreEqual()) {
            // We want to wait a little for the stacks in AM and WM to have equal bounds as there
            // might be a transition animation ongoing when we got the states from WM AM separately.
            logAlways("***stackBoundsInAMAndWMAreEqual=false");
            return true;
        }
        try {
            // Temporary fix to avoid catching intermediate state with different task bounds in AM
            // and WM.
            assertValidBounds(compareTaskAndStackBounds);
        } catch (AssertionError e) {
            logAlways("***taskBoundsInAMAndWMAreEqual=false : " + e.getMessage());
            return true;
        }
        final int stackCount = mAmState.getStackCount();
        if (stackCount == 0) {
            logAlways("***stackCount=" + stackCount);
            return true;
        }
        final int resumedActivitiesCount = mAmState.getResumedActivitiesCount();
        if (!mAmState.getKeyguardControllerState().keyguardShowing && resumedActivitiesCount != 1) {
            logAlways("***resumedActivitiesCount=" + resumedActivitiesCount);
            return true;
        }
        if (mAmState.getFocusedActivity() == null) {
            logAlways("***focusedActivity=null");
            return true;
        }
        return false;
    }

    /**
     * @return true if should wait for some activities to become visible.
     */
    private boolean shouldWaitForActivities(WaitForValidActivityState... waitForActivitiesVisible) {
        if (waitForActivitiesVisible == null || waitForActivitiesVisible.length == 0) {
            return false;
        }
        // If the caller is interested in us waiting for some particular activity windows to be
        // visible before compute the state. Check for the visibility of those activity windows
        // and for placing them in correct stacks (if requested).
        boolean allActivityWindowsVisible = true;
        boolean tasksInCorrectStacks = true;
        for (final WaitForValidActivityState state : waitForActivitiesVisible) {
            final ComponentName activityName = state.activityName;
            final String windowName = state.windowName;
            final int stackId = state.stackId;
            final int windowingMode = state.windowingMode;
            final int activityType = state.activityType;

            final List<WindowState> matchingWindowStates =
                    mWmState.getMatchingVisibleWindowState(windowName);
            boolean activityWindowVisible = !matchingWindowStates.isEmpty();
            if (!activityWindowVisible) {
                logAlways("Activity window not visible: " + windowName);
                allActivityWindowsVisible = false;
            } else if (activityName != null
                    && !mAmState.isActivityVisible(activityName)) {
                logAlways("Activity not visible: " + getActivityName(activityName));
                allActivityWindowsVisible = false;
            } else {
                // Check if window is already the correct state requested by test.
                boolean windowInCorrectState = false;
                for (WindowState ws : matchingWindowStates) {
                    if (stackId != INVALID_STACK_ID && ws.getStackId() != stackId) {
                        continue;
                    }
                    if (windowingMode != WINDOWING_MODE_UNDEFINED
                            && ws.getWindowingMode() != windowingMode) {
                        continue;
                    }
                    if (activityType != ACTIVITY_TYPE_UNDEFINED
                            && ws.getActivityType() != activityType) {
                        continue;
                    }
                    windowInCorrectState = true;
                    break;
                }

                if (!windowInCorrectState) {
                    logAlways("Window in incorrect stack: " + state);
                    tasksInCorrectStacks = false;
                }
            }
        }
        return !allActivityWindowsVisible || !tasksInCorrectStacks;
    }

    /**
     * @return true if should wait valid windows state.
     */
    private boolean shouldWaitForWindows() {
        if (mWmState.getFrontWindow() == null) {
            logAlways("***frontWindow=null");
            return true;
        }
        if (mWmState.getFocusedWindow() == null) {
            logAlways("***focusedWindow=null");
            return true;
        }
        if (mWmState.getFocusedApp() == null) {
            logAlways("***focusedApp=null");
            return true;
        }

        return false;
    }

    private boolean shouldWaitForDebuggerWindow(ComponentName activityName) {
        List<WindowState> matchingWindowStates =
                mWmState.getMatchingVisibleWindowState(activityName.getPackageName());
        for (WindowState ws : matchingWindowStates) {
            if (ws.isDebuggerWindow()) {
                return false;
            }
        }
        logAlways("Debugger window not available yet");
        return true;
    }

    private boolean shouldWaitForActivityRecords(ComponentName... activityNames) {
        // Check if the activity records we're looking for is already added.
        for (final ComponentName activityName : activityNames) {
            if (!mAmState.isActivityVisible(activityName)) {
                logAlways("ActivityRecord " + getActivityName(activityName) + " not visible yet");
                return true;
            }
        }
        return false;
    }

    private boolean shouldWaitForSanityCheck(boolean compareTaskAndStackBounds) {
        try {
            assertSanity();
            assertValidBounds(compareTaskAndStackBounds);
        } catch (Throwable t) {
            logAlways("Waiting for sanity check: " + t.toString());
            return true;
        }
        return false;
    }

    ActivityManagerState getAmState() {
        return mAmState;
    }

    public WindowManagerState getWmState() {
        return mWmState;
    }

    void assertSanity() {
        assertThat("Must have stacks", mAmState.getStackCount(), greaterThan(0));
        if (!mAmState.getKeyguardControllerState().keyguardShowing) {
            assertEquals("There should be one and only one resumed activity in the system.",
                    1, mAmState.getResumedActivitiesCount());
        }
        assertNotNull("Must have focus activity.", mAmState.getFocusedActivity());

        for (ActivityStack aStack : mAmState.getStacks()) {
            final int stackId = aStack.mStackId;
            for (ActivityTask aTask : aStack.getTasks()) {
                assertEquals("Stack can only contain its own tasks", stackId, aTask.mStackId);
            }
        }

        assertNotNull("Must have front window.", mWmState.getFrontWindow());
        assertNotNull("Must have focused window.", mWmState.getFocusedWindow());
        assertNotNull("Must have app.", mWmState.getFocusedApp());
    }

    void assertContainsStack(String msg, int windowingMode, int activityType) {
        assertTrue(msg, mAmState.containsStack(windowingMode, activityType));
        assertTrue(msg, mWmState.containsStack(windowingMode, activityType));
    }

    void assertDoesNotContainStack(String msg, int windowingMode, int activityType) {
        assertFalse(msg, mAmState.containsStack(windowingMode, activityType));
        assertFalse(msg, mWmState.containsStack(windowingMode, activityType));
    }

    void assertFrontStack(String msg, int stackId) {
        assertEquals(msg, stackId, mAmState.getFrontStackId(DEFAULT_DISPLAY));
        assertEquals(msg, stackId, mWmState.getFrontStackId(DEFAULT_DISPLAY));
    }

    void assertFrontStack(String msg, int windowingMode, int activityType) {
        if (windowingMode != WINDOWING_MODE_UNDEFINED) {
            assertEquals(msg, windowingMode,
                    mAmState.getFrontStackWindowingMode(DEFAULT_DISPLAY));
        }
        if (activityType != ACTIVITY_TYPE_UNDEFINED) {
            assertEquals(msg, activityType, mAmState.getFrontStackActivityType(DEFAULT_DISPLAY));
        }
    }

    void assertFrontStackActivityType(String msg, int activityType) {
        assertEquals(msg, activityType, mAmState.getFrontStackActivityType(DEFAULT_DISPLAY));
        assertEquals(msg, activityType, mWmState.getFrontStackActivityType(DEFAULT_DISPLAY));
    }

    @Deprecated
    void assertFocusedStack(String msg, int stackId) {
        assertEquals(msg, stackId, mAmState.getFocusedStackId());
    }

    void assertFocusedStack(String msg, int windowingMode, int activityType) {
        if (windowingMode != WINDOWING_MODE_UNDEFINED) {
            assertEquals(msg, windowingMode, mAmState.getFocusedStackWindowingMode());
        }
        if (activityType != ACTIVITY_TYPE_UNDEFINED) {
            assertEquals(msg, activityType, mAmState.getFocusedStackActivityType());
        }
    }

    void assertFocusedActivity(final String msg, final ComponentName activityName) {
        final String activityComponentName = getActivityName(activityName);
        assertEquals(msg, activityComponentName, mAmState.getFocusedActivity());
        assertEquals(msg, activityComponentName, mWmState.getFocusedApp());
    }

    void assertNotFocusedActivity(String msg, ComponentName activityName) {
        assertNotEquals(msg, mAmState.getFocusedActivity(), getActivityName(activityName));
        assertNotEquals(msg, mWmState.getFocusedApp(), getActivityName(activityName));
    }

    public void assertResumedActivity(final String msg, final ComponentName activityName) {
        assertEquals(msg, getActivityName(activityName), mAmState.getResumedActivity());
    }

    void assertNotResumedActivity(String msg, ComponentName activityName) {
        assertNotEquals(msg, mAmState.getResumedActivity(), getActivityName(activityName));
    }

    void assertFocusedWindow(String msg, String windowName) {
        assertEquals(msg, windowName, mWmState.getFocusedWindow());
    }

    void assertNotFocusedWindow(String msg, String windowName) {
        assertNotEquals(msg, mWmState.getFocusedWindow(), windowName);
    }

    void assertFrontWindow(String msg, String windowName) {
        assertEquals(msg, windowName, mWmState.getFrontWindow());
    }

    public void assertVisibility(final ComponentName activityName, final boolean visible) {
        final String windowName = getWindowName(activityName);
        final boolean activityVisible = mAmState.isActivityVisible(activityName);
        final boolean windowVisible = mWmState.isWindowVisible(windowName);

        assertEquals("Activity=" + getActivityName(activityName) + " must" + (visible ? "" : " NOT")
                + " be visible.", visible, activityVisible);
        assertEquals("Window=" + windowName + " must" + (visible ? "" : " NOT") + " be visible.",
                visible, windowVisible);
    }

    void assertHomeActivityVisible(boolean visible) {
        final ComponentName homeActivity = mAmState.getHomeActivityName();
        assertNotNull(homeActivity);
        assertVisibility(homeActivity, visible);
    }

    /**
     * Asserts that the device default display minimim width is larger than the minimum task width.
     */
    void assertDeviceDefaultDisplaySize(String errorMessage) {
        computeState(true);
        final int minTaskSizePx = defaultMinimalTaskSize(DEFAULT_DISPLAY);
        final Display display = getWmState().getDisplay(DEFAULT_DISPLAY);
        final Rect displayRect = display.getDisplayRect();
        if (Math.min(displayRect.width(), displayRect.height()) < minTaskSizePx) {
            fail(errorMessage);
        }
    }

    public void assertKeyguardShowingAndOccluded() {
        assertTrue("Keyguard is showing",
                getAmState().getKeyguardControllerState().keyguardShowing);
        assertTrue("Keyguard is occluded",
                getAmState().getKeyguardControllerState().keyguardOccluded);
    }

    public void assertKeyguardShowingAndNotOccluded() {
        assertTrue("Keyguard is showing",
                getAmState().getKeyguardControllerState().keyguardShowing);
        assertFalse("Keyguard is not occluded",
                getAmState().getKeyguardControllerState().keyguardOccluded);
    }

    public void assertKeyguardGone() {
        assertFalse("Keyguard is not shown",
                getAmState().getKeyguardControllerState().keyguardShowing);
    }

    boolean taskListsInAmAndWmAreEqual() {
        for (ActivityStack aStack : mAmState.getStacks()) {
            final int stackId = aStack.mStackId;
            final WindowStack wStack = mWmState.getStack(stackId);
            if (wStack == null) {
                log("Waiting for stack setup in WM, stackId=" + stackId);
                return false;
            }

            for (ActivityTask aTask : aStack.getTasks()) {
                if (wStack.getTask(aTask.mTaskId) == null) {
                    log("Task is in AM but not in WM, waiting for it to settle, taskId="
                            + aTask.mTaskId);
                    return false;
                }
            }

            for (WindowTask wTask : wStack.mTasks) {
                if (aStack.getTask(wTask.mTaskId) == null) {
                    log("Task is in WM but not in AM, waiting for it to settle, taskId="
                            + wTask.mTaskId);
                    return false;
                }
            }
        }
        return true;
    }

    /** Get the stack position on its display. */
    int getStackIndexByActivityType(int activityType) {
        int wmStackIndex = mWmState.getStackIndexByActivityType(activityType);
        int amStackIndex = mAmState.getStackIndexByActivityType(activityType);
        assertEquals("Window and activity manager must have the same stack position index",
                amStackIndex, wmStackIndex);
        return wmStackIndex;
    }

    boolean stackBoundsInAMAndWMAreEqual() {
        for (ActivityStack aStack : mAmState.getStacks()) {
            final int stackId = aStack.mStackId;
            final WindowStack wStack = mWmState.getStack(stackId);
            if (aStack.isFullscreen() != wStack.isFullscreen()) {
                log("Waiting for correct fullscreen state, stackId=" + stackId);
                return false;
            }

            final Rect aStackBounds = aStack.getBounds();
            final Rect wStackBounds = wStack.getBounds();

            if (aStack.isFullscreen()) {
                if (aStackBounds != null) {
                    log("Waiting for correct stack state in AM, stackId=" + stackId);
                    return false;
                }
            } else if (!Objects.equals(aStackBounds, wStackBounds)) {
                // If stack is not fullscreen - comparing bounds. Not doing it always because
                // for fullscreen stack bounds in WM can be either null or equal to display size.
                log("Waiting for stack bound equality in AM and WM, stackId=" + stackId);
                return false;
            }
        }

        return true;
    }

    /**
     * Check task bounds when docked to top/left.
     */
    void assertDockedTaskBounds(int taskWidth, int taskHeight, ComponentName activityName) {
        // Task size can be affected by default minimal size.
        int defaultMinimalTaskSize = defaultMinimalTaskSize(
                mAmState.getStandardStackByWindowingMode(
                        WINDOWING_MODE_SPLIT_SCREEN_PRIMARY).mDisplayId);
        int targetWidth = Math.max(taskWidth, defaultMinimalTaskSize);
        int targetHeight = Math.max(taskHeight, defaultMinimalTaskSize);

        assertEquals(new Rect(0, 0, targetWidth, targetHeight),
                mAmState.getTaskByActivity(activityName).getBounds());
    }

    void assertValidBounds(boolean compareTaskAndStackBounds) {
        // Cycle through the stacks and tasks to figure out if the home stack is resizable
        final ActivityTask homeTask = mAmState.getHomeTask();
        final boolean homeStackIsResizable = homeTask != null
                && homeTask.getResizeMode() == RESIZE_MODE_RESIZEABLE;

        for (ActivityStack aStack : mAmState.getStacks()) {
            final int stackId = aStack.mStackId;
            final WindowStack wStack = mWmState.getStack(stackId);
            assertNotNull("stackId=" + stackId + " in AM but not in WM?", wStack);

            assertEquals("Stack fullscreen state in AM and WM must be equal stackId=" + stackId,
                    aStack.isFullscreen(), wStack.isFullscreen());

            final Rect aStackBounds = aStack.getBounds();
            final Rect wStackBounds = wStack.getBounds();

            if (aStack.isFullscreen()) {
                assertNull("Stack bounds in AM must be null stackId=" + stackId, aStackBounds);
            } else {
                assertEquals("Stack bounds in AM and WM must be equal stackId=" + stackId,
                        aStackBounds, wStackBounds);
            }

            for (ActivityTask aTask : aStack.getTasks()) {
                final int taskId = aTask.mTaskId;
                final WindowTask wTask = wStack.getTask(taskId);
                assertNotNull(
                        "taskId=" + taskId + " in AM but not in WM? stackId=" + stackId, wTask);

                final boolean aTaskIsFullscreen = aTask.isFullscreen();
                final boolean wTaskIsFullscreen = wTask.isFullscreen();
                assertEquals("Task fullscreen state in AM and WM must be equal taskId=" + taskId
                        + ", stackId=" + stackId, aTaskIsFullscreen, wTaskIsFullscreen);

                final Rect aTaskBounds = aTask.getBounds();
                final Rect wTaskBounds = wTask.getBounds();

                if (aTaskIsFullscreen) {
                    assertNull("Task bounds in AM must be null for fullscreen taskId=" + taskId,
                            aTaskBounds);
                } else if (!homeStackIsResizable && mWmState.isDockedStackMinimized()
                        && !isScreenPortrait(aStack.mDisplayId)) {
                    // When minimized using non-resizable launcher in landscape mode, it will move
                    // the task offscreen in the negative x direction unlike portrait that crops.
                    // The x value in the task bounds will not match the stack bounds since the
                    // only the task was moved.
                    assertEquals("Task bounds in AM and WM must match width taskId=" + taskId
                                    + ", stackId" + stackId, aTaskBounds.width(),
                            wTaskBounds.width());
                    assertEquals("Task bounds in AM and WM must match height taskId=" + taskId
                                    + ", stackId" + stackId, aTaskBounds.height(),
                            wTaskBounds.height());
                    assertEquals("Task bounds must match stack bounds y taskId=" + taskId
                                    + ", stackId" + stackId, aTaskBounds.top,
                            wTaskBounds.top);
                    assertEquals("Task and stack bounds must match width taskId=" + taskId
                                    + ", stackId" + stackId, aStackBounds.width(),
                            wTaskBounds.width());
                    assertEquals("Task and stack bounds must match height taskId=" + taskId
                                    + ", stackId" + stackId, aStackBounds.height(),
                            wTaskBounds.height());
                    assertEquals("Task and stack bounds must match y taskId=" + taskId
                                    + ", stackId" + stackId, aStackBounds.top,
                            wTaskBounds.top);
                } else {
                    assertEquals("Task bounds in AM and WM must be equal taskId=" + taskId
                            + ", stackId=" + stackId, aTaskBounds, wTaskBounds);

                    if (compareTaskAndStackBounds
                            && aStack.getWindowingMode() != WINDOWING_MODE_FREEFORM) {
                        int aTaskMinWidth = aTask.getMinWidth();
                        int aTaskMinHeight = aTask.getMinHeight();

                        if (aTaskMinWidth == -1 || aTaskMinHeight == -1) {
                            // Minimal dimension(s) not set for task - it should be using defaults.
                            int defaultMinimalSize =
                                    aStack.getWindowingMode() == WINDOWING_MODE_PINNED
                                    ? defaultMinimalPinnedTaskSize(aStack.mDisplayId)
                                    : defaultMinimalTaskSize(aStack.mDisplayId);

                            if (aTaskMinWidth == -1) {
                                aTaskMinWidth = defaultMinimalSize;
                            }
                            if (aTaskMinHeight == -1) {
                                aTaskMinHeight = defaultMinimalSize;
                            }
                        }

                        if (aStackBounds.width() >= aTaskMinWidth
                                && aStackBounds.height() >= aTaskMinHeight
                                || aStack.getWindowingMode() == WINDOWING_MODE_PINNED) {
                            // Bounds are not smaller then minimal possible, so stack and task
                            // bounds must be equal.
                            assertEquals("Task bounds must be equal to stack bounds taskId="
                                    + taskId + ", stackId=" + stackId, aStackBounds, wTaskBounds);
                        } else if (aStack.getWindowingMode() == WINDOWING_MODE_SPLIT_SCREEN_PRIMARY
                                && homeStackIsResizable && mWmState.isDockedStackMinimized()) {
                            // Portrait if the display height is larger than the width
                            if (isScreenPortrait(aStack.mDisplayId)) {
                                assertEquals("Task width must be equal to stack width taskId="
                                                + taskId + ", stackId=" + stackId,
                                        aStackBounds.width(), wTaskBounds.width());
                                assertThat("Task height must be greater than stack height "
                                                + "taskId=" + taskId + ", stackId=" + stackId,
                                        aStackBounds.height(), lessThan(wTaskBounds.height()));
                                assertEquals("Task and stack x position must be equal taskId="
                                                + taskId + ", stackId=" + stackId,
                                        wTaskBounds.left, wStackBounds.left);
                            } else {
                                assertThat("Task width must be greater than stack width taskId="
                                                + taskId + ", stackId=" + stackId,
                                        aStackBounds.width(), lessThan(wTaskBounds.width()));
                                assertEquals("Task height must be equal to stack height taskId="
                                                + taskId + ", stackId=" + stackId,
                                        aStackBounds.height(), wTaskBounds.height());
                                assertEquals("Task and stack y position must be equal taskId="
                                                + taskId + ", stackId=" + stackId, wTaskBounds.top,
                                        wStackBounds.top);
                            }
                        } else {
                            // Minimal dimensions affect task size, so bounds of task and stack must
                            // be different - will compare dimensions instead.
                            int targetWidth = Math.max(aTaskMinWidth, aStackBounds.width());
                            assertEquals("Task width must be set according to minimal width"
                                            + " taskId=" + taskId + ", stackId=" + stackId,
                                    targetWidth, wTaskBounds.width());
                            int targetHeight = Math.max(aTaskMinHeight, aStackBounds.height());
                            assertEquals("Task height must be set according to minimal height"
                                            + " taskId=" + taskId + ", stackId=" + stackId,
                                    targetHeight, wTaskBounds.height());
                        }
                    }
                }
            }
        }
    }

    public void assertActivityDisplayed(final ComponentName activityName) throws Exception {
        assertWindowDisplayed(getWindowName(activityName));
    }

    public void assertWindowDisplayed(final String windowName) throws Exception {
        waitForValidState(WaitForValidActivityState.forWindow(windowName));
        assertTrue(windowName + "is visible", getWmState().isWindowVisible(windowName));
    }

    boolean isScreenPortrait() {
        final int displayId = mAmState.getStandardStackByWindowingMode(
            WINDOWING_MODE_SPLIT_SCREEN_PRIMARY).mDisplayId;
        return isScreenPortrait(displayId);
    }

    boolean isScreenPortrait(int displayId) {
        final Rect displayRect = mWmState.getDisplay(displayId).getDisplayRect();
        return displayRect.height() > displayRect.width();
    }

    static int dpToPx(float dp, int densityDpi) {
        return (int) (dp * densityDpi / DENSITY_DEFAULT + 0.5f);
    }

    private int defaultMinimalTaskSize(int displayId) {
        return dpToPx(DEFAULT_RESIZABLE_TASK_SIZE_DP, mWmState.getDisplay(displayId).getDpi());
    }

    private int defaultMinimalPinnedTaskSize(int displayId) {
        return dpToPx(DEFAULT_PIP_RESIZABLE_TASK_SIZE_DP, mWmState.getDisplay(displayId).getDpi());
    }
}
