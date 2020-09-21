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

import static android.server.am.Components.SHOW_WHEN_LOCKED_ACTIVITY;
import static android.server.am.Components.SHOW_WHEN_LOCKED_ATTR_ACTIVITY;
import static android.server.am.Components.SHOW_WHEN_LOCKED_ATTR_REMOVE_ATTR_ACTIVITY;
import static android.server.am.Components.SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.Components.WALLPAPAER_ACTIVITY;
import static android.server.am.WindowManagerState.TRANSIT_ACTIVITY_OPEN;
import static android.server.am.WindowManagerState.TRANSIT_KEYGUARD_GOING_AWAY;
import static android.server.am.WindowManagerState.TRANSIT_KEYGUARD_GOING_AWAY_ON_WALLPAPER;
import static android.server.am.WindowManagerState.TRANSIT_KEYGUARD_OCCLUDE;
import static android.server.am.WindowManagerState.TRANSIT_KEYGUARD_UNOCCLUDE;

import static org.junit.Assert.assertEquals;
import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;

import org.junit.Before;
import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:KeyguardTransitionTests
 */
public class KeyguardTransitionTests extends ActivityManagerTestBase {

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();

        assumeTrue(supportsInsecureLock());
        assumeFalse(isUiModeLockedToVrHeadset());
    }

    @Test
    public void testUnlock() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(TEST_ACTIVITY);
            lockScreenSession.gotoKeyguard()
                    .unlockDevice();
            mAmWmState.computeState(TEST_ACTIVITY);
            assertEquals("Picked wrong transition", TRANSIT_KEYGUARD_GOING_AWAY,
                    mAmWmState.getWmState().getLastTransition());
        }
    }

    @Test
    public void testUnlockWallpaper() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(WALLPAPAER_ACTIVITY);
            lockScreenSession.gotoKeyguard()
                    .unlockDevice();
            mAmWmState.computeState(WALLPAPAER_ACTIVITY);
            assertEquals("Picked wrong transition", TRANSIT_KEYGUARD_GOING_AWAY_ON_WALLPAPER,
                    mAmWmState.getWmState().getLastTransition());
        }
    }

    @Test
    public void testOcclude() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.gotoKeyguard();
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ACTIVITY);
            assertEquals("Picked wrong transition", TRANSIT_KEYGUARD_OCCLUDE,
                    mAmWmState.getWmState().getLastTransition());
        }
    }

    @Test
    public void testUnocclude() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.gotoKeyguard();
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            launchActivity(TEST_ACTIVITY);
            mAmWmState.waitForKeyguardShowingAndNotOccluded();
            mAmWmState.computeState(true);
            assertEquals("Picked wrong transition", TRANSIT_KEYGUARD_UNOCCLUDE,
                    mAmWmState.getWmState().getLastTransition());
        }
    }

    @Test
    public void testNewActivityDuringOccluded() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(SHOW_WHEN_LOCKED_ACTIVITY);
            lockScreenSession.gotoKeyguard();
            launchActivity(SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY);
            assertEquals("Picked wrong transition", TRANSIT_ACTIVITY_OPEN,
                    mAmWmState.getWmState().getLastTransition());
        }
    }

    @Test
    public void testOccludeManifestAttr() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.gotoKeyguard();
            final LogSeparator logSeparator = separateLogs();
            launchActivity(SHOW_WHEN_LOCKED_ATTR_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ATTR_ACTIVITY);
            assertEquals("Picked wrong transition", TRANSIT_KEYGUARD_OCCLUDE,
                    mAmWmState.getWmState().getLastTransition());
            assertSingleLaunch(SHOW_WHEN_LOCKED_ATTR_ACTIVITY, logSeparator);
        }
    }

    @Test
    public void testOccludeAttrRemove() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.gotoKeyguard();
            LogSeparator logSeparator = separateLogs();
            launchActivity(SHOW_WHEN_LOCKED_ATTR_REMOVE_ATTR_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ATTR_REMOVE_ATTR_ACTIVITY);
            assertEquals("Picked wrong transition", TRANSIT_KEYGUARD_OCCLUDE,
                    mAmWmState.getWmState().getLastTransition());
            assertSingleLaunch(SHOW_WHEN_LOCKED_ATTR_REMOVE_ATTR_ACTIVITY, logSeparator);

            lockScreenSession.gotoKeyguard();
            logSeparator = separateLogs();
            launchActivity(SHOW_WHEN_LOCKED_ATTR_REMOVE_ATTR_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_ATTR_REMOVE_ATTR_ACTIVITY);
            assertSingleStartAndStop(SHOW_WHEN_LOCKED_ATTR_REMOVE_ATTR_ACTIVITY, logSeparator);
        }
    }

    @Test
    public void testNewActivityDuringOccludedWithAttr() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            launchActivity(SHOW_WHEN_LOCKED_ATTR_ACTIVITY);
            lockScreenSession.gotoKeyguard();
            launchActivity(SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY);
            mAmWmState.computeState(SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY);
            assertEquals("Picked wrong transition", TRANSIT_ACTIVITY_OPEN,
                    mAmWmState.getWmState().getLastTransition());
        }
    }
}
