/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.device;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.TestAppConstants;
import com.android.tradefed.config.Option;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;

/**
 * Long running functional tests for {@link TestDevice} that verify an operation can be run
 * many times in sequence
 * <p/>
 * Requires a physical device to be connected.
 */
public class TestDeviceStressTest extends DeviceTestCase {

    @Option(name = "iterations", description = "number of iterations to test")
    private int mIterations = 50;

    @Option(name = "stop-on-failure", description =
            "stops the rest of the iteration on a failure")
    private boolean mStopOnFailure = true;

    private static final String LOG_TAG = "TestDeviceStressTest";
    private static final int TEST_FILE_COUNT= 200;
    private TestDevice mTestDevice;
    private IDeviceStateMonitor mMonitor;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTestDevice = (TestDevice)getDevice();
        mMonitor = mTestDevice.getDeviceStateMonitor();
    }

    private File createTempTestFiles() throws IOException {
        File tmpDir = null;
        File tmpFile = null;

        tmpDir = FileUtil.createTempDir("testDir");

        final String fileContents = "this is the test file contents";
        for (int i = 0; i < TEST_FILE_COUNT; i++) {
            tmpFile = FileUtil.createTempFile(String.format("tmp_%d", i), ".txt", tmpDir);
            FileUtil.writeToFile(fileContents, tmpFile);
        }
        return tmpDir;

    }

    public void testManyReboots() throws DeviceNotAvailableException {
        for (int i=0; i < mIterations; i++) {
            Log.i(LOG_TAG, String.format("testReboot attempt %d", i));
            mTestDevice.reboot();
            assertEquals(TestDeviceState.ONLINE, mMonitor.getDeviceState());
        }
    }

    public void testManyRebootBootloaders() throws DeviceNotAvailableException {
        for (int i=0; i < mIterations; i++) {
            Log.i(LOG_TAG, String.format("testRebootBootloader attempt %d", i));
            mTestDevice.rebootIntoBootloader();
            assertEquals(TestDeviceState.FASTBOOT, mMonitor.getDeviceState());
            mTestDevice.reboot();
            assertEquals(TestDeviceState.ONLINE, mMonitor.getDeviceState());
        }
    }

    public void testManyDisableKeyguard() throws DeviceNotAvailableException {
        int passed = 0;
        boolean iterationPassed;
        for (int i=0; i < mIterations; i++) {
            Log.i(LOG_TAG, String.format("testDisableKeyguard attempt %d", i));
            mTestDevice.reboot();
            iterationPassed = runUITests();
            if (mStopOnFailure){
                assertTrue(iterationPassed);
            }
            passed += iterationPassed? 1 : 0;
        }
        assertEquals(mIterations, passed);
    }

    /**
     * Stress test to push a folder which contains 200 text file to device
     * internal storage.
     */
    public void testPushFolderWithManyFiles() throws IOException, DeviceNotAvailableException {
        File tmpDir = null;
        String deviceFilePath = null;
        String externalStorePath = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        assertNotNull(externalStorePath);
        deviceFilePath = String.format("%s/%s", externalStorePath, "testDir");

        // start the stress test
        try {
            // Create the test folder and make sure the test folder doesn't exist in
            // device before the test start.
            tmpDir = createTempTestFiles();
            for (int i = 0; i < mIterations; i++) {
                mTestDevice.executeShellCommand(String.format("rm -r %s", deviceFilePath));
                assertFalse(String.format("%s exists", deviceFilePath),
                        mTestDevice.doesFileExist(deviceFilePath));
                assertTrue(mTestDevice.pushDir(tmpDir, deviceFilePath));
                assertTrue(mTestDevice.doesFileExist(deviceFilePath));
            }
        } finally {
            if (tmpDir != null) {
                FileUtil.recursiveDelete(tmpDir);
            }
            mTestDevice.executeShellCommand(String.format("rm -r %s", deviceFilePath));
            assertFalse(String.format("%s exists", deviceFilePath),
                    mTestDevice.doesFileExist(deviceFilePath));
        }
    }

    /**
     * Test to sync a host side folder to device
     */
    public void testSyncDataWithManyFiles() throws IOException, DeviceNotAvailableException {
        File tmpDir = null;
        String deviceFilePath = null;
        String externalStorePath = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        assertNotNull(externalStorePath);
        deviceFilePath = String.format("%s/%s", externalStorePath, "testDir");

        // start the stress test
        try {
            // Create the test folder and make sure the test folder doesn't exist in
            // device before the test start.
            tmpDir = createTempTestFiles();
            assertTrue(String.format(
                    "failed to sync test data from local-data-path %s to %s on device %s",
                    tmpDir.getAbsolutePath(), deviceFilePath, mTestDevice.getSerialNumber()),
                    mTestDevice.syncFiles(tmpDir, deviceFilePath));
        } finally {
            if (tmpDir != null) {
                FileUtil.recursiveDelete(tmpDir);
            }
            mTestDevice.executeShellCommand(String.format("rm -r %s", deviceFilePath));
            assertFalse(String.format("%s exists", deviceFilePath),
                    mTestDevice.doesFileExist(deviceFilePath));
        }
    }

    /**
     * Run the test app UI tests and return true if they all pass.
     */
    private boolean runUITests() throws DeviceNotAvailableException {
        RemoteAndroidTestRunner uirunner = new RemoteAndroidTestRunner(
                TestAppConstants.UITESTAPP_PACKAGE, getDevice().getIDevice());
        CollectingTestListener uilistener = new CollectingTestListener();
        getDevice().runInstrumentationTests(uirunner, uilistener);
        return TestAppConstants.UI_TOTAL_TESTS == uilistener.getNumTestsInState(TestStatus.PASSED);
    }

    /**
     * Return the number of iterations.
     * <p/>
     * Exposed for unit testing
     */
    public int getIterations() {
        return mIterations;
    }

    /**
     * Set the iterations
     */
    void setIterations(int iterations) {
        mIterations = iterations;
    }
}
