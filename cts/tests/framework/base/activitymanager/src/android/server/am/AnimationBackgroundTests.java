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
 * limitations under the License.
 */

package android.server.am;

import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.server.am.Components.ANIMATION_TEST_ACTIVITY;
import static android.server.am.Components.LAUNCHING_ACTIVITY;
import static android.view.Display.DEFAULT_DISPLAY;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:AnimationBackgroundTests
 */
public class AnimationBackgroundTests extends ActivityManagerTestBase {

    @Test
    public void testAnimationBackground_duringAnimation() throws Exception {
        launchActivityOnDisplay(LAUNCHING_ACTIVITY, DEFAULT_DISPLAY);
        getLaunchActivityBuilder()
                .setTargetActivity(ANIMATION_TEST_ACTIVITY)
                .setWaitForLaunched(false)
                .execute();

        // Make sure we are in the middle of the animation.
        mAmWmState.waitForWithWmState(state -> state
                .getStandardStackByWindowingMode(WINDOWING_MODE_FULLSCREEN)
                        .isWindowAnimationBackgroundSurfaceShowing(),
                "***Waiting for animation background showing");
        assertTrue("window animation background needs to be showing", mAmWmState.getWmState()
                .getStandardStackByWindowingMode(WINDOWING_MODE_FULLSCREEN)
                .isWindowAnimationBackgroundSurfaceShowing());
    }

    @Test
    public void testAnimationBackground_gone() throws Exception {
        launchActivityOnDisplay(LAUNCHING_ACTIVITY, DEFAULT_DISPLAY);
        getLaunchActivityBuilder().setTargetActivity(ANIMATION_TEST_ACTIVITY).execute();
        mAmWmState.computeState(ANIMATION_TEST_ACTIVITY);
        mAmWmState.waitForAppTransitionIdle();
        assertFalse("window animation background needs to be gone", mAmWmState.getWmState()
                .getStandardStackByWindowingMode(WINDOWING_MODE_FULLSCREEN)
                .isWindowAnimationBackgroundSurfaceShowing());
    }
}
