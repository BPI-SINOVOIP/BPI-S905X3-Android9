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

import static android.Manifest.permission.SEND_SHOW_SUSPENDED_APP_DETAILS;
import static android.Manifest.permission.SUSPEND_APPS;
import static android.content.Intent.ACTION_SHOW_SUSPENDED_APP_DETAILS;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class SuspendAppsPermissionTest {

    private PackageManager mPackageManager;

    @Before
    public void setUp() {
        mPackageManager = InstrumentationRegistry.getTargetContext().getPackageManager();
    }

    @Test
    public void testNumberOfAppsWithPermission() {
        final List<PackageInfo> packagesWithPerm = mPackageManager.getPackagesHoldingPermissions(
                new String[]{SUSPEND_APPS}, 0);
        assertTrue("At most one app can hold the permission " + SUSPEND_APPS + ", but found more: "
                + packagesWithPerm, packagesWithPerm.size() <= 1);
    }

    @Test
    public void testShowSuspendedAppDetailsDeclared() {
        final List<PackageInfo> packagesWithPerm = mPackageManager.getPackagesHoldingPermissions(
                new String[]{SUSPEND_APPS}, 0);
        final Intent showDetailsIntent = new Intent(ACTION_SHOW_SUSPENDED_APP_DETAILS);
        boolean success = true;
        StringBuilder errorString = new StringBuilder();
        for (PackageInfo packageInfo : packagesWithPerm) {
            showDetailsIntent.setPackage(packageInfo.packageName);
            final ResolveInfo resolveInfo = mPackageManager.resolveActivity(showDetailsIntent, 0);
            if (resolveInfo == null || resolveInfo.activityInfo == null) {
                errorString.append("No activity found for " + ACTION_SHOW_SUSPENDED_APP_DETAILS
                        + " inside package " + packageInfo.packageName);
                success = false;
            }
            else if (!SEND_SHOW_SUSPENDED_APP_DETAILS.equals(resolveInfo.activityInfo.permission)) {
                errorString.append("Activity handling " + ACTION_SHOW_SUSPENDED_APP_DETAILS
                        + " not protected with permission " + SEND_SHOW_SUSPENDED_APP_DETAILS);
                success = false;
            }
        }
        if (!success) {
            fail(errorString.toString());
        }
    }
}
