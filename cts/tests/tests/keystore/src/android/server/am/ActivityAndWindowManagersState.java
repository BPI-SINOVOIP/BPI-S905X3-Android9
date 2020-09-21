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

import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.lessThan;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThat;

import android.content.ComponentName;
import android.graphics.Rect;
import android.os.SystemClock;
import android.server.am.ActivityManagerState.ActivityStack;
import android.server.am.ActivityManagerState.ActivityTask;
import android.server.am.WindowManagerState.WindowStack;
import android.server.am.WindowManagerState.WindowState;
import android.server.am.WindowManagerState.WindowTask;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.function.BiPredicate;
import java.util.function.BooleanSupplier;
import java.util.function.Predicate;

/**
 * Combined state of the activity manager and window manager.
 */
public class ActivityAndWindowManagersState {

    // Clone of android DisplayMetrics.DENSITY_DEFAULT (DENSITY_MEDIUM)
    // (Needed in host-side tests to convert dp to px.)
    private static final int DISPLAY_DENSITY_DEFAULT = 160;

    // Default minimal size of resizable task, used if none is set explicitly.
    // Must be kept in sync with 'default_minimal_size_resizable_task' dimen from frameworks/base.
    private static final int DEFAULT_RESIZABLE_TASK_SIZE_DP = 220;

    // Default minimal size of a resizable PiP task, used if none is set explicitly.
    // Must be kept in sync with 'default_minimal_size_pip_resizable_task' dimen from
    // frameworks/base.
    private static final int DEFAULT_PIP_RESIZABLE_TASK_SIZE_DP = 108;

    private ActivityManagerState mAmState = new ActivityManagerState();
    private WindowManagerState mWmState = new WindowManagerState();

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

    boolean waitForHomeActivityVisible() {
        ComponentName homeActivity = mAmState.getHomeActivityName();
        // Sometimes this function is called before we know what Home Activity is
        if (homeActivity == null) {
            log("Computing state to determine Home Activity");
            computeState(true);
            homeActivity = mAmState.getHomeActivityName();
        }
        assertNotNull("homeActivity should not be null", homeActivity);
        waitForValidState(new WaitForValidActivityState(homeActivity));
        return mAmState.isHomeActivityVisible();
    }

    public void waitForKeyguardShowingAndNotOccluded() {
        waitForWithAmState(state -> state.getKeyguardControllerState().keyguardShowing
                        && !state.getKeyguardControllerState().keyguardOccluded,
                "***Waiting for Keyguard showing...");
    }

    @Deprecated
    void waitForFocusedStack(int stackId) {
        waitForWithAmState(state -> state.getFocusedStackId() == stackId,
                "***Waiting for focused stack...");
    }

    void waitForWithAmState(Predicate<ActivityManagerState> waitCondition, String message) {
        waitFor((amState, wmState) -> waitCondition.test(amState), message);
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
        List<WindowState> matchingWindowStates = new ArrayList<>();
        for (final WaitForValidActivityState state : waitForActivitiesVisible) {
            final ComponentName activityName = state.activityName;
            final String windowName = state.windowName;
            final int stackId = state.stackId;
            final int windowingMode = state.windowingMode;
            final int activityType = state.activityType;

            mWmState.getMatchingVisibleWindowState(windowName, matchingWindowStates);
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

    @Deprecated
    void assertFocusedStack(String msg, int stackId) {
        assertEquals(msg, stackId, mAmState.getFocusedStackId());
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
                            int targetWidth = (int) Math.max(aTaskMinWidth,
                                    aStackBounds.width());
                            assertEquals("Task width must be set according to minimal width"
                                            + " taskId=" + taskId + ", stackId=" + stackId,
                                    targetWidth, (int) wTaskBounds.width());
                            int targetHeight = (int) Math.max(aTaskMinHeight,
                                    aStackBounds.height());
                            assertEquals("Task height must be set according to minimal height"
                                            + " taskId=" + taskId + ", stackId=" + stackId,
                                    targetHeight, (int) wTaskBounds.height());
                        }
                    }
                }
            }
        }
    }

    boolean isScreenPortrait(int displayId) {
        final Rect displayRect = mWmState.getDisplay(displayId).getDisplayRect();
        return displayRect.height() > displayRect.width();
    }

    static int dpToPx(float dp, int densityDpi) {
        return (int) (dp * densityDpi / DISPLAY_DENSITY_DEFAULT + 0.5f);
    }

    private int defaultMinimalTaskSize(int displayId) {
        return dpToPx(DEFAULT_RESIZABLE_TASK_SIZE_DP, mWmState.getDisplay(displayId).getDpi());
    }

    private int defaultMinimalPinnedTaskSize(int displayId) {
        return dpToPx(DEFAULT_PIP_RESIZABLE_TASK_SIZE_DP, mWmState.getDisplay(displayId).getDpi());
    }
}
