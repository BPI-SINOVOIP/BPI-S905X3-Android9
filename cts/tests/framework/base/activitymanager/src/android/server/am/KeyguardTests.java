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
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.server.am.ComponentNameUtils.getWindowName;
import static android.server.am.Components.BROADCAST_RECEIVER_ACTIVITY;
import static android.server.am.Components.BroadcastReceiverActivity.ACTION_TRIGGER_BROADCAST;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_DISMISS_KEYGUARD;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_DISMISS_KEYGUARD_METHOD;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_FINISH_BROADCAST;
import static android.server.am.Components.DISMISS_KEYGUARD_ACTIVITY;
import static android.server.am.Components.DISMISS_KEYGUARD_METHOD_ACTIVITY;
import static android.server.am.Components.KEYGUARD_LOCK_ACTIVITY;
import static android.server.am.Components.LAUNCHING_ACTIVITY;
import static android.server.am.Components.SHOW_WHEN_LOCKED_ACTIVITY;
import static android.server.am.Components.SHOW_WHEN_LOCKED_ATTR_ACTIVITY;
import static android.server.am.Components.SHOW_WHEN_LOCKED_DIALOG_ACTIVITY;
import static android.server.am.Components.SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY;
import static android.server.am.Components.SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.Components.TURN_SCREEN_ON_ATTR_DISMISS_KEYGUARD_ACTIVITY;
import static android.server.am.Components.TURN_SCREEN_ON_DISMISS_KEYGUARD_ACTIVITY;
import static android.server.am.UiDeviceUtils.pressBackButton;
import static android.server.am.UiDeviceUtils.pressHomeButton;
import static android.view.Surface.ROTATION_90;
import static android.view.WindowManager.LayoutParams.TYPE_WALLPAPER;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.platform.test.annotations.Presubmit;
import android.server.am.WindowManagerState.WindowState;

import org.junit.Before;
import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:KeyguardTests
 */
public class KeyguardTests extends KeyguardTestBase {

    // TODO(b/70247058): Use {@link Context#sendBroadcast(Intent).
    // Shell command to dismiss keyguard via {@link #BROADCAST_RECEIVER_ACTIVITY}.
    private static final String DISMISS_KEYGUARD_BROADCAST = "am broadcast -a "
            + ACTION_TRIGGER_BROADCAST + " --ez " + EXTRA_DISMISS_KEYGUARD + " true";
    // Shell command to dismiss keyguard via {@link #BROADCAST_RECEIVER_ACTIVITY} method.
    private static final String DISMISS_KEYGUARD_METHOD_BROADCAST = "am broadcast -a "
            + ACTION_TRIGGER_BROADCAST + " --ez " + EXTRA_DISMISS_KEYGUARD_METHOD + " true";
    // Shell command to finish {@link #BROADCAST_RECEIVER_ACTIVITY}.
    private static final String FINISH_ACTIVITY_BROADCAST = "am broadcast -a "
            + ACTION_TRIGGER_BROADCAST + " --ez " + EXTRA_FINISH_BROADCAST + " true";

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();

