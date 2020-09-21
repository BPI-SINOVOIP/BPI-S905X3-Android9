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
import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.server.am.Components.BroadcastReceiverActivity.ACTION_TRIGGER_BROADCAST;
import static android.server.am.Components.BroadcastReceiverActivity.EXTRA_DISMISS_KEYGUARD;
import static android.server.am.Components.DISMISS_KEYGUARD_ACTIVITY;
import static android.server.am.Components.DISMISS_KEYGUARD_METHOD_ACTIVITY;
import static android.server.am.Components.PIP_ACTIVITY;
import static android.server.am.Components.PipActivity.ACTION_ENTER_PIP;
import static android.server.am.Components.PipActivity.EXTRA_ENTER_PIP;
import static android.server.am.Components.PipActivity.EXTRA_SHOW_OVER_KEYGUARD;
import static android.server.am.Components.SHOW_WHEN_LOCKED_ACTIVITY;
import static android.server.am.Components.TURN_SCREEN_ON_ATTR_DISMISS_KEYGUARD_ACTIVITY;
import static android.server.am.UiDeviceUtils.pressBackButton;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.content.ComponentName;

import org.junit.Before;
import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:KeyguardLockedTests
 */
public class KeyguardLockedTests extends KeyguardTestBase {

    // TODO(b/70247058): Use {@link Context#sendBroadcast(Intent).
    // Shell command to dismiss keyguard via {@link #BROADCAST_RECEIVER_ACTIVITY}.
    private static final String DISMISS_KEYGUARD_BROADCAST = "am broadcast -a "
            + ACTION_TRIGGER_BROADCAST + " --ez " + EXTRA_DISMISS_KEYGUARD + " true";

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();
        assumeTrue(supportsSecureLock());
    }

    @Test
    public void testLockAndUnlock() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential()
                    .gotoKeyguard();
            assertTrue(mKeyguardManager.isKeyguardLocked());
            assertTrue(mKeyguardManager.isDeviceLocked());
            assertTrue(mKeyguardManager.isDeviceSecure());
            assertTrue(mKeyguardManager.isKeyguardSecure());
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            lockScreenSession.unlockDevice()
                    .enterAndConfirmLockCredential();
            mAmWmState.waitForKeyguardGone();
            mAmWmState.assertKeyguardGone();
            assertFalse(mKeyguardManager.isDeviceLocked());
            assertFalse(mKeyguardManager.isKeyguardLocked());
        }
    }

    @Test
    public void testDismissKeyguard() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential()
                    .gotoKeyguard();
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            launchActivity(DISMISS_KEYGUARD_ACTIVITY);
            lockScreenSession.enterAndConfirmLockCredential();
            mAmWmState.waitForKeyguardGone();
            mAmWmState.assertKeyguardGone();
            mAmWmState.assertVisibility(DISMISS_KEYGUARD_ACTIVITY, true);
        }
    }

    @Test
    public void testDismissKeyguard_whileOccluded() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential()
                    .gotoKeyguard();
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            launchActivity(DISMISS_KEYGUARD_ACTIVITY);
            lockScreenSession.enterAndConfirmLockCredential();
            mAmWmState.waitForKeyguardGone();
            mAmWmState.assertKeyguardGone();
            boolean isDismissTranslucent =
                    mAmWmState.getAmState().isActivityTranslucent(DISMISS_KEYGUARD_ACTIVITY);
            mAmWmState.assertVisibility(DISMISS_KEYGUARD_ACTIVITY, true);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, isDismissTranslucent);
        }
    }

    @Test
    public void testDismissKeyguard_fromShowWhenLocked_notAllowed() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential()
                    .gotoKeyguard();
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            executeShellCommand(DISMISS_KEYGUARD_BROADCAST);
            lockScreenSession.enterAndConfirmLockCredential();

            // Make sure we stay on Keyguard.
            mAmWmState.assertKeyguardShowingAndOccluded();
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
        }
    }

    @Test
    public void testDismissKeyguardActivity_method() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential();
            final LogSeparator logSeparator = separateLogs();
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            launchActivity(DISMISS_KEYGUARD_METHOD_ACTIVITY);
            lockScreenSession.enterAndConfirmLockCredential();
            mAmWmState.waitForKeyguardGone();
            mAmWmState.computeState(DISMISS_KEYGUARD_METHOD_ACTIVITY);
            mAmWmState.assertVisibility(DISMISS_KEYGUARD_METHOD_ACTIVITY, true);
            assertFalse(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            assertOnDismissSucceededInLogcat(logSeparator);
        }
    }

    @Test
    public void testDismissKeyguardActivity_method_cancelled() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential();
            final LogSeparator logSeparator = separateLogs();
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            launchActivity(DISMISS_KEYGUARD_METHOD_ACTIVITY);
            pressBackButton();
            assertOnDismissCancelledInLogcat(logSeparator);
            mAmWmState.computeState(true);
            mAmWmState.assertVisibility(DISMISS_KEYGUARD_METHOD_ACTIVITY, false);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
        }
    }

    @Test
    public void testDismissKeyguardAttrActivity_method_turnScreenOn_withSecureKeyguard()
            throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential().sleepDevice();

            mAmWmState.computeState(true);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            launchActivity(TURN_SCREEN_ON_ATTR_DISMISS_KEYGUARD_ACTIVITY);
            mAmWmState.waitForKeyguardShowingAndNotOccluded();
            mAmWmState.assertVisibility(TURN_SCREEN_ON_ATTR_DISMISS_KEYGUARD_ACTIVITY, false);
            assertTrue(mAmWmState.getAmState().getKeyguardControllerState().keyguardShowing);
            assertTrue(isDisplayOn());
        }
    }

    @Test
    public void testEnterPipOverKeyguard() throws Exception {
        assumeTrue(supportsPip());

        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential();

            // Show the PiP activity in fullscreen
            launchActivity(PIP_ACTIVITY, EXTRA_SHOW_OVER_KEYGUARD, "true");

            // Go to the lockscreen
            lockScreenSession.gotoKeyguard();
            mAmWmState.waitForKeyguardShowingAndOccluded();
            mAmWmState.assertKeyguardShowingAndOccluded();

            // Request that the PiP activity enter picture-in-picture mode (ensure it does not)
            executeShellCommand("am broadcast -a " + ACTION_ENTER_PIP);
            waitForEnterPip(PIP_ACTIVITY);
            mAmWmState.assertDoesNotContainStack("Must not contain pinned stack.",
                    WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);

            // Enter the credentials and ensure that the activity actually entered picture-in
            // -picture
            lockScreenSession.enterAndConfirmLockCredential();
            mAmWmState.waitForKeyguardGone();
            mAmWmState.assertKeyguardGone();
            waitForEnterPip(PIP_ACTIVITY);
            mAmWmState.assertContainsStack("Must contain pinned stack.", WINDOWING_MODE_PINNED,
                    ACTIVITY_TYPE_STANDARD);
        }
    }

    @Test
    public void testShowWhenLockedActivityAndPipActivity() throws Exception {
        assumeTrue(supportsPip());

        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential();

            // Show an activity in PIP
            launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
            waitForEnterPip(PIP_ACTIVITY);
            mAmWmState.assertContainsStack("Must contain pinned stack.", WINDOWING_MODE_PINNED,
                    ACTIVITY_TYPE_STANDARD);
            mAmWmState.assertVisibility(PIP_ACTIVITY, true);

            // Show an activity that will keep above the keyguard
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);

            // Lock the screen and ensure that the fullscreen activity showing over the lockscreen
            // is visible, but not the PiP activity
            lockScreenSession.gotoKeyguard();
            mAmWmState.computeState(true);
            mAmWmState.assertKeyguardShowingAndOccluded();
            mAmWmState.assertVisibility(SHOW_WHEN_LOCKED_ACTIVITY, true);
            mAmWmState.assertVisibility(PIP_ACTIVITY, false);
        }
    }

    @Test
    public void testShowWhenLockedPipActivity() throws Exception {
        assumeTrue(supportsPip());

        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential();

            // Show an activity in PIP
            launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true",
                    EXTRA_SHOW_OVER_KEYGUARD, "true");
            waitForEnterPip(PIP_ACTIVITY);
            mAmWmState.assertContainsStack("Must contain pinned stack.", WINDOWING_MODE_PINNED,
                    ACTIVITY_TYPE_STANDARD);
            mAmWmState.assertVisibility(PIP_ACTIVITY, true);

            // Lock the screen and ensure the PiP activity is not visible on the lockscreen even
            // though it's marked as showing over the lockscreen itself
            lockScreenSession.gotoKeyguard();
            mAmWmState.assertKeyguardShowingAndNotOccluded();
            mAmWmState.assertVisibility(PIP_ACTIVITY, false);
        }
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
}
