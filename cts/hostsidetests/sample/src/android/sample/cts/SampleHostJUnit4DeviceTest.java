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

package android.sample.cts;

import android.platform.test.annotations.AppModeInstant;
import android.platform.test.annotations.AppModeFull;

import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test that collects test results from test package android.sample.cts.app2.
 *
 * When this test builds, it also builds a support APK containing
 * {@link android.sample.cts.app2.SampleDeviceTest}, the results of which are
 * collected from the hostside and reported accordingly.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class SampleHostJUnit4DeviceTest extends BaseHostJUnit4Test {
    private static final String TEST_PKG = "android.sample.cts.app2";
    private static final String TEST_CLASS = TEST_PKG + "." + "SampleDeviceTest";
    private static final String TEST_APP = "CtsSampleDeviceApp2.apk";

    private static final String TEST_PASSES = "testPasses";
    private static final String TEST_ASSUME_FAILS = "testAssumeFails";
    private static final String TEST_FAILS = "testFails";

    @Before
    public void setUp() throws Exception {
        uninstallPackage(getDevice(), TEST_PKG);
    }

    @Test
    @AppModeInstant
    public void testRunDeviceTestsPassesInstant() throws Exception {
        installPackage(true);
        Assert.assertTrue(runDeviceTests(TEST_PKG, TEST_CLASS, TEST_PASSES));
    }

    @Test
    @AppModeFull
    public void testRunDeviceTestsPassesFull() throws Exception {
        installPackage(false);
        Assert.assertTrue(runDeviceTests(TEST_PKG, TEST_CLASS, TEST_PASSES));
    }

    @Test(expected=AssertionError.class)
    @AppModeInstant
    public void testRunDeviceTestsFailsInstant() throws Exception {
        installPackage(true);
        Assert.assertTrue(runDeviceTests(TEST_PKG, TEST_CLASS, TEST_FAILS));
    }

    @Test(expected=AssertionError.class)
    @AppModeFull
    public void testRunDeviceTestsFailsFull() throws Exception {
        installPackage(false);
        Assert.assertTrue(runDeviceTests(TEST_PKG, TEST_CLASS, TEST_FAILS));
    }

    @Test
    @AppModeInstant
    public void testRunDeviceTestsAssumeFailsInstant() throws Exception {
        installPackage(true);
        Assert.assertTrue(runDeviceTests(TEST_PKG, TEST_CLASS, TEST_ASSUME_FAILS));
    }

    @Test
    @AppModeFull
    public void testRunDeviceTestsAssumeFailsFull() throws Exception {
        installPackage(false);
        Assert.assertTrue(runDeviceTests(TEST_PKG, TEST_CLASS, TEST_ASSUME_FAILS));
    }

    @After
    public void tearDown() throws Exception {
        uninstallPackage(getDevice(), TEST_PKG);
    }

    private void installPackage(boolean instant) throws Exception {
        installPackage(TEST_APP, instant ? new String[]{"--instant"} : new String[0]);
    }

}
