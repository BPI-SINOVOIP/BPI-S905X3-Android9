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

package android.server.am.lifecycle;

import android.app.Activity;
import android.content.Intent;
import android.server.am.lifecycle.ActivityLifecycleClientTestBase;
import org.junit.Test;

import static android.server.am.ActivityManagerState.STATE_RESUMED;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_RESUME;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityStarterTests
 */
public class ActivityStarterTests extends ActivityLifecycleClientTestBase {
    /**
     * Ensures that the following launch flag combination works when starting an activity which is
     * already running:
     * - {@code FLAG_ACTIVITY_CLEAR_TOP}
     * - {@code FLAG_ACTIVITY_RESET_TASK_IF_NEEDED}
     * - {@code FLAG_ACTIVITY_NEW_TASK}
     */
    @Test
    public void testClearTopNewTaskResetTask() throws Exception {
        // Start activity normally
        final Activity initialActivity = mFirstActivityTestRule.launchActivity(new Intent());
        waitAndAssertActivityStates(state(initialActivity, ON_RESUME));

        // Navigate home
        launchHomeActivity();

        // Start activity again with flags in question. Verify activity is resumed.
        final Intent intent = new Intent().addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP
                | Intent.FLAG_ACTIVITY_NEW_TASK
                | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
        final Activity secondLaunchActivity = mFirstActivityTestRule.launchActivity(intent);
        waitAndAssertActivityStates(state(secondLaunchActivity, ON_RESUME));
    }
}
