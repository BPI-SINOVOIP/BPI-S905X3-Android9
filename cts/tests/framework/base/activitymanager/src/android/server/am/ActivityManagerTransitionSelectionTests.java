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

import static android.server.am.Components.BOTTOM_ACTIVITY;
import static android.server.am.Components.BottomActivity.EXTRA_BOTTOM_WALLPAPER;
import static android.server.am.Components.BottomActivity.EXTRA_STOP_DELAY;
import static android.server.am.Components.TOP_ACTIVITY;
import static android.server.am.Components.TRANSLUCENT_TOP_ACTIVITY;
import static android.server.am.Components.TopActivity.EXTRA_FINISH_DELAY;
import static android.server.am.Components.TopActivity.EXTRA_TOP_WALLPAPER;
import static android.server.am.WindowManagerState.TRANSIT_ACTIVITY_CLOSE;
import static android.server.am.WindowManagerState.TRANSIT_ACTIVITY_OPEN;
import static android.server.am.WindowManagerState.TRANSIT_TASK_CLOSE;
import static android.server.am.WindowManagerState.TRANSIT_TASK_OPEN;
import static android.server.am.WindowManagerState.TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE;
import static android.server.am.WindowManagerState.TRANSIT_WALLPAPER_CLOSE;
import static android.server.am.WindowManagerState.TRANSIT_WALLPAPER_INTRA_CLOSE;
import static android.server.am.WindowManagerState.TRANSIT_WALLPAPER_INTRA_OPEN;
import static android.server.am.WindowManagerState.TRANSIT_WALLPAPER_OPEN;

import static org.junit.Assert.assertEquals;

import android.content.ComponentName;
import android.os.SystemClock;
import android.platform.test.annotations.Presubmit;
import android.support.test.filters.FlakyTest;

import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;

/**
 * This test tests the transition type selection logic in ActivityManager/WindowManager.
 * BottomActivity is started first, then TopActivity, and we check the transition type that the
 * system selects when TopActivity enters or exits under various setups.
 *
 * Note that we only require the correct transition type to be reported (eg. TRANSIT_ACTIVITY_OPEN,
 * TRANSIT_TASK_CLOSE, TRANSIT_WALLPAPER_OPEN, etc.). The exact animation is unspecified and can be
 * overridden.
 *
 * <p>Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityManagerTransitionSelectionTests
 */
@Presubmit
@FlakyTest(bugId = 71792333)
public class ActivityManagerTransitionSelectionTests extends ActivityManagerTestBase {

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();

