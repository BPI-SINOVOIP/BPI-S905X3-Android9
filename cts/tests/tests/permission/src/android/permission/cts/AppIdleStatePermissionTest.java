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

import static org.junit.Assert.fail;

import org.junit.runner.RunWith;
import org.junit.Test;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.UserHandle;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import java.util.List;

/**
 * Build, install and run tests with following command:
 * atest AppIdleStatePermissionTest
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class AppIdleStatePermissionTest {

    /**
     * Verify that the {@link android.Manifest.permission#CHANGE_APP_IDLE_STATE}
     * permission is only held by at most one package.
     */
    @Test
    public void testChangeAppIdleStatePermission() throws Exception {
        final PackageManager pm = InstrumentationRegistry.getContext().getPackageManager();
        final List<PackageInfo> holding = pm.getPackagesHoldingPermissions(new String[]{
                android.Manifest.permission.CHANGE_APP_IDLE_STATE
        }, PackageManager.MATCH_SYSTEM_ONLY);

        int count = 0;
        String pkgNames = "";
        for (PackageInfo pkg : holding) {
            int uid = pm.getApplicationInfo(pkg.packageName, 0).uid;
            if (UserHandle.isApp(uid)) {
                pkgNames += pkg.packageName + "\n";
                count++;
            }
        }
        if (count > 1) {
            fail("Only one app may hold the CHANGE_APP_IDLE_STATE permission; found packages: \n"
                    + pkgNames);
        }
    }
}