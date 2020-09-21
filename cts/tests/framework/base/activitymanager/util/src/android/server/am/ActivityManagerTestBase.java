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

import static android.app.ActivityManager.SPLIT_SCREEN_CREATE_MODE_TOP_OR_LEFT;
import static android.app.ActivityManager.StackId.INVALID_STACK_ID;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_ASSISTANT;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_RECENTS;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_UNDEFINED;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.content.Intent.FLAG_ACTIVITY_MULTIPLE_TASK;
import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;
import static android.content.pm.PackageManager.FEATURE_EMBEDDED;
import static android.content.pm.PackageManager.FEATURE_FREEFORM_WINDOW_MANAGEMENT;
import static android.content.pm.PackageManager.FEATURE_LEANBACK;
import static android.content.pm.PackageManager.FEATURE_PICTURE_IN_PICTURE;
import static android.content.pm.PackageManager.FEATURE_SCREEN_LANDSCAPE;
import static android.content.pm.PackageManager.FEATURE_SCREEN_PORTRAIT;
import static android.content.pm.PackageManager.FEATURE_VR_MODE_HIGH_PERFORMANCE;
import static android.content.pm.PackageManager.FEATURE_WATCH;
import static android.server.am.ActivityLauncher.KEY_DISPLAY_ID;
import static android.server.am.ActivityLauncher.KEY_LAUNCH_ACTIVITY;
import static android.server.am.ActivityLauncher.KEY_LAUNCH_TO_SIDE;
import static android.server.am.ActivityLauncher.KEY_MULTIPLE_TASK;
import static android.server.am.ActivityLauncher.KEY_NEW_TASK;
import static android.server.am.ActivityLauncher.KEY_RANDOM_DATA;
import static android.server.am.ActivityLauncher.KEY_REORDER_TO_FRONT;
import static android.server.am.ActivityLauncher.KEY_SUPPRESS_EXCEPTIONS;
import static android.server.am.ActivityLauncher.KEY_TARGET_COMPONENT;
import static android.server.am.ActivityLauncher.KEY_USE_APPLICATION_CONTEXT;
import static android.server.am.ActivityLauncher.KEY_USE_INSTRUMENTATION;
import static android.server.am.ActivityLauncher.launchActivityFromExtras;
import static android.server.am.ActivityManagerState.STATE_RESUMED;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.ComponentNameUtils.getLogTag;
import static android.server.am.Components.LAUNCHING_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.StateLogger.log;
import static android.server.am.StateLogger.logAlways;
import static android.server.am.StateLogger.logE;
import static android.server.am.UiDeviceUtils.pressAppSwitchButton;
import static android.server.am.UiDeviceUtils.pressBackButton;
import static android.server.am.UiDeviceUtils.pressEnterButton;
import static android.server.am.UiDeviceUtils.pressHomeButton;
import static android.server.am.UiDeviceUtils.pressSleepButton;
import static android.server.am.UiDeviceUtils.pressUnlockButton;
import static android.server.am.UiDeviceUtils.pressWakeupButton;
import static android.server.am.UiDeviceUtils.waitForDeviceIdle;
import static android.view.Display.DEFAULT_DISPLAY;
import static android.view.Display.INVALID_DISPLAY;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import static java.lang.Integer.toHexString;

import android.accessibilityservice.AccessibilityService;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.graphics.Bitmap;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemClock;
import android.provider.Settings;
import android.server.am.settings.SettingsSession;
import android.support.test.InstrumentationRegistry;

import android.support.test.rule.ActivityTestRule;

