/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.FileUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Runs a set of instrumentation tests by specifying a list of line separated test classes and
 * methods in a file pushed to device (expected format: com.android.foo.FooClassName#testMethodName)
 * <p>
 * Note: Requires a runner that supports test execution from a file. Will default to serial tests
 * execution via {@link InstrumentationSerialTest} if any issues with file creation are encountered
 * or if all tests in the created file fail to successfully finish execution.
 */
class InstrumentationFileTest implements IRemoteTest {

    // on device test folder location where the test file should be saved
    private static final String ON_DEVICE_TEST_DIR_LOCATION = "/data/local/tmp/";

    private InstrumentationTest mInstrumentationTest = null;

    /** the set of tests to run */
    private final Collection<TestDescription> mTests;

    private String mFilePathOnDevice = null;

    private int mAttemps;

    private int mMaxAttemps;

    private boolean mRetrySerially;

    /**
     * Creates a {@link InstrumentationFileTest}.
     *
     * @param instrumentationTest {@link InstrumentationTest} used to configure this class
     * @param testsToRun a {@link Collection} of tests to run. Note this {@link Collection} will be
     *     used as is (ie a reference to the testsToRun object will be kept).
     */
    InstrumentationFileTest(
            InstrumentationTest instrumentationTest,
            Collection<TestDescription> testsToRun,
            boolean retrySerially,
            int maxAttempts)
            throws ConfigurationException {
        // reuse the InstrumentationTest class to perform actual test run
        mInstrumentationTest = createInstrumentationTest();
        // copy all options from the original InstrumentationTest
        OptionCopier.copyOptions(instrumentationTest, mInstrumentationTest);
        mInstrumentationTest.setDevice(instrumentationTest.getDevice());
        mInstrumentationTest.setForceAbi(instrumentationTest.getForceAbi());
        mInstrumentationTest.setReRunUsingTestFile(true);
        // no need to rerun when executing tests one by one
        mInstrumentationTest.setRerunMode(false);
        // keep local copy of tests to be run
        mTests = testsToRun;
        mAttemps = 0;
        mRetrySerially = retrySerially;
        mMaxAttemps = maxAttempts;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(final ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (mInstrumentationTest.getDevice() == null) {
            throw new IllegalArgumentException("Device has not been set");
        }
        // reuse InstrumentationTest class to perform actual test run
        writeTestsToFileAndRun(mTests, listener);
    }


    /**
     * Creates a file based on the {@link Collection} of tests to run. Upon successful file creation
     * will push the file onto the test device and attempt to run them via {@link
     * InstrumentationTest}. If something goes wrong, will default to serial test execution.
     *
     * @param tests a {@link Collection} of tests to run
     * @param listener the test result listener
     * @throws DeviceNotAvailableException
     */
    private void writeTestsToFileAndRun(
            Collection<TestDescription> tests, final ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        mAttemps += 1;
        if (mMaxAttemps > 0 && mAttemps <= mMaxAttemps) {
            CLog.d("Try to run tests from file for the %d/%d attempts",
                    mAttemps, mMaxAttemps);
        } else if (mMaxAttemps > 0) {
            if (mRetrySerially) {
                CLog.d("Running tests from file exceeded max attempts."
                        + " Try to run tests serially.");
                reRunTestsSerially(mInstrumentationTest, listener);
            } else {
                CLog.d("Running tests from file exceeded max attempts. Ignore the rest tests");
                return;
            }
        }
        File testFile = null;
        try {
            // create and populate test file
            testFile = FileUtil.createTempFile(
                    "tf_testFile_" + InstrumentationFileTest.class.getCanonicalName(), ".txt");
            try (BufferedWriter bw = new BufferedWriter(new FileWriter(testFile))) {
                // Remove parameterized tests to only re-run their base method.
                Collection<TestDescription> uniqueMethods = createRerunSet(tests);

                for (TestDescription testToRun : uniqueMethods) {
                    // We use getTestNameNoParams to avoid attempting re-running individual
                    // parameterized tests. Instead ask the base method to re-run them all.
                    bw.write(
                            String.format(
                                    "%s#%s",
                                    testToRun.getClassName(),
                                    testToRun.getTestNameWithoutParams()));
                    bw.newLine();
                }
                CLog.d("Test file %s was successfully created", testFile.getAbsolutePath());
            }
            // push test file to the device and run
            mFilePathOnDevice = ON_DEVICE_TEST_DIR_LOCATION + testFile.getName();
            if (pushFileToTestDevice(testFile, mFilePathOnDevice)) {
                mInstrumentationTest.setTestFilePathOnDevice(mFilePathOnDevice);
                CLog.d("Test file %s was successfully pushed to %s on device",
                        testFile.getAbsolutePath(), mFilePathOnDevice);
                runTests(mInstrumentationTest, listener);
            } else {
                if (mRetrySerially) {
                    CLog.e("Failed to push file to device, re-running tests serially");
                    reRunTestsSerially(mInstrumentationTest, listener);
                } else {
                    CLog.e("Failed to push file to device, ignore the rest of tests");
                }
            }
        } catch (IOException e) {
            if (mRetrySerially) {
                CLog.e("Failed to run tests from file, re-running tests serially: %s",
                        e.getMessage());
                reRunTestsSerially(mInstrumentationTest, listener);
            } else {
                CLog.e("Failed to push file to device, ignore the rest of tests");
            }
        } finally {
            // clean up test file, if it was created
            FileUtil.deleteFile(testFile);
        }
    }

    /**
     * Run all tests from file. Attempt to re-run not finished tests.
     * If all tests in file fail to run default to executing them serially.
     */
    private void runTests(InstrumentationTest runner, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        CollectingTestListener testTracker = new CollectingTestListener();
        try {
            runner.run(new ResultForwarder(listener, testTracker));
        } finally {
            deleteTestFileFromDevice(mFilePathOnDevice);
            Collection<TestDescription> completedTests =
                    testTracker.getCurrentRunResults().getCompletedTests();
            if (mTests.removeAll(completedTests) && !mTests.isEmpty()) {
                // re-run remaining tests from file
                writeTestsToFileAndRun(mTests, listener);
            } else if (!mTests.isEmpty()) {
                if (mRetrySerially) {
                    CLog.e("all remaining tests failed to run from file, re-running tests serially");
                    reRunTestsSerially(runner, listener);
                } else {
                    CLog.e("all remaining tests failed to run from file, will be ignored");
                }
            }
        }
    }

    /**
     * Re-runs remaining tests one-by-one
     */
    private void reRunTestsSerially(InstrumentationTest runner, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        // clear file path arguments to ensure it won't get used.
        runner.setTestFilePathOnDevice(null);
        // enforce serial re-run
        runner.setReRunUsingTestFile(false);
        // Set tests to run
        runner.setTestsToRun(mTests);
        runner.run(listener);
    }

    /**
     * Returns a new collection of {@link TestDescription} where only one instance of each
     * parameterized method is in the list.
     */
    private Collection<TestDescription> createRerunSet(Collection<TestDescription> tests) {
        Map<String, TestDescription> uniqueMethods = new LinkedHashMap<>();
        for (TestDescription test : tests) {
            uniqueMethods.put(test.getTestNameWithoutParams(), test);
        }
        return uniqueMethods.values();
    }

    /**
     * Util method to push file to a device. Exposed for unit testing.
     *
     * @return if file was pushed to the device successfully
     * @throws DeviceNotAvailableException
     */
    boolean pushFileToTestDevice(File file, String destinationPath)
            throws DeviceNotAvailableException {
        return mInstrumentationTest.getDevice().pushFile(file, destinationPath);
    }

    /**
     * Delete file from the device if it exists
     */
    void deleteTestFileFromDevice(String pathToFile) throws DeviceNotAvailableException {
        if (mInstrumentationTest.getDevice().doesFileExist(pathToFile)) {
            mInstrumentationTest.getDevice()
                    .executeShellCommand(String.format("rm %s", pathToFile));
            CLog.d("Removed test file from device: %s", pathToFile);
        }
    }

    /** @return the {@link InstrumentationTest} to use. Exposed for unit testing. */
    @VisibleForTesting
    InstrumentationTest createInstrumentationTest() {
        return new InstrumentationTest();
    }
}
