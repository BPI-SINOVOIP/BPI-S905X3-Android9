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

package com.android.tradefed.testtype;

import com.android.ddmlib.FileListingService;
import com.android.ddmlib.Log;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IFileEntry;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * A Test that runs a native stress test executable on given device.
 * <p/>
 * It uses {@link NativeStressTestParser} to parse out number of iterations completed and report
 * those results to the {@link ITestInvocationListener}s.
 */
@OptionClass(alias = "native-stress")
public class NativeStressTest implements IDeviceTest, IRemoteTest {

    private static final String LOG_TAG = "NativeStressTest";
    static final String DEFAULT_TEST_PATH = "data/nativestresstest";

    // The metrics key names to report to listeners
    // TODO: these key names are temporary
    static final String AVG_ITERATION_TIME_KEY = "avg-iteration-time";
    static final String ITERATION_KEY = "iterations";

    private ITestDevice mDevice = null;

    @Option(name = "native-stress-device-path",
      description="The path on the device where native stress tests are located.")
    private String mDeviceTestPath = DEFAULT_TEST_PATH;

    @Option(name = "stress-module-name",
            description="The name of the native test module to run. " +
            "If not specified all tests in --native-stress-device-path will be run.")
    private String mTestModule = null;

    @Option(name = "iterations",
            description="The number of stress test iterations per run.",
            importance = Importance.IF_UNSET)
    private Integer mNumIterations = null;

    @Option(name = "runs",
            description="The number of stress test runs to perform.")
    private int mNumRuns = 1;

    @Option(name = "max-iteration-time", description =
        "The maximum time to allow for one stress test iteration in ms.")
    private int mMaxIterationTime = 5 * 60 * 1000;

    // TODO: consider sharing code with {@link GTest}

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
     * Set the Android native stress test module to run.
     *
     * @param moduleName The name of the native test module to run
     */
    public void setModuleName(String moduleName) {
        mTestModule = moduleName;
    }

    /**
     * Get the Android native test module to run.
     *
     * @return the name of the native test module to run, or null if not set
     */
    public String getModuleName() {
        return mTestModule;
    }

    /**
     * Set the number of iterations to execute per run
     */
    void setNumIterations(int iterations) {
        mNumIterations = iterations;
    }

    /**
     * Set the number of runs to execute
     */
    void setNumRuns(int runs) {
        mNumRuns = runs;
    }

    /**
     * Gets the path where native stress tests live on the device.
     *
     * @return The path on the device where the native tests live.
     */
    private String getTestPath() {
        StringBuilder testPath = new StringBuilder(mDeviceTestPath);
        if (mTestModule != null) {
            testPath.append(FileListingService.FILE_SEPARATOR);
            testPath.append(mTestModule);
        }
        return testPath.toString();
    }

    /**
     * Executes all native stress tests in a folder as well as in all subfolders recursively.
     *
     * @param rootEntry The root folder to begin searching for native tests
     * @param testDevice The device to run tests on
     * @param listener the run listener
     * @throws DeviceNotAvailableException
     */
    private void doRunAllTestsInSubdirectory(
            IFileEntry rootEntry, ITestDevice testDevice, ITestInvocationListener listener)
            throws DeviceNotAvailableException {

        if (rootEntry.isDirectory()) {
            // recursively run tests in all subdirectories
            for (IFileEntry childEntry : rootEntry.getChildren(true)) {
                doRunAllTestsInSubdirectory(childEntry, testDevice, listener);
            }
        } else {
            // assume every file is a valid stress test binary.
            // use name of file as run name
            NativeStressTestParser resultParser = createResultParser(rootEntry.getName());
            String fullPath = rootEntry.getFullEscapedPath();
            Log.i(LOG_TAG, String.format("Running native stress test %s on %s", fullPath,
                    mDevice.getSerialNumber()));
            // force file to be executable
            testDevice.executeShellCommand(String.format("chmod 755 %s", fullPath));
            int startIteration = 0;
            int endIteration = mNumIterations - 1;
            long startTime = System.currentTimeMillis();
            listener.testRunStarted(resultParser.getRunName(), 0);
            try {
                for (int i = 0; i < mNumRuns; i++) {
                    Log.i(LOG_TAG, String.format("Running %s for %d iterations",
                            rootEntry.getName(), mNumIterations));
                    // -s is start iteration, -e means end iteration
                    // use maxShellOutputResponseTime to enforce the max iteration time
                    // it won't be exact, but should be close
                    testDevice.executeShellCommand(String.format("%s -s %d -e %d", fullPath,
                            startIteration, endIteration), resultParser,
                            mMaxIterationTime, TimeUnit.MILLISECONDS, 0);
                    // iteration count is also used as a random seed value, so want use different
                    // values for each run
                    startIteration += mNumIterations;
                    endIteration += mNumIterations;
                }
                // TODO: is catching exceptions, and reporting testRunFailed necessary?
            } finally {
                reportTestCompleted(startTime, listener, resultParser);
            }

        }
    }

    private void reportTestCompleted(
            long startTime, ITestInvocationListener listener, NativeStressTestParser parser) {
        final long elapsedTime = System.currentTimeMillis() - startTime;
        int iterationsComplete = parser.getIterationsCompleted();
        float avgIterationTime = iterationsComplete > 0 ? elapsedTime / iterationsComplete : 0;
        Map<String, String> metricMap = new HashMap<String, String>(2);
        Log.i(LOG_TAG, String.format(
                "Stress test %s is finished. Num iterations %d, avg time %f ms",
                parser.getRunName(), iterationsComplete, avgIterationTime));
        metricMap.put(ITERATION_KEY, Integer.toString(iterationsComplete));
        metricMap.put(AVG_ITERATION_TIME_KEY, Float.toString(avgIterationTime));
        listener.testRunEnded(elapsedTime, TfMetricProtoUtil.upgradeConvert(metricMap));
    }

    /**
     * Factory method for creating a {@link NativeStressTestParser} that parses test output
     * <p/>
     * Exposed so unit tests can mock.
     *
     * @param runName
     * @return a {@link NativeStressTestParser}
     */
    NativeStressTestParser createResultParser(String runName) {
        return new NativeStressTestParser(runName);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (mDevice == null) {
            throw new IllegalArgumentException("Device has not been set");
        }
        if (mNumIterations == null || mNumIterations <= 0) {
            throw new IllegalArgumentException("number of iterations has not been set");
        }

        String testPath = getTestPath();
        IFileEntry nativeTestDirectory = mDevice.getFileEntry(testPath);
        if (nativeTestDirectory == null) {
            Log.w(LOG_TAG, String.format("Could not find native stress test directory %s in %s!",
                    testPath, mDevice.getSerialNumber()));
            return;
        }
        doRunAllTestsInSubdirectory(nativeTestDirectory, mDevice, listener);
    }
}
