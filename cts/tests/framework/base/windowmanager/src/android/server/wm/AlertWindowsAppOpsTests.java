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

package android.server.wm;

import android.app.AppOpsManager;
import android.os.Process;
import android.support.test.InstrumentationRegistry;
import android.platform.test.annotations.Presubmit;
import android.support.test.filters.FlakyTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import com.android.compatibility.common.util.AppOpsUtils;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import static android.support.test.InstrumentationRegistry.getContext;

import static android.app.AppOpsManager.MODE_ALLOWED;
import static android.app.AppOpsManager.MODE_ERRORED;
import static android.app.AppOpsManager.OPSTR_SYSTEM_ALERT_WINDOW;
import static android.app.AppOpsManager.OP_SYSTEM_ALERT_WINDOW;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

/**
 * Test whether system alert window properly interacts with app ops.
 *
 * Build/Install/Run: atest CtsWindowManagerDeviceTestCases:AlertWindowsAppOpsTests
 */
@Presubmit
@FlakyTest(detail = "Can be promoted to pre-submit once confirmed stable.")
@RunWith(AndroidJUnit4.class)
public class AlertWindowsAppOpsTests {
    private static final long APP_OP_CHANGE_TIMEOUT_MILLIS = TimeUnit.SECONDS.toMillis(2);

    @Rule
    public final ActivityTestRule<AlertWindowsAppOpsTestsActivity> mActivityRule =
            new ActivityTestRule<>(AlertWindowsAppOpsTestsActivity.class);

    @BeforeClass
    public static void grantSystemAlertWindowAccess() throws IOException {
        AppOpsUtils.setOpMode(getContext().getPackageName(),
                OPSTR_SYSTEM_ALERT_WINDOW, MODE_ALLOWED);
    }

    @AfterClass
    public static void revokeSystemAlertWindowAccess() throws IOException {
        AppOpsUtils.setOpMode(getContext().getPackageName(),
                OPSTR_SYSTEM_ALERT_WINDOW, MODE_ERRORED);
    }

    @Test
    public void testSystemAlertWindowAppOpsInitiallyAllowed() {
        final String packageName = getContext().getPackageName();
        final int uid = Process.myUid();

        final AppOpsManager appOpsManager = getContext().getSystemService(AppOpsManager.class);
        final AppOpsManager.OnOpActiveChangedListener listener = mock(
                AppOpsManager.OnOpActiveChangedListener.class);

        // Launch our activity.
        final AlertWindowsAppOpsTestsActivity activity = mActivityRule.getActivity();

        // Start watching for app op
        appOpsManager.startWatchingActive(new int[] {OP_SYSTEM_ALERT_WINDOW}, listener);

        // Assert the app op is not started
        assertFalse(appOpsManager.isOperationActive(OP_SYSTEM_ALERT_WINDOW, uid, packageName));


        // Show a system alert window.
        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                activity::showSystemAlertWindow);

        // The app op should start
        verify(listener, timeout(APP_OP_CHANGE_TIMEOUT_MILLIS)
                .only()).onOpActiveChanged(eq(OP_SYSTEM_ALERT_WINDOW),
                eq(uid), eq(packageName), eq(true));

        // The app op should be reported as started
        assertTrue(appOpsManager.isOperationActive(OP_SYSTEM_ALERT_WINDOW,
                uid, packageName));


        // Start with a clean slate
        reset(listener);

        // Hide a system alert window.
        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                activity::hideSystemAlertWindow);

        // The app op should finish
        verify(listener, timeout(APP_OP_CHANGE_TIMEOUT_MILLIS)
                .only()).onOpActiveChanged(eq(OP_SYSTEM_ALERT_WINDOW),
                eq(uid), eq(packageName), eq(false));

        // The app op should be reported as finished
        assertFalse(appOpsManager.isOperationActive(OP_SYSTEM_ALERT_WINDOW, uid, packageName));


        // Start with a clean slate
        reset(listener);

        // Stop watching for app op
        appOpsManager.stopWatchingActive(listener);

        // Show a system alert window
        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                activity::showSystemAlertWindow);

        // No other callbacks expected
        verify(listener, timeout(APP_OP_CHANGE_TIMEOUT_MILLIS).times(0))
                .onOpActiveChanged(eq(OP_SYSTEM_ALERT_WINDOW),
                        anyInt(), anyString(), anyBoolean());

        // The app op should be reported as started
        assertTrue(appOpsManager.isOperationActive(OP_SYSTEM_ALERT_WINDOW, uid, packageName));
    }
}