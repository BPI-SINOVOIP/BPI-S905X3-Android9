/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
package android.appsecurity.cts;

import android.platform.test.annotations.AppModeFull;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(DeviceJUnit4ClassRunner.class)
public class ClassloaderSplitsTest extends BaseHostJUnit4Test implements IBuildReceiver {
    private static final String PKG = "com.android.cts.classloadersplitapp";
    private static final String TEST_CLASS = PKG + ".SplitAppTest";

    /* The feature hierarchy looks like this:

        APK_BASE (PathClassLoader)
          ^
          |
        APK_FEATURE_A (DelegateLastClassLoader)
          ^
          |
        APK_FEATURE_B (PathClassLoader)

     */

    private static final String APK_BASE = "CtsClassloaderSplitApp.apk";
    private static final String APK_FEATURE_A = "CtsClassloaderSplitAppFeatureA.apk";
    private static final String APK_FEATURE_B = "CtsClassloaderSplitAppFeatureB.apk";

    @Before
    public void setUp() throws Exception {
        Utils.prepareSingleUser(getDevice());
        getDevice().uninstallPackage(PKG);
    }

    @After
    public void tearDown() throws Exception {
        getDevice().uninstallPackage(PKG);
    }

    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testBaseClassLoader() throws Exception {
        new InstallMultiple().addApk(APK_BASE).run();
        runDeviceTests(getDevice(), PKG, TEST_CLASS, "testBaseClassLoader");
    }

    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testFeatureAClassLoader() throws Exception {
        new InstallMultiple().addApk(APK_BASE).addApk(APK_FEATURE_A).run();
        runDeviceTests(getDevice(), PKG, TEST_CLASS, "testBaseClassLoader");
        runDeviceTests(getDevice(), PKG, TEST_CLASS, "testFeatureAClassLoader");
    }

    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testFeatureBClassLoader() throws Exception {
        new InstallMultiple().addApk(APK_BASE).addApk(APK_FEATURE_A).addApk(APK_FEATURE_B).run();
        runDeviceTests(getDevice(), PKG, TEST_CLASS, "testBaseClassLoader");
        runDeviceTests(getDevice(), PKG, TEST_CLASS, "testFeatureAClassLoader");
        runDeviceTests(getDevice(), PKG, TEST_CLASS, "testFeatureBClassLoader");
    }

    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testReceiverClassLoaders() throws Exception {
        new InstallMultiple().addApk(APK_BASE).addApk(APK_FEATURE_A).addApk(APK_FEATURE_B).run();
        runDeviceTests(getDevice(), PKG, TEST_CLASS, "testBaseClassLoader");
        runDeviceTests(getDevice(), PKG, TEST_CLASS, "testAllReceivers");
    }

    private class InstallMultiple extends BaseInstallMultiple<InstallMultiple> {
        public InstallMultiple() {
            super(getDevice(), getBuild(), null);
        }
    }
}
