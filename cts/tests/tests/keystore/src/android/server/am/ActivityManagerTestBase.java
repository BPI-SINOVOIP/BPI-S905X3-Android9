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

import static android.app.WindowConfiguration.ACTIVITY_TYPE_ASSISTANT;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_RECENTS;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_UNDEFINED;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.ComponentNameUtils.getWindowName;
import static android.server.am.StateLogger.log;
import static android.server.am.StateLogger.logAlways;
import static android.server.am.StateLogger.logE;
import static android.server.am.UiDeviceUtils.pressBackButton;
import static android.server.am.UiDeviceUtils.pressEnterButton;
import static android.server.am.UiDeviceUtils.pressHomeButton;
import static android.server.am.UiDeviceUtils.pressSleepButton;
import static android.server.am.UiDeviceUtils.pressUnlockButton;
import static android.server.am.UiDeviceUtils.pressWakeupButton;
import static android.server.am.UiDeviceUtils.waitForDeviceIdle;

import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;

import com.android.compatibility.common.util.SystemUtil;

import org.junit.After;
import org.junit.Before;

import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public abstract class ActivityManagerTestBase {
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

    private static final String LOCK_CREDENTIAL = "1234";

    private static final int UI_MODE_TYPE_MASK = 0x0f;
    private static final int UI_MODE_TYPE_VR_HEADSET = 0x07;

    protected Context mContext;
    protected ActivityManager mAm;

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

    protected ActivityAndWindowManagersState mAmWmState = new ActivityAndWindowManagersState();

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getContext();
        mAm = mContext.getSystemService(ActivityManager.class);

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

    protected void launchActivity(final ComponentName activityName, final String... keyValuePairs) {
        executeShellCommand(getAmStartCmd(activityName, keyValuePairs));
        mAmWmState.waitForValidState(new WaitForValidActivityState(activityName));
    }

    private static void waitForIdle() {
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
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

    protected void setActivityTaskWindowingMode(ComponentName activityName, int windowingMode) {
        final int taskId = getActivityTaskId(activityName);
        mAm.setTaskWindowingMode(taskId, windowingMode, true /* toTop */);
        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .setWindowingMode(windowingMode)
                .build());
    }

    @Deprecated
    protected int getActivityTaskId(final ComponentName activityName) {
        final String windowName = getWindowName(activityName);
        final String output = executeShellCommand(AM_STACK_LIST);
        final Pattern activityPattern = Pattern.compile("(.*) " + windowName + " (.*)");
        for (final String line : output.split("\\n")) {
            final Matcher matcher = activityPattern.matcher(line);
            if (matcher.matches()) {
                for (String word : line.split("\\s+")) {
                    if (word.startsWith(TASK_ID_PREFIX)) {
                        final String withColon = word.split("=")[1];
                        return Integer.parseInt(withColon.substring(0, withColon.length() - 1));
                    }
                }
            }
        }
        return -1;
    }

    protected boolean isTablet() {
        // Larger than approx 7" tablets
        return mContext.getResources().getConfiguration().smallestScreenWidthDp >= 600;
    }

    // TODO: Switch to using a feature flag, when available.
    protected boolean isUiModeLockedToVrHeadset() {
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

        public LockScreenSession sleepDevice() {
            pressSleepButton();
            waitForDisplayStateWithRetry(false /* displayOn */ );
            return this;
        }

        protected LockScreenSession wakeUpDevice() {
            pressWakeupButton();
            waitForDisplayStateWithRetry(true /* displayOn */ );
            return this;
        }

        /**
         * If the device has a PIN or password set, this doesn't immediately unlock the device.
         * Instead, it shows the keyguard and brings the entry field into focus, ready for a
         * call to enterAndConfirmLockCredential().
         */
        protected LockScreenSession unlockDevice() {
            pressUnlockButton();
            SystemClock.sleep(200);
            return this;
        }

        public LockScreenSession gotoKeyguard() {
            if (DEBUG && isLockDisabled()) {
                logE("LockScreenSession.gotoKeyguard() is called without lock enabled.");
            }
            sleepDevice();
            wakeUpDevice();
            unlockDevice();
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

        protected void waitForDisplayStateWithRetry(boolean displayOn) {
            for (int retry = 1; isDisplayOn() != displayOn && retry <= 5; retry++) {
                logAlways("***Waiting for display state... retry " + retry);
                SystemClock.sleep(TimeUnit.SECONDS.toMillis(1));
            }
        }
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

    // TODO: Now that our test are device side, we can convert these to a more direct communication
    // channel vs. depending on logs.
    private static final Pattern sDisplayStatePattern =
            Pattern.compile("Display Power: state=(.+)");
    private static final Pattern sCurrentUiModePattern = Pattern.compile("mCurUiMode=0x(\\d+)");
    private static final Pattern sUiModeLockedPattern =
            Pattern.compile("mUiModeLocked=(true|false)");

    protected void stopTestPackage(final ComponentName activityName) {
        executeShellCommand("am force-stop " + activityName.getPackageName());
    }
}