        // Transition selection tests are currently disabled on Wear because
        // config_windowSwipeToDismiss is set to true, which breaks all kinds of assumptions in the
        // transition selection logic.
        Assume.assumeTrue(!isWatch());
    }

    // Test activity open/close under normal timing
    @Test
    public void testOpenActivity_NeitherWallpaper() {
        testOpenActivity(false /*bottomWallpaper*/, false /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_ACTIVITY_OPEN);
    }

    @Test
    public void testCloseActivity_NeitherWallpaper() {
        testCloseActivity(false /*bottomWallpaper*/, false /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_ACTIVITY_CLOSE);
    }

    @Test
    public void testOpenActivity_BottomWallpaper() {
        testOpenActivity(true /*bottomWallpaper*/, false /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_WALLPAPER_CLOSE);
    }

    @Test
    public void testCloseActivity_BottomWallpaper() {
        testCloseActivity(true /*bottomWallpaper*/, false /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_WALLPAPER_OPEN);
    }

    @Test
    public void testOpenActivity_BothWallpaper() {
        testOpenActivity(true /*bottomWallpaper*/, true /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_WALLPAPER_INTRA_OPEN);
    }

    @Test
    public void testCloseActivity_BothWallpaper() {
        testCloseActivity(true /*bottomWallpaper*/, true /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_WALLPAPER_INTRA_CLOSE);
    }

    //------------------------------------------------------------------------//

    // Test task open/close under normal timing
    @Test
    public void testOpenTask_NeitherWallpaper() {
        testOpenTask(false /*bottomWallpaper*/, false /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_TASK_OPEN);
    }

    @FlakyTest(bugId = 71792333)
    @Test
    public void testCloseTask_NeitherWallpaper() {
        testCloseTask(false /*bottomWallpaper*/, false /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_TASK_CLOSE);
    }

    @Test
    public void testOpenTask_BottomWallpaper() {
        testOpenTask(true /*bottomWallpaper*/, false /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_WALLPAPER_CLOSE);
    }

    @Test
    public void testCloseTask_BottomWallpaper() {
        testCloseTask(true /*bottomWallpaper*/, false /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_WALLPAPER_OPEN);
    }

    @Test
    public void testOpenTask_BothWallpaper() {
        testOpenTask(true /*bottomWallpaper*/, true /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_WALLPAPER_INTRA_OPEN);
    }

    @Test
    public void testCloseTask_BothWallpaper() {
        testCloseTask(true /*bottomWallpaper*/, true /*topWallpaper*/,
                false /*slowStop*/, TRANSIT_WALLPAPER_INTRA_CLOSE);
    }

    //------------------------------------------------------------------------//

    // Test activity close -- bottom activity slow in stopping
    // These simulate the case where the bottom activity is resumed
    // before AM receives its activitiyStopped
    @Test
    public void testCloseActivity_NeitherWallpaper_SlowStop() {
        testCloseActivity(false /*bottomWallpaper*/, false /*topWallpaper*/,
                true /*slowStop*/, TRANSIT_ACTIVITY_CLOSE);
    }

    @Test
    public void testCloseActivity_BottomWallpaper_SlowStop() {
        testCloseActivity(true /*bottomWallpaper*/, false /*topWallpaper*/,
                true /*slowStop*/, TRANSIT_WALLPAPER_OPEN);
    }

    @Test
    public void testCloseActivity_BothWallpaper_SlowStop() {
        testCloseActivity(true /*bottomWallpaper*/, true /*topWallpaper*/,
                true /*slowStop*/, TRANSIT_WALLPAPER_INTRA_CLOSE);
    }

    //------------------------------------------------------------------------//

    // Test task close -- bottom task top activity slow in stopping
    // These simulate the case where the bottom activity is resumed
    // before AM receives its activitiyStopped
    @FlakyTest(bugId = 71792333)
    @Test
    public void testCloseTask_NeitherWallpaper_SlowStop() {
        testCloseTask(false /*bottomWallpaper*/, false /*topWallpaper*/,
                true /*slowStop*/, TRANSIT_TASK_CLOSE);
    }

    @Test
    public void testCloseTask_BottomWallpaper_SlowStop() {
        testCloseTask(true /*bottomWallpaper*/, false /*topWallpaper*/,
                true /*slowStop*/, TRANSIT_WALLPAPER_OPEN);
    }

    @Test
    public void testCloseTask_BothWallpaper_SlowStop() {
        testCloseTask(true /*bottomWallpaper*/, true /*topWallpaper*/,
                true /*slowStop*/, TRANSIT_WALLPAPER_INTRA_CLOSE);
    }

    //------------------------------------------------------------------------//

    /// Test closing of translucent activity/task
    @Test
    public void testCloseActivity_NeitherWallpaper_Translucent() {
        testCloseActivityTranslucent(false /*bottomWallpaper*/, false /*topWallpaper*/,
                TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE);
    }

    @Test
    public void testCloseActivity_BottomWallpaper_Translucent() {
        testCloseActivityTranslucent(true /*bottomWallpaper*/, false /*topWallpaper*/,
                TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE);
    }

    @Test
    public void testCloseActivity_BothWallpaper_Translucent() {
        testCloseActivityTranslucent(true /*bottomWallpaper*/, true /*topWallpaper*/,
                TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE);
    }

    @Test
    public void testCloseTask_NeitherWallpaper_Translucent() {
        testCloseTaskTranslucent(false /*bottomWallpaper*/, false /*topWallpaper*/,
                TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE);
    }

    @FlakyTest(bugId = 71792333)
    @Test
    public void testCloseTask_BottomWallpaper_Translucent() {
        testCloseTaskTranslucent(true /*bottomWallpaper*/, false /*topWallpaper*/,
                TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE);
    }

    @Test
    public void testCloseTask_BothWallpaper_Translucent() {
        testCloseTaskTranslucent(true /*bottomWallpaper*/, true /*topWallpaper*/,
                TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE);
    }

    //------------------------------------------------------------------------//

    private void testOpenActivity(boolean bottomWallpaper,
            boolean topWallpaper, boolean slowStop, String expectedTransit) {
        testTransitionSelection(true /*testOpen*/, false /*testNewTask*/,
                bottomWallpaper, topWallpaper, false /*topTranslucent*/, slowStop, expectedTransit);
    }

    private void testCloseActivity(boolean bottomWallpaper,
            boolean topWallpaper, boolean slowStop, String expectedTransit) {
        testTransitionSelection(false /*testOpen*/, false /*testNewTask*/,
                bottomWallpaper, topWallpaper, false /*topTranslucent*/, slowStop, expectedTransit);
    }

    private void testOpenTask(boolean bottomWallpaper,
            boolean topWallpaper, boolean slowStop, String expectedTransit) {
        testTransitionSelection(true /*testOpen*/, true /*testNewTask*/,
                bottomWallpaper, topWallpaper, false /*topTranslucent*/, slowStop, expectedTransit);
    }

    private void testCloseTask(boolean bottomWallpaper,
            boolean topWallpaper, boolean slowStop, String expectedTransit) {
        testTransitionSelection(false /*testOpen*/, true /*testNewTask*/,
                bottomWallpaper, topWallpaper, false /*topTranslucent*/, slowStop, expectedTransit);
    }

    private void testCloseActivityTranslucent(boolean bottomWallpaper,
            boolean topWallpaper, String expectedTransit) {
        testTransitionSelection(false /*testOpen*/, false /*testNewTask*/,
                bottomWallpaper, topWallpaper, true /*topTranslucent*/,
                false /*slowStop*/, expectedTransit);
    }

    private void testCloseTaskTranslucent(boolean bottomWallpaper,
            boolean topWallpaper, String expectedTransit) {
        testTransitionSelection(false /*testOpen*/, true /*testNewTask*/,
                bottomWallpaper, topWallpaper, true /*topTranslucent*/,
                false /*slowStop*/, expectedTransit);
    }

    //------------------------------------------------------------------------//

    private void testTransitionSelection(
            boolean testOpen, boolean testNewTask,
            boolean bottomWallpaper, boolean topWallpaper, boolean topTranslucent,
            boolean testSlowStop, String expectedTransit) {
        String bottomStartCmd = getAmStartCmd(BOTTOM_ACTIVITY);
        if (bottomWallpaper) {
            bottomStartCmd += " --ez " + EXTRA_BOTTOM_WALLPAPER + " true";
        }
        if (testSlowStop) {
            bottomStartCmd += " --ei " + EXTRA_STOP_DELAY + " 3000";
        }
        executeShellCommand(bottomStartCmd);

        mAmWmState.computeState(BOTTOM_ACTIVITY);

        final ComponentName topActivity = topTranslucent ? TRANSLUCENT_TOP_ACTIVITY : TOP_ACTIVITY;
        String topStartCmd = getAmStartCmd(topActivity);
        if (testNewTask) {
            topStartCmd += " -f 0x18000000";
        }
        if (topWallpaper) {
            topStartCmd += " --ez " + EXTRA_TOP_WALLPAPER + " true";
        }
        if (!testOpen) {
            topStartCmd += " --ei " + EXTRA_FINISH_DELAY + " 1000";
        }
        executeShellCommand(topStartCmd);

        SystemClock.sleep(5000);
        if (testOpen) {
            mAmWmState.computeState(topActivity);
        } else {
            mAmWmState.computeState(BOTTOM_ACTIVITY);
        }

        assertEquals("Picked wrong transition", expectedTransit,
                mAmWmState.getWmState().getLastTransition());
    }
}
