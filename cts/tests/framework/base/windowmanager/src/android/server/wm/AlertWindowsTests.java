/*
 * Copyright (C) 2017 The Android Open Source Project
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

import static android.app.AppOpsManager.MODE_ALLOWED;
import static android.app.AppOpsManager.MODE_ERRORED;
import static android.app.AppOpsManager.OPSTR_SYSTEM_ALERT_WINDOW;
import static android.server.wm.alertwindowapp.Components.ALERT_WINDOW_TEST_ACTIVITY;
import static android.server.wm.alertwindowappsdk25.Components.SDK25_ALERT_WINDOW_TEST_ACTIVITY;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.ComponentName;
import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.Presubmit;
import android.server.am.ActivityManagerTestBase;
import android.server.am.WaitForValidActivityState;
import android.server.am.WindowManagerState;

import com.android.compatibility.common.util.AppOpsUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.List;

/**
 * Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:AlertWindowsTests
 */
@Presubmit
@AppModeFull(reason = "Requires android.permission.MANAGE_ACTIVITY_STACKS")
public class AlertWindowsTests extends ActivityManagerTestBase {

    // From WindowManager.java
    private static final int TYPE_BASE_APPLICATION      = 1;
    private static final int FIRST_SYSTEM_WINDOW        = 2000;

    private static final int TYPE_PHONE                 = FIRST_SYSTEM_WINDOW + 2;
    private static final int TYPE_SYSTEM_ALERT          = FIRST_SYSTEM_WINDOW + 3;
    private static final int TYPE_SYSTEM_OVERLAY        = FIRST_SYSTEM_WINDOW + 6;
    private static final int TYPE_PRIORITY_PHONE        = FIRST_SYSTEM_WINDOW + 7;
    private static final int TYPE_SYSTEM_ERROR          = FIRST_SYSTEM_WINDOW + 10;
    private static final int TYPE_APPLICATION_OVERLAY   = FIRST_SYSTEM_WINDOW + 38;

    private static final int TYPE_STATUS_BAR            = FIRST_SYSTEM_WINDOW;
    private static final int TYPE_INPUT_METHOD          = FIRST_SYSTEM_WINDOW + 11;
    private static final int TYPE_NAVIGATION_BAR        = FIRST_SYSTEM_WINDOW + 19;