import com.android.compatibility.common.util.SystemUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public abstract class ActivityManagerTestBase {
    private static final boolean PRETEND_DEVICE_SUPPORTS_PIP = false;
    private static final boolean PRETEND_DEVICE_SUPPORTS_FREEFORM = false;
    private static final String LOG_SEPARATOR = "LOG_SEPARATOR";

    protected static final int[] ALL_ACTIVITY_TYPE_BUT_HOME = {
            ACTIVITY_TYPE_STANDARD, ACTIVITY_TYPE_ASSISTANT, ACTIVITY_TYPE_RECENTS,
            ACTIVITY_TYPE_UNDEFINED
    };

    private static final String TASK_ID_PREFIX = "taskId";

    private static final String AM_STACK_LIST = "am stack list";

    private static final String AM_FORCE_STOP_TEST_PACKAGE = "am force-stop android.server.am";
    private static final String AM_FORCE_STOP_SECOND_TEST_PACKAGE
            = "am force-stop android.server.am.second";
    private static final String AM_FORCE_STOP_THIRD_TEST_PACKAGE
            = "am force-stop android.server.am.third";

    protected static final String AM_START_HOME_ACTIVITY_COMMAND =
            "am start -a android.intent.action.MAIN -c android.intent.category.HOME";

    private static final String AM_MOVE_TOP_ACTIVITY_TO_PINNED_STACK_COMMAND_FORMAT =
            "am stack move-top-activity-to-pinned-stack %1d 0 0 500 500";

    private static final String AM_RESIZE_DOCKED_STACK = "am stack resize-docked-stack ";
    private static final String AM_RESIZE_STACK = "am stack resize ";

    static final String AM_MOVE_TASK = "am stack move-task ";

    private static final String AM_NO_HOME_SCREEN = "am no-home-screen";

    private static final String LOCK_CREDENTIAL = "1234";

    private static final int UI_MODE_TYPE_MASK = 0x0f;
    private static final int UI_MODE_TYPE_VR_HEADSET = 0x07;

    private static Boolean sHasHomeScreen = null;

    protected static final int INVALID_DEVICE_ROTATION = -1;

    protected Context mContext;
    protected ActivityManager mAm;

    @Rule
    public final ActivityTestRule<SideActivity> mSideActivityRule =
            new ActivityTestRule<>(SideActivity.class, true /* initialTouchMode */,
                    false /* launchActivity */);

    /**
     * @return the am command to start the given activity with the following extra key/value pairs.
     *         {@param keyValuePairs} must be a list of arguments defining each key/value extra.
     */
    // TODO: Make this more generic, for instance accepting flags or extras of other types.
    protected static String getAmStartCmd(final ComponentName activityName,
            final String... keyValuePairs) {
        return getAmStartCmdInternal(getActivityName(activityName), keyValuePairs);
    }

    private static String getAmStartCmdInternal(final String activityName,
            final String... keyValuePairs) {
        return appendKeyValuePairs(
                new StringBuilder("am start -n ").append(activityName),
                keyValuePairs);
    }

    private static String appendKeyValuePairs(
            final StringBuilder cmd, final String... keyValuePairs) {
        if (keyValuePairs.length % 2 != 0) {
            throw new RuntimeException("keyValuePairs must be pairs of key/value arguments");
        }
        for (int i = 0; i < keyValuePairs.length; i += 2) {
            final String key = keyValuePairs[i];
            final String value = keyValuePairs[i + 1];
            cmd.append(" --es ")
                    .append(key)
                    .append(" ")
                    .append(value);
        }
        return cmd.toString();
    }

    protected static String getAmStartCmd(final ComponentName activityName, final int displayId,
            final String... keyValuePair) {
        return getAmStartCmdInternal(getActivityName(activityName), displayId, keyValuePair);
    }

    private static String getAmStartCmdInternal(final String activityName, final int displayId,
            final String... keyValuePairs) {
        return appendKeyValuePairs(
                new StringBuilder("am start -n ")
                        .append(activityName)
                        .append(" -f 0x")
                        .append(toHexString(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_MULTIPLE_TASK))
                        .append(" --display ")
                        .append(displayId),
                keyValuePairs);
    }

    protected static String getAmStartCmdInNewTask(final ComponentName activityName) {
        return "am start -n " + getActivityName(activityName) + " -f 0x18000000";
    }

    protected static String getAmStartCmdOverHome(final ComponentName activityName) {
        return "am start --activity-task-on-home -n " + getActivityName(activityName);
    }

    protected static String getMoveToPinnedStackCommand(int stackId) {
        return String.format(AM_MOVE_TOP_ACTIVITY_TO_PINNED_STACK_COMMAND_FORMAT, stackId);
    }

    protected ActivityAndWindowManagersState mAmWmState = new ActivityAndWindowManagersState();

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getContext();
        mAm = mContext.getSystemService(ActivityManager.class);

        InstrumentationRegistry.getInstrumentation().getUiAutomation().grantRuntimePermission(
                mContext.getPackageName(), android.Manifest.permission.MANAGE_ACTIVITY_STACKS);
        InstrumentationRegistry.getInstrumentation().getUiAutomation().grantRuntimePermission(
                mContext.getPackageName(), android.Manifest.permission.ACTIVITY_EMBEDDING);

        pressWakeupButton();
        pressUnlockButton();
        pressHomeButton();
        removeStacksWithActivityTypes(ALL_ACTIVITY_TYPE_BUT_HOME);
    }

    @After
    public void tearDown() throws Exception {
        // Synchronous execution of removeStacksWithActivityTypes() ensures that all activities but
        // home are cleaned up from the stack at the end of each test. Am force stop shell commands
        // might be asynchronous and could interrupt the stack cleanup process if executed first.
        removeStacksWithActivityTypes(ALL_ACTIVITY_TYPE_BUT_HOME);
        executeShellCommand(AM_FORCE_STOP_TEST_PACKAGE);
        executeShellCommand(AM_FORCE_STOP_SECOND_TEST_PACKAGE);
        executeShellCommand(AM_FORCE_STOP_THIRD_TEST_PACKAGE);
        pressHomeButton();
    }

    protected void removeStacksWithActivityTypes(int... activityTypes) {
        mAm.removeStacksWithActivityTypes(activityTypes);
        waitForIdle();
    }

    protected void removeStacksInWindowingModes(int... windowingModes) {
        mAm.removeStacksInWindowingModes(windowingModes);
        waitForIdle();
    }

    public static String executeShellCommand(String command) {
        log("Shell command: " + command);
        try {
            return SystemUtil
                    .runShellCommand(InstrumentationRegistry.getInstrumentation(), command);
        } catch (IOException e) {
            //bubble it up
            logE("Error running shell command: " + command);
            throw new RuntimeException(e);
        }
    }

    protected Bitmap takeScreenshot() {
        return InstrumentationRegistry.getInstrumentation().getUiAutomation().takeScreenshot();
    }

    protected void launchActivity(final ComponentName activityName, final String... keyValuePairs) {
        launchActivityNoWait(activityName, keyValuePairs);
        mAmWmState.waitForValidState(activityName);
    }

    protected void launchActivityNoWait(final ComponentName activityName,
            final String... keyValuePairs) {
        executeShellCommand(getAmStartCmd(activityName, keyValuePairs));
    }

    protected void launchActivityInNewTask(final ComponentName activityName) {
        executeShellCommand(getAmStartCmdInNewTask(activityName));
        mAmWmState.waitForValidState(activityName);
    }

    /**
     * Starts an activity in a new stack.
     * @return the stack id of the newly created stack.
     */
    @Deprecated
    protected int launchActivityInNewDynamicStack(ComponentName activityName) {
        HashSet<Integer> stackIds = getStackIds();
        executeShellCommand("am stack start " + DEFAULT_DISPLAY + " "
                + getActivityName(activityName));
        HashSet<Integer> newStackIds = getStackIds();
        newStackIds.removeAll(stackIds);
        if (newStackIds.isEmpty()) {
            return INVALID_STACK_ID;
        } else {
            assertTrue(newStackIds.size() == 1);
            return newStackIds.iterator().next();
        }
    }

    private static void waitForIdle() {
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }

    /** Returns the set of stack ids. */
    private HashSet<Integer> getStackIds() {
        mAmWmState.computeState(true);
        final List<ActivityManagerState.ActivityStack> stacks = mAmWmState.getAmState().getStacks();
        final HashSet<Integer> stackIds = new HashSet<>();
        for (ActivityManagerState.ActivityStack s : stacks) {
            stackIds.add(s.mStackId);
        }
        return stackIds;
    }

    protected void launchHomeActivity() {
        executeShellCommand(AM_START_HOME_ACTIVITY_COMMAND);
        mAmWmState.waitForHomeActivityVisible();
    }

    protected void launchActivity(ComponentName activityName, int windowingMode,
            final String... keyValuePairs) {
        executeShellCommand(getAmStartCmd(activityName, keyValuePairs)
                + " --windowingMode " + windowingMode);
        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setWindowingMode(windowingMode)
                .build());
    }

    protected void launchActivityOnDisplay(ComponentName activityName, int displayId,
            String... keyValuePairs) {
        launchActivityOnDisplayNoWait(activityName, displayId, keyValuePairs);
        mAmWmState.waitForValidState(activityName);
    }

    protected void launchActivityOnDisplayNoWait(ComponentName activityName, int displayId,
            String... keyValuePairs) {
        executeShellCommand(getAmStartCmd(activityName, displayId, keyValuePairs));
    }

    /**
     * Launches {@param  activityName} into split-screen primary windowing mode and also makes
     * the recents activity visible to the side of it.
     * NOTE: Recents view may be combined with home screen on some devices, so using this to wait
     * for Recents only makes sense when {@link ActivityManagerState#isHomeRecentsComponent()} is
     * {@code false}.
     */
    protected void launchActivityInSplitScreenWithRecents(ComponentName activityName) {
        launchActivityInSplitScreenWithRecents(activityName, SPLIT_SCREEN_CREATE_MODE_TOP_OR_LEFT);
    }

    protected void launchActivityInSplitScreenWithRecents(ComponentName activityName,
            int createMode) {
        launchActivity(activityName);
        final int taskId = mAmWmState.getAmState().getTaskByActivity(activityName).mTaskId;
        mAm.setTaskWindowingModeSplitScreenPrimary(taskId, createMode, true /* onTop */,
                false /* animate */, null /* initialBounds */, true /* showRecents */);

        mAmWmState.waitForValidState(
                new WaitForValidActivityState.Builder(activityName)
                        .setWindowingMode(WINDOWING_MODE_SPLIT_SCREEN_PRIMARY)
                        .setActivityType(ACTIVITY_TYPE_STANDARD)
                        .build());
        mAmWmState.waitForRecentsActivityVisible();
    }

    public void moveTaskToPrimarySplitScreen(int taskId) {
        moveTaskToPrimarySplitScreen(taskId, false /* launchSideActivityIfNeeded */);
    }

    /**
     * Moves the device into split-screen with the specified task into the primary stack.
     * @param taskId                        The id of the task to move into the primary stack.
     * @param launchSideActivityIfNeeded    Whether a placeholder activity should be launched if no
     *                                      recents activity is available.
     */
    public void moveTaskToPrimarySplitScreen(int taskId, boolean launchSideActivityIfNeeded) {
        mAm.setTaskWindowingModeSplitScreenPrimary(taskId, SPLIT_SCREEN_CREATE_MODE_TOP_OR_LEFT,
                true /* onTop */, false /* animate */, null /* initialBounds */,
                true /* showRecents */);
        mAmWmState.waitForRecentsActivityVisible();

        if (mAmWmState.getAmState().isHomeRecentsComponent() && launchSideActivityIfNeeded) {
            // Launch Placeholder Recents
            final Activity recentsActivity = mSideActivityRule.launchActivity(new Intent());
            mAmWmState.waitForActivityState(recentsActivity.getComponentName(), STATE_RESUMED);
        }
    }

    /**
     * Launches {@param primaryActivity} into split-screen primary windowing mode
     * and {@param secondaryActivity} to the side in split-screen secondary windowing mode.
     */
    protected void launchActivitiesInSplitScreen(LaunchActivityBuilder primaryActivity,
            LaunchActivityBuilder secondaryActivity) {
        // Launch split-screen primary.
        primaryActivity
                .setUseInstrumentation()
                .setWaitForLaunched(true)
                .execute();

        final int taskId = mAmWmState.getAmState().getTaskByActivity(
                primaryActivity.mTargetActivity).mTaskId;
        moveTaskToPrimarySplitScreen(taskId);

        // Launch split-screen secondary
        // Recents become focused, so we can just launch new task in focused stack
        secondaryActivity
                .setUseInstrumentation()
                .setWaitForLaunched(true)
                .setNewTask(true)
                .setMultipleTask(true)
                .execute();
    }

    protected void setActivityTaskWindowingMode(ComponentName activityName, int windowingMode) {
        mAmWmState.computeState(activityName);
        final int taskId = mAmWmState.getAmState().getTaskByActivity(activityName).mTaskId;
        mAm.setTaskWindowingMode(taskId, windowingMode, true /* toTop */);
        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .setWindowingMode(windowingMode)
                .build());
    }

    protected void moveActivityToStack(ComponentName activityName, int stackId) {
        mAmWmState.computeState(activityName);
        final int taskId = mAmWmState.getAmState().getTaskByActivity(activityName).mTaskId;
        final String cmd = AM_MOVE_TASK + taskId + " " + stackId + " true";
        executeShellCommand(cmd);

        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setStackId(stackId)
                .build());
    }

    protected void resizeActivityTask(
            ComponentName activityName, int left, int top, int right, int bottom) {
        mAmWmState.computeState(activityName);
        final int taskId = mAmWmState.getAmState().getTaskByActivity(activityName).mTaskId;
        final String cmd = "am task resize "
                + taskId + " " + left + " " + top + " " + right + " " + bottom;
        executeShellCommand(cmd);
    }

    protected void resizeDockedStack(
            int stackWidth, int stackHeight, int taskWidth, int taskHeight) {
        executeShellCommand(AM_RESIZE_DOCKED_STACK
                + "0 0 " + stackWidth + " " + stackHeight
                + " 0 0 " + taskWidth + " " + taskHeight);
    }

    protected void resizeStack(int stackId, int stackLeft, int stackTop, int stackWidth,
            int stackHeight) {
        executeShellCommand(AM_RESIZE_STACK + String.format("%d %d %d %d %d", stackId, stackLeft,
                stackTop, stackWidth, stackHeight));
    }

    protected void pressAppSwitchButtonAndWaitForRecents() {
        pressAppSwitchButton();
        mAmWmState.waitForRecentsActivityVisible();
        mAmWmState.waitForAppTransitionIdle();
    }

    // Utility method for debugging, not used directly here, but useful, so kept around.
    protected void printStacksAndTasks() {
        String output = executeShellCommand(AM_STACK_LIST);
        for (String line : output.split("\\n")) {
            log(line);
        }
    }

    protected boolean supportsVrMode() {
        return hasDeviceFeature(FEATURE_VR_MODE_HIGH_PERFORMANCE);
    }

    protected boolean supportsPip() {
        return hasDeviceFeature(FEATURE_PICTURE_IN_PICTURE)
                || PRETEND_DEVICE_SUPPORTS_PIP;
    }

    protected boolean supportsFreeform() {
        return hasDeviceFeature(FEATURE_FREEFORM_WINDOW_MANAGEMENT)
                || PRETEND_DEVICE_SUPPORTS_FREEFORM;
    }

    /** Whether or not the device pin/pattern/password lock. */
    protected boolean supportsSecureLock() {
        return !hasDeviceFeature(FEATURE_LEANBACK)
                && !hasDeviceFeature(FEATURE_EMBEDDED);
    }

    /** Whether or not the device supports "swipe" lock. */
    protected boolean supportsInsecureLock() {
        return !hasDeviceFeature(FEATURE_LEANBACK)
                && !hasDeviceFeature(FEATURE_WATCH)
                && !hasDeviceFeature(FEATURE_EMBEDDED);
    }

    protected boolean isWatch() {
        return hasDeviceFeature(FEATURE_WATCH);
    }

    protected boolean isTablet() {
        // Larger than approx 7" tablets
        return mContext.getResources().getConfiguration().smallestScreenWidthDp >= 600;
    }

    // TODO: Switch to using a feature flag, when available.
    protected static boolean isUiModeLockedToVrHeadset() {
        final String output = runCommandAndPrintOutput("dumpsys uimode");

        Integer curUiMode = null;
        Boolean uiModeLocked = null;
        for (String line : output.split("\\n")) {
            line = line.trim();
            Matcher matcher = sCurrentUiModePattern.matcher(line);
            if (matcher.find()) {
                curUiMode = Integer.parseInt(matcher.group(1), 16);
            }
            matcher = sUiModeLockedPattern.matcher(line);
            if (matcher.find()) {
                uiModeLocked = matcher.group(1).equals("true");
            }
        }

        boolean uiModeLockedToVrHeadset = (curUiMode != null) && (uiModeLocked != null)
                && ((curUiMode & UI_MODE_TYPE_MASK) == UI_MODE_TYPE_VR_HEADSET) && uiModeLocked;

        if (uiModeLockedToVrHeadset) {
            log("UI mode is locked to VR headset");
        }

        return uiModeLockedToVrHeadset;
    }

    protected boolean supportsSplitScreenMultiWindow() {
        return ActivityManager.supportsSplitScreenMultiWindow(mContext);
    }

    protected boolean hasHomeScreen() {
        if (sHasHomeScreen == null) {
            sHasHomeScreen = !executeShellCommand(AM_NO_HOME_SCREEN).startsWith("true");
        }
        return sHasHomeScreen;
    }

    /**
     * Rotation support is indicated by explicitly having both landscape and portrait
     * features or not listing either at all.
     */
    protected boolean supportsRotation() {
        final boolean supportsLandscape = hasDeviceFeature(FEATURE_SCREEN_LANDSCAPE);
        final boolean supportsPortrait = hasDeviceFeature(FEATURE_SCREEN_PORTRAIT);
        return (supportsLandscape && supportsPortrait)
                || (!supportsLandscape && !supportsPortrait);
    }

    protected boolean hasDeviceFeature(final String requiredFeature) {
        return InstrumentationRegistry.getContext()
                .getPackageManager()
                .hasSystemFeature(requiredFeature);
    }

    protected boolean isDisplayOn() {
        final String output = executeShellCommand("dumpsys power");
        final Matcher matcher = sDisplayStatePattern.matcher(output);
        if (matcher.find()) {
            final String state = matcher.group(1);
            log("power state=" + state);
            return "ON".equals(state);
        }
        logAlways("power state :(");
        return false;
    }

    /**
     * Test @Rule class that disables screen doze settings before each test method running and
     * restoring to initial values after test method finished.
     */
    protected static class DisableScreenDozeRule implements TestRule {

        /** Copied from android.provider.Settings.Secure since these keys are hiden. */
        private static final String[] DOZE_SETTINGS = {
                "doze_enabled",
                "doze_always_on",
                "doze_pulse_on_pick_up",
                "doze_pulse_on_long_press",
                "doze_pulse_on_double_tap"
        };

        private String get(String key) {
            return executeShellCommand("settings get secure " + key).trim();
        }

        private void put(String key, String value) {
            executeShellCommand("settings put secure " + key + " " + value);
        }

        @Override
        public Statement apply(final Statement base, final Description description) {
            return new Statement() {
                @Override
                public void evaluate() throws Throwable {
                    final Map<String, String> initialValues = new HashMap<>();
                    Arrays.stream(DOZE_SETTINGS).forEach(k -> initialValues.put(k, get(k)));
                    try {
                        Arrays.stream(DOZE_SETTINGS).forEach(k -> put(k, "0"));
                        base.evaluate();
                    } finally {
                        Arrays.stream(DOZE_SETTINGS).forEach(k -> put(k, initialValues.get(k)));
                    }
                }
            };
        }
    }

    protected class LockScreenSession implements AutoCloseable {
        private static final boolean DEBUG = false;

        private final boolean mIsLockDisabled;
        private boolean mLockCredentialSet;

        public LockScreenSession() {
            mIsLockDisabled = isLockDisabled();
            mLockCredentialSet = false;
            // Enable lock screen (swipe) by default.
            setLockDisabled(false);
        }

        public LockScreenSession setLockCredential() {
            mLockCredentialSet = true;
            runCommandAndPrintOutput("locksettings set-pin " + LOCK_CREDENTIAL);
            return this;
        }

        public LockScreenSession enterAndConfirmLockCredential() {
            waitForDeviceIdle(3000);

            runCommandAndPrintOutput("input text " + LOCK_CREDENTIAL);
            pressEnterButton();
            return this;
        }

        private void removeLockCredential() {
            runCommandAndPrintOutput("locksettings clear --old " + LOCK_CREDENTIAL);
            mLockCredentialSet = false;
        }

        LockScreenSession disableLockScreen() {
            setLockDisabled(true);
            return this;
        }

        LockScreenSession sleepDevice() {
            pressSleepButton();
            // Not all device variants lock when we go to sleep, so we need to explicitly lock the
            // device. Note that pressSleepButton() above is redundant because the action also
            // puts the device to sleep, but kept around for clarity.
            InstrumentationRegistry.getInstrumentation().getUiAutomation().performGlobalAction(
                    AccessibilityService.GLOBAL_ACTION_LOCK_SCREEN);
            for (int retry = 1; isDisplayOn() && retry <= 5; retry++) {
                logAlways("***Waiting for display to turn off... retry=" + retry);
                SystemClock.sleep(TimeUnit.SECONDS.toMillis(1));
            }
            return this;
        }

        LockScreenSession wakeUpDevice() {
            pressWakeupButton();
            return this;
        }

        LockScreenSession unlockDevice() {
            pressUnlockButton();
            return this;
        }

        public LockScreenSession gotoKeyguard() {
            if (DEBUG && isLockDisabled()) {
                logE("LockScreenSession.gotoKeyguard() is called without lock enabled.");
            }
            sleepDevice();
            wakeUpDevice();
            mAmWmState.waitForKeyguardShowingAndNotOccluded();
            return this;
        }

        @Override
        public void close() throws Exception {
            setLockDisabled(mIsLockDisabled);
            if (mLockCredentialSet) {
                removeLockCredential();
            }

            // Dismiss active keyguard after credential is cleared, so keyguard doesn't ask for
            // the stale credential.
            pressBackButton();
            sleepDevice();
            wakeUpDevice();
            unlockDevice();
        }

        /**
         * Returns whether the lock screen is disabled.
         *
         * @return true if the lock screen is disabled, false otherwise.
         */
        private boolean isLockDisabled() {
            final String isLockDisabled = runCommandAndPrintOutput(
                    "locksettings get-disabled").trim();
            return !"null".equals(isLockDisabled) && Boolean.parseBoolean(isLockDisabled);
        }

        /**
         * Disable the lock screen.
         *
         * @param lockDisabled true if should disable, false otherwise.
         */
        protected void setLockDisabled(boolean lockDisabled) {
            runCommandAndPrintOutput("locksettings set-disabled " + lockDisabled);
        }
    }

    /** Helper class to save, set & wait, and restore rotation related preferences. */
    protected class RotationSession extends SettingsSession<Integer> {
        private final SettingsSession<Integer> mUserRotation;

        public RotationSession() throws Exception {
            // Save accelerometer_rotation preference.
            super(Settings.System.getUriFor(Settings.System.ACCELEROMETER_ROTATION),
                    Settings.System::getInt, Settings.System::putInt);
            mUserRotation = new SettingsSession<>(
                    Settings.System.getUriFor(Settings.System.USER_ROTATION),
                    Settings.System::getInt, Settings.System::putInt);
            // Disable accelerometer_rotation.
            super.set(0);
        }

        @Override
        public void set(@NonNull Integer value) throws Exception {
            mUserRotation.set(value);
            // Wait for settling rotation.
            mAmWmState.waitForRotation(value);
        }

        @Override
        public void close() throws Exception {
            mUserRotation.close();
            // Restore accelerometer_rotation preference.
            super.close();
        }
    }

    protected int getDeviceRotation(int displayId) {
        final String displays = runCommandAndPrintOutput("dumpsys display displays").trim();
        Pattern pattern = Pattern.compile(
                "(mDisplayId=" + displayId + ")([\\s\\S]*)(mOverrideDisplayInfo)(.*)"
                        + "(rotation)(\\s+)(\\d+)");
        Matcher matcher = pattern.matcher(displays);
        if (matcher.find()) {
            final String match = matcher.group(7);
            return Integer.parseInt(match);
        }

        return INVALID_DEVICE_ROTATION;
    }

    protected static String runCommandAndPrintOutput(String command) {
        final String output = executeShellCommand(command);
        log(output);
        return output;
    }

    protected static class LogSeparator {
        private final String mUniqueString;

        private LogSeparator() {
            mUniqueString = UUID.randomUUID().toString();
        }

        @Override
        public String toString() {
            return mUniqueString;
        }
    }

    /**
     * Inserts a log separator so we can always find the starting point from where to evaluate
     * following logs.
     * @return Unique log separator.
     */
    protected LogSeparator separateLogs() {
        final LogSeparator logSeparator = new LogSeparator();
        executeShellCommand("log -t " + LOG_SEPARATOR + " " + logSeparator);
        return logSeparator;
    }

    protected static String[] getDeviceLogsForComponents(
            LogSeparator logSeparator, String... logTags) {
        String filters = LOG_SEPARATOR + ":I ";
        for (String component : logTags) {
            filters += component + ":I ";
        }
        final String[] result = executeShellCommand("logcat -v brief -d " + filters + " *:S")
                .split("\\n");
        if (logSeparator == null) {
            return result;
        }

        // Make sure that we only check logs after the separator.
        int i = 0;
        boolean lookingForSeparator = true;
        while (i < result.length && lookingForSeparator) {
            if (result[i].contains(logSeparator.toString())) {
                lookingForSeparator = false;
            }
            i++;
        }
        final String[] filteredResult = new String[result.length - i];
        for (int curPos = 0; i < result.length; curPos++, i++) {
            filteredResult[curPos] = result[i];
        }
        return filteredResult;
    }

    /**
     * Base helper class for retrying validator success.
     */
    private abstract static class RetryValidator {

        private static final int RETRY_LIMIT = 5;
        private static final long RETRY_INTERVAL = TimeUnit.SECONDS.toMillis(1);

        /**
         * @return Error string if validation is failed, null if everything is fine.
         **/
        @Nullable
        protected abstract String validate();

        /**
         * Executes {@link #validate()}. Retries {@link #RETRY_LIMIT} times with
         * {@link #RETRY_INTERVAL} interval.
         *
         * @param waitingMessage logging message while waiting validation.
         */
        void assertValidator(String waitingMessage) {
            String resultString = null;
            for (int retry = 1; retry <= RETRY_LIMIT; retry++) {
                resultString = validate();
                if (resultString == null) {
                    return;
                }
                logAlways(waitingMessage + ": " + resultString);
                SystemClock.sleep(RETRY_INTERVAL);
            }
            fail(resultString);
        }
    }

    private static class ActivityLifecycleCountsValidator extends RetryValidator {
        private final ComponentName mActivityName;
        private final LogSeparator mLogSeparator;
        private final int mCreateCount;
        private final int mStartCount;
        private final int mResumeCount;
        private final int mPauseCount;
        private final int mStopCount;
        private final int mDestroyCount;

        ActivityLifecycleCountsValidator(ComponentName activityName, LogSeparator logSeparator,
                int createCount, int startCount, int resumeCount, int pauseCount, int stopCount,
                int destroyCount) {
            mActivityName = activityName;
            mLogSeparator = logSeparator;
            mCreateCount = createCount;
            mStartCount = startCount;
            mResumeCount = resumeCount;
            mPauseCount = pauseCount;
            mStopCount = stopCount;
            mDestroyCount = destroyCount;
        }

        @Override
        @Nullable
        protected String validate() {
            final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(
                    mActivityName, mLogSeparator);
            if (lifecycleCounts.mCreateCount == mCreateCount
                    && lifecycleCounts.mStartCount == mStartCount
                    && lifecycleCounts.mResumeCount == mResumeCount
                    && lifecycleCounts.mPauseCount == mPauseCount
                    && lifecycleCounts.mStopCount == mStopCount
                    && lifecycleCounts.mDestroyCount == mDestroyCount) {
                return null;
            }
            final String expected = IntStream.of(mCreateCount, mStopCount, mResumeCount,
                    mPauseCount, mStopCount, mDestroyCount)
                    .mapToObj(Integer::toString)
                    .collect(Collectors.joining("/"));
            return getActivityName(mActivityName) + " lifecycle count mismatched:"
                    + " expected=" + expected
                    + " actual=" + lifecycleCounts.counters();
        }
    }

    void assertActivityLifecycle(ComponentName activityName, boolean relaunched,
            LogSeparator logSeparator) {
        new RetryValidator() {

            @Nullable
            @Override
            protected String validate() {
                final ActivityLifecycleCounts lifecycleCounts =
                        new ActivityLifecycleCounts(activityName, logSeparator);
                final String logTag = getLogTag(activityName);
                if (relaunched) {
                    if (lifecycleCounts.mDestroyCount < 1) {
                        return logTag + " must have been destroyed. mDestroyCount="
                                + lifecycleCounts.mDestroyCount;
                    }
                    if (lifecycleCounts.mCreateCount < 1) {
                        return logTag + " must have been (re)created. mCreateCount="
                                + lifecycleCounts.mCreateCount;
                    }
                    return null;
                }
                if (lifecycleCounts.mDestroyCount > 0) {
                    return logTag + " must *NOT* have been destroyed. mDestroyCount="
                            + lifecycleCounts.mDestroyCount;
                }
                if (lifecycleCounts.mCreateCount > 0) {
                    return logTag + " must *NOT* have been (re)created. mCreateCount="
                            + lifecycleCounts.mCreateCount;
                }
                if (lifecycleCounts.mConfigurationChangedCount < 1) {
                    return logTag + " must have received configuration changed. "
                            + "mConfigurationChangedCount="
                            + lifecycleCounts.mConfigurationChangedCount;
                }
                return null;
            }
        }.assertValidator("***Waiting for valid lifecycle state");
    }

    protected void assertRelaunchOrConfigChanged(ComponentName activityName, int numRelaunch,
            int numConfigChange, LogSeparator logSeparator) {
        new RetryValidator() {

            @Nullable
            @Override
            protected String validate() {
                final ActivityLifecycleCounts lifecycleCounts =
                        new ActivityLifecycleCounts(activityName, logSeparator);
                final String logTag = getLogTag(activityName);
                if (lifecycleCounts.mDestroyCount != numRelaunch) {
                    return logTag + " has been destroyed " + lifecycleCounts.mDestroyCount
                            + " time(s), expecting " + numRelaunch;
                } else if (lifecycleCounts.mCreateCount != numRelaunch) {
                    return logTag + " has been (re)created " + lifecycleCounts.mCreateCount
                            + " time(s), expecting " + numRelaunch;
                } else if (lifecycleCounts.mConfigurationChangedCount != numConfigChange) {
                    return logTag + " has received "
                            + lifecycleCounts.mConfigurationChangedCount
                            + " onConfigurationChanged() calls, expecting " + numConfigChange;
                }
                return null;
            }
        }.assertValidator("***Waiting for relaunch or config changed");
    }

    protected void assertActivityDestroyed(ComponentName activityName, LogSeparator logSeparator) {
        new RetryValidator() {

            @Nullable
            @Override
            protected String validate() {
                final ActivityLifecycleCounts lifecycleCounts =
                        new ActivityLifecycleCounts(activityName, logSeparator);
                final String logTag = getLogTag(activityName);
                if (lifecycleCounts.mDestroyCount != 1) {
                    return logTag + " has been destroyed " + lifecycleCounts.mDestroyCount
                            + " time(s), expecting single destruction.";
                }
                if (lifecycleCounts.mCreateCount != 0) {
                    return logTag + " has been (re)created " + lifecycleCounts.mCreateCount
                            + " time(s), not expecting any.";
                }
                if (lifecycleCounts.mConfigurationChangedCount != 0) {
                    return logTag + " has received " + lifecycleCounts.mConfigurationChangedCount
                            + " onConfigurationChanged() calls, not expecting any.";
                }
                return null;
            }
        }.assertValidator("***Waiting for activity destroyed");
    }

    void assertLifecycleCounts(ComponentName activityName, LogSeparator logSeparator,
            int createCount, int startCount, int resumeCount, int pauseCount, int stopCount,
            int destroyCount, int configurationChangeCount) {
        new RetryValidator() {
            @Override
            protected String validate() {
                final ActivityLifecycleCounts lifecycleCounts =
                        new ActivityLifecycleCounts(activityName, logSeparator);
                final String logTag = getLogTag(activityName);
                if (createCount != lifecycleCounts.mCreateCount) {
                    return logTag + " has been created " + lifecycleCounts.mCreateCount
                            + " time(s), expecting " + createCount;
                }
                if (startCount != lifecycleCounts.mStartCount) {
                    return logTag + " has been started " + lifecycleCounts.mStartCount
                            + " time(s), expecting " + startCount;
                }
                if (resumeCount != lifecycleCounts.mResumeCount) {
                    return logTag + " has been resumed " + lifecycleCounts.mResumeCount
                            + " time(s), expecting " + resumeCount;
                }
                if (pauseCount != lifecycleCounts.mPauseCount) {
                    return logTag + " has been paused " + lifecycleCounts.mPauseCount
                            + " time(s), expecting " + pauseCount;
                }
                if (stopCount != lifecycleCounts.mStopCount) {
                    return logTag + " has been stopped " + lifecycleCounts.mStopCount
                            + " time(s), expecting " + stopCount;
                }
                if (destroyCount != lifecycleCounts.mDestroyCount) {
                    return logTag + " has been destroyed " + lifecycleCounts.mDestroyCount
                            + " time(s), expecting " + destroyCount;
                }
                if (configurationChangeCount != lifecycleCounts.mConfigurationChangedCount) {
                    return logTag + " has received config changes "
                            + lifecycleCounts.mConfigurationChangedCount
                            + " time(s), expecting " + configurationChangeCount;
                }
                return null;
            }
        }.assertValidator("***Waiting for activity lifecycle counts");
    }

    void assertSingleLaunch(ComponentName activityName, LogSeparator logSeparator) {
        new ActivityLifecycleCountsValidator(activityName, logSeparator, 1 /* createCount */,
                1 /* startCount */, 1 /* resumeCount */, 0 /* pauseCount */, 0 /* stopCount */,
                0 /* destroyCount */)
                .assertValidator("***Waiting for activity create, start, and resume");
    }

    void assertSingleLaunchAndStop(ComponentName activityName, LogSeparator logSeparator) {
        new ActivityLifecycleCountsValidator(activityName, logSeparator, 1 /* createCount */,
                1 /* startCount */, 1 /* resumeCount */, 1 /* pauseCount */, 1 /* stopCount */,
                0 /* destroyCount */)
                .assertValidator("***Waiting for activity create, start, resume, pause, and stop");
    }

    void assertSingleStartAndStop(ComponentName activityName, LogSeparator logSeparator) {
        new ActivityLifecycleCountsValidator(activityName, logSeparator, 0 /* createCount */,
                1 /* startCount */, 1 /* resumeCount */, 1 /* pauseCount */, 1 /* stopCount */,
                0 /* destroyCount */)
                .assertValidator("***Waiting for activity start, resume, pause, and stop");
    }

    void assertSingleStart(ComponentName activityName, LogSeparator logSeparator) {
        new ActivityLifecycleCountsValidator(activityName, logSeparator, 0 /* createCount */,
                1 /* startCount */, 1 /* resumeCount */, 0 /* pauseCount */, 0 /* stopCount */,
                0 /* destroyCount */)
                .assertValidator("***Waiting for activity start and resume");
    }

    // TODO: Now that our test are device side, we can convert these to a more direct communication
    // channel vs. depending on logs.
    private static final Pattern sCreatePattern = Pattern.compile("(.+): onCreate");
    private static final Pattern sStartPattern = Pattern.compile("(.+): onStart");
    private static final Pattern sResumePattern = Pattern.compile("(.+): onResume");
    private static final Pattern sPausePattern = Pattern.compile("(.+): onPause");
    private static final Pattern sConfigurationChangedPattern =
            Pattern.compile("(.+): onConfigurationChanged");
    private static final Pattern sMovedToDisplayPattern =
            Pattern.compile("(.+): onMovedToDisplay");
    private static final Pattern sStopPattern = Pattern.compile("(.+): onStop");
    private static final Pattern sDestroyPattern = Pattern.compile("(.+): onDestroy");
    private static final Pattern sMultiWindowModeChangedPattern =
            Pattern.compile("(.+): onMultiWindowModeChanged");
    private static final Pattern sPictureInPictureModeChangedPattern =
            Pattern.compile("(.+): onPictureInPictureModeChanged");
    private static final Pattern sUserLeaveHintPattern = Pattern.compile("(.+): onUserLeaveHint");
    private static final Pattern sNewConfigPattern = Pattern.compile(
            "(.+): config size=\\((\\d+),(\\d+)\\) displaySize=\\((\\d+),(\\d+)\\)"
            + " metricsSize=\\((\\d+),(\\d+)\\) smallestScreenWidth=(\\d+) densityDpi=(\\d+)"
            + " orientation=(\\d+)");
    private static final Pattern sDisplayStatePattern =
            Pattern.compile("Display Power: state=(.+)");
    private static final Pattern sCurrentUiModePattern = Pattern.compile("mCurUiMode=0x(\\d+)");
    private static final Pattern sUiModeLockedPattern =
            Pattern.compile("mUiModeLocked=(true|false)");

    static class ReportedSizes {
        int widthDp;
        int heightDp;
        int displayWidth;
        int displayHeight;
        int metricsWidth;
        int metricsHeight;
        int smallestWidthDp;
        int densityDpi;
        int orientation;

        @Override
        public String toString() {
            return "ReportedSizes: {widthDp=" + widthDp + " heightDp=" + heightDp
                    + " displayWidth=" + displayWidth + " displayHeight=" + displayHeight
                    + " metricsWidth=" + metricsWidth + " metricsHeight=" + metricsHeight
                    + " smallestWidthDp=" + smallestWidthDp + " densityDpi=" + densityDpi
                    + " orientation=" + orientation + "}";
        }

        @Override
        public boolean equals(Object obj) {
            if ( this == obj ) return true;
            if ( !(obj instanceof ReportedSizes) ) return false;
            ReportedSizes that = (ReportedSizes) obj;
            return widthDp == that.widthDp
                    && heightDp == that.heightDp
                    && displayWidth == that.displayWidth
                    && displayHeight == that.displayHeight
                    && metricsWidth == that.metricsWidth
                    && metricsHeight == that.metricsHeight
                    && smallestWidthDp == that.smallestWidthDp
                    && densityDpi == that.densityDpi
                    && orientation == that.orientation;
        }
    }

    @Nullable
    ReportedSizes getLastReportedSizesForActivity(
            ComponentName activityName, LogSeparator logSeparator) {
        final String logTag = getLogTag(activityName);
        for (int retry = 1; retry <= 5; retry++ ) {
            final ReportedSizes result = readLastReportedSizes(logSeparator, logTag);
            if (result != null) {
                return result;
            }
            logAlways("***Waiting for sizes to be reported... retry=" + retry);
            SystemClock.sleep(1000);
        }
        logE("***Waiting for activity size failed: activityName=" + logTag);
        return null;
    }

    private ReportedSizes readLastReportedSizes(LogSeparator logSeparator, String logTag) {
        final String[] lines = getDeviceLogsForComponents(logSeparator, logTag);
        for (int i = lines.length - 1; i >= 0; i--) {
            final String line = lines[i].trim();
            final Matcher matcher = sNewConfigPattern.matcher(line);
            if (matcher.matches()) {
                ReportedSizes details = new ReportedSizes();
                details.widthDp = Integer.parseInt(matcher.group(2));
                details.heightDp = Integer.parseInt(matcher.group(3));
                details.displayWidth = Integer.parseInt(matcher.group(4));
                details.displayHeight = Integer.parseInt(matcher.group(5));
                details.metricsWidth = Integer.parseInt(matcher.group(6));
                details.metricsHeight = Integer.parseInt(matcher.group(7));
                details.smallestWidthDp = Integer.parseInt(matcher.group(8));
                details.densityDpi = Integer.parseInt(matcher.group(9));
                details.orientation = Integer.parseInt(matcher.group(10));
                return details;
            }
        }
        return null;
    }

    /** Waits for at least one onMultiWindowModeChanged event. */
    ActivityLifecycleCounts waitForOnMultiWindowModeChanged(ComponentName activityName,
            LogSeparator logSeparator) {
        int retry = 1;
        ActivityLifecycleCounts result;
        do {
            result = new ActivityLifecycleCounts(activityName, logSeparator);
            if (result.mMultiWindowModeChangedCount >= 1) {
                return result;
            }
            logAlways("***waitForOnMultiWindowModeChanged... retry=" + retry);
            SystemClock.sleep(TimeUnit.SECONDS.toMillis(1));
        } while (retry++ <= 5);
        return result;
    }

    // TODO: Now that our test are device side, we can convert these to a more direct communication
    // channel vs. depending on logs.
    static class ActivityLifecycleCounts {
        int mCreateCount;
        int mStartCount;
        int mResumeCount;
        int mConfigurationChangedCount;
        int mLastConfigurationChangedLineIndex;
        int mMovedToDisplayCount;
        int mMultiWindowModeChangedCount;
        int mLastMultiWindowModeChangedLineIndex;
        int mPictureInPictureModeChangedCount;
        int mLastPictureInPictureModeChangedLineIndex;
        int mUserLeaveHintCount;
        int mPauseCount;
        int mStopCount;
        int mLastStopLineIndex;
        int mDestroyCount;

        ActivityLifecycleCounts(ComponentName componentName, LogSeparator logSeparator) {
            int lineIndex = 0;
            waitForIdle();
            for (String line : getDeviceLogsForComponents(logSeparator, getLogTag(componentName))) {
                line = line.trim();
                lineIndex++;

                Matcher matcher = sCreatePattern.matcher(line);
                if (matcher.matches()) {
                    mCreateCount++;
                    continue;
                }

                matcher = sStartPattern.matcher(line);
                if (matcher.matches()) {
                    mStartCount++;
                    continue;
                }

                matcher = sResumePattern.matcher(line);
                if (matcher.matches()) {
                    mResumeCount++;
                    continue;
                }

                matcher = sConfigurationChangedPattern.matcher(line);
                if (matcher.matches()) {
                    mConfigurationChangedCount++;
                    mLastConfigurationChangedLineIndex = lineIndex;
                    continue;
                }

                matcher = sMovedToDisplayPattern.matcher(line);
                if (matcher.matches()) {
                    mMovedToDisplayCount++;
                    continue;
                }

                matcher = sMultiWindowModeChangedPattern.matcher(line);
                if (matcher.matches()) {
                    mMultiWindowModeChangedCount++;
                    mLastMultiWindowModeChangedLineIndex = lineIndex;
                    continue;
                }

                matcher = sPictureInPictureModeChangedPattern.matcher(line);
                if (matcher.matches()) {
                    mPictureInPictureModeChangedCount++;
                    mLastPictureInPictureModeChangedLineIndex = lineIndex;
                    continue;
                }

                matcher = sUserLeaveHintPattern.matcher(line);
                if (matcher.matches()) {
                    mUserLeaveHintCount++;
                    continue;
                }

                matcher = sPausePattern.matcher(line);
                if (matcher.matches()) {
                    mPauseCount++;
                    continue;
                }

                matcher = sStopPattern.matcher(line);
                if (matcher.matches()) {
                    mStopCount++;
                    mLastStopLineIndex = lineIndex;
                    continue;
                }

                matcher = sDestroyPattern.matcher(line);
                if (matcher.matches()) {
                    mDestroyCount++;
                    continue;
                }
            }
        }

        String counters() {
            return IntStream.of(mCreateCount, mStartCount, mResumeCount, mPauseCount, mStopCount,
                    mDestroyCount)
                    .mapToObj(Integer::toString)
                    .collect(Collectors.joining("/"));
        }
    }

    protected void stopTestPackage(final ComponentName activityName) {
        executeShellCommand("am force-stop " + activityName.getPackageName());
    }

    protected LaunchActivityBuilder getLaunchActivityBuilder() {
        return new LaunchActivityBuilder(mAmWmState);
    }

    protected static class LaunchActivityBuilder {
        private final ActivityAndWindowManagersState mAmWmState;

        // The activity to be launched
        private ComponentName mTargetActivity = TEST_ACTIVITY;
        private boolean mUseApplicationContext;
        private boolean mToSide;
        private boolean mRandomData;
        private boolean mNewTask;
        private boolean mMultipleTask;
        private int mDisplayId = INVALID_DISPLAY;
        // A proxy activity that launches other activities including mTargetActivityName
        private ComponentName mLaunchingActivity = LAUNCHING_ACTIVITY;
        private boolean mReorderToFront;
        private boolean mWaitForLaunched;
        private boolean mSuppressExceptions;
        // Use of the following variables indicates that a broadcast receiver should be used instead
        // of a launching activity;
        private ComponentName mBroadcastReceiver;
        private String mBroadcastReceiverAction;

        private enum LauncherType {
            INSTRUMENTATION, LAUNCHING_ACTIVITY, BROADCAST_RECEIVER
        }
        private LauncherType mLauncherType = LauncherType.LAUNCHING_ACTIVITY;

        public LaunchActivityBuilder(ActivityAndWindowManagersState amWmState) {
            mAmWmState = amWmState;
            mWaitForLaunched = true;
        }

        public LaunchActivityBuilder setToSide(boolean toSide) {
            mToSide = toSide;
            return this;
        }

        public LaunchActivityBuilder setRandomData(boolean randomData) {
            mRandomData = randomData;
            return this;
        }

        public LaunchActivityBuilder setNewTask(boolean newTask) {
            mNewTask = newTask;
            return this;
        }

        public LaunchActivityBuilder setMultipleTask(boolean multipleTask) {
            mMultipleTask = multipleTask;
            return this;
        }

        public LaunchActivityBuilder setReorderToFront(boolean reorderToFront) {
            mReorderToFront = reorderToFront;
            return this;
        }

        public LaunchActivityBuilder setUseApplicationContext(boolean useApplicationContext) {
            mUseApplicationContext = useApplicationContext;
            return this;
        }

        public ComponentName getTargetActivity() {
            return mTargetActivity;
        }

        public boolean isTargetActivityTranslucent() {
            return mAmWmState.getAmState().isActivityTranslucent(mTargetActivity);
        }

        public LaunchActivityBuilder setTargetActivity(ComponentName targetActivity) {
            mTargetActivity = targetActivity;
            return this;
        }

        public LaunchActivityBuilder setDisplayId(int id) {
            mDisplayId = id;
            return this;
        }

        public LaunchActivityBuilder setLaunchingActivity(ComponentName launchingActivity) {
            mLaunchingActivity = launchingActivity;
            mLauncherType = LauncherType.LAUNCHING_ACTIVITY;
            return this;
        }

        public LaunchActivityBuilder setWaitForLaunched(boolean shouldWait) {
            mWaitForLaunched = shouldWait;
            return this;
        }

        /** Use broadcast receiver as a launchpad for activities. */
        public LaunchActivityBuilder setUseBroadcastReceiver(final ComponentName broadcastReceiver,
                final String broadcastAction) {
            mBroadcastReceiver = broadcastReceiver;
            mBroadcastReceiverAction = broadcastAction;
            mLauncherType = LauncherType.BROADCAST_RECEIVER;
            return this;
        }

        /** Use {@link android.app.Instrumentation} as a launchpad for activities. */
        public LaunchActivityBuilder setUseInstrumentation() {
            mLauncherType = LauncherType.INSTRUMENTATION;
            // Calling startActivity() from outside of an Activity context requires the
            // FLAG_ACTIVITY_NEW_TASK flag.
            setNewTask(true);
            return this;
        }

        public LaunchActivityBuilder setSuppressExceptions(boolean suppress) {
            mSuppressExceptions = suppress;
            return this;
        }

        public void execute() {
            switch (mLauncherType) {
                case INSTRUMENTATION:
                    launchUsingInstrumentation();
                    break;
                case LAUNCHING_ACTIVITY:
                case BROADCAST_RECEIVER:
                    launchUsingShellCommand();
            }

            if (mWaitForLaunched) {
                mAmWmState.waitForValidState(mTargetActivity);
            }
        }

        /** Launch an activity using instrumentation. */
        private void launchUsingInstrumentation() {
            final Bundle b = new Bundle();
            b.putBoolean(KEY_USE_INSTRUMENTATION, true);
            b.putBoolean(KEY_LAUNCH_ACTIVITY, true);
            b.putBoolean(KEY_LAUNCH_TO_SIDE, mToSide);
            b.putBoolean(KEY_RANDOM_DATA, mRandomData);
            b.putBoolean(KEY_NEW_TASK, mNewTask);
            b.putBoolean(KEY_MULTIPLE_TASK, mMultipleTask);
            b.putBoolean(KEY_REORDER_TO_FRONT, mReorderToFront);
            b.putInt(KEY_DISPLAY_ID, mDisplayId);
            b.putBoolean(KEY_USE_APPLICATION_CONTEXT, mUseApplicationContext);
            b.putString(KEY_TARGET_COMPONENT, getActivityName(mTargetActivity));
            b.putBoolean(KEY_SUPPRESS_EXCEPTIONS, mSuppressExceptions);
            final Context context = InstrumentationRegistry.getContext();
            launchActivityFromExtras(context, b);
        }

        /** Build and execute a shell command to launch an activity. */
        private void launchUsingShellCommand() {
            StringBuilder commandBuilder = new StringBuilder();
            if (mBroadcastReceiver != null && mBroadcastReceiverAction != null) {
                // Use broadcast receiver to launch the target.
                commandBuilder.append("am broadcast -a ").append(mBroadcastReceiverAction)
                        .append(" -p ").append(mBroadcastReceiver.getPackageName())
                        // Include stopped packages
                        .append(" -f 0x00000020");
            } else {
                // Use launching activity to launch the target.
                commandBuilder.append(getAmStartCmd(mLaunchingActivity))
                        .append(" -f 0x20000020");
            }

            // Add a flag to ensure we actually mean to launch an activity.
            commandBuilder.append(" --ez " + KEY_LAUNCH_ACTIVITY + " true");

            if (mToSide) {
                commandBuilder.append(" --ez " + KEY_LAUNCH_TO_SIDE + " true");
            }
            if (mRandomData) {
                commandBuilder.append(" --ez " + KEY_RANDOM_DATA + " true");
            }
            if (mNewTask) {
                commandBuilder.append(" --ez " + KEY_NEW_TASK + " true");
            }
            if (mMultipleTask) {
                commandBuilder.append(" --ez " + KEY_MULTIPLE_TASK + " true");
            }
            if (mReorderToFront) {
                commandBuilder.append(" --ez " + KEY_REORDER_TO_FRONT + " true");
            }
            if (mDisplayId != INVALID_DISPLAY) {
                commandBuilder.append(" --ei " + KEY_DISPLAY_ID + " ").append(mDisplayId);
            }

            if (mUseApplicationContext) {
                commandBuilder.append(" --ez " + KEY_USE_APPLICATION_CONTEXT + " true");
            }

            if (mTargetActivity != null) {
                // {@link ActivityLauncher} parses this extra string by
                // {@link ComponentName#unflattenFromString(String)}.
                commandBuilder.append(" --es " + KEY_TARGET_COMPONENT + " ")
                        .append(getActivityName(mTargetActivity));
            }

            if (mSuppressExceptions) {
                commandBuilder.append(" --ez " + KEY_SUPPRESS_EXCEPTIONS + " true");
            }
            executeShellCommand(commandBuilder.toString());
        }
    }

    // Activity used in place of recents when home is the recents component.
    public static class SideActivity extends Activity {
    }
}
