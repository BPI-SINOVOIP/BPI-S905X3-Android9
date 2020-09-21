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

package com.android.tradefed.testtype;

import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.Log;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ITestLifeCycleReceiver;

import java.util.concurrent.TimeUnit;

/**
 * A Test that runs a system fuzz test package (part of Vendor Test Suite, VTS) on given device.
 */
@OptionClass(alias = "vtsfuzztest")
public class VtsFuzzTest implements IDeviceTest, IRemoteTest {

    private static final String LOG_TAG = "VtsFuzzTest";
    static final String DEFAULT_FUZZER_BINARY_PATH = "/system/bin/fuzzer";

    static final float DEFAULT_TARGET_VERSION = -1;
    static final int DEFAULT_EPOCH_COUNT = 10;

    // fuzzer flags
    private static final String VTS_FUZZ_TEST_FLAG_CLASS = "--class";
    private static final String VTS_FUZZ_TEST_FLAG_TYPE = "--type";
    private static final String VTS_FUZZ_TEST_FLAG_VERSION = "--version";
    private static final String VTS_FUZZ_TEST_FLAG_EPOCH_COUNT = "--epoch_count";

    private ITestDevice mDevice = null;

    @Option(name = "fuzzer-binary-path",
            description="The path on the device where fuzzer is located.")
    private String mFuzzerBinaryPath = DEFAULT_FUZZER_BINARY_PATH;

    @Option(name = "target-component-path",
            description="The path of a target component (e.g., .so file path).")
    private String mTargetComponentPath = null;

    @Option(name = "target-class",
            description="The target component class.")
    private String mTargetClass = null;

    @Option(name = "target-type",
            description="The target component type.")
    private String mTargetType = null;

    @Option(name = "target-version",
            description="The target component version.")
    private float mTargetVersion = DEFAULT_TARGET_VERSION;

    @Option(name = "epoch-count",
            description="The epoch count.")
    private int mEpochCount = DEFAULT_EPOCH_COUNT;

    @Option(name = "test-timeout", description =
            "The max time in ms for a VTS test to run. " +
            "Test run will be aborted if any test takes longer.")
    private int mMaxTestTimeMs = 15 * 60 * 1000;  // 15 minutes

    @Option(name = "runtime-hint",
            isTimeVal=true,
            description="The hint about the test's runtime.")
    private long mRuntimeHint = 5 * 60 * 1000;  // 5 minutes

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * Set the target component path variable.
     *
     * @param path The path to set.
     */
    public void setTargetComponentPath(String path) {
        mTargetComponentPath = path;
    }

    /**
     * Get the target component path variable.
     *
     * @return The target component path.
     */
    public String getTargetComponentPath() {
        return mTargetComponentPath;
    }

    /**
     * Set the target class variable.
     *
     * @param class_name The class name to set.
     */
    public void setTargetClass(String class_name) {
        mTargetClass = class_name;
    }

    /**
     * Get the target class name variable.
     *
     * @return The target class name.
     */
    public String getTargetClass() {
        return mTargetClass;
    }

    /**
     * Set the target type variable.
     *
     * @param type_name The type name to set.
     */
    public void setTargetType(String type_name) {
        mTargetType = type_name;
    }

    /**
     * Get the target type name variable.
     *
     * @return The target type name.
     */
    public String getTargetType() {
        return mTargetType;
    }

    /**
     * Set the target version variable.
     *
     * @param version The version to set.
     */
    public void setTargetVersion(float version) {
        mTargetVersion = version;
    }

    /**
     * Get the target version variable.
     *
     * @return The version.
     */
    public float getTargetVersion() {
        return mTargetVersion;
    }

    /**
     * Set the epoch count variable.
     *
     * @param count The epoch count to set.
     */
    public void setEpochCount(int count) {
        mEpochCount = count;
    }

    /**
     * Get the epoch count variable.
     *
     * @return The epoch count.
     */
    public float getEpochCount() {
        return mEpochCount;
    }

