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
package android.packageinstaller.externalsources.cts;

import static org.junit.Assert.assertFalse;

import android.app.AppOpsManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.platform.test.annotations.AppModeInstant;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.AppOpsUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;

@RunWith(AndroidJUnit4.class)
@SmallTest
@AppModeInstant
public class ExternalSourcesInstantAppsTest {

    private Context mContext;
    private PackageManager mPm;
    private String mPackageName;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mPm = mContext.getPackageManager();
        mPackageName = mContext.getPackageName();
    }

    private void setAppOpsMode(int mode) throws IOException {
        AppOpsUtils.setOpMode(mPackageName, "REQUEST_INSTALL_PACKAGES", mode);
    }

    @Test
    public void blockedSourceTest() throws Exception {
        setAppOpsMode(AppOpsManager.MODE_ERRORED);
        final boolean isTrusted = mPm.canRequestPackageInstalls();
        assertFalse("Instant app " + mPackageName + " allowed to install packages", isTrusted);
    }

    @Test
    public void allowedSourceTest() throws Exception {
        setAppOpsMode(AppOpsManager.MODE_ALLOWED);
        final boolean isTrusted = mPm.canRequestPackageInstalls();
        assertFalse("Instant app " + mPackageName + " allowed to install packages", isTrusted);
    }

    @Test
    public void defaultSourceTest() throws Exception {
        setAppOpsMode(AppOpsManager.MODE_DEFAULT);
        final boolean isTrusted = mPm.canRequestPackageInstalls();
        assertFalse("Instant app " + mPackageName + " allowed to install packages", isTrusted);
    }

    @After
    public void tearDown() throws Exception {
        setAppOpsMode(AppOpsManager.MODE_DEFAULT);
    }
}
