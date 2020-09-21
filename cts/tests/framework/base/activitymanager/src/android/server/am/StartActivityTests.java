/*
 * Copyright (C) 2018 The Android Open Source Project
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

import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.app27.Components.SDK_27_LAUNCHING_ACTIVITY;

import static org.junit.Assert.assertFalse;

import android.app.Activity;
import android.content.ComponentName;
import android.platform.test.annotations.Presubmit;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.FlakyTest;
import android.support.test.rule.ActivityTestRule;

import org.junit.Rule;
import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:StartActivityTests
 */
@Presubmit
@FlakyTest
public class StartActivityTests extends ActivityManagerTestBase {

    @Rule
    public final ActivityTestRule<TestActivity2> mTestActivity2Rule =
            new ActivityTestRule<>(TestActivity2.class);

    /**
     * Ensures {@link Activity} can only be launched from an {@link Activity}
     * {@link android.content.Context}.
     */
    @Test
    public void testStartActivityContexts() throws Exception {
        // Launch Activity from application context.
        getLaunchActivityBuilder()
                .setTargetActivity(TEST_ACTIVITY)
                .setUseApplicationContext(true)
                .setSuppressExceptions(true)
                .execute();

        // Launch second Activity from Activity Context to ensure previous Activity has launched.
        final Activity testActivity2 = mTestActivity2Rule.launchActivity(null);

        mAmWmState.computeState(testActivity2.getComponentName());

        // Verify Activity was not started.
        assertFalse(mAmWmState.getAmState().containsActivity(TEST_ACTIVITY));
        mAmWmState.assertResumedActivity(
                "Activity launched from activity context should be present",
                testActivity2.getComponentName());
    }

    /**
     * Ensures you can start an {@link Activity} from a non {@link Activity}
     * {@link android.content.Context} with the {@code FLAG_ACTIVITY_NEW_TASK}.
     */
    @Test
    public void testStartActivityNewTask() throws Exception {
        // Launch Activity from application context.
        getLaunchActivityBuilder()
                .setTargetActivity(TEST_ACTIVITY)
                .setUseApplicationContext(true)
                .setSuppressExceptions(true)
                .setNewTask(true)
                .execute();

        mAmWmState.computeState(TEST_ACTIVITY);
        mAmWmState.assertResumedActivity("Test Activity should be started with new task flag",
                TEST_ACTIVITY);
    }

    /**
     * Ensures you can start an {@link Activity} from a non {@link Activity}
     * {@link android.content.Context} when the target sdk is between N and O Mr1.
     * @throws Exception
     */
    @Test
    public void testLegacyStartActivityFromNonActivityContext() throws Exception {
        getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY)
                .setLaunchingActivity(SDK_27_LAUNCHING_ACTIVITY)
                .setUseApplicationContext(true)
                .execute();

        mAmWmState.computeState(TEST_ACTIVITY);
        mAmWmState.assertResumedActivity("Test Activity should be resumed without older sdk",
                TEST_ACTIVITY);
    }

    public static class TestActivity2 extends Activity {
    }
}
