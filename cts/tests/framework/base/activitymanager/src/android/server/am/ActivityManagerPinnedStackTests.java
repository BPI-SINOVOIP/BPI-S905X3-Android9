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
import static android.app.WindowConfiguration.ACTIVITY_TYPE_HOME;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.server.am.ActivityManagerState.STATE_RESUMED;
import static android.server.am.ActivityManagerState.STATE_STOPPED;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.ComponentNameUtils.getLogTag;
import static android.server.am.ComponentNameUtils.getWindowName;
import static android.server.am.Components.ALWAYS_FOCUSABLE_PIP_ACTIVITY;
import static android.server.am.Components.LAUNCHING_ACTIVITY;
import static android.server.am.Components.LAUNCH_ENTER_PIP_ACTIVITY;
import static android.server.am.Components.LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY;
import static android.server.am.Components.NON_RESIZEABLE_ACTIVITY;
import static android.server.am.Components.NO_RELAUNCH_ACTIVITY;
import static android.server.am.Components.PIP_ACTIVITY;
import static android.server.am.Components.PIP_ACTIVITY2;
import static android.server.am.Components.PIP_ACTIVITY_WITH_SAME_AFFINITY;
import static android.server.am.Components.PIP_ON_STOP_ACTIVITY;
import static android.server.am.Components.PipActivity.ACTION_ENTER_PIP;
import static android.server.am.Components.PipActivity.ACTION_EXPAND_PIP;
import static android.server.am.Components.PipActivity.ACTION_FINISH;
import static android.server.am.Components.PipActivity.ACTION_MOVE_TO_BACK;
import static android.server.am.Components.PipActivity.ACTION_SET_REQUESTED_ORIENTATION;
import static android.server.am.Components.PipActivity.EXTRA_ASSERT_NO_ON_STOP_BEFORE_PIP;
import static android.server.am.Components.PipActivity.EXTRA_ENTER_PIP;
import static android.server.am.Components.PipActivity.EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR;
import static android.server.am.Components.PipActivity.EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR;
import static android.server.am.Components.PipActivity.EXTRA_ENTER_PIP_ON_PAUSE;
import static android.server.am.Components.PipActivity.EXTRA_FINISH_SELF_ON_RESUME;
import static android.server.am.Components.PipActivity.EXTRA_ON_PAUSE_DELAY;
import static android.server.am.Components.PipActivity.EXTRA_PIP_ORIENTATION;
import static android.server.am.Components.PipActivity.EXTRA_REENTER_PIP_ON_EXIT;
import static android.server.am.Components.PipActivity.EXTRA_SET_ASPECT_RATIO_DENOMINATOR;
import static android.server.am.Components.PipActivity.EXTRA_SET_ASPECT_RATIO_NUMERATOR;
import static android.server.am.Components.PipActivity.EXTRA_SET_ASPECT_RATIO_WITH_DELAY_DENOMINATOR;
import static android.server.am.Components.PipActivity.EXTRA_SET_ASPECT_RATIO_WITH_DELAY_NUMERATOR;
import static android.server.am.Components.PipActivity.EXTRA_START_ACTIVITY;
import static android.server.am.Components.PipActivity.EXTRA_TAP_TO_FINISH;
import static android.server.am.Components.RESUME_WHILE_PAUSING_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY_WITH_SAME_AFFINITY;
import static android.server.am.Components.TRANSLUCENT_TEST_ACTIVITY;
import static android.server.am.Components.TestActivity.EXTRA_FIXED_ORIENTATION;
import static android.server.am.Components.TestActivity.TEST_ACTIVITY_ACTION_FINISH_SELF;
import static android.server.am.UiDeviceUtils.pressWindowButton;
import static android.view.Display.DEFAULT_DISPLAY;
import static org.hamcrest.Matchers.lessThan;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.content.ComponentName;
import android.content.Context;
import android.database.ContentObserver;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Looper;
import android.platform.test.annotations.Presubmit;
import android.provider.Settings;
import android.server.am.ActivityManagerState.ActivityStack;
import android.server.am.ActivityManagerState.ActivityTask;
import android.server.am.WindowManagerState.WindowStack;
import android.server.am.settings.SettingsSession;
import android.support.test.filters.FlakyTest;
import android.support.test.InstrumentationRegistry;
import android.util.Log;
import android.util.Size;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.junit.Ignore;
import org.junit.Test;

/**
 * Build/Install/Run:
 * atest CtsActivityManagerDeviceTestCases:ActivityManagerPinnedStackTests
 */
@FlakyTest(bugId = 71792368)
public class ActivityManagerPinnedStackTests extends ActivityManagerTestBase {
    private static final String TAG = ActivityManagerPinnedStackTests.class.getSimpleName();

    private static final String APP_OPS_OP_ENTER_PICTURE_IN_PICTURE = "PICTURE_IN_PICTURE";
    private static final int APP_OPS_MODE_ALLOWED = 0;
    private static final int APP_OPS_MODE_IGNORED = 1;
    private static final int APP_OPS_MODE_ERRORED = 2;

    private static final int ROTATION_0 = 0;
    private static final int ROTATION_90 = 1;
    private static final int ROTATION_180 = 2;
    private static final int ROTATION_270 = 3;

    // Corresponds to ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
    private static final int ORIENTATION_LANDSCAPE = 0;
    // Corresponds to ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
    private static final int ORIENTATION_PORTRAIT = 1;

    private static final float FLOAT_COMPARE_EPSILON = 0.005f;

    // Corresponds to com.android.internal.R.dimen.config_pictureInPictureMinAspectRatio
    private static final int MIN_ASPECT_RATIO_NUMERATOR = 100;
    private static final int MIN_ASPECT_RATIO_DENOMINATOR = 239;
    private static final int BELOW_MIN_ASPECT_RATIO_DENOMINATOR = MIN_ASPECT_RATIO_DENOMINATOR + 1;
    // Corresponds to com.android.internal.R.dimen.config_pictureInPictureMaxAspectRatio
    private static final int MAX_ASPECT_RATIO_NUMERATOR = 239;
    private static final int MAX_ASPECT_RATIO_DENOMINATOR = 100;
    private static final int ABOVE_MAX_ASPECT_RATIO_NUMERATOR = MAX_ASPECT_RATIO_NUMERATOR + 1;

    @Test
    public void testMinimumDeviceSize() throws Exception {
        assumeTrue(supportsPip());

        mAmWmState.assertDeviceDefaultDisplaySize(
                "Devices supporting picture-in-picture must be larger than the default minimum"
                        + " task size");
    }

    @Presubmit
    @Test
    public void testEnterPictureInPictureMode() throws Exception {
        pinnedStackTester(getAmStartCmd(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true"),
                PIP_ACTIVITY, PIP_ACTIVITY, false /* moveTopToPinnedStack */,
                false /* isFocusable */);
    }

    @FlakyTest(bugId = 71444628)
    @Presubmit
    @Test
    public void testMoveTopActivityToPinnedStack() throws Exception {
        pinnedStackTester(getAmStartCmd(PIP_ACTIVITY), PIP_ACTIVITY, PIP_ACTIVITY,
                true /* moveTopToPinnedStack */, false /* isFocusable */);
    }

    // This test is black-listed in cts-known-failures.xml (b/35314835).
    @Ignore
    @Test
    public void testAlwaysFocusablePipActivity() throws Exception {
        pinnedStackTester(getAmStartCmd(ALWAYS_FOCUSABLE_PIP_ACTIVITY),
                ALWAYS_FOCUSABLE_PIP_ACTIVITY, ALWAYS_FOCUSABLE_PIP_ACTIVITY,
                false /* moveTopToPinnedStack */, true /* isFocusable */);
    }

    // This test is black-listed in cts-known-failures.xml (b/35314835).
    @Ignore
    @Presubmit
    @Test
    public void testLaunchIntoPinnedStack() throws Exception {
        pinnedStackTester(getAmStartCmd(LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY),
                LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY, ALWAYS_FOCUSABLE_PIP_ACTIVITY,
                false /* moveTopToPinnedStack */, true /* isFocusable */);
    }

    @Test
    public void testNonTappablePipActivity() throws Exception {
        assumeTrue(supportsPip());

        // Launch the tap-to-finish activity at a specific place
        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP, "true",
                EXTRA_TAP_TO_FINISH, "true");
        // Wait for animation complete since we are tapping on specific bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Tap the screen at a known location in the pinned stack bounds, and ensure that it is
        // not passed down to the top task
        tapToFinishPip();
        mAmWmState.computeState(false /* compareTaskAndStackBounds */,
                new WaitForValidActivityState(PIP_ACTIVITY));
        mAmWmState.assertVisibility(PIP_ACTIVITY, true);
    }

