/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.cts.usb;

import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.AppModeInstant;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.util.AbiUtils;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Functional tests for usb connection
 */
public class TestUsbTest extends DeviceTestCase implements IAbiReceiver, IBuildReceiver {

    private static final String CTS_RUNNER = "android.support.test.runner.AndroidJUnitRunner";
    private static final String PACKAGE_NAME = "com.android.cts.usb.serialtest";
    private static final String TEST_CLASS_NAME = PACKAGE_NAME + ".UsbSerialTest";
    private static final String APK_NAME="CtsUsbSerialTestApp.apk";
    private ITestDevice mDevice;
    private IAbi mAbi;
    private IBuildInfo mBuild;

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuild = buildInfo;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevice = getDevice();
        mDevice.uninstallPackage(PACKAGE_NAME);
    }

    private void installApp(boolean installAsInstantApp)
            throws FileNotFoundException, DeviceNotAvailableException {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuild);
        File app = buildHelper.getTestFile(APK_NAME);
        String[] options;

        if (installAsInstantApp) {
            options = new String[]{AbiUtils.createAbiFlag(mAbi.getName()), "--instant"};
        } else {
            options = new String[]{AbiUtils.createAbiFlag(mAbi.getName())};
        }
        mDevice.installPackage(app, false, true, options);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mDevice.uninstallPackage(PACKAGE_NAME);
    }

    private void runTestOnDevice(String testMethod) throws DeviceNotAvailableException {
        CollectingTestListener listener = new CollectingTestListener();
        RemoteAndroidTestRunner testRunner = new RemoteAndroidTestRunner(PACKAGE_NAME, CTS_RUNNER,
                mDevice.getIDevice());
        testRunner.setMethodName(TEST_CLASS_NAME, testMethod);
        mDevice.runInstrumentationTests(testRunner, listener);

        while (!listener.getCurrentRunResults().isRunComplete()) {
            // wait
        }

        TestRunResult runResult = listener.getCurrentRunResults();
        if (runResult.isRunFailure()) {
            fail(runResult.getRunFailureMessage());
        }

        for (TestResult result : runResult.getTestResults().values()) {
            if (!result.getStatus().equals(TestStatus.PASSED)) {
                fail(result.getStackTrace());
            }
        }
    }

    /**
     * Check if adb serial number, USB serial number, ro.serialno, and android.os.Build.SERIAL
     * all matches and meets the format requirement [a-zA-Z0-9]{6,20}
     */
    @AppModeInstant(reason = "only instant apps fail when reading serial")
    public void testInstantAppsCannotReadSerial() throws Exception {
        installApp(true);

        runTestOnDevice("verifySerialCannotBeRead");
    }

    /**
     * Check if adb serial number, USB serial number, ro.serialno, and android.os.Build.SERIAL
     * all matches and meets the format requirement [a-zA-Z0-9]{6,20}
     */
    @AppModeFull(reason = "serial can not be read by instant apps")
    public void testUsbSerialReadOnDeviceMatches() throws Exception {
        installApp(false);

        String adbSerial = mDevice.getSerialNumber().toLowerCase().trim();
        if (adbSerial.startsWith("emulator-")) {
            return;
        }
        if (mDevice.isAdbTcp()) { // adb over WiFi, no point checking it
            return;
        }

        String roSerial = mDevice.executeShellCommand("getprop ro.serialno").toLowerCase().
                trim();
        assertEquals("adb serial != ro.serialno" , adbSerial, roSerial);

        CommandResult result = RunUtil.getDefault().runTimedCmd(5000, "lsusb", "-v");
        assertTrue("lsusb -v failed", result.getStatus() == CommandStatus.SUCCESS);
        String lsusbOutput = result.getStdout();
        Pattern pattern = Pattern.compile("^\\s+iSerial\\s+\\d+\\s+([a-zA-Z0-9]{6,20})",
                Pattern.MULTILINE);
        Matcher matcher = pattern.matcher(lsusbOutput);
        String usbSerial = "";
        while (matcher.find()) {
            String currentSerial = matcher.group(1).toLowerCase();
            if (adbSerial.compareTo(currentSerial) == 0) {
                usbSerial = currentSerial;
                break;
            }
        }
        assertEquals("usb serial != adb serial" , usbSerial, adbSerial);

        // now check Build.SERIAL
        clearLogCat();
        runTestOnDevice("logSerial");
        String logs = mDevice.executeAdbCommand(
                "logcat", "-v", "brief", "-d", "CtsUsbSerialTest:W", "*:S");
        pattern = Pattern.compile("^.*CtsUsbSerialTest\\(.*\\):\\s+([a-zA-Z0-9]{6,20})",
                Pattern.MULTILINE);
        matcher = pattern.matcher(logs);
        String buildSerial = "";
        while (matcher.find()) {
            String currentSerial = matcher.group(1).toLowerCase();
            if (usbSerial.compareTo(currentSerial) == 0) {
                buildSerial = currentSerial;
                break;
            }
        }
        assertEquals("usb serial != Build.SERIAL" , usbSerial, buildSerial);
    }

    private void clearLogCat() throws DeviceNotAvailableException {
        mDevice.executeAdbCommand("logcat", "-c");
    }
}
