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
 * limitations under the License
 */

package android.appsecurity.cts;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.platform.test.annotations.AppModeFull;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests for package resolution.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class PackageResolutionHostTest extends BaseAppSecurityTest {

    private static final String TINY_APK = "CtsOrderedActivityApp.apk";
    private static final String TINY_PKG = "android.appsecurity.cts.orderedactivity";

    private String mOldVerifierValue;
    private CompatibilityBuildHelper mBuildHelper;

    @Before
    public void setUp() throws Exception {
        getDevice().uninstallPackage(TINY_PKG);
        mBuildHelper = new CompatibilityBuildHelper(getBuild());
    }

    @After
    public void tearDown() throws Exception {
        getDevice().uninstallPackage(TINY_PKG);
    }

    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testResolveOrderedActivity() throws Exception {
        getDevice().installPackage(mBuildHelper.getTestFile(TINY_APK), true);
        Utils.runDeviceTests(getDevice(), TINY_PKG,
                ".PackageResolutionTest", "queryActivityOrdered");
        getDevice().uninstallPackage(TINY_PKG);
    }

    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testResolveOrderedService() throws Exception {
        getDevice().installPackage(mBuildHelper.getTestFile(TINY_APK), true);
        Utils.runDeviceTests(getDevice(), TINY_PKG,
                ".PackageResolutionTest", "queryServiceOrdered");
        getDevice().uninstallPackage(TINY_PKG);
    }

    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testResolveOrderedReceiver() throws Exception {
        getDevice().installPackage(mBuildHelper.getTestFile(TINY_APK), true);
        Utils.runDeviceTests(getDevice(), TINY_PKG,
                ".PackageResolutionTest", "queryReceiverOrdered");
        getDevice().uninstallPackage(TINY_PKG);
    }
}