    private static final int[] ALERT_WINDOW_TYPES = {
            TYPE_PHONE,
            TYPE_PRIORITY_PHONE,
            TYPE_SYSTEM_ALERT,
            TYPE_SYSTEM_ERROR,
            TYPE_SYSTEM_OVERLAY,
            TYPE_APPLICATION_OVERLAY
    };
    private static final int[] SYSTEM_WINDOW_TYPES = {
            TYPE_STATUS_BAR,
            TYPE_INPUT_METHOD,
            TYPE_NAVIGATION_BAR
    };

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();
        resetPermissionState(ALERT_WINDOW_TEST_ACTIVITY);
        resetPermissionState(SDK25_ALERT_WINDOW_TEST_ACTIVITY);
    }

    @After
    @Override
    public void tearDown() throws Exception {
        super.tearDown();
        resetPermissionState(ALERT_WINDOW_TEST_ACTIVITY);
        resetPermissionState(SDK25_ALERT_WINDOW_TEST_ACTIVITY);
        stopTestPackage(ALERT_WINDOW_TEST_ACTIVITY);
        stopTestPackage(SDK25_ALERT_WINDOW_TEST_ACTIVITY);
    }

    @Test
    public void testAlertWindowAllowed() throws Exception {
        runAlertWindowTest(ALERT_WINDOW_TEST_ACTIVITY, true /* hasAlertWindowPermission */,
                true /* atLeastO */);
    }

    @Test
    public void testAlertWindowDisallowed() throws Exception {
        runAlertWindowTest(ALERT_WINDOW_TEST_ACTIVITY, false /* hasAlertWindowPermission */,
                true /* atLeastO */);
    }

    @Test
    public void testAlertWindowAllowedSdk25() throws Exception {
        runAlertWindowTest(SDK25_ALERT_WINDOW_TEST_ACTIVITY, true /* hasAlertWindowPermission */,
                false /* atLeastO */);
    }

    @Test
    public void testAlertWindowDisallowedSdk25() throws Exception {
        runAlertWindowTest(SDK25_ALERT_WINDOW_TEST_ACTIVITY, false /* hasAlertWindowPermission */,
                false /* atLeastO */);
    }

    private void runAlertWindowTest(final ComponentName activityName,
            final boolean hasAlertWindowPermission, final boolean atLeastO) throws Exception {
        setAlertWindowPermission(activityName, hasAlertWindowPermission);

        executeShellCommand(getAmStartCmd(activityName));
        mAmWmState.computeState(new WaitForValidActivityState(activityName));
        mAmWmState.assertVisibility(activityName, true);

        assertAlertWindows(activityName, hasAlertWindowPermission, atLeastO);
    }

    private boolean allWindowsHidden(List<WindowManagerState.WindowState> windows) {
        for (WindowManagerState.WindowState ws : windows) {
            if (ws.isShown()) {
                return false;
            }
        }
        return true;
    }

    private void assertAlertWindows(final ComponentName activityName,
            final boolean hasAlertWindowPermission, final boolean atLeastO) throws Exception {
        final String packageName = activityName.getPackageName();
        final WindowManagerState wmState = mAmWmState.getWmState();

        final List<WindowManagerState.WindowState> alertWindows =
                wmState.getWindowsByPackageName(packageName, ALERT_WINDOW_TYPES);

        if (!hasAlertWindowPermission) {
            // When running in VR Mode, an App Op restriction is
            // in place for SYSTEM_ALERT_WINDOW, which allows the window
            // to be created, but will be hidden instead.
            if (isUiModeLockedToVrHeadset()) {
                assertTrue("Should not be empty alertWindows=" + alertWindows,
                        !alertWindows.isEmpty());
                assertTrue("All alert windows should be hidden",
                        allWindowsHidden(alertWindows));
            } else {
                assertTrue("Should be empty alertWindows=" + alertWindows,
                        alertWindows.isEmpty());
                assertTrue(AppOpsUtils.rejectedOperationLogged(packageName,
                        OPSTR_SYSTEM_ALERT_WINDOW));
                return;
            }
        }

        if (atLeastO) {
            // Assert that only TYPE_APPLICATION_OVERLAY was created.
            for (WindowManagerState.WindowState win : alertWindows) {
                assertTrue("Can't create win=" + win + " on SDK O or greater",
                        win.getType() == TYPE_APPLICATION_OVERLAY);
            }
        }

        final WindowManagerState.WindowState mainAppWindow =
                wmState.getWindowByPackageName(packageName, TYPE_BASE_APPLICATION);

        assertNotNull(mainAppWindow);

        final WindowManagerState.WindowState lowestAlertWindow = alertWindows.get(0);
        final WindowManagerState.WindowState highestAlertWindow =
                alertWindows.get(alertWindows.size() - 1);

        // Assert that the alert windows have higher z-order than the main app window
        assertTrue("lowestAlertWindow=" + lowestAlertWindow + " less than mainAppWindow="
                + mainAppWindow,
                wmState.getZOrder(lowestAlertWindow) > wmState.getZOrder(mainAppWindow));

        // Assert that legacy alert windows have a lower z-order than the new alert window layer.
        final WindowManagerState.WindowState appOverlayWindow =
                wmState.getWindowByPackageName(packageName, TYPE_APPLICATION_OVERLAY);
        if (appOverlayWindow != null && highestAlertWindow != appOverlayWindow) {
            assertTrue("highestAlertWindow=" + highestAlertWindow
                            + " greater than appOverlayWindow=" + appOverlayWindow,
                    wmState.getZOrder(highestAlertWindow) < wmState.getZOrder(appOverlayWindow));
        }

        // Assert that alert windows are below key system windows.
        final List<WindowManagerState.WindowState> systemWindows =
                wmState.getWindowsByPackageName(packageName, SYSTEM_WINDOW_TYPES);
        if (!systemWindows.isEmpty()) {
            final WindowManagerState.WindowState lowestSystemWindow = alertWindows.get(0);
            assertTrue("highestAlertWindow=" + highestAlertWindow
                            + " greater than lowestSystemWindow=" + lowestSystemWindow,
                    wmState.getZOrder(highestAlertWindow) < wmState.getZOrder(lowestSystemWindow));
        }
        assertTrue(AppOpsUtils.allowedOperationLogged(packageName, OPSTR_SYSTEM_ALERT_WINDOW));
    }

    // Resets the permission states for a package to the system defaults.
    // Also clears the app operation logs for this package, required to test that displaying
    // the alert window gets logged.
    private void resetPermissionState(ComponentName activityName) throws Exception {
        AppOpsUtils.reset(activityName.getPackageName());
    }

    private void setAlertWindowPermission(final ComponentName activityName, final boolean allow)
            throws Exception {
        int mode = allow ? MODE_ALLOWED : MODE_ERRORED;
        AppOpsUtils.setOpMode(activityName.getPackageName(), OPSTR_SYSTEM_ALERT_WINDOW, mode);
    }
}
