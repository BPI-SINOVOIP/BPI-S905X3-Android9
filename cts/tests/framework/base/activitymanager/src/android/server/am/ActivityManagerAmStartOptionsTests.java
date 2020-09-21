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

import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.Components.ENTRY_POINT_ALIAS_ACTIVITY;
import static android.server.am.Components.SINGLE_TASK_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.UiDeviceUtils.pressHomeButton;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.greaterThanOrEqualTo;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.fail;

import android.content.ComponentName;

import org.junit.Test;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityManagerAmStartOptionsTests
 */
public class ActivityManagerAmStartOptionsTests extends ActivityManagerTestBase {

    @Test
    public void testDashD() {
        // Run at least 2 rounds to verify that -D works with an existing process.
        // -D could fail in this case if the force stop of process is broken.
        int prevProcId = -1;
        for (int i = 0; i < 2; i++) {
            executeShellCommand("am start -n " + getActivityName(TEST_ACTIVITY) + " -D");

            mAmWmState.waitForDebuggerWindowVisible(TEST_ACTIVITY);
            int procId = mAmWmState.getAmState().getActivityProcId(TEST_ACTIVITY);

            assertThat("Invalid ProcId.", procId, greaterThanOrEqualTo(0));
            if (i > 0) {
                assertNotEquals("Run " + i + " didn't start new proc.", prevProcId, procId);
            }
            prevProcId = procId;
        }
    }

    @Test
    public void testDashW_Direct() throws Exception {
        testDashW(SINGLE_TASK_ACTIVITY, SINGLE_TASK_ACTIVITY);
    }

    @Test
    public void testDashW_Indirect() throws Exception {
        testDashW(ENTRY_POINT_ALIAS_ACTIVITY, SINGLE_TASK_ACTIVITY);
    }

    private void testDashW(final ComponentName entryActivity, final ComponentName actualActivity)
            throws Exception {
        // Test cold start
        startActivityAndVerifyResult(entryActivity, actualActivity, true);

        // Test warm start
        pressHomeButton();
        startActivityAndVerifyResult(entryActivity, actualActivity, false);

        // Test "hot" start (app already in front)
        startActivityAndVerifyResult(entryActivity, actualActivity, false);
    }

    private void startActivityAndVerifyResult(final ComponentName entryActivity,
            final ComponentName actualActivity, boolean shouldStart) {
        // See TODO below
        // final LogSeparator logSeparator = separateLogs();

        // Pass in different data only when cold starting. This is to make the intent
        // different in subsequent warm/hot launches, so that the entrypoint alias
        // activity is always started, but the actual activity is not started again
        // because of the NEW_TASK and singleTask flags.
        final String result = executeShellCommand(
                "am start -n " + getActivityName(entryActivity) + " -W"
                + (shouldStart ? " -d about:blank" : ""));

        // Verify shell command return value
        verifyShellOutput(result, actualActivity, shouldStart);

        // TODO: Disable logcat check for now.
        // Logcat of WM or AM tag could be lost (eg. chatty if earlier events generated
        // too many lines), and make the test look flaky. We need to either use event
        // log or swith to other mechanisms. Only verify shell output for now, it should
        // still catch most failures.

        // Verify adb logcat log
        //verifyLogcat(actualActivity, shouldStart, logSeparator);
    }

    private static final Pattern sNotStartedWarningPattern = Pattern.compile(
            "Warning: Activity not started(.*)");
    private static final Pattern sStatusPattern = Pattern.compile(
            "Status: (.*)");
    private static final Pattern sActivityPattern = Pattern.compile(
            "Activity: (.*)");
    private static final String sStatusOk = "ok";

    private void verifyShellOutput(
            final String result, final ComponentName activity, boolean shouldStart) {
        boolean warningFound = false;
        String status = null;
        String reportedActivity = null;

        final String[] lines = result.split("\\n");
        // Going from the end of logs to beginning in case if some other activity is started first.
        for (int i = lines.length - 1; i >= 0; i--) {
            final String line = lines[i].trim();
            Matcher matcher = sNotStartedWarningPattern.matcher(line);
            if (matcher.matches()) {
                warningFound = true;
                continue;
            }
            matcher = sStatusPattern.matcher(line);
            if (matcher.matches()) {
                status = matcher.group(1);
                continue;
            }
            matcher = sActivityPattern.matcher(line);
            if (matcher.matches()) {
                reportedActivity = matcher.group(1);
                continue;
            }
        }

        assertEquals("Status is ok", sStatusOk, status);
        assertEquals("Reported activity is " +  getActivityName(activity),
                getActivityName(activity), reportedActivity);

        if (shouldStart && warningFound) {
            fail("Should start new activity but brought something to front.");
        } else if (!shouldStart && !warningFound){
            fail("Should bring existing activity to front but started new activity.");
        }
    }
}