    @Test
    public void testPinnedStackDefaultBounds() throws Exception {
        assumeTrue(supportsPip());

        // Launch a PIP activity
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);

        try (final RotationSession rotationSession = new RotationSession()) {
            rotationSession.set(ROTATION_0);

            WindowManagerState wmState = mAmWmState.getWmState();
            wmState.computeState();
            Rect defaultPipBounds = wmState.getDefaultPinnedStackBounds();
            Rect stableBounds = wmState.getStableBounds();
            assertTrue(defaultPipBounds.width() > 0 && defaultPipBounds.height() > 0);
            assertTrue(stableBounds.contains(defaultPipBounds));

            rotationSession.set(ROTATION_90);
            wmState = mAmWmState.getWmState();
            wmState.computeState();
            defaultPipBounds = wmState.getDefaultPinnedStackBounds();
            stableBounds = wmState.getStableBounds();
            assertTrue(defaultPipBounds.width() > 0 && defaultPipBounds.height() > 0);
            assertTrue(stableBounds.contains(defaultPipBounds));
        }
    }

    @Test
    public void testPinnedStackMovementBounds() throws Exception {
        assumeTrue(supportsPip());

        // Launch a PIP activity
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);

        try (final RotationSession rotationSession = new RotationSession()) {
            rotationSession.set(ROTATION_0);
            WindowManagerState wmState = mAmWmState.getWmState();
            wmState.computeState();
            Rect pipMovementBounds = wmState.getPinnedStackMovementBounds();
            Rect stableBounds = wmState.getStableBounds();
            assertTrue(pipMovementBounds.width() > 0 && pipMovementBounds.height() > 0);
            assertTrue(stableBounds.contains(pipMovementBounds));

            rotationSession.set(ROTATION_90);
            wmState = mAmWmState.getWmState();
            wmState.computeState();
            pipMovementBounds = wmState.getPinnedStackMovementBounds();
            stableBounds = wmState.getStableBounds();
            assertTrue(pipMovementBounds.width() > 0 && pipMovementBounds.height() > 0);
            assertTrue(stableBounds.contains(pipMovementBounds));
        }
    }

    @Test
    @FlakyTest // TODO: Reintroduce to presubmit once b/71508234 is resolved.
    public void testPinnedStackOutOfBoundsInsetsNonNegative() throws Exception {
        assumeTrue(supportsPip());

        final WindowManagerState wmState = mAmWmState.getWmState();

        // Launch an activity into the pinned stack
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true",
                EXTRA_TAP_TO_FINISH, "true");
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);

        // Get the display dimensions
        WindowManagerState.WindowState windowState = getWindowState(PIP_ACTIVITY);
        WindowManagerState.Display display = wmState.getDisplay(windowState.getDisplayId());
        Rect displayRect = display.getDisplayRect();

        // Move the pinned stack offscreen
        final int stackId = getPinnedStack().mStackId;
        final int top = 0;
        final int left = displayRect.width() - 200;
        resizeStack(stackId, left, top, left + 500, top + 500);

        // Ensure that the surface insets are not negative
        windowState = getWindowState(PIP_ACTIVITY);
        Rect contentInsets = windowState.getContentInsets();
        if (contentInsets != null) {
            assertTrue(contentInsets.left >= 0 && contentInsets.top >= 0
                    && contentInsets.width() >= 0 && contentInsets.height() >= 0);
        }
    }

    @Test
    public void testPinnedStackInBoundsAfterRotation() throws Exception {
        assumeTrue(supportsPip());

        // Launch an activity into the pinned stack
        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP, "true",
                EXTRA_TAP_TO_FINISH, "true");
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);

        // Ensure that the PIP stack is fully visible in each orientation
        try (final RotationSession rotationSession = new RotationSession()) {
            rotationSession.set(ROTATION_0);
            assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
            rotationSession.set(ROTATION_90);
            assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
            rotationSession.set(ROTATION_180);
            assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
            rotationSession.set(ROTATION_270);
            assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
        }
    }

    @Test
    public void testEnterPipToOtherOrientation() throws Exception {
        assumeTrue(supportsPip());

        // Launch a portrait only app on the fullscreen stack
        launchActivity(TEST_ACTIVITY,
                EXTRA_FIXED_ORIENTATION, String.valueOf(ORIENTATION_PORTRAIT));
        // Launch the PiP activity fixed as landscape
        launchActivity(PIP_ACTIVITY,
                EXTRA_PIP_ORIENTATION, String.valueOf(ORIENTATION_LANDSCAPE));
        // Enter PiP, and assert that the PiP is within bounds now that the device is back in
        // portrait
        executeShellCommand("am broadcast -a " + ACTION_ENTER_PIP);
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();
        assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
    }

    @Test
    public void testEnterPipAspectRatioMin() throws Exception {
        testEnterPipAspectRatio(MIN_ASPECT_RATIO_NUMERATOR, MIN_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testEnterPipAspectRatioMax() throws Exception {
        testEnterPipAspectRatio(MAX_ASPECT_RATIO_NUMERATOR, MAX_ASPECT_RATIO_DENOMINATOR);
    }

    private void testEnterPipAspectRatio(int num, int denom) throws Exception {
        assumeTrue(supportsPip());

        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP, "true",
                EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR, Integer.toString(num),
                EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR, Integer.toString(denom));
        // Wait for animation complete since we are comparing aspect ratio
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Assert that we have entered PIP and that the aspect ratio is correct
        Rect pinnedStackBounds = getPinnedStackBounds();
        assertFloatEquals((float) pinnedStackBounds.width() / pinnedStackBounds.height(),
                (float) num / denom);
    }

    @Test
    public void testResizePipAspectRatioMin() throws Exception {
        testResizePipAspectRatio(MIN_ASPECT_RATIO_NUMERATOR, MIN_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testResizePipAspectRatioMax() throws Exception {
        testResizePipAspectRatio(MAX_ASPECT_RATIO_NUMERATOR, MAX_ASPECT_RATIO_DENOMINATOR);
    }

    private void testResizePipAspectRatio(int num, int denom) throws Exception {
        assumeTrue(supportsPip());

        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP, "true",
                EXTRA_SET_ASPECT_RATIO_NUMERATOR, Integer.toString(num),
                EXTRA_SET_ASPECT_RATIO_DENOMINATOR, Integer.toString(denom));
        // Wait for animation complete since we are comparing aspect ratio
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();
        waitForValidAspectRatio(num, denom);
        Rect bounds = getPinnedStackBounds();
        assertFloatEquals((float) bounds.width() / bounds.height(), (float) num / denom);
    }

    @Test
    public void testEnterPipExtremeAspectRatioMin() throws Exception {
        testEnterPipExtremeAspectRatio(MIN_ASPECT_RATIO_NUMERATOR,
                BELOW_MIN_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testEnterPipExtremeAspectRatioMax() throws Exception {
        testEnterPipExtremeAspectRatio(ABOVE_MAX_ASPECT_RATIO_NUMERATOR,
                MAX_ASPECT_RATIO_DENOMINATOR);
    }

    private void testEnterPipExtremeAspectRatio(int num, int denom) throws Exception {
        assumeTrue(supportsPip());

        // Assert that we could not create a pinned stack with an extreme aspect ratio
        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP, "true",
                EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR, Integer.toString(num),
                EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR, Integer.toString(denom));
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testSetPipExtremeAspectRatioMin() throws Exception {
        testSetPipExtremeAspectRatio(MIN_ASPECT_RATIO_NUMERATOR,
                BELOW_MIN_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testSetPipExtremeAspectRatioMax() throws Exception {
        testSetPipExtremeAspectRatio(ABOVE_MAX_ASPECT_RATIO_NUMERATOR,
                MAX_ASPECT_RATIO_DENOMINATOR);
    }

    private void testSetPipExtremeAspectRatio(int num, int denom) throws Exception {
        assumeTrue(supportsPip());

        // Try to resize the a normal pinned stack to an extreme aspect ratio and ensure that
        // fails (the aspect ratio remains the same)
        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP, "true",
                EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR,
                        Integer.toString(MAX_ASPECT_RATIO_NUMERATOR),
                EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR,
                        Integer.toString(MAX_ASPECT_RATIO_DENOMINATOR),
                EXTRA_SET_ASPECT_RATIO_NUMERATOR, Integer.toString(num),
                EXTRA_SET_ASPECT_RATIO_DENOMINATOR, Integer.toString(denom));
        // Wait for animation complete since we are comparing aspect ratio
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();
        Rect pinnedStackBounds = getPinnedStackBounds();
        assertFloatEquals((float) pinnedStackBounds.width() / pinnedStackBounds.height(),
                (float) MAX_ASPECT_RATIO_NUMERATOR / MAX_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testDisallowPipLaunchFromStoppedActivity() throws Exception {
        assumeTrue(supportsPip());

        // Launch the bottom pip activity which will launch a new activity on top and attempt to
        // enter pip when it is stopped
        launchActivity(PIP_ON_STOP_ACTIVITY);

        // Wait for the bottom pip activity to be stopped
        mAmWmState.waitForActivityState(PIP_ON_STOP_ACTIVITY, STATE_STOPPED);

        // Assert that there is no pinned stack (that enterPictureInPicture() failed)
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testAutoEnterPictureInPicture() throws Exception {
        assumeTrue(supportsPip());

        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch the PIP activity on pause
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP_ON_PAUSE, "true");
        assertPinnedStackDoesNotExist();

        // Go home and ensure that there is a pinned stack
        launchHomeActivity();
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
    }

    @Test
    public void testAutoEnterPictureInPictureLaunchActivity() throws Exception {
        assumeTrue(supportsPip());

        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch the PIP activity on pause, and have it start another activity on
        // top of itself.  Wait for the new activity to be visible and ensure that the pinned stack
        // was not created in the process
        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP_ON_PAUSE, "true",
                EXTRA_START_ACTIVITY, getActivityName(NON_RESIZEABLE_ACTIVITY));
        mAmWmState.computeState(false /* compareTaskAndStackBounds */,
                new WaitForValidActivityState(NON_RESIZEABLE_ACTIVITY));
        assertPinnedStackDoesNotExist();

        // Go home while the pip activity is open and ensure the previous activity is not PIPed
        launchHomeActivity();
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testAutoEnterPictureInPictureFinish() throws Exception {
        assumeTrue(supportsPip());

        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch the PIP activity on pause, and set it to finish itself after
        // some period.  Wait for the previous activity to be visible, and ensure that the pinned
        // stack was not created in the process
        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP_ON_PAUSE, "true",
                EXTRA_FINISH_SELF_ON_RESUME, "true");
        assertPinnedStackDoesNotExist();
    }

    @Presubmit
    @Test
    public void testAutoEnterPictureInPictureAspectRatio() throws Exception {
        assumeTrue(supportsPip());

        // Launch the PIP activity on pause, and set the aspect ratio
        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP_ON_PAUSE, "true",
                EXTRA_SET_ASPECT_RATIO_NUMERATOR, Integer.toString(MAX_ASPECT_RATIO_NUMERATOR),
                EXTRA_SET_ASPECT_RATIO_DENOMINATOR, Integer.toString(MAX_ASPECT_RATIO_DENOMINATOR));

        // Go home while the pip activity is open to trigger auto-PIP
        launchHomeActivity();
        // Wait for animation complete since we are comparing aspect ratio
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        waitForValidAspectRatio(MAX_ASPECT_RATIO_NUMERATOR, MAX_ASPECT_RATIO_DENOMINATOR);
        Rect bounds = getPinnedStackBounds();
        assertFloatEquals((float) bounds.width() / bounds.height(),
                (float) MAX_ASPECT_RATIO_NUMERATOR / MAX_ASPECT_RATIO_DENOMINATOR);
    }

    @Presubmit
    @Test
    public void testAutoEnterPictureInPictureOverPip() throws Exception {
        assumeTrue(supportsPip());

        // Launch another PIP activity
        launchActivity(LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY);
        waitForEnterPip(ALWAYS_FOCUSABLE_PIP_ACTIVITY);
        assertPinnedStackExists();

        // Launch the PIP activity on pause
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP_ON_PAUSE, "true");

        // Go home while the PIP activity is open to try to trigger auto-enter PIP
        launchHomeActivity();
        assertPinnedStackExists();

        // Ensure that auto-enter pip failed and that the resumed activity in the pinned stack is
        // still the first activity
        final ActivityStack pinnedStack = getPinnedStack();
        assertEquals(1, pinnedStack.getTasks().size());
        assertEquals(getActivityName(ALWAYS_FOCUSABLE_PIP_ACTIVITY),
                pinnedStack.getTasks().get(0).mRealActivity);
    }

    @Presubmit
    @Test
    public void testDisallowMultipleTasksInPinnedStack() throws Exception {
        assumeTrue(supportsPip());

        // Launch a test activity so that we have multiple fullscreen tasks
        launchActivity(TEST_ACTIVITY);

        // Launch first PIP activity
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);

        // Launch second PIP activity
        launchActivity(PIP_ACTIVITY2, EXTRA_ENTER_PIP, "true");

        final ActivityStack pinnedStack = getPinnedStack();
        assertEquals(1, pinnedStack.getTasks().size());
        assertTrue(mAmWmState.getAmState().containsActivityInWindowingMode(
                PIP_ACTIVITY2, WINDOWING_MODE_PINNED));
        assertTrue(mAmWmState.getAmState().containsActivityInWindowingMode(
                PIP_ACTIVITY, WINDOWING_MODE_FULLSCREEN));
    }

    @Test
    public void testPipUnPipOverHome() throws Exception {
        assumeTrue(supportsPip());

        // Go home
        launchHomeActivity();
        // Launch an auto pip activity
        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP, "true",
                EXTRA_REENTER_PIP_ON_EXIT, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Relaunch the activity to fullscreen to trigger the activity to exit and re-enter pip
        launchActivity(PIP_ACTIVITY);
        mAmWmState.waitForWithAmState(amState ->
                amState.getFrontStackWindowingMode(DEFAULT_DISPLAY) == WINDOWING_MODE_FULLSCREEN,
                "Waiting for PIP to exit to fullscreen");
        mAmWmState.waitForWithAmState(amState ->
                amState.getFrontStackWindowingMode(DEFAULT_DISPLAY) == WINDOWING_MODE_PINNED,
                "Waiting to re-enter PIP");
        mAmWmState.assertHomeActivityVisible(true);
    }

    @Test
    public void testPipUnPipOverApp() throws Exception {
        assumeTrue(supportsPip());

        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch an auto pip activity
        launchActivity(PIP_ACTIVITY,
                EXTRA_ENTER_PIP, "true",
                EXTRA_REENTER_PIP_ON_EXIT, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Relaunch the activity to fullscreen to trigger the activity to exit and re-enter pip
        launchActivity(PIP_ACTIVITY);
        mAmWmState.waitForWithAmState(amState ->
                amState.getFrontStackWindowingMode(DEFAULT_DISPLAY) == WINDOWING_MODE_FULLSCREEN,
                "Waiting for PIP to exit to fullscreen");
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        mAmWmState.assertVisibility(TEST_ACTIVITY, true);
    }

    @Presubmit
    @Test
    public void testRemovePipWithNoFullscreenStack() throws Exception {
        assumeTrue(supportsPip());

        // Launch a pip activity
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Remove the stack and ensure that the task is now in the fullscreen stack (when no
        // fullscreen stack existed before)
        removeStacksInWindowingModes(WINDOWING_MODE_PINNED);
        assertPinnedStackStateOnMoveToFullscreen(PIP_ACTIVITY,
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME);
    }

    @Presubmit
    @Test
    public void testRemovePipWithVisibleFullscreenStack() throws Exception {
        assumeTrue(supportsPip());

        // Launch a fullscreen activity, and a pip activity over that
        launchActivity(TEST_ACTIVITY);
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Remove the stack and ensure that the task is placed in the fullscreen stack, behind the
        // top fullscreen activity
        removeStacksInWindowingModes(WINDOWING_MODE_PINNED);
        assertPinnedStackStateOnMoveToFullscreen(PIP_ACTIVITY,
                WINDOWING_MODE_FULLSCREEN, ACTIVITY_TYPE_STANDARD);
    }

    @FlakyTest(bugId = 70746098)
    @Presubmit
    @Test
    public void testRemovePipWithHiddenFullscreenStack() throws Exception {
        assumeTrue(supportsPip());

        // Launch a fullscreen activity, return home and while the fullscreen stack is hidden,
        // launch a pip activity over home
        launchActivity(TEST_ACTIVITY);
        launchHomeActivity();
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Remove the stack and ensure that the task is placed on top of the hidden fullscreen
        // stack, but that the home stack is still focused
        removeStacksInWindowingModes(WINDOWING_MODE_PINNED);
        assertPinnedStackStateOnMoveToFullscreen(PIP_ACTIVITY,
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME);
    }

    @Test
    public void testMovePipToBackWithNoFullscreenStack() throws Exception {
        assumeTrue(supportsPip());

        // Start with a clean slate, remove all the stacks but home
        removeStacksWithActivityTypes(ALL_ACTIVITY_TYPE_BUT_HOME);

        // Launch a pip activity
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Remove the stack and ensure that the task is now in the fullscreen stack (when no
        // fullscreen stack existed before)
        executeShellCommand("am broadcast -a " + ACTION_MOVE_TO_BACK);
        assertPinnedStackStateOnMoveToFullscreen(PIP_ACTIVITY,
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME);
    }

    @FlakyTest(bugId = 70906499)
    @Presubmit
    @Test
    public void testMovePipToBackWithVisibleFullscreenStack() throws Exception {
        assumeTrue(supportsPip());

        // Launch a fullscreen activity, and a pip activity over that
        launchActivity(TEST_ACTIVITY);
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Remove the stack and ensure that the task is placed in the fullscreen stack, behind the
        // top fullscreen activity
        executeShellCommand("am broadcast -a " + ACTION_MOVE_TO_BACK);
        assertPinnedStackStateOnMoveToFullscreen(PIP_ACTIVITY,
                WINDOWING_MODE_FULLSCREEN, ACTIVITY_TYPE_STANDARD);
    }

    @FlakyTest(bugId = 70906499)
    @Presubmit
    @Test
    public void testMovePipToBackWithHiddenFullscreenStack() throws Exception {
        assumeTrue(supportsPip());

        // Launch a fullscreen activity, return home and while the fullscreen stack is hidden,
        // launch a pip activity over home
        launchActivity(TEST_ACTIVITY);
        launchHomeActivity();
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Remove the stack and ensure that the task is placed on top of the hidden fullscreen
        // stack, but that the home stack is still focused
        executeShellCommand("am broadcast -a " + ACTION_MOVE_TO_BACK);
        assertPinnedStackStateOnMoveToFullscreen(
                PIP_ACTIVITY, WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME);
    }

    @Test
    public void testPinnedStackAlwaysOnTop() throws Exception {
        assumeTrue(supportsPip());

        // Launch activity into pinned stack and assert it's on top.
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
        assertPinnedStackIsOnTop();

        // Launch another activity in fullscreen stack and check that pinned stack is still on top.
        launchActivity(TEST_ACTIVITY);
        assertPinnedStackExists();
        assertPinnedStackIsOnTop();

        // Launch home and check that pinned stack is still on top.
        launchHomeActivity();
        assertPinnedStackExists();
        assertPinnedStackIsOnTop();
    }

    @Test
    public void testAppOpsDenyPipOnPause() throws Exception {
        assumeTrue(supportsPip());

        try (final AppOpsSession appOpsSession = new AppOpsSession(PIP_ACTIVITY)) {
            // Disable enter-pip and try to enter pip
            appOpsSession.setOpToMode(APP_OPS_OP_ENTER_PICTURE_IN_PICTURE, APP_OPS_MODE_IGNORED);

            // Launch the PIP activity on pause
            launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
            assertPinnedStackDoesNotExist();

            // Go home and ensure that there is no pinned stack
            launchHomeActivity();
            assertPinnedStackDoesNotExist();
        }
    }

    @Test
    public void testEnterPipFromTaskWithMultipleActivities() throws Exception {
        assumeTrue(supportsPip());

        // Try to enter picture-in-picture from an activity that has more than one activity in the
        // task and ensure that it works
        launchActivity(LAUNCH_ENTER_PIP_ACTIVITY);
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
    }

    @Test
    public void testEnterPipWithResumeWhilePausingActivityNoStop() throws Exception {
        assumeTrue(supportsPip());

        /*
         * Launch the resumeWhilePausing activity and ensure that the PiP activity did not get
         * stopped and actually went into the pinned stack.
         *
         * Note that this is a workaround because to trigger the path that we want to happen in
         * activity manager, we need to add the leaving activity to the stopping state, which only
         * happens when a hidden stack is brought forward. Normally, this happens when you go home,
         * but since we can't launch into the home stack directly, we have a workaround.
         *
         * 1) Launch an activity in a new dynamic stack
         * 2) Resize the dynamic stack to non-fullscreen bounds
         * 3) Start the PiP activity that will enter picture-in-picture when paused in the
         *    fullscreen stack
         * 4) Bring the activity in the dynamic stack forward to trigger PiP
         */
        int stackId = launchActivityInNewDynamicStack(RESUME_WHILE_PAUSING_ACTIVITY);
        resizeStack(stackId, 0, 0, 500, 500);
        // Launch an activity that will enter PiP when it is paused with a delay that is long enough
        // for the next resumeWhilePausing activity to finish resuming, but slow enough to not
        // trigger the current system pause timeout (currently 500ms)
        launchActivity(PIP_ACTIVITY, WINDOWING_MODE_FULLSCREEN,
                EXTRA_ENTER_PIP_ON_PAUSE, "true",
                EXTRA_ON_PAUSE_DELAY, "350",
                EXTRA_ASSERT_NO_ON_STOP_BEFORE_PIP, "true");
        launchActivity(RESUME_WHILE_PAUSING_ACTIVITY);
        assertPinnedStackExists();
    }

    @Test
    public void testDisallowEnterPipActivityLocked() throws Exception {
        assumeTrue(supportsPip());

        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP_ON_PAUSE, "true");
        ActivityTask task = mAmWmState.getAmState().getStandardStackByWindowingMode(
                WINDOWING_MODE_FULLSCREEN).getTopTask();

        // Lock the task and ensure that we can't enter picture-in-picture both explicitly and
        // when paused
        executeShellCommand("am task lock " + task.mTaskId);
        executeShellCommand("am broadcast -a " + ACTION_ENTER_PIP);
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackDoesNotExist();
        launchHomeActivity();
        assertPinnedStackDoesNotExist();
        executeShellCommand("am task lock stop");
    }

    @FlakyTest(bugId = 70328524)
    @Presubmit
    @Test
    public void testConfigurationChangeOrderDuringTransition() throws Exception {
        assumeTrue(supportsPip());

        // Launch a PiP activity and ensure configuration change only happened once, and that the
        // configuration change happened after the picture-in-picture and multi-window callbacks
        launchActivity(PIP_ACTIVITY);
        LogSeparator logSeparator = separateLogs();
        executeShellCommand("am broadcast -a " + ACTION_ENTER_PIP);
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
        waitForValidPictureInPictureCallbacks(PIP_ACTIVITY, logSeparator);
        assertValidPictureInPictureCallbackOrder(PIP_ACTIVITY, logSeparator);

        // Trigger it to go back to fullscreen and ensure that only triggered one configuration
        // change as well
        logSeparator = separateLogs();
        launchActivity(PIP_ACTIVITY);
        waitForValidPictureInPictureCallbacks(PIP_ACTIVITY, logSeparator);
        assertValidPictureInPictureCallbackOrder(PIP_ACTIVITY, logSeparator);
    }

    /** Helper class to save, set, and restore transition_animation_scale preferences. */
    private static class TransitionAnimationScaleSession extends SettingsSession<Float> {
        TransitionAnimationScaleSession() {
            super(Settings.Global.getUriFor(Settings.Global.TRANSITION_ANIMATION_SCALE),
                    Settings.Global::getFloat,
                    Settings.Global::putFloat);
        }

        @Override
        public void close() throws Exception {
            // Wait for the restored setting to apply before we continue on with the next test
            final CountDownLatch waitLock = new CountDownLatch(1);
            final Context context = InstrumentationRegistry.getTargetContext();
            context.getContentResolver().registerContentObserver(mUri, false,
                    new ContentObserver(new Handler(Looper.getMainLooper())) {
                        @Override
                        public void onChange(boolean selfChange) {
                            waitLock.countDown();
                        }
                    });
            super.close();
            if (!waitLock.await(2, TimeUnit.SECONDS)) {
                Log.i(TAG, "TransitionAnimationScaleSession value not restored");
            }
        }
    }

    @Test
    public void testEnterPipInterruptedCallbacks() throws Exception {
        assumeTrue(supportsPip());

        try (final TransitionAnimationScaleSession transitionAnimationScaleSession =
                new TransitionAnimationScaleSession()) {
            // Slow down the transition animations for this test
            transitionAnimationScaleSession.set(20f);

            // Launch a PiP activity
            launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
            // Wait until the PiP activity has moved into the pinned stack (happens before the
            // transition has started)
            waitForEnterPip(PIP_ACTIVITY);
            assertPinnedStackExists();

            // Relaunch the PiP activity back into fullscreen
            LogSeparator logSeparator = separateLogs();
            launchActivity(PIP_ACTIVITY);
            // Wait until the PiP activity is reparented into the fullscreen stack (happens after
            // the transition has finished)
            waitForExitPipToFullscreen(PIP_ACTIVITY);

            // Ensure that we get the callbacks indicating that PiP/MW mode was cancelled, but no
            // configuration change (since none was sent)
            final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(
                    PIP_ACTIVITY, logSeparator);
            assertEquals("onConfigurationChanged", 0, lifecycleCounts.mConfigurationChangedCount);
            assertEquals("onPictureInPictureModeChanged", 1,
                    lifecycleCounts.mPictureInPictureModeChangedCount);
            assertEquals("onMultiWindowModeChanged", 1,
                    lifecycleCounts.mMultiWindowModeChangedCount);
        }
    }

    @FlakyTest(bugId = 71564769)
    @Presubmit
    @Test
    public void testStopBeforeMultiWindowCallbacksOnDismiss() throws Exception {
        assumeTrue(supportsPip());

        // Launch a PiP activity
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Dismiss it
        LogSeparator logSeparator = separateLogs();
        removeStacksInWindowingModes(WINDOWING_MODE_PINNED);
        waitForExitPipToFullscreen(PIP_ACTIVITY);

        // Confirm that we get stop before the multi-window and picture-in-picture mode change
        // callbacks
        final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(PIP_ACTIVITY,
                logSeparator);
        assertEquals("onStop", 1, lifecycleCounts.mStopCount);
        assertEquals("onPictureInPictureModeChanged", 1,
                lifecycleCounts.mPictureInPictureModeChangedCount);
        assertEquals("onMultiWindowModeChanged", 1, lifecycleCounts.mMultiWindowModeChangedCount);
        final int lastStopLine = lifecycleCounts.mLastStopLineIndex;
        final int lastPipLine = lifecycleCounts.mLastPictureInPictureModeChangedLineIndex;
        final int lastMwLine = lifecycleCounts.mLastMultiWindowModeChangedLineIndex;
        assertThat("onStop should be before onPictureInPictureModeChanged",
                lastStopLine, lessThan(lastPipLine));
        assertThat("onPictureInPictureModeChanged should be before onMultiWindowModeChanged",
                lastPipLine, lessThan(lastMwLine));
    }

    @Test
    public void testPreventSetAspectRatioWhileExpanding() throws Exception {
        assumeTrue(supportsPip());

        // Launch the PiP activity
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);

        // Trigger it to go back to fullscreen and try to set the aspect ratio, and ensure that the
        // call to set the aspect ratio did not prevent the PiP from returning to fullscreen
        executeShellCommand("am broadcast -a " + ACTION_EXPAND_PIP
                + " -e " + EXTRA_SET_ASPECT_RATIO_WITH_DELAY_NUMERATOR + " 123456789"
                + " -e " + EXTRA_SET_ASPECT_RATIO_WITH_DELAY_DENOMINATOR + " 100000000");
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testSetRequestedOrientationWhilePinned() throws Exception {
        assumeTrue(supportsPip());

        // Launch the PiP activity fixed as portrait, and enter picture-in-picture
        launchActivity(PIP_ACTIVITY,
                EXTRA_PIP_ORIENTATION, String.valueOf(ORIENTATION_PORTRAIT),
                EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Request that the orientation is set to landscape
        executeShellCommand("am broadcast -a "
                + ACTION_SET_REQUESTED_ORIENTATION + " -e "
                + EXTRA_PIP_ORIENTATION + " "
                + String.valueOf(ORIENTATION_LANDSCAPE));

        // Launch the activity back into fullscreen and ensure that it is now in landscape
        launchActivity(PIP_ACTIVITY);
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        assertPinnedStackDoesNotExist();
        assertEquals(ORIENTATION_LANDSCAPE, mAmWmState.getWmState().getLastOrientation());
    }

    @Test
    public void testWindowButtonEntersPip() throws Exception {
        assumeTrue(supportsPip());
        assumeTrue(!mAmWmState.getAmState().isHomeRecentsComponent());

        // Launch the PiP activity trigger the window button, ensure that we have entered PiP
        launchActivity(PIP_ACTIVITY);
        pressWindowButton();
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
    }

    @Test
    public void testFinishPipActivityWithTaskOverlay() throws Exception {
        assumeTrue(supportsPip());

        // Launch PiP activity
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
        int taskId = mAmWmState.getAmState().getStandardStackByWindowingMode(
                WINDOWING_MODE_PINNED).getTopTask().mTaskId;

        // Ensure that we don't any any other overlays as a result of launching into PIP
        launchHomeActivity();

        // Launch task overlay activity into PiP activity task
        launchPinnedActivityAsTaskOverlay(TRANSLUCENT_TEST_ACTIVITY, taskId);

        // Finish the PiP activity and ensure that there is no pinned stack
        executeShellCommand("am broadcast -a " + ACTION_FINISH);
        waitForPinnedStackRemoved();
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testNoResumeAfterTaskOverlayFinishes() throws Exception {
        assumeTrue(supportsPip());

        // Launch PiP activity
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
        ActivityStack stack = mAmWmState.getAmState().getStandardStackByWindowingMode(
                WINDOWING_MODE_PINNED);
        int stackId = stack.mStackId;
        int taskId = stack.getTopTask().mTaskId;

        // Launch task overlay activity into PiP activity task
        launchPinnedActivityAsTaskOverlay(TRANSLUCENT_TEST_ACTIVITY, taskId);

        // Finish the task overlay activity while animating and ensure that the PiP activity never
        // got resumed
        LogSeparator logSeparator = separateLogs();
        executeShellCommand("am stack resize-animated " + stackId + " 20 20 500 500");
        executeShellCommand("am broadcast -a " + TEST_ACTIVITY_ACTION_FINISH_SELF);
        mAmWmState.waitFor((amState, wmState) ->
                        !amState.containsActivity(TRANSLUCENT_TEST_ACTIVITY),
                "Waiting for test activity to finish...");
        final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(PIP_ACTIVITY,
                logSeparator);
        assertEquals("onResume", 0, lifecycleCounts.mResumeCount);
        assertEquals("onPause", 0, lifecycleCounts.mPauseCount);
    }

    @Test
    public void testPinnedStackWithDockedStack() throws Exception {
        assumeTrue(supportsPip());
        assumeTrue(supportsSplitScreenMultiWindow());

        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPip(PIP_ACTIVITY);
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY)
                        .setRandomData(true)
                        .setMultipleTask(false)
        );
        mAmWmState.assertVisibility(PIP_ACTIVITY, true);
        mAmWmState.assertVisibility(LAUNCHING_ACTIVITY, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY, true);

        // Launch the activities again to take focus and make sure nothing is hidden
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY)
                        .setRandomData(true)
                        .setMultipleTask(false)
        );
        mAmWmState.assertVisibility(LAUNCHING_ACTIVITY, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY, true);

        // Go to recents to make sure that fullscreen stack is invisible
        // Some devices do not support recents or implement it differently (instead of using a
        // separate stack id or as an activity), for those cases the visibility asserts will be
        // ignored
        pressAppSwitchButtonAndWaitForRecents();
        mAmWmState.assertVisibility(LAUNCHING_ACTIVITY, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY, false);
    }

    @Test
    public void testLaunchTaskByComponentMatchMultipleTasks() throws Exception {
        assumeTrue(supportsPip());

        // Launch a fullscreen activity which will launch a PiP activity in a new task with the same
        // affinity
        launchActivity(TEST_ACTIVITY_WITH_SAME_AFFINITY);
        launchActivity(PIP_ACTIVITY_WITH_SAME_AFFINITY);
        assertPinnedStackExists();

        // Launch the root activity again...
        int rootActivityTaskId = mAmWmState.getAmState().getTaskByActivity(
                TEST_ACTIVITY_WITH_SAME_AFFINITY).mTaskId;
        launchHomeActivity();
        launchActivity(TEST_ACTIVITY_WITH_SAME_AFFINITY);

        // ...and ensure that the root activity task is found and reused, and that the pinned stack
        // is unaffected
        assertPinnedStackExists();
        mAmWmState.assertFocusedActivity("Expected root activity focused",
                TEST_ACTIVITY_WITH_SAME_AFFINITY);
        assertEquals(rootActivityTaskId, mAmWmState.getAmState().getTaskByActivity(
                TEST_ACTIVITY_WITH_SAME_AFFINITY).mTaskId);
    }

    @Test
    public void testLaunchTaskByAffinityMatchMultipleTasks() throws Exception {
        assumeTrue(supportsPip());

        // Launch a fullscreen activity which will launch a PiP activity in a new task with the same
        // affinity, and also launch another activity in the same task, while finishing itself. As
        // a result, the task will not have a component matching the same activity as what it was
        // started with
        launchActivity(TEST_ACTIVITY_WITH_SAME_AFFINITY,
                EXTRA_START_ACTIVITY, getActivityName(TEST_ACTIVITY),
                EXTRA_FINISH_SELF_ON_RESUME, "true");
        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(TEST_ACTIVITY)
                .setWindowingMode(WINDOWING_MODE_FULLSCREEN)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .build());
        launchActivity(PIP_ACTIVITY_WITH_SAME_AFFINITY);
        waitForEnterPip(PIP_ACTIVITY_WITH_SAME_AFFINITY);
        assertPinnedStackExists();

        // Launch the root activity again...
        int rootActivityTaskId = mAmWmState.getAmState().getTaskByActivity(
                TEST_ACTIVITY).mTaskId;
        launchHomeActivity();
        launchActivity(TEST_ACTIVITY_WITH_SAME_AFFINITY);

        // ...and ensure that even while matching purely by task affinity, the root activity task is
        // found and reused, and that the pinned stack is unaffected
        assertPinnedStackExists();
        mAmWmState.assertFocusedActivity("Expected root activity focused", TEST_ACTIVITY);
        assertEquals(rootActivityTaskId, mAmWmState.getAmState().getTaskByActivity(
                TEST_ACTIVITY).mTaskId);
    }

    @Test
    public void testLaunchTaskByAffinityMatchSingleTask() throws Exception {
        assumeTrue(supportsPip());

        // Launch an activity into the pinned stack with a fixed affinity
        launchActivity(TEST_ACTIVITY_WITH_SAME_AFFINITY,
                EXTRA_ENTER_PIP, "true",
                EXTRA_START_ACTIVITY, getActivityName(PIP_ACTIVITY),
                EXTRA_FINISH_SELF_ON_RESUME, "true");
        waitForEnterPip(TEST_ACTIVITY_WITH_SAME_AFFINITY);
        assertPinnedStackExists();

        // Launch the root activity again, of the matching task and ensure that we expand to
        // fullscreen
        int activityTaskId = mAmWmState.getAmState().getTaskByActivity(PIP_ACTIVITY).mTaskId;
        launchHomeActivity();
        launchActivity(TEST_ACTIVITY_WITH_SAME_AFFINITY);
        waitForExitPipToFullscreen(TEST_ACTIVITY_WITH_SAME_AFFINITY);
        assertPinnedStackDoesNotExist();
        assertEquals(activityTaskId, mAmWmState.getAmState().getTaskByActivity(
                PIP_ACTIVITY).mTaskId);
    }

    /** Test that reported display size corresponds to fullscreen after exiting PiP. */
    @FlakyTest
    @Presubmit
    @Test
    public void testDisplayMetricsPinUnpin() throws Exception {
        assumeTrue(supportsPip());

        LogSeparator logSeparator = separateLogs();
        launchActivity(TEST_ACTIVITY);
        final int defaultWindowingMode = mAmWmState.getAmState()
                .getTaskByActivity(TEST_ACTIVITY).getWindowingMode();
        final ReportedSizes initialSizes = getLastReportedSizesForActivity(TEST_ACTIVITY,
                logSeparator);
        final Rect initialAppBounds = readAppBounds(TEST_ACTIVITY, logSeparator);
        assertNotNull("Must report display dimensions", initialSizes);
        assertNotNull("Must report app bounds", initialAppBounds);

        logSeparator = separateLogs();
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        final ReportedSizes pinnedSizes = getLastReportedSizesForActivity(PIP_ACTIVITY,
                logSeparator);
        final Rect pinnedAppBounds = readAppBounds(PIP_ACTIVITY, logSeparator);
        assertNotEquals("Reported display size when pinned must be different from default",
                initialSizes, pinnedSizes);
        final Size initialAppSize = new Size(initialAppBounds.width(), initialAppBounds.height());
        final Size pinnedAppSize = new Size(pinnedAppBounds.width(), pinnedAppBounds.height());
        assertNotEquals("Reported app size when pinned must be different from default",
                initialAppSize, pinnedAppSize);

        logSeparator = separateLogs();
        launchActivity(PIP_ACTIVITY, defaultWindowingMode);
        final ReportedSizes finalSizes = getLastReportedSizesForActivity(PIP_ACTIVITY,
                logSeparator);
        final Rect finalAppBounds = readAppBounds(PIP_ACTIVITY, logSeparator);
        final Size finalAppSize = new Size(finalAppBounds.width(), finalAppBounds.height());
        assertEquals("Must report default size after exiting PiP", initialSizes, finalSizes);
        assertEquals("Must report default app size after exiting PiP", initialAppSize,
                finalAppSize);
    }

    @Presubmit
    @Test
    public void testEnterPictureInPictureSavePosition() throws Exception {
        assumeTrue(supportsPip());

        // Ensure we have static shelf offset by running this test over a non-home activity
        launchActivity(NO_RELAUNCH_ACTIVITY);
        mAmWmState.waitForActivityState(mAmWmState.getAmState().getHomeActivityName(),
                STATE_STOPPED);

        // Launch PiP activity with auto-enter PiP, save the default position of the PiP
        // (while the PiP is still animating sleep)
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Move the PiP to a new position on screen
        final Rect initialBounds = new Rect();
        final Rect offsetBounds = new Rect();
        offsetPipWithinMovementBounds(100 /* offsetY */, initialBounds, offsetBounds);

        // Expand the PiP back to fullscreen and back into PiP and ensure that it is in the same
        // position as before we expanded (and that the default bounds reflect that)
        executeShellCommand("am broadcast -a " + ACTION_EXPAND_PIP);
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        executeShellCommand("am broadcast -a " + ACTION_ENTER_PIP);
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        mAmWmState.computeState(true);
        // Due to rounding in how we save and apply the snap fraction we may be a pixel off, so just
        // account for that in this check
        offsetBounds.inset(-1, -1);
        assertTrue("Expected offsetBounds=" + offsetBounds + " to contain bounds="
                + getPinnedStackBounds(), offsetBounds.contains(getPinnedStackBounds()));

        // Expand the PiP, then launch an activity in a new task, and ensure that the PiP goes back
        // to the default position (and not the saved position) the next time it is launched
        executeShellCommand("am broadcast -a " + ACTION_EXPAND_PIP);
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        launchActivity(TEST_ACTIVITY);
        executeShellCommand("am broadcast -a " + TEST_ACTIVITY_ACTION_FINISH_SELF);
        mAmWmState.waitForActivityState(PIP_ACTIVITY, STATE_RESUMED);
        mAmWmState.waitForAppTransitionIdle();
        executeShellCommand("am broadcast -a " + ACTION_ENTER_PIP);
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();
        assertTrue("Expected initialBounds=" + initialBounds + " to equal bounds="
                + getPinnedStackBounds(), initialBounds.equals(getPinnedStackBounds()));
    }

    @Presubmit
    @Test
    @FlakyTest(bugId = 71792368)
    public void testEnterPictureInPictureDiscardSavedPositionOnFinish() throws Exception {
        assumeTrue(supportsPip());

        // Ensure we have static shelf offset by running this test over a non-home activity
        launchActivity(NO_RELAUNCH_ACTIVITY);
        mAmWmState.waitForActivityState(mAmWmState.getAmState().getHomeActivityName(),
                STATE_STOPPED);

        // Launch PiP activity with auto-enter PiP, save the default position of the PiP
        // (while the PiP is still animating sleep)
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Move the PiP to a new position on screen
        final Rect initialBounds = new Rect();
        final Rect offsetBounds = new Rect();
        offsetPipWithinMovementBounds(100 /* offsetY */, initialBounds, offsetBounds);

        // Finish the activity
        executeShellCommand("am broadcast -a " + ACTION_FINISH);
        waitForPinnedStackRemoved();
        assertPinnedStackDoesNotExist();

        // Ensure that starting the same PiP activity after it finished will go to the default
        // bounds
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();
        assertTrue("Expected initialBounds=" + initialBounds + " to equal bounds="
                + getPinnedStackBounds(), initialBounds.equals(getPinnedStackBounds()));
    }

    /**
     * Offsets the PiP in a direction by {@param offsetY} such that it is still within the movement
     * bounds.
     */
    private void offsetPipWithinMovementBounds(int offsetY, Rect initialBoundsOut,
            Rect offsetBoundsOut) {
        final ActivityStack stack = getPinnedStack();
        final Rect displayRect = mAmWmState.getWmState().getDisplay(stack.mDisplayId)
                .getDisplayRect();
        initialBoundsOut.set(getPinnedStackBounds());
        offsetBoundsOut.set(initialBoundsOut);
        if (initialBoundsOut.top < displayRect.centerY()) {
            // If the default gravity is top-aligned, offset down instead of up
            offsetBoundsOut.offset(0, offsetY);
        } else {
            offsetBoundsOut.offset(0, -offsetY);
        }
        resizeStack(stack.mStackId, offsetBoundsOut.left, offsetBoundsOut.top,
                offsetBoundsOut.right, offsetBoundsOut.bottom);
    }

    private static final Pattern sAppBoundsPattern = Pattern.compile(
            "(.+)mAppBounds=Rect\\((\\d+), (\\d+) - (\\d+), (\\d+)\\)(.*)");

    /** Read app bounds in last applied configuration from logs. */
    private Rect readAppBounds(ComponentName activityName, LogSeparator logSeparator) {
        final String[] lines = getDeviceLogsForComponents(logSeparator, getLogTag(activityName));
        for (int i = lines.length - 1; i >= 0; i--) {
            final String line = lines[i].trim();
            final Matcher matcher = sAppBoundsPattern.matcher(line);
            if (matcher.matches()) {
                final int left = Integer.parseInt(matcher.group(2));
                final int top = Integer.parseInt(matcher.group(3));
                final int right = Integer.parseInt(matcher.group(4));
                final int bottom = Integer.parseInt(matcher.group(5));
                return new Rect(left, top, right - left, bottom - top);
            }
        }
        return null;
    }

    /**
     * Called after the given {@param activityName} has been moved to the fullscreen stack. Ensures
     * that the stack matching the {@param windowingMode} and {@param activityType} is focused, and
     * checks the top and/or bottom tasks in the fullscreen stack if
     * {@param expectTopTaskHasActivity} or {@param expectBottomTaskHasActivity} are set
     * respectively.
     */
    private void assertPinnedStackStateOnMoveToFullscreen(ComponentName activityName,
            int windowingMode, int activityType) {
        mAmWmState.waitForFocusedStack(windowingMode, activityType);
        mAmWmState.assertFocusedStack("Wrong focused stack", windowingMode, activityType);
        mAmWmState.waitForActivityState(activityName, STATE_STOPPED);
        assertTrue(mAmWmState.getAmState().hasActivityState(activityName, STATE_STOPPED));
        assertTrue(mAmWmState.getAmState().containsActivityInWindowingMode(
                activityName, WINDOWING_MODE_FULLSCREEN));
        assertPinnedStackDoesNotExist();
    }

    /**
     * Asserts that the pinned stack bounds does not intersect with the IME bounds.
     */
    private void assertPinnedStackDoesNotIntersectIME() {
        // Ensure that the IME is visible
        WindowManagerState wmState = mAmWmState.getWmState();
        wmState.computeState();
        WindowManagerState.WindowState imeWinState = wmState.getInputMethodWindowState();
        assertTrue(imeWinState != null);

        // Ensure that the PIP movement is constrained by the display bounds intersecting the
        // non-IME bounds
        Rect imeContentFrame = imeWinState.getContentFrame();
        Rect imeContentInsets = imeWinState.getGivenContentInsets();
        Rect imeBounds = new Rect(imeContentFrame.left + imeContentInsets.left,
                imeContentFrame.top + imeContentInsets.top,
                imeContentFrame.right - imeContentInsets.width(),
                imeContentFrame.bottom - imeContentInsets.height());
        wmState.computeState();
        Rect pipMovementBounds = wmState.getPinnedStackMovementBounds();
        assertTrue(!Rect.intersects(pipMovementBounds, imeBounds));
    }

    /**
     * Asserts that the pinned stack bounds is contained in the display bounds.
     */
    private void assertPinnedStackActivityIsInDisplayBounds(ComponentName activityName) {
        final WindowManagerState.WindowState windowState = getWindowState(activityName);
        final WindowManagerState.Display display = mAmWmState.getWmState().getDisplay(
                windowState.getDisplayId());
        final Rect displayRect = display.getDisplayRect();
        final Rect pinnedStackBounds = getPinnedStackBounds();
        assertTrue(displayRect.contains(pinnedStackBounds));
    }

    /**
     * Asserts that the pinned stack exists.
     */
    private void assertPinnedStackExists() {
        mAmWmState.assertContainsStack("Must contain pinned stack.", WINDOWING_MODE_PINNED,
                ACTIVITY_TYPE_STANDARD);
    }

    /**
     * Asserts that the pinned stack does not exist.
     */
    private void assertPinnedStackDoesNotExist() {
        mAmWmState.assertDoesNotContainStack("Must not contain pinned stack.",
                WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
    }

    /**
     * Asserts that the pinned stack is the front stack.
     */
    private void assertPinnedStackIsOnTop() {
        mAmWmState.assertFrontStack("Pinned stack must always be on top.",
                WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
    }

    /**
     * Asserts that the activity received exactly one of each of the callbacks when entering and
     * exiting picture-in-picture.
     */
    private void assertValidPictureInPictureCallbackOrder(
            ComponentName activityName, LogSeparator logSeparator) {
        final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(activityName,
                logSeparator);

        assertEquals(getActivityName(activityName) + " onConfigurationChanged()",
                1, lifecycleCounts.mConfigurationChangedCount);
        assertEquals(getActivityName(activityName) + " onPictureInPictureModeChanged()",
                1, lifecycleCounts.mPictureInPictureModeChangedCount);
        assertEquals(getActivityName(activityName) + " onMultiWindowModeChanged",
                1, lifecycleCounts.mMultiWindowModeChangedCount);
        int lastPipLine = lifecycleCounts.mLastPictureInPictureModeChangedLineIndex;
        int lastMwLine = lifecycleCounts.mLastMultiWindowModeChangedLineIndex;
        int lastConfigLine = lifecycleCounts.mLastConfigurationChangedLineIndex;
        assertThat("onPictureInPictureModeChanged should be before onMultiWindowModeChanged",
                lastPipLine, lessThan(lastMwLine));
        assertThat("onMultiWindowModeChanged should be before onConfigurationChanged",
                lastMwLine, lessThan(lastConfigLine));
    }

    /**
     * Waits until the given activity has entered picture-in-picture mode (allowing for the
     * subsequent animation to start).
     */
    private void waitForEnterPip(ComponentName activityName) {
        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setWindowingMode(WINDOWING_MODE_PINNED)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .build());
    }

    /**
     * Waits until the picture-in-picture animation has finished.
     */
    private void waitForEnterPipAnimationComplete(ComponentName activityName) {
        waitForEnterPip(activityName);
        mAmWmState.waitFor((amState, wmState) -> {
                WindowStack stack = wmState.getStandardStackByWindowingMode(WINDOWING_MODE_PINNED);
                return stack != null && !stack.mAnimatingBounds;
            }, "Waiting for pinned stack bounds animation to finish");
    }

    /**
     * Waits until the pinned stack has been removed.
     */
    private void waitForPinnedStackRemoved() {
        mAmWmState.waitFor((amState, wmState) -> {
            return !amState.containsStack(WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD)
                    && !wmState.containsStack(WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
        }, "Waiting for pinned stack to be removed...");
    }

    /**
     * Waits until the picture-in-picture animation to fullscreen has finished.
     */
    private void waitForExitPipToFullscreen(ComponentName activityName) {
        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setWindowingMode(WINDOWING_MODE_FULLSCREEN)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .build());
    }

    /**
     * Waits until the expected picture-in-picture callbacks have been made.
     */
    private void waitForValidPictureInPictureCallbacks(ComponentName activityName,
            LogSeparator logSeparator) {
        mAmWmState.waitFor((amState, wmState) -> {
            final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(
                    activityName, logSeparator);
            return lifecycleCounts.mConfigurationChangedCount == 1 &&
                    lifecycleCounts.mPictureInPictureModeChangedCount == 1 &&
                    lifecycleCounts.mMultiWindowModeChangedCount == 1;
        }, "Waiting for picture-in-picture activity callbacks...");
    }

    private void waitForValidAspectRatio(int num, int denom) {
        // Hacky, but we need to wait for the auto-enter picture-in-picture animation to complete
        // and before we can check the pinned stack bounds
        mAmWmState.waitForWithAmState((state) -> {
            Rect bounds = state.getStandardStackByWindowingMode(WINDOWING_MODE_PINNED).getBounds();
            return floatEquals((float) bounds.width() / bounds.height(), (float) num / denom);
        }, "waitForValidAspectRatio");
    }

    /**
     * @return the window state for the given {@param activityName}'s window.
     */
    private WindowManagerState.WindowState getWindowState(ComponentName activityName) {
        String windowName = getWindowName(activityName);
        mAmWmState.computeState(activityName);
        final List<WindowManagerState.WindowState> tempWindowList =
                mAmWmState.getWmState().getMatchingVisibleWindowState(windowName);
        return tempWindowList.get(0);
    }

    /**
     * @return the current pinned stack.
     */
    private ActivityStack getPinnedStack() {
        return mAmWmState.getAmState().getStandardStackByWindowingMode(WINDOWING_MODE_PINNED);
    }

    /**
     * @return the current pinned stack bounds.
     */
    private Rect getPinnedStackBounds() {
        return getPinnedStack().getBounds();
    }

    /**
     * Compares two floats with a common epsilon.
     */
    private void assertFloatEquals(float actual, float expected) {
        if (!floatEquals(actual, expected)) {
            fail(expected + " not equal to " + actual);
        }
    }

    private boolean floatEquals(float a, float b) {
        return Math.abs(a - b) < FLOAT_COMPARE_EPSILON;
    }

    /**
     * Triggers a tap over the pinned stack bounds to trigger the PIP to close.
     */
    private void tapToFinishPip() {
        Rect pinnedStackBounds = getPinnedStackBounds();
        int tapX = pinnedStackBounds.left + pinnedStackBounds.width() - 100;
        int tapY = pinnedStackBounds.top + pinnedStackBounds.height() - 100;
        executeShellCommand(String.format("input tap %d %d", tapX, tapY));
    }

    /**
     * Launches the given {@param activityName} into the {@param taskId} as a task overlay.
     */
    private void launchPinnedActivityAsTaskOverlay(ComponentName activityName, int taskId) {
        executeShellCommand(getAmStartCmd(activityName) + " --task " + taskId + " --task-overlay");

        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setWindowingMode(WINDOWING_MODE_PINNED)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .build());
    }

    private static class AppOpsSession implements AutoCloseable {

        private final String mPackageName;

        AppOpsSession(ComponentName activityName) {
            mPackageName = activityName.getPackageName();
        }

        void setOpToMode(String op, int mode) {
            setAppOpsOpToMode(mPackageName, op, mode);
        }

        @Override
        public void close() {
            executeShellCommand("appops reset " + mPackageName);
        }

        /**
         * Sets an app-ops op for a given package to a given mode.
         */
        private void setAppOpsOpToMode(String packageName, String op, int mode) {
            executeShellCommand(String.format("appops set %s %s %d", packageName, op, mode));
        }
    }

    /**
     * TODO: Improve tests check to actually check that apps are not interactive instead of checking
     *       if the stack is focused.
     */
    private void pinnedStackTester(String startActivityCmd, ComponentName startActivity,
            ComponentName topActivityName, boolean moveTopToPinnedStack, boolean isFocusable) {
        executeShellCommand(startActivityCmd);
        mAmWmState.waitForValidState(startActivity);

        if (moveTopToPinnedStack) {
            final int stackId = mAmWmState.getAmState().getStackIdByActivity(topActivityName);

            assertNotEquals(stackId, INVALID_STACK_ID);
            executeShellCommand(getMoveToPinnedStackCommand(stackId));
        }

        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(topActivityName)
                .setWindowingMode(WINDOWING_MODE_PINNED)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .build());
        mAmWmState.computeState(true);

        if (supportsPip()) {
            final String windowName = getWindowName(topActivityName);
            assertPinnedStackExists();
            mAmWmState.assertFrontStack("Pinned stack must be the front stack.",
                    WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
            mAmWmState.assertVisibility(topActivityName, true);

            if (isFocusable) {
                mAmWmState.assertFocusedStack("Pinned stack must be the focused stack.",
                        WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
                mAmWmState.assertFocusedActivity(
                        "Pinned activity must be focused activity.", topActivityName);
                mAmWmState.assertFocusedWindow(
                        "Pinned window must be focused window.", windowName);
                // Not checking for resumed state here because PiP overlay can be launched on top
                // in different task by SystemUI.
            } else {
                // Don't assert that the stack is not focused as a focusable PiP overlay can be
                // launched on top as a task overlay by SystemUI.
                mAmWmState.assertNotFocusedActivity(
                        "Pinned activity can't be the focused activity.", topActivityName);
                mAmWmState.assertNotResumedActivity(
                        "Pinned activity can't be the resumed activity.", topActivityName);
                mAmWmState.assertNotFocusedWindow(
                        "Pinned window can't be focused window.", windowName);
            }
        } else {
            mAmWmState.assertDoesNotContainStack("Must not contain pinned stack.",
                    WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
        }
    }
}