        assumeTrue(supportsInsecureLock());
        assertFalse(isUiModeLockedToVrHeadset());
    }

    @Test
    public void testKeyguardHidesActivity() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(TEST_ACTIVITY);
            mAmWmState.computeState(TEST_ACTIVITY);
            mAmWmState.assertVisibility(TEST_ACTIVITY, true);
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            assertTrue(mKeyguardManager.isKeyguardLocked());
            mAmWmState.assertVisibility(TEST_ACTIVITY, false);
        }
        assertFalse(mKeyguardManager.isKeyguardLocked());
    }

    @Test
    public void testShowWhenLockedActivity() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            mAmWmState.assertKeyguardShowingAndOccluded();
        }
    }

    /**
     * Tests whether dialogs from SHOW_WHEN_LOCKED activities are also visible if Keyguard is
     * showing.
     */
    @Test
    public void testShowWhenLockedActivity_withDialog() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY, true);
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY, true);
            assertTrue(mAmWmState.getWmState().allWindowsVisible(
                    getWindowName(SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY)));
            mAmWmState.assertKeyguardShowingAndOccluded();
        }
    }

    /**
     * Tests whether multiple SHOW_WHEN_LOCKED activities are shown if the topmost is translucent.
     */
    @Test
    public void testMultipleShowWhenLockedActivities() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            launchActivity(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ACTIVITY,
                    SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY, true);
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY, true);
            mAmWmState.assertKeyguardShowingAndOccluded();
        }
    }

    /**
     * If we have a translucent SHOW_WHEN_LOCKED_ACTIVITY, the wallpaper should also be showing.
     */
    @Test
    public void testTranslucentShowWhenLockedActivity() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY, true);
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY, true);
            assertWallpaperShowing();
            mAmWmState.assertKeyguardShowingAndOccluded();
        }
    }

    /**
     * If we have a translucent SHOW_WHEN_LOCKED activity, the activity behind should not be shown.
     */
    @Test
    public void testTranslucentDoesntRevealBehind() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(TEST_ACTIVITY);
            launchActivity(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY);
            mAmWmState.computeState(TEST_ACTIVITY, SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY);
            mAmWmState.assertVisibility(TEST_ACTIVITY, true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY, true);
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY, true);
            mAmWmState.assertVisibility(TEST_ACTIVITY, false);
            mAmWmState.assertKeyguardShowingAndOccluded();
        }
    }

    @Test
    public void testDialogShowWhenLockedActivity() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(SHOW_WHEN_LOCKED_DIALOG_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_DIALOG_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_DIALOG_ACTIVITY, true);
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_DIALOG_ACTIVITY, true);
            assertWallpaperShowing();
            mAmWmState.assertKeyguardShowingAndOccluded();
        }
    }

    /**
     * Test that showWhenLocked activity is fullscreen when shown over keyguard
     */
    @Test
    @Presubmit
    public void testShowWhenLockedActivityWhileSplit() throws Exception {
        assumeTrue(supportsSplitScreenMultiWindow());

        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivitiesInSplitScreen(
                    getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                    getLaunchActivityBuilder().setTargetActivity(SHOW_WHEN_LOCKED_ACTIVITY)
                            .setRandomData(true)
                            .setMultipleTask(false)
            );
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            mAmWmState.assertKeyguardShowingAndOccluded();
            mAmWmState.assertDoesNotContainStack("Activity must be full screen.",
                    WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);
        }
    }

    /**
     * Tests whether a FLAG_DISMISS_KEYGUARD activity occludes Keyguard.
     */
    @Test
    public void testDismissKeyguardActivity() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            launchActivity(DISMISS_KEYGUARD_ACTIVITY);
            mAmWmState.waitForKeyguardShowingAndOccluded();
            mAmWmState.computeState(DISMISS_KEYGUARD_ACTIVITY);
            mAmWmState.assertVisibility(DISMISS_KEYGUARD_ACTIVITY, true);
            mAmWmState.assertKeyguardShowingAndOccluded();
        }
    }

    @Test
    public void testDismissKeyguardActivity_method() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            final LogSeparator logSeparator = separateLogs();
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            launchActivity(DISMISS_KEYGUARD_METHOD_ACTIVITY);
            mAmWmState.waitForKeyguardGone();
            mAmWmState.computeState(DISMISS_KEYGUARD_METHOD_ACTIVITY);
            mAmWmState.assertVisibility(DISMISS_KEYGUARD_METHOD_ACTIVITY, true);
            assertFalse(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            assertOnDismissSucceededInLogcat(logSeparator);
        }
    }

    @Test
    public void testDismissKeyguardActivity_method_notTop() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            final LogSeparator logSeparator = separateLogs();
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            launchActivity(BROADCAST_RECEIVER_ACTIVITY);
            launchActivity(TEST_ACTIVITY);
            executeShellCommand(DISMISS_KEYGUARD_METHOD_BROADCAST);
            assertOnDismissErrorInLogcat(logSeparator);
        }
    }

    @Test
    public void testDismissKeyguardActivity_method_turnScreenOn() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            final LogSeparator logSeparator = separateLogs();
            lockScreenSession.sleepDevice();
            mAmWmState.computeState(true);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            launchActivity(TURN_SCREEN_ON_DISMISS_KEYGUARD_ACTIVITY);
            mAmWmState.waitForKeyguardGone();
            mAmWmState.computeState(TURN_SCREEN_ON_DISMISS_KEYGUARD_ACTIVITY);
            mAmWmState.assertVisibility(TURN_SCREEN_ON_DISMISS_KEYGUARD_ACTIVITY, true);
            assertFalse(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            assertOnDismissSucceededInLogcat(logSeparator);
            assertTrue(isDisplayOn());
        }
    }

    @Test
    public void testDismissKeyguard_fromShowWhenLocked_notAllowed() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.gotoKeyguard();
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            mAmWmState.assertKeyguardShowingAndOccluded();
            executeShellCommand(DISMISS_KEYGUARD_BROADCAST);
            mAmWmState.assertKeyguardShowingAndOccluded();
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
        }
    }

    @Test
    public void testKeyguardLock() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.gotoKeyguard();
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            launchActivity(KEYGUARD_LOCK_ACTIVITY);
            mAmWmState.computeState(KEYGUARD_LOCK_ACTIVITY);
            mAmWmState.assertVisibility(KEYGUARD_LOCK_ACTIVITY, true);
            executeShellCommand(FINISH_ACTIVITY_BROADCAST);
            mAmWmState.waitForKeyguardShowingAndNotOccluded();
            mAmWmState.assertKeyguardShowingAndNotOccluded();
        }
    }

    @Test
    public void testUnoccludeRotationChange() throws Exception {

        // Go home now to make sure Home is behind Keyguard.
        pressHomeButton();
        try (final LockScreenSession lockScreenSession = new LockScreenSession();
             final RotationSession rotationSession = new RotationSession()) {
            lockScreenSession.gotoKeyguard();
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);

            rotationSession.set(ROTATION_90);
            pressBackButton();
            mAmWmState.waitForKeyguardShowingAndNotOccluded();
            mAmWmState.waitForDisplayUnfrozen();
            mAmWmState.assertSanity();
            mAmWmState.assertHomeActivityVisible(false);
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, false);
        }
    }

    private void assertWallpaperShowing() {
        WindowState wallpaper =
                mAmWmState.getWmState().findFirstWindowWithType(TYPE_WALLPAPER);
        assertNotNull(wallpaper);
        assertTrue(wallpaper.isShown());
    }

    @Test
    public void testDismissKeyguardAttrActivity_method_turnScreenOn() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.sleepDevice();

            final LogSeparator logSeparator = separateLogs();
            mAmWmState.computeState(true);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            launchActivity(TURN_SCREEN_ON_ATTR_DISMISS_KEYGUARD_ACTIVITY);
            mAmWmState.waitForKeyguardGone();
            mAmWmState.assertVisibility(TURN_SCREEN_ON_ATTR_DISMISS_KEYGUARD_ACTIVITY, true);
            assertFalse(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            assertOnDismissSucceededInLogcat(logSeparator);
            assertTrue(isDisplayOn());
        }
    }

    @Test
    public void testScreenOffWhileOccludedStopsActivity() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            final LogSeparator logSeparator = separateLogs();
            lockScreenSession.gotoKeyguard();
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            launchActivity(SHOW_WHEN_LOCKED_ATTR_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ATTR_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ATTR_ACTIVITY, true);
            mAmWmState.assertKeyguardShowingAndOccluded();
            lockScreenSession.sleepDevice();
            assertSingleLaunchAndStop(SHOW_WHEN_LOCKED_ATTR_ACTIVITY, logSeparator);
        }
    }

    @Test
    public void testScreenOffCausesSingleStop() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            final LogSeparator logSeparator = separateLogs();
            launchActivity(TEST_ACTIVITY);
            mAmWmState.assertVisibility(TEST_ACTIVITY, true);
            lockScreenSession.sleepDevice();
            assertSingleLaunchAndStop(TEST_ACTIVITY, logSeparator);
        }
    }
}
