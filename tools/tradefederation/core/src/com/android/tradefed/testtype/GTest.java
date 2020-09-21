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
import com.android.ddmlib.IShellOutputReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.FileUtil;

import com.google.common.annotations.VisibleForTesting;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

/** A Test that runs a native test package on given device. */
@OptionClass(alias = "gtest")
public class GTest
        implements IDeviceTest,
                IRemoteTest,
                ITestFilterReceiver,
                IRuntimeHintProvider,
                ITestCollector,
                IShardableTest,
                IStrictShardableTest {

    static final String DEFAULT_NATIVETEST_PATH = "/data/nativetest";
    private static final Pattern EXE_FILE = Pattern.compile("^[-l]r.x.+");

    private ITestDevice mDevice = null;
    private boolean mRunDisabledTests = false;

    @Option(name = "native-test-device-path",
            description="The path on the device where native tests are located.")
    private String mNativeTestDevicePath = DEFAULT_NATIVETEST_PATH;

    @Option(name = "file-exclusion-filter-regex",
            description = "Regex to exclude certain files from executing. Can be repeated")
    private List<String> mFileExclusionFilterRegex = new ArrayList<>();

    @Option(name = "module-name",
            description="The name of the native test module to run.")
    private String mTestModule = null;

    @Option(name = "positive-testname-filter",
            description="The GTest-based positive filter of the test name to run.")
    private String mTestNamePositiveFilter = null;
    @Option(name = "negative-testname-filter",
            description="The GTest-based negative filter of the test name to run.")
    private String mTestNameNegativeFilter = null;

    @Option(name = "include-filter",
            description="The GTest-based positive filter of the test names to run.")
    private Set<String> mIncludeFilters = new HashSet<>();
    @Option(name = "exclude-filter",
            description="The GTest-based negative filter of the test names to run.")
    private Set<String> mExcludeFilters = new HashSet<>();

    @Option(
        name = "native-test-timeout",
        description =
                "The max time for a gtest to run. Test run will be aborted if any test "
                        + "takes longer.",
        isTimeVal = true
    )
    private long mMaxTestTimeMs = 1 * 60 * 1000L;

    @Option(name = "send-coverage",
            description = "Send coverage target info to test listeners.")
    private boolean mSendCoverage = true;

    @Option(name ="prepend-filename",
            description = "Prepend filename as part of the classname for the tests.")
    private boolean mPrependFileName = false;

    @Option(name = "before-test-cmd",
            description = "adb shell command(s) to run before GTest.")
    private List<String> mBeforeTestCmd = new ArrayList<>();


    @Option(
        name = "reboot-before-test",
        description = "Reboot the device before the test suite starts."
    )
    private boolean mRebootBeforeTest = false;

    @Option(name = "after-test-cmd",
            description = "adb shell command(s) to run after GTest.")
    private List<String> mAfterTestCmd = new ArrayList<>();

    @Option(name = "run-test-as", description = "User to execute test binary as.")
    private String mRunTestAs = null;

    @Option(name = "ld-library-path",
            description = "LD_LIBRARY_PATH value to include in the GTest execution command.")
    private String mLdLibraryPath = null;

    @Option(name = "native-test-flag", description =
            "Additional flag values to pass to the native test's shell command. " +
            "Flags should be complete, including any necessary dashes: \"--flag=value\"")
    private List<String> mGTestFlags = new ArrayList<>();

    @Option(name = "runtime-hint", description="The hint about the test's runtime.",
            isTimeVal = true)
    private long mRuntimeHint = 60000;// 1 minute

    @Option(name = "xml-output", description = "Use gtest xml output for test results, "
            + "if test binaries crash, no output will be available.")
    private boolean mEnableXmlOutput = false;

    @Option(name = "stop-runtime",
            description = "Stops the Java application runtime before test execution.")
    private boolean mStopRuntime = false;

    @Option(name = "collect-tests-only",
            description = "Only invoke the test binary to collect list of applicable test cases. "
                    + "All test run callbacks will be triggered, but test execution will "
                    + "not be actually carried out. This option ignores sharding parameters, so "
                    + "each shard will end up collecting all tests.")
    private boolean mCollectTestsOnly = false;

    @Option(name = "test-filter-key",
            description = "run the gtest with the --gtest_filter populated with the filter from "
                    + "the json filter file associated with the binary, the filter file will have "
                    + "the same name as the binary with the .json extension.")
    private String mTestFilterKey = null;

    private int mShardCount = 0;
    private int mShardIndex = 0;
    private boolean mIsSharded = false;

    /** coverage target value. Just report all gtests as 'native' for now */
    private static final String COVERAGE_TARGET = "Native";

    // GTest flags...
    private static final String GTEST_FLAG_PRINT_TIME = "--gtest_print_time";
    private static final String GTEST_FLAG_FILTER = "--gtest_filter";
    private static final String GTEST_FLAG_RUN_DISABLED_TESTS = "--gtest_also_run_disabled_tests";
    private static final String GTEST_FLAG_LIST_TESTS = "--gtest_list_tests";
    private static final String GTEST_XML_OUTPUT = "--gtest_output=xml:%s";
    // Max characters allowed for executing GTest via command line
    private static final int GTEST_CMD_CHAR_LIMIT = 1000;
    // Expected extension for the filter file associated with the binary (json formatted file)
    protected static final String FILTER_EXTENSION = ".filter";
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
     * Set the Android native test module to run.
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
     * Set whether GTest should run disabled tests.
     */
    public void setRunDisabled(boolean runDisabled) {
        mRunDisabledTests = runDisabled;
    }

    /**
     * Get whether GTest should run disabled tests.
     *
     * @return True if disabled tests should be run, false otherwise
     */
    public boolean getRunDisabledTests() {
        return mRunDisabledTests;
    }

    /**
     * Set the max time in ms for a gtest to run.
     */
    @VisibleForTesting
    void setMaxTestTimeMs(int timeout) {
        mMaxTestTimeMs = timeout;
    }

    /**
     * Adds an exclusion file filter regex.
     *
     * @param regex to exclude file.
     */
    @VisibleForTesting
    void addFileExclusionFilterRegex(String regex) {
        mFileExclusionFilterRegex.add(regex);
    }

    /**
     * Sets the shard index of this test.
     */
    @VisibleForTesting
    void setShardIndex(int shardIndex) {
        mShardIndex = shardIndex;
    }

    /**
     * Gets the shard index of this test.
     */
    @VisibleForTesting
    int getShardIndex() {
        return mShardIndex;
    }

    /**
     * Sets the shard count of this test.
     */
    @VisibleForTesting
    void setShardCount(int shardCount) {
        mShardCount = shardCount;
    }

    /**
     * Sets the shard count of this test.
     */
    @VisibleForTesting
    int getShardCount() {
        return mShardCount;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long getRuntimeHint() {
        return mRuntimeHint;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addIncludeFilter(String filter) {
        mIncludeFilters.add(cleanFilter(filter));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllIncludeFilters(Set<String> filters) {
        for (String filter : filters) {
            mIncludeFilters.add(cleanFilter(filter));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addExcludeFilter(String filter) {
        mExcludeFilters.add(cleanFilter(filter));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllExcludeFilters(Set<String> filters) {
        for (String filter : filters) {
            mExcludeFilters.add(cleanFilter(filter));
        }
    }

    /*
     * Conforms filters using a {@link TestDescription} format to be recognized by the GTest
     * executable.
     */
    private String cleanFilter(String filter) {
        return filter.replace('#', '.');
    }

    /**
     * Helper to get the adb gtest filter of test to run.
     *
     * Note that filters filter on the function name only (eg: Google Test "Test"); all Google Test
     * "Test Cases" will be considered.
     *
     * @param binaryOnDevice the full path of the binary on the device.
     * @return the full filter flag to pass to the Gtest, or an empty string if none have been
     * specified
     */
    private String getGTestFilters(String binaryOnDevice) throws DeviceNotAvailableException {
        StringBuilder filter = new StringBuilder();
        if (mTestNamePositiveFilter != null) {
            mIncludeFilters.add(mTestNamePositiveFilter);
        }
        if (mTestNameNegativeFilter != null) {
            mExcludeFilters.add(mTestNameNegativeFilter);
        }
        if (mTestFilterKey != null) {
            if (!mIncludeFilters.isEmpty() || !mExcludeFilters.isEmpty()) {
                CLog.w("Using json file filter, --include/exclude-filter will be ignored.");
            }
            String fileFilters = loadFilter(binaryOnDevice);
            if (fileFilters != null && !fileFilters.isEmpty()) {
                filter.append(GTEST_FLAG_FILTER);
                filter.append("=");
                filter.append(fileFilters);
            }
        } else {
            if (!mIncludeFilters.isEmpty() || !mExcludeFilters.isEmpty()) {
                filter.append(GTEST_FLAG_FILTER);
                filter.append("=");
                if (!mIncludeFilters.isEmpty()) {
                  filter.append(ArrayUtil.join(":", mIncludeFilters));
                }
                if (!mExcludeFilters.isEmpty()) {
                  filter.append("-");
                  filter.append(ArrayUtil.join(":", mExcludeFilters));
              }
            }
        }
        return filter.toString();
    }

    private String loadFilter(String binaryOnDevice) throws DeviceNotAvailableException {
        CLog.i("Loading filter from file for key: '%s'", mTestFilterKey);
        String filterFile = String.format("%s%s", binaryOnDevice, FILTER_EXTENSION);
        if (getDevice().doesFileExist(filterFile)) {
            String content =
                    getDevice().executeShellCommand(String.format("cat \"%s\"", filterFile));
            if (content != null && !content.isEmpty()) {
                try {
                    JSONObject filter = new JSONObject(content);
                    String key = mTestFilterKey;
                    JSONObject filterObject = filter.getJSONObject(key);
                    return filterObject.getString("filter");
                } catch (JSONException e) {
                    CLog.e(e);
                }
            }
            CLog.e("Error with content of the filter file %s: %s", filterFile, content);
        } else {
            CLog.e("Filter file %s not found", filterFile);
        }
        return null;
    }

    /**
     * Helper to get all the GTest flags to pass into the adb shell command.
     *
     * @param binaryOnDevice the full path of the binary on the device.
     * @return the {@link String} of all the GTest flags that should be passed to the GTest
     */
    private String getAllGTestFlags(String binaryOnDevice) throws DeviceNotAvailableException {
        String flags = String.format("%s %s", GTEST_FLAG_PRINT_TIME,
                getGTestFilters(binaryOnDevice));

        if (mRunDisabledTests) {
            flags = String.format("%s %s", flags, GTEST_FLAG_RUN_DISABLED_TESTS);
        }

        if (mCollectTestsOnly) {
            flags = String.format("%s %s", flags, GTEST_FLAG_LIST_TESTS);
        }

        for (String gTestFlag : mGTestFlags) {
            flags = String.format("%s %s", flags, gTestFlag);
        }
        return flags;
    }

    /**
     * Gets the path where native tests live on the device.
     *
     * @return The path on the device where the native tests live.
     */
    private String getTestPath() {
        StringBuilder testPath = new StringBuilder(mNativeTestDevicePath);
        if (mTestModule != null) {
            testPath.append(FileListingService.FILE_SEPARATOR);
            testPath.append(mTestModule);
        }
        return testPath.toString();
    }

    /**
     * Executes all native tests in a folder as well as in all subfolders recursively.
     *
     * @param root The root folder to begin searching for native tests
     * @param testDevice The device to run tests on
     * @param listener the {@link ITestInvocationListener}
     * @throws DeviceNotAvailableException
     */
    @VisibleForTesting
    void doRunAllTestsInSubdirectory(
            String root, ITestDevice testDevice, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        if (testDevice.isDirectory(root)) {
            // recursively run tests in all subdirectories
            for (String child : testDevice.getChildren(root)) {
                doRunAllTestsInSubdirectory(root + "/" + child, testDevice, listener);
            }
        } else {
            // assume every file is a valid gtest binary.
            IShellOutputReceiver resultParser = createResultParser(getFileName(root), listener);
            if (shouldSkipFile(root)) {
                return;
            }
            String flags = getAllGTestFlags(root);
            CLog.i("Running gtest %s %s on %s", root, flags, testDevice.getSerialNumber());
            if (mEnableXmlOutput) {
                runTestXml(testDevice, root, flags, listener);
            } else {
                runTest(testDevice, resultParser, root, flags);
            }
        }
    }

    String getFileName(String fullPath) {
        int pos = fullPath.lastIndexOf('/');
        if (pos == -1) {
            return fullPath;
        }
        String fileName = fullPath.substring(pos + 1);
        if (fileName.isEmpty()) {
            throw new IllegalArgumentException("input should not end with \"/\"");
        }
        return fileName;
    }

    protected boolean isDeviceFileExecutable(String fullPath) throws DeviceNotAvailableException {
        String fileMode = mDevice.executeShellCommand(String.format("ls -l %s", fullPath));
        if (fileMode != null) {
            return EXE_FILE.matcher(fileMode).find();
        }
        return false;
    }

    /**
     * Helper method to determine if we should skip the execution of a given file.
     *
     * @param fullPath the full path of the file in question
     * @return true if we should skip the said file.
     */
    protected boolean shouldSkipFile(String fullPath) throws DeviceNotAvailableException {
        if (fullPath == null || fullPath.isEmpty()) {
            return true;
        }
        // skip any file that's not executable
        if (!isDeviceFileExecutable(fullPath)) {
            return true;
        }
        if (mFileExclusionFilterRegex == null || mFileExclusionFilterRegex.isEmpty()) {
            return false;
        }
        for (String regex : mFileExclusionFilterRegex) {
            if (fullPath.matches(regex)) {
                CLog.i("File %s matches exclusion file regex %s, skipping", fullPath, regex);
                return true;
            }
        }
        return false;
    }

    /**
     * Helper method to run a gtest command from a temporary script, in the case that the command
     * is too long to be run directly by adb.
     * @param testDevice the device on which to run the command
     * @param cmd the command string to run
     * @param resultParser the output receiver for reading test results
     */
    protected void executeCommandByScript(final ITestDevice testDevice, final String cmd,
            final IShellOutputReceiver resultParser) throws DeviceNotAvailableException {
        String tmpFileDevice = "/data/local/tmp/gtest_script.sh";
        testDevice.pushString(String.format("#!/bin/bash\n%s", cmd), tmpFileDevice);
        // force file to be executable
        testDevice.executeShellCommand(String.format("chmod 755 %s", tmpFileDevice));
        testDevice.executeShellCommand(String.format("sh %s", tmpFileDevice),
                resultParser, mMaxTestTimeMs /* maxTimeToShellOutputResponse */,
                TimeUnit.MILLISECONDS, 0 /* retry attempts */);
        testDevice.executeShellCommand(String.format("rm %s", tmpFileDevice));
    }

    /**
     * Run the given gtest binary
     *
     * @param testDevice the {@link ITestDevice}
     * @param resultParser the test run output parser
     * @param fullPath absolute file system path to gtest binary on device
     * @param flags gtest execution flags
     * @throws DeviceNotAvailableException
     */
    private void runTest(final ITestDevice testDevice, final IShellOutputReceiver resultParser,
            final String fullPath, final String flags) throws DeviceNotAvailableException {
        // TODO: add individual test timeout support, and rerun support
        try {
            for (String cmd : mBeforeTestCmd) {
                testDevice.executeShellCommand(cmd);
            }

            if (mRebootBeforeTest) {
                CLog.d("Rebooting device before test starts as requested.");
                testDevice.reboot();
            }

            String cmd = getGTestCmdLine(fullPath, flags);
            // ensure that command is not too long for adb
            if (cmd.length() < GTEST_CMD_CHAR_LIMIT) {
                testDevice.executeShellCommand(cmd, resultParser,
                        mMaxTestTimeMs /* maxTimeToShellOutputResponse */,
                        TimeUnit.MILLISECONDS,
                        0 /* retryAttempts */);
            } else {
                // wrap adb shell command in script if command is too long for direct execution
                executeCommandByScript(testDevice, cmd, resultParser);
            }
        } catch (DeviceNotAvailableException e) {
            throw e;
        } catch (RuntimeException e) {
            throw e;
        } finally {
            // TODO: consider moving the flush of parser data on exceptions to TestDevice or
            // AdbHelper
            resultParser.flush();
            for (String cmd : mAfterTestCmd) {
                testDevice.executeShellCommand(cmd);
            }
        }
    }

    /**
     * Run the given gtest binary and parse XML results This methods typically requires the filter
     * for .tff and .xml files, otherwise it will post some unwanted results.
     *
     * @param testDevice the {@link ITestDevice}
     * @param fullPath absolute file system path to gtest binary on device
     * @param flags gtest execution flags
     * @param listener the {@link ITestInvocationListener}
     * @throws DeviceNotAvailableException
     */
    private void runTestXml(
            final ITestDevice testDevice,
            final String fullPath,
            final String flags,
            ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        CollectingOutputReceiver outputCollector = new CollectingOutputReceiver();
        File tmpOutput = null;
        try {
            String testRunName = fullPath.substring(fullPath.lastIndexOf("/") + 1);
            tmpOutput = FileUtil.createTempFile(testRunName, ".xml");
            String tmpResName = fullPath + "_res.xml";
            String extraFlag = String.format(GTEST_XML_OUTPUT, tmpResName);
            String fullFlagCmd =  String.format("%s %s", flags, extraFlag);

            // Run the tests with modified flags
            runTest(testDevice, outputCollector, fullPath, fullFlagCmd);
            // Pull the result file, may not exist if issue with the test.
            testDevice.pullFile(tmpResName, tmpOutput);
            // Clean the file on the device
            testDevice.executeShellCommand("rm " + tmpResName);
            GTestXmlResultParser parser = createXmlParser(testRunName, listener);
            // Attempt to parse the file, doesn't matter if the content is invalid.
            if (tmpOutput.exists()) {
                parser.parseResult(tmpOutput, outputCollector);
            }
        } catch (DeviceNotAvailableException | RuntimeException e) {
            throw e;
        } catch (IOException e) {
            throw new RuntimeException(e);
        } finally {
            outputCollector.flush();
            for (String cmd : mAfterTestCmd) {
                testDevice.executeShellCommand(cmd);
            }
            FileUtil.deleteFile(tmpOutput);
        }
    }

    /**
     * Exposed for testing
     *
     * @param testRunName
     * @param listener
     * @return a {@link GTestXmlResultParser}
     */
    @VisibleForTesting
    GTestXmlResultParser createXmlParser(String testRunName, ITestInvocationListener listener) {
        return new GTestXmlResultParser(testRunName, listener);
    }

    /**
     * Helper method to build the gtest command to run.
     *
     * @param fullPath absolute file system path to gtest binary on device
     * @param flags gtest execution flags
     * @return the shell command line to run for the gtest
     */
    protected String getGTestCmdLine(String fullPath, String flags) {
        StringBuilder gTestCmdLine = new StringBuilder();
        if (mLdLibraryPath != null) {
            gTestCmdLine.append(String.format("LD_LIBRARY_PATH=%s ", mLdLibraryPath));
        }
        if (mShardCount > 0) {
            if (mCollectTestsOnly) {
                CLog.w("--collect-tests-only option ignores sharding parameters, and will cause "
                        + "each shard to collect all tests.");
            }
            gTestCmdLine.append(String.format("GTEST_SHARD_INDEX=%s ", mShardIndex));
            gTestCmdLine.append(String.format("GTEST_TOTAL_SHARDS=%s ", mShardCount));
        }

        // su to requested user
        if (mRunTestAs != null) {
            gTestCmdLine.append(String.format("su %s ", mRunTestAs));
        }

        gTestCmdLine.append(String.format("%s %s", fullPath, flags));
        return gTestCmdLine.toString();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IRemoteTest getTestShard(int shardCount, int shardIndex) {
        GTest shard = new GTest();
        OptionCopier.copyOptionsNoThrow(this, shard);
        shard.mShardIndex = shardIndex;
        shard.mShardCount = shardCount;
        shard.mIsSharded = true;
        // We approximate the runtime of each shard to be equal since we can't know.
        shard.mRuntimeHint = mRuntimeHint / shardCount;
        return shard;
    }

    /** {@inheritDoc} */
    @Override
    public Collection<IRemoteTest> split(int shardCountHint) {
        if (shardCountHint <= 1 || mIsSharded) {
            return null;
        }
        Collection<IRemoteTest> tests = new ArrayList<>();
        for (int i = 0; i < shardCountHint; i++) {
            tests.add(getTestShard(shardCountHint, i));
        }
        return tests;
    }

    /**
     * Factory method for creating a {@link IShellOutputReceiver} that parses test output and
     * forwards results to the result listener.
     *
     * @param listener
     * @param runName
     * @return a {@link IShellOutputReceiver}
     */
    @VisibleForTesting
    IShellOutputReceiver createResultParser(String runName, ITestInvocationListener listener) {
        IShellOutputReceiver receiver = null;
        if (mCollectTestsOnly) {
            GTestListTestParser resultParser = new GTestListTestParser(runName, listener);
            resultParser.setPrependFileName(mPrependFileName);
            receiver = resultParser;
        } else {
            GTestResultParser resultParser = new GTestResultParser(runName, listener);
            resultParser.setPrependFileName(mPrependFileName);
            // TODO: find a better solution for sending coverage info
            if (mSendCoverage) {
                resultParser.setCoverageTarget(COVERAGE_TARGET);
            }
            receiver = resultParser;
        }
        return receiver;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // TODO: add support for rerunning tests
        if (mDevice == null) {
            throw new IllegalArgumentException("Device has not been set");
        }

        String testPath = getTestPath();
        if (!mDevice.doesFileExist(testPath)) {
            CLog.w("Could not find native test directory %s in %s!", testPath,
                    mDevice.getSerialNumber());
            return;
        }
        if (mStopRuntime) {
            mDevice.executeShellCommand("stop");
        }
        Throwable throwable = null;
        try {
            doRunAllTestsInSubdirectory(testPath, mDevice, listener);
        } catch (Throwable t) {
            throwable = t;
            throw t;
        } finally {
            if (!(throwable instanceof DeviceNotAvailableException)) {
                if (mStopRuntime) {
                    mDevice.executeShellCommand("start");
                    mDevice.waitForDeviceAvailable();
                }
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setCollectTestsOnly(boolean shouldCollectTest) {
        mCollectTestsOnly = shouldCollectTest;
    }

}