    /**
     * Helper to get all the VtsFuzzTest flags to pass into the adb shell command.
     *
     * @return the {@link String} of all the VtsFuzzTest flags that should be passed to the VtsFuzzTest
     * @throws IllegalArgumentException
     */
    private String getAllVtsFuzzTestFlags() {
        String flags;

        if (mTargetClass == null) {
            throw new IllegalArgumentException(VTS_FUZZ_TEST_FLAG_CLASS + " must be set.");
        }
        flags = String.format("%s=%s", VTS_FUZZ_TEST_FLAG_CLASS, mTargetClass);

        if (mTargetType == null) {
            throw new IllegalArgumentException(VTS_FUZZ_TEST_FLAG_TYPE + " must be set.");
        }
        flags = String.format("%s %s=%s", flags, VTS_FUZZ_TEST_FLAG_TYPE, mTargetType);

        if (mTargetVersion == DEFAULT_TARGET_VERSION) {
            throw new IllegalArgumentException(VTS_FUZZ_TEST_FLAG_VERSION + " must be set.");
        }
        flags = String.format("%s %s=%s", flags, VTS_FUZZ_TEST_FLAG_VERSION, mTargetVersion);

        flags = String.format("%s %s=%s", flags, VTS_FUZZ_TEST_FLAG_EPOCH_COUNT, mEpochCount);

        flags = String.format("%s %s", flags, mTargetComponentPath);
        return flags;
    }

    /**
     * Run the given fuzzer binary using the given flags
     *
     * @param testDevice the {@link ITestDevice}
     * @param resultParser the test run output parser
     * @param fullPath absolute file system path to gtest binary on device
     * @param flags fuzzer execution flags
     * @throws DeviceNotAvailableException
     */
    private void runTest(final ITestDevice testDevice, final IShellOutputReceiver resultParser,
            final String fullPath, final String flags) throws DeviceNotAvailableException {
        // TODO: add individual test timeout support, and rerun support
        try {
            String cmd = getVtsFuzzTestCmdLine(fullPath, flags);
            testDevice.executeShellCommand(cmd, resultParser,
                    mMaxTestTimeMs,
                    TimeUnit.MILLISECONDS,
                    0 /* retryAttempts */);
        } catch (DeviceNotAvailableException e) {
            // TODO: consider moving the flush of parser data on exceptions to TestDevice or
            // AdbHelper
            resultParser.flush();
            throw e;
        } catch (RuntimeException e) {
            resultParser.flush();
            throw e;
        }
    }

    /**
     * Helper method to build the fuzzer command to run.
     *
     * @param fullPath absolute file system path to fuzzer binary on device
     * @param flags fuzzer execution flags
     * @return the shell command line to run for the fuzzer
     */
    protected String getVtsFuzzTestCmdLine(String fullPath, String flags) {
        StringBuilder cmdLine = new StringBuilder();
        cmdLine.append(String.format("%s %s", fullPath, flags));
        return cmdLine.toString();
    }

    /**
     * Factory method for creating a {@link IShellOutputReceiver} that parses test output and
     * forwards results to the result listener.
     * <p/>
     * Exposed so unit tests can mock
     *
     * @param listener
     * @param runName
     * @return a {@link IShellOutputReceiver}
     */
    IShellOutputReceiver createResultParser(String runName, ITestLifeCycleReceiver listener) {
        VtsFuzzTestResultParser resultParser = new VtsFuzzTestResultParser(runName, listener);
        return resultParser;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (mDevice == null) {
            throw new IllegalArgumentException("Device has not been set");
        }

        if (!mDevice.doesFileExist(mFuzzerBinaryPath)) {
            Log.w(LOG_TAG, String.format("Could not find fuzzer binary file %s in %s!",
                    mFuzzerBinaryPath, mDevice.getSerialNumber()));
            return;
        }

        IShellOutputReceiver resultParser = createResultParser(mTargetComponentPath, listener);
        String flags = getAllVtsFuzzTestFlags();
        Log.i(LOG_TAG, String.format("Running fuzzer '%s %s' on %s",
                mFuzzerBinaryPath, flags, mDevice.getSerialNumber()));
        runTest(mDevice, resultParser, mFuzzerBinaryPath, flags);
    }
}
