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

import com.android.ddmlib.FileListingService;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * A Test that runs a Google benchmark test package on given device.
 */
@OptionClass(alias = "gbenchmark")
public class GoogleBenchmarkTest implements IDeviceTest, IRemoteTest {

    static final String DEFAULT_TEST_PATH = "/data/benchmarktest";

    private static final String GBENCHMARK_JSON_OUTPUT_FORMAT = "--benchmark_format=json";
    private static final String GBENCHMARK_LIST_TESTS_OPTION = "--benchmark_list_tests=true";
    private static final String EXECUTABLE_BUILD_ID = "BuildID=";

    @Option(name = "file-exclusion-filter-regex",
            description = "Regex to exclude certain files from executing. Can be repeated")
    private List<String> mFileExclusionFilterRegex = new ArrayList<>();

    @Option(name = "native-benchmark-device-path",
            description="The path on the device where native stress tests are located.")
    private String mDeviceTestPath = DEFAULT_TEST_PATH;

    @Option(name = "benchmark-module-name",
          description="The name of the native benchmark test module to run. " +
          "If not specified all tests in --native-benchmark-device-path will be run.")
    private String mTestModule = null;

    @Option(name = "benchmark-run-name",
          description="Optional name to pass to test reporters. If unspecified, will use " +
          "test binary as run name.")
    private String mReportRunName = null;

    @Option(name = "max-run-time", description =
            "The maximum time to allow for each benchmark run in ms.", isTimeVal=true)
    private long mMaxRunTime = 15 * 60 * 1000;

    private ITestDevice mDevice = null;

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
     * Set the Android native benchmark test module to run.
     *
     * @param moduleName The name of the native test module to run
     */
    public void setModuleName(String moduleName) {
        mTestModule = moduleName;
    }

    /**
     * Get the Android native benchmark test module to run.
     *
     * @return the name of the native test module to run, or null if not set
     */
    public String getModuleName() {
        return mTestModule;
    }

    public void setReportRunName(String reportRunName) {
        mReportRunName = reportRunName;
    }

    /**
     * Adds an exclusion file filter regex.
     * <p/>
     * Exposed for unit testing
     *
     * @param regex to exclude file.
     */
    void addFileExclusionFilterRegex(String regex) {
        mFileExclusionFilterRegex.add(regex);
    }

    /**
     * Gets the path where native benchmark tests live on the device.
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
     * Executes all native benchmark tests in a folder as well as in all subfolders recursively.
     *
     * @param root The root folder to begin searching for native tests
     * @param testDevice The device to run tests on
     * @param listener the run listener
     * @throws DeviceNotAvailableException
     */
    private void doRunAllTestsInSubdirectory(String root, ITestDevice testDevice,
            ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (testDevice.isDirectory(root)) {
            // recursively run tests in all subdirectories
            for (String child : testDevice.getChildren(root)) {
                doRunAllTestsInSubdirectory(root + "/" + child, testDevice, listener);
            }
        } else {
            // assume every file is a valid benchmark test binary.
            // use name of file as run name
            String rootEntry = root.substring(root.lastIndexOf("/") + 1);
            String runName = (mReportRunName == null ? rootEntry : mReportRunName);

            // force file to be executable
            testDevice.executeShellCommand(String.format("chmod 755 %s", root));
            if (shouldSkipFile(root)) {
                return;
            }
            long startTime = System.currentTimeMillis();

            // Count expected number of tests
            int numTests = countExpectedTests(testDevice, root);
            if (numTests == 0) {
                CLog.d("No tests to run.");
                return;
            }

            Map<String, String> metricMap = new HashMap<String, String>();
            CollectingOutputReceiver outputCollector = createOutputCollector();
            GoogleBenchmarkResultParser resultParser = createResultParser(runName, listener);
            listener.testRunStarted(runName, numTests);
            try {
                String cmd = String.format("%s %s", root, GBENCHMARK_JSON_OUTPUT_FORMAT);
                CLog.i(String.format("Running google benchmark test on %s: %s",
                        mDevice.getSerialNumber(), cmd));
                testDevice.executeShellCommand(cmd, outputCollector,
                        mMaxRunTime, TimeUnit.MILLISECONDS, 0);
                metricMap = resultParser.parse(outputCollector);
            } catch (DeviceNotAvailableException e) {
                listener.testRunFailed(e.getMessage());
                throw e;
            } finally {
                final long elapsedTime = System.currentTimeMillis() - startTime;
                listener.testRunEnded(elapsedTime, TfMetricProtoUtil.upgradeConvert(metricMap));
            }
        }
    }

    private int countExpectedTests(ITestDevice testDevice, String fullBinaryPath)
            throws DeviceNotAvailableException {
        String exec = testDevice.executeShellCommand(String.format("file %s", fullBinaryPath));
        // When inspecting our files, only the one with the marker of an executable should be ran.
        if (!exec.contains(EXECUTABLE_BUILD_ID)) {
            CLog.d("%s does not look like an executable", fullBinaryPath);
            return 0;
        }
        String cmd = String.format("%s %s", fullBinaryPath, GBENCHMARK_LIST_TESTS_OPTION);
        String list_output = testDevice.executeShellCommand(cmd);
        String[] list = list_output.trim().split("\n");
        CLog.d("List that will be used: %s", Arrays.asList(list));
        return list.length;
    }

    /**
     * Helper method to determine if we should skip the execution of a given file.
     * @param fullPath the full path of the file in question
     * @return true if we should skip the said file.
     */
    protected boolean shouldSkipFile(String fullPath) {
        if (fullPath == null || fullPath.isEmpty()) {
            return true;
        }
        if (mFileExclusionFilterRegex == null || mFileExclusionFilterRegex.isEmpty()) {
            return false;
        }
        for (String regex : mFileExclusionFilterRegex) {
            if (fullPath.matches(regex)) {
                CLog.i(String.format("File %s matches exclusion file regex %s, skipping",
                        fullPath, regex));
                return true;
            }
        }
        return false;
    }

    /**
     * Exposed for testing
     */
    CollectingOutputReceiver createOutputCollector() {
        return new CollectingOutputReceiver();
    }

    /**
     * Exposed for testing
     */
    GoogleBenchmarkResultParser createResultParser(String runName,
            ITestInvocationListener listener) {
        return new GoogleBenchmarkResultParser(runName, listener);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (mDevice == null) {
            throw new IllegalArgumentException("Device has not been set");
        }
        String testPath = getTestPath();
        if (!mDevice.doesFileExist(testPath)) {
            CLog.w(String.format("Could not find native benchmark test directory %s in %s!",
                    testPath, mDevice.getSerialNumber()));
            throw new RuntimeException(
                    String.format("Could not find native benchmark test directory %s", testPath));
        }
        doRunAllTestsInSubdirectory(testPath, mDevice, listener);
    }
}
