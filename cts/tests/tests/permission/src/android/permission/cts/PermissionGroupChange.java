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
 * limitations under the License.
 */

package android.permission.cts;

import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;
import static android.content.pm.PackageInfo.REQUESTED_PERMISSION_GRANTED;
import static android.content.pm.PackageManager.GET_PERMISSIONS;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.platform.test.annotations.SecurityTest;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;
import android.widget.ScrollView;

import com.android.compatibility.common.util.SystemUtil;

import org.junit.Before;
import org.junit.Test;

import java.util.concurrent.TimeUnit;

public class PermissionGroupChange {
    private static final String APP_PKG_NAME = "android.permission.cts.appthatrequestpermission";
    private static final long EXPECTED_BEHAVIOR_TIMEOUT_SEC = 15;
    private static final long UNEXPECTED_BEHAVIOR_TIMEOUT_SEC = 2;

    private Context mContext;
    private UiDevice mUiDevice;

    @Before
    public void setContextAndUiDevice() {
        mContext = InstrumentationRegistry.getTargetContext();
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
    }

    @Before
    public void wakeUpScreen() {
        SystemUtil.runShellCommand("input keyevent KEYCODE_WAKEUP");
    }

    /**
     * Retry for a time until the runnable stops throwing.
     *
     * @param runnable The condition to execute
     * @param timeoutSec The time to try
     */
    private void eventually(ThrowingRunnable runnable, long timeoutSec) throws Throwable {
        long startTime = System.nanoTime();
        while (true) {
            try {
                runnable.run();
                return;
            } catch (Throwable t) {
                if (System.nanoTime() - startTime < TimeUnit.SECONDS.toNanos(timeoutSec)) {
                    Thread.sleep(100);
                    continue;
                }

                throw t;
            }
        }
    }


    private void scrollToBottomIfWatch() throws Exception {
        if (mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            UiScrollable scrollable = new UiScrollable(
                    new UiSelector().className(ScrollView.class));
            if (scrollable.exists()) {
                scrollable.flingToEnd(10);
            }
        }
    }

    protected void clickAllowButton() throws Exception {
        mUiDevice.findObject(new UiSelector().resourceId(
                "com.android.packageinstaller:id/permission_allow_button")).click();
    }

    private void grantPermissionViaUi() throws Throwable {
        eventually(() -> {
            scrollToBottomIfWatch();
            clickAllowButton();
        }, EXPECTED_BEHAVIOR_TIMEOUT_SEC);
    }

    private void waitUntilPermissionGranted(String permName, long timeoutSec) throws Throwable {
        eventually(() -> {
            PackageInfo appInfo = mContext.getPackageManager().getPackageInfo(APP_PKG_NAME,
                    GET_PERMISSIONS);

            for (int i = 0; i < appInfo.requestedPermissions.length; i++) {
                if (appInfo.requestedPermissions[i].equals(permName)
                        && ((appInfo.requestedPermissionsFlags[i] & REQUESTED_PERMISSION_GRANTED)
                        != 0)) {
                    return;
                }
            }

            fail(permName + " not granted");
        }, timeoutSec);
    }

    private void installApp(String apk) {
        String installResult = SystemUtil.runShellCommand(
                "pm install -r data/local/tmp/cts/permissions/" + apk + ".apk");
        assertEquals("Success", installResult.trim());
    }

    /**
     * Start the app. The app will request the permissions.
     */
    private void startApp() {
        Intent startApp = new Intent();
        startApp.setComponent(new ComponentName(APP_PKG_NAME, APP_PKG_NAME + ".RequestPermission"));
        startApp.setFlags(FLAG_ACTIVITY_NEW_TASK);

        mContext.startActivity(startApp);
    }

    @SecurityTest
    @Test
    public void permissionGroupShouldNotBeAutoGrantedIfNewMember() throws Throwable {
        installApp("CtsAppThatRequestsPermissionAandB");

        startApp();
        grantPermissionViaUi();
        waitUntilPermissionGranted("android.permission.cts.appthatrequestpermission.A",
                EXPECTED_BEHAVIOR_TIMEOUT_SEC);

        // Update app which changes the permission group of "android.permission.cts
        // .appthatrequestpermission.A" to the same as "android.permission.cts.C"
        installApp("CtsAppThatRequestsPermissionAandC");

        startApp();
        try {
            // The permission should not be auto-granted
            waitUntilPermissionGranted("android.permission.cts.C", UNEXPECTED_BEHAVIOR_TIMEOUT_SEC);
            fail("android.permission.cts.C was auto-granted");
        } catch (Throwable expected) {
            assertEquals("android.permission.cts.C not granted", expected.getMessage());
        }
    }

    private interface ThrowingRunnable {
        void run() throws Throwable;
    }
}
