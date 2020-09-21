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

package android.harmfulappwarning.cts;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.PackageInfo;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Scanner;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeFalse;

/**
 * Host-side tests for the harmful app launch warning
 *
 * <p>These tests have a few different components. This is the host-side part of the test, which is
 * responsible for setting up the environment and passing off execution to the device-side part
 * of the test.
 *
 * <p>The {@link HarmfulAppWarningDeviceTest} class is the device side of these tests. It attempts to launch
 * the sample warned application, and verifies the correct behavior of the harmful app warning that
 * is shown by the platform.
 *
 * <p>The third component is the sample app, which is just a placeholder app with a basic activity
 * that only serves as a target for the harmful app warning.
 *
 * <p>Run with: atest CtsHarmfulAppWarningHostTestCases
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class HarmfulAppWarningTest extends BaseHostJUnit4Test {

    private static final String FEATURE_WEARABLE = "android.hardware.type.watch";

    private static final String TEST_APP_PACKAGE_NAME = "android.harmfulappwarning.sampleapp";
    private static final String TEST_APP_ACTIVITY_CLASS_NAME = "SampleDeviceActivity";
    private static final String TEST_APP_LAUNCHED_STRING = "Sample activity started.";

    private static final String WARNING_MESSAGE = "This is a warning message.";
    private static final String SET_HARMFUL_APP_WARNING_COMMAND = String.format(
            "cmd package set-harmful-app-warning %s \"" + WARNING_MESSAGE + "\"",
            TEST_APP_PACKAGE_NAME);

    private static final String CLEAR_HARMFUL_APP_WARNING_COMMAND = String.format(
            "cmd package set-harmful-app-warning %s", TEST_APP_PACKAGE_NAME);

    private static final String GET_HARMFUL_APP_WARNING_COMMAND = String.format(
            "cmd package get-harmful-app-warning %s", TEST_APP_PACKAGE_NAME);

    private ITestDevice mDevice;

    @Before
    public void setUp() throws Exception {
        installPackage("CtsHarmfulAppWarningSampleApp.apk");
        mDevice = getDevice();
        mDevice.clearLogcat();

        // Skip the tests for wearable devices. This feature is not used on wearable
        // devices, for now (no wearable UI, etc.)
        assumeFalse(hasFeature(FEATURE_WEARABLE));
    }

    private void runDeviceTest(String testName) throws DeviceNotAvailableException {
        runDeviceTests("android.harmfulappwarning.testapp",
                "android.harmfulappwarning.testapp.HarmfulAppWarningDeviceTest",
                testName);
    }

    private void verifyHarmfulAppWarningSet() throws DeviceNotAvailableException {
        String warning = getDevice().executeShellCommand(GET_HARMFUL_APP_WARNING_COMMAND);
        assertEquals(WARNING_MESSAGE, warning.trim());
    }

    private void verifyHarmfulAppWarningUnset() throws DeviceNotAvailableException {
        String warning = getDevice().executeShellCommand(GET_HARMFUL_APP_WARNING_COMMAND);
        if (warning != null) {
            warning = warning.trim();
        }
        assertTrue(warning == null || warning.length() == 0);
    }

    private void verifySampleAppUninstalled() throws DeviceNotAvailableException {
        PackageInfo info = getDevice().getAppPackageInfo(TEST_APP_PACKAGE_NAME);
        Assert.assertNull("Harmful application was not uninstalled", info);
    }

    private void verifySampleAppInstalled() throws DeviceNotAvailableException {
        PackageInfo info = getDevice().getAppPackageInfo(TEST_APP_PACKAGE_NAME);
        Assert.assertNotNull("Harmful application was uninstalled", info);
    }

    /**
     * A basic smoke test to ensure that we're able to detect the launch of the activity when there
     * is no warning.
     */
    @Test
    public void testNormalLaunch() throws Exception {
        runDeviceTest("testNormalLaunch");
    }

    /**
     * Tests that when the user clicks "launch anyway" on the harmful app warning dialog, the
     * warning is cleared and the activity is launched.
     */
    @Test
    public void testLaunchAnyway() throws DeviceNotAvailableException {
        mDevice.executeShellCommand(SET_HARMFUL_APP_WARNING_COMMAND);
        runDeviceTest("testLaunchAnyway");

        verifyHarmfulAppWarningUnset();
    }

    /**
     * Tests that when the user clicks "uninstall" on the harmful app warning dialog, the
     * application is uninstalled.
     */
    @Test
    public void testUninstall() throws DeviceNotAvailableException {
        mDevice.executeShellCommand(SET_HARMFUL_APP_WARNING_COMMAND);
        runDeviceTest("testUninstall");
        verifySampleAppUninstalled();
    }

    /**
     * Tests that no action is taken when the user dismisses the harmful app warning
     */
    @Test
    public void testDismissDialog() throws DeviceNotAvailableException {
        mDevice.executeShellCommand(SET_HARMFUL_APP_WARNING_COMMAND);
        runDeviceTest("testDismissDialog");
        verifyHarmfulAppWarningSet();
        verifySampleAppInstalled();
    }

    protected boolean hasFeature(String featureName) throws DeviceNotAvailableException {
        return getDevice().executeShellCommand("pm list features").contains(featureName);
    }
}
