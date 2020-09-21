/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ITestLifeCycleReceiver;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.targetprep.VtsCoveragePreparer;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.JsonUtil;
import com.android.tradefed.util.RunInterruptedException;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.VtsDashboardUtil;
import com.android.tradefed.util.VtsPythonRunnerHelper;
import com.android.tradefed.util.VtsVendorConfigFileUtil;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Set;
import java.util.TreeSet;

/**
 * A Test that runs a vts multi device test package (part of Vendor Test Suite,
 * VTS) on given device.
 */

@OptionClass(alias = "vtsmultidevicetest")
public class VtsMultiDeviceTest
        implements IDeviceTest, IRemoteTest, ITestFilterReceiver, IRuntimeHintProvider,
                   ITestCollector, IBuildReceiver, IAbiReceiver, IInvocationContextReceiver {
    static final String ANDROIDDEVICE = "AndroidDevice";
    static final String BUILD = "build";
    static final String BUILD_ID = "build_id";
    static final String BUILD_TARGET = "build_target";
    static final String COVERAGE_PROPERTY = "ro.vts.coverage";
    static final String DATA_FILE_PATH = "data_file_path";
    static final String LOG_PATH = "log_path";
    static final String LOG_SEVERITY = "log_severity";
    static final String NAME = "name";
    static final String SERIAL = "serial";
    static final String TESTMODULE = "TestModule";
    static final String TEST_BED = "test_bed";
    static final String TEST_PLAN_REPORT_FILE = "TEST_PLAN_REPORT_FILE";
    static final String TEST_SUITE = "test_suite";
    static final String TEST_MAX_TIMEOUT = "test_max_timeout";
    static final String VIRTUAL_ENV_PATH = "VIRTUALENVPATH";
    static final String ABI_NAME = "abi_name";
    static final String ABI_BITNESS = "abi_bitness";
    static final String SKIP_ON_32BIT_ABI = "skip_on_32bit_abi";
    static final String SKIP_ON_64BIT_ABI = "skip_on_64bit_abi";
    static final String SKIP_IF_THERMAL_THROTTLING = "skip_if_thermal_throttling";
    static final String DISABLE_CPU_FREQUENCY_SCALING = "disable_cpu_frequency_scaling";
    static final String RUN_32BIT_ON_64BIT_ABI = "run_32bit_on_64bit_abi";
    static final String CONFIG_FILE_EXTENSION = ".config";
    static final String INCLUDE_FILTER = "include_filter";
    static final String EXCLUDE_FILTER = "exclude_filter";
    static final String EXCLUDE_OVER_INCLUDE = "exclude_over_include";
    static final String BINARY_TEST_SOURCE = "binary_test_source";
    static final String BINARY_TEST_WORKING_DIRECTORY = "binary_test_working_directory";
    static final String BINARY_TEST_ENVP = "binary_test_envp";
    static final String BINARY_TEST_ARGS = "binary_test_args";
    static final String BINARY_TEST_LD_LIBRARY_PATH = "binary_test_ld_library_path";
    static final String BINARY_TEST_PROFILING_LIBRARY_PATH = "binary_test_profiling_library_path";
    static final String BINARY_TEST_DISABLE_FRAMEWORK = "binary_test_disable_framework";
    static final String BINARY_TEST_STOP_NATIVE_SERVERS = "binary_test_stop_native_servers";
    static final String BINARY_TEST_TYPE_GTEST = "gtest";
    static final String BINARY_TEST_TYPE_LLVMFUZZER = "llvmfuzzer";
    static final String BINARY_TEST_TYPE_HAL_HIDL_GTEST = "hal_hidl_gtest";
    static final String BINARY_TEST_TYPE_HAL_HIDL_REPLAY_TEST = "hal_hidl_replay_test";
    static final String BINARY_TEST_TYPE_HOST_BINARY_TEST = "host_binary_test";
    static final String BUG_REPORT_ON_FAILURE = "BUG_REPORT_ON_FAILURE";
    static final String COLLECT_TESTS_ONLY = "collect_tests_only";
    static final String LOGCAT_ON_FAILURE = "LOGCAT_ON_FAILURE";
    static final String ENABLE_COVERAGE = "enable_coverage";
    static final String EXCLUDE_COVERAGE_PATH = "exclude_coverage_path";
    static final String ENABLE_PROFILING = "enable_profiling";
    static final String ENABLE_SANCOV = "enable_sancov";
    static final String GTEST_BATCH_MODE = "gtest_batch_mode";
    static final String SAVE_TRACE_FIEL_REMOTE = "save_trace_file_remote";
    static final String OUTPUT_COVERAGE_REPORT = "output_coverage_report";
    static final String COVERAGE_REPORT_PATH = "coverage_report_path";
    static final String GLOBAL_COVERAGE = "global_coverage";
    static final String LTP_NUMBER_OF_THREADS = "ltp_number_of_threads";
    static final String MOBLY_TEST_MODULE = "MOBLY_TEST_MODULE";
    static final String NATIVE_SERVER_PROCESS_NAME = "native_server_process_name";
    static final String PASSTHROUGH_MODE = "passthrough_mode";
    static final String PRECONDITION_HWBINDER_SERVICE = "precondition_hwbinder_service";
    static final String PRECONDITION_FEATURE = "precondition_feature";
    static final String PRECONDITION_FILE_PATH_PREFIX = "precondition_file_path_prefix";
    static final String PRECONDITION_FIRST_API_LEVEL = "precondition_first_api_level";
    static final String PRECONDITION_LSHAL = "precondition_lshal";
    static final String PRECONDITION_SYSPROP = "precondition_sysprop";
    static final String PRECONDITION_VINTF = "precondition_vintf";
    static final String ENABLE_SYSTRACE = "enable_systrace";
    static final String HAL_HIDL_REPLAY_TEST_TRACE_PATHS = "hal_hidl_replay_test_trace_paths";
    static final String HAL_HIDL_PACKAGE_NAME = "hal_hidl_package_name";
    static final String REPORT_MESSAGE_FILE_NAME = "report_proto.msg";
    static final String RUN_AS_VTS_SELF_TEST = "run_as_vts_self_test";
    static final String RUN_AS_COMPLIANCE_TEST = "run_as_compliance_test";
    static final String SYSTRACE_PROCESS_NAME = "systrace_process_name";
    static final String TEMPLATE_BINARY_TEST_PATH = "vts/testcases/template/binary_test/binary_test";
    static final String TEMPLATE_GTEST_BINARY_TEST_PATH = "vts/testcases/template/gtest_binary_test/gtest_binary_test";
    static final String TEMPLATE_LLVMFUZZER_TEST_PATH = "vts/testcases/template/llvmfuzzer_test/llvmfuzzer_test";
    static final String TEMPLATE_MOBLY_TEST_PATH = "vts/testcases/template/mobly/mobly_test";
    static final String TEMPLATE_HAL_HIDL_GTEST_PATH = "vts/testcases/template/hal_hidl_gtest/hal_hidl_gtest";
    static final String TEMPLATE_HAL_HIDL_REPLAY_TEST_PATH = "vts/testcases/template/hal_hidl_replay_test/hal_hidl_replay_test";
    static final String TEMPLATE_HOST_BINARY_TEST_PATH = "vts/testcases/template/host_binary_test/host_binary_test";
    static final long TEST_ABORT_TIMEOUT_MSECS = 1000 * 15;
    static final String TEST_RUN_SUMMARY_FILE_NAME = "test_run_summary.json";
    static final float DEFAULT_TARGET_VERSION = -1;
    static final String DEFAULT_TESTCASE_CONFIG_PATH =
            "vts/tools/vts-tradefed/res/default/DefaultTestCase.runner_conf";

    private ITestDevice mDevice = null;
    private IAbi mAbi = null;

    @Option(name = "test-timeout",
            description = "The amount of time (in milliseconds) for a test invocation. "
                    + "If the test cannot finish before timeout, it should interrupt itself and "
                    + "clean up in " + TEST_ABORT_TIMEOUT_MSECS + "ms. Hence the actual timeout "
                    + "is the specified value + " + TEST_ABORT_TIMEOUT_MSECS + "ms.",
            isTimeVal = true)
    private long mTestTimeout = 1000 * 60 * 60 * 3;

    @Option(name = "test-module-name",
        description = "The name for a test module.")
    private String mTestModuleName = null;

    @Option(name = "test-case-path",
            description = "The path for test case.")
    private String mTestCasePath = null;

    @Option(name = "test-case-path-type",
            description = "The type of test case path ('module' by default or 'file').")
    private String mTestCasePathType = null;

    @Option(name = "python-version", description = "The version of a Python interpreter to use.")
    private String mPythonVersion = "";

    @Option(name = "test-config-path",
            description = "The path for test case config file.")
    private String mTestConfigPath = null;

    @Option(name = "precondition-hwbinder-service",
            description = "The name of a HW binder service needed to run the test.")
    private String mPreconditionHwBinderServiceName = null;

    @Option(name = "precondition-feature",
        description = "The name of a `pm`-listable feature needed to run the test.")
    private String mPreconditionFeature = null;

    @Option(name = "precondition-file-path-prefix",
            description = "The path prefix of a target-side file needed to run the test."
                    + "Format of tags:"
                    + "    <source>: source without tag."
                    + "    <tag>::<source>: <tag> specifies bitness of testcase: _32bit or _64bit"
                    + "    Note: multiple sources are ANDed"
                    + "Format of each source string:"
                    + "    <source>: absolute path of file prefix on device")
    private Collection<String> mPreconditionFilePathPrefix = new ArrayList<>();

    @Option(name = "precondition-first-api-level",
            description = "The lowest first API level required to run the test.")
    private int mPreconditionFirstApiLevel = 0;

    @Option(name = "precondition-lshal",
        description = "The name of a `lshal`-listable feature needed to run the test.")
    private String mPreconditionLshal = null;

    @Option(name = "precondition-sysprop",
            description = "The name=value for a system property configuration that needs "
                    + "to be met to run the test.")
    private String mPreconditionSysProp = null;

    @Option(name = "precondition-vintf",
            description = "The full name of a HAL specified in vendor/manifest.xml and "
                    + "needed to run the test (e.g., android.hardware.graphics.mapper@2.0). "
                    + "this can override precondition-lshal option.")
    private String mPreconditionVintf = null;

    @Option(name = "use-stdout-logs",
            description = "Flag that determines whether to use std:out to parse output.")
    private boolean mUseStdoutLogs = false;

    @Option(name = "include-filter",
            description = "The positive filter of the test names to run.")
    private Set<String> mIncludeFilters = new TreeSet<>();

    @Option(name = "exclude-filter",
            description = "The negative filter of the test names to run.")
    private Set<String> mExcludeFilters = new TreeSet<>();

    @Option(name = "exclude-over-include",
            description = "The negative filter of the test names to run.")
    private boolean mExcludeOverInclude = false;

    @Option(name = "runtime-hint", description = "The hint about the test's runtime.",
            isTimeVal = true)
    private long mRuntimeHint = 60000;  // 1 minute

    @Option(name = "enable-profiling", description = "Enable profiling for the tests.")
    private boolean mEnableProfiling = false;

    @Option(name = "save-trace-file-remote",
            description = "Whether to save the trace file in remote storage.")
    private boolean mSaveTraceFileRemote = false;

    @Option(name = "enable-systrace", description = "Enable systrace for the tests.")
    private boolean mEnableSystrace = false;

    @Option(name = "enable-coverage",
            description = "Enable coverage for the tests. In order for coverage to be measured, " +
                          "ro.vts.coverage system must have value \"1\" to indicate the target " +
                          "build is coverage instrumented.")
    private boolean mEnableCoverage = true;

    @Option(name = "global-coverage", description = "True to measure coverage for entire test, "
                    + "measure coverage for each test case otherwise. Currently, only global "
                    + "coverage is supported for binary tests")
    private boolean mGlobalCoverage = true;

    @Option(name = "enable-sancov",
            description = "Enable sanitizer coverage for the tests. In order for coverage to be "
                    + "measured, the device must be a sancov build with its build info and "
                    + "unstripped binaries available to the sancov preparer class.")
    private boolean mEnableSancov = true;

    @Option(name = "output-coverage-report", description = "Whether to store raw coverage report.")
    private boolean mOutputCoverageReport = false;

    // Another design option is to parse a string or use enum for host preference on BINDER,
    // PASSTHROUGH and DEFAULT(which is BINDER). Also in the future, we might want to deal with
    // the case of target preference on PASSTHROUGH (if host does not specify to use BINDER mode).
    @Option(name = "passthrough-mode", description = "Set getStub to use passthrough mode. "
        + "Value true means use passthrough mode if available; false for binderized mode if "
        + "available. Default is false")
    private boolean mPassthroughMode = false;

    @Option(name = "ltp-number-of-threads",
            description = "Number of threads to run the LTP test cases. "
                    + "0 means using number of avaiable CPU threads.")
    private int mLtpNumberOfThreads = -1;

    @Option(name = "skip-on-32bit-abi",
        description = "Whether to skip tests on 32bit ABI.")
    private boolean mSkipOn32BitAbi = false;

    @Option(name = "skip-on-64bit-abi",
        description = "Whether to skip tests on 64bit ABI.")
    private boolean mSkipOn64BitAbi = false;

    @Option(name = "skip-if-thermal-throttling",
            description = "Whether to skip tests if target device suffers from thermal throttling.")
    private boolean mSkipIfThermalThrottling = false;

    @Option(name = "disable-cpu-frequency-scaling",
            description = "Whether to disable cpu frequency scaling for test.")
    private boolean mDisableCpuFrequencyScaling = true;

    @Option(name = "run-32bit-on-64bit-abi",
            description = "Whether to run 32bit tests on 64bit ABI.")
    private boolean mRun32bBitOn64BitAbi = false;

    @Option(name = "binary-test-source",
            description = "Binary test source paths relative to vts testcase directory on host."
                    + "Format of tags:"
                    + "    <source>: source without tag."
                    + "    <tag>::<source>: source with tag. Can be used to separate 32bit and 64"
                    + "            bit tests with same file name."
                    + "    <tag1>::<source1>, <tag1>::<source2>, <tag2>::<source3>: multiple"
                    + "            sources connected by comma. White spaces in-between"
                    + "            will be ignored."
                    + "Format of each source string:"
                    + "    <source file>: push file and create test case."
                    + "            Source is relative path of source file under vts's testcases"
                    + "            folder. Source file will be pushed to working directory"
                    + "            discarding original directory structure, and test cases will"
                    + "            be created using the pushed file."
                    + "    <source file>-><destination file>: push file and create test case."
                    + "            Destination path is absolute path on device. Test cases will"
                    + "            be created using the pushed file."
                    + "    <source file>->: push file only."
                    + "            Push the source file to its' tag's corresponding"
                    + "            working directory. Test case will not be created on"
                    + "            this file. This is equivalent to but simpler than specifying a"
                    + "            working directory for the tag and use VtsFilePusher to push the"
                    + "            file to the directory."
                    + "    -><destination file>: create test case only."
                    + "            Destination is absolute device side path."
                    + "    Note: each path in source string can be a directory. However, the"
                    + "          default binary test runner and gtest binary test runner does not"
                    + "          support creating test cases from a directory. You will need to"
                    + "          override the binary test runner's CreateTestCase method in python."
                    + "    If you wish to push a source file to a specific destination and not"
                    + "    create a test case from it, please use VtsFilePusher.")
    private Collection<String> mBinaryTestSource = new ArrayList<>();

    @Option(name = "binary-test-working-directory", description = "Working directories for binary "
                    + "tests. Tags can be added to the front of each directory using '::' as delimiter. "
                    + "However, each tag should only has one working directory. This option is optional for "
                    + "binary tests. If not specified, different directories will be used for files with "
                    + "different tags.")
    private Collection<String> mBinaryTestWorkingDirectory = new ArrayList<>();

    @Option(name = "binary-test-envp", description = "Additional environment path for binary "
        + "tests. Tags can be added to the front of each directory using '::' as delimiter. "
        + "There can be multiple instances of binary-test-envp for a same tag, which will "
        + "later automatically be combined.")
    private Collection<String> mBinaryTestEnvp = new ArrayList<>();

    @Option(name = "binary-test-args", description = "Additional args or flags for binary "
        + "tests. Tags can be added to the front of each directory using '::' as delimiter. "
        + "There can be multiple instances of binary-test-args for a same tag, which will "
        + "later automatically be combined.")
    private Collection<String> mBinaryTestArgs = new ArrayList<>();

    @Option(name = "binary-test-ld-library-path", description = "LD_LIBRARY_PATH for binary "
                    + "tests. Tags can be added to the front of each instance using '::' as delimiter. "
                    + "Multiple directories can be added under a same tag using ':' as delimiter. "
                    + "There can be multiple instances of ld-library-path for a same tag, which will "
                    + "later automatically be combined using ':' as delimiter. Paths without a tag "
                    + "will only used for binaries without tag. This option is optional for binary tests.")
    private Collection<String> mBinaryTestLdLibraryPath = new ArrayList<>();

    @Option(name = "binary-test-profiling-library-path", description = "Path to lookup and load "
            + "profiling libraries for tests with profiling enabled. Tags can be added to the "
            + "front of each directory using '::' as delimiter. Only one directory could be "
            + "specified for the same tag. This option is optional for binary tests. If not "
            + "specified, default directories will be used for files with different tags.")
    private Collection<String> mBinaryTestProfilingLibraryPath = new ArrayList<>();

    @Option(name = "binary-test-disable-framework", description = "Adb stop/start before/after test.")
    private boolean mBinaryTestDisableFramework = false;

    @Option(name = "binary-test-stop-native-servers",
            description = "Set to stop all properly configured native servers during the testing.")
    private boolean mBinaryTestStopNativeServers = false;

    @Option(name = "bug-report-on-failure",
            description = "To catch bugreport zip file at the end of failed test cases. "
                    + "If set to true, a report will be caught through adh shell command at the "
                    + "end of each failed test cases.")
    private boolean mBugReportOnFailure = false;

    @Option(name = "logcat-on-failure",
            description = "To catch logcat from each buffers at the end of failed test cases. "
                    + "If set to true, a report will be caught through adh shell command at the "
                    + "end of each failed test cases.")
    private boolean mLogcatOnFailure = true;

    @Option(name = "native-server-process-name",
            description = "Name of a native server process. The runner checks to make sure "
                    + "each specified native server process is not running after the framework stop.")
    private Collection<String> mNativeServerProcessName = new ArrayList<>();

    @Option(name = "binary-test-type", description = "Binary test type. Only specify this when "
            + "running an extended binary test without a python test file. Available options: gtest")
    private String mBinaryTestType = "";

    @Option(name = "hal-hidl-replay-test-trace-path", description = "The path of a trace file to replay.")
    private Collection<String> mHalHidlReplayTestTracePaths = new ArrayList<>();

    @Option(name = "hal-hidl-package-name", description = "The name of a target HIDL HAL package "
            + "e.g., 'android.hardware.light@2.0'.")
    private String mHalHidlPackageName = null;

    @Option(name = "systrace-process-name", description = "Process name for systrace.")
    private String mSystraceProcessName = null;

    @Option(name = "collect-tests-only",
            description = "Only invoke setUpClass, generate*, and tearDownClass to collect list "
                    + "of applicable test cases. All collected tests pass without being executed.")
    private boolean mCollectTestsOnly = false;

    @Option(name = "gtest-batch-mode", description = "Run Gtest binaries in batch mode.")
    private boolean mGtestBatchMode = false;

    @Option(name = "log-severity", description = "Set the log severity level.")
    private String mLogSeverity = "INFO";

    @Option(name = "python-binary", description = "python binary to use "
            + "(optional)")
    private String mPythonBin = null;

    @Option(name = "run-as-vts-self-test",
            description = "Run the module as vts-selftest. "
                    + "When the value is set to true, only setUpClass and tearDownClass function "
                    + "of the module will be called to ensure the framework is free of bug. "
                    + "Note that exception in tearDownClass will not be reported as failure.")
    private boolean mRunAsVtsSelfTest = false;

    @Option(name = "exclude-coverage-path",
            description = "The coverage path that should be excluded in results. "
                    + "Used only when enable-coverage is true.")
    private Collection<String> mExcludeCoveragePath = new ArrayList<>();

    @Option(name = "mobly-test-module",
            description = "Mobly test module name. "
                    + "If this value is specified, VTS will use mobly test template "
                    + "with the configurations."
                    + "Multiple values can be added by repeatly using this option.")
    private Collection<String> mMoblyTestModule = new ArrayList<>();

    private IRunUtil mRunUtil = null;
    private IBuildInfo mBuildInfo = null;
    private String mRunName = "VtsHostDrivenTest";
    // the path of a dir which contains the test data files.
    private String mTestCaseDataDir = "./";

    private VtsVendorConfigFileUtil configReader = null;
    private IInvocationContext mInvocationContext = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setInvocationContext(IInvocationContext context) {
        mInvocationContext = context;
        setDevice(context.getDevices().get(0));
        setBuild(context.getBuildInfos().get(0));
    }

    /**
     * @return the mInvocationContext
     */
    public IInvocationContext getInvocationContext() {
        return mInvocationContext;
    }

    /**
     * @return the mRunUtil
     */
    public IRunUtil getRunUtil() {
        return mRunUtil;
    }

    /**
     * @param mRunUtil the mRunUtil to set
     */
    public void setRunUtil(IRunUtil mRunUtil) {
        this.mRunUtil = mRunUtil;
    }

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

    public void setTestCasePath(String path){
        mTestCasePath = path;
    }

    public void setTestConfigPath(String path){
        mTestConfigPath = path;
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

    /**
     * Conforms filters using a {@link TestDescription} format
     * to be recognized by the GTest executable.
     */
    private String cleanFilter(String filter) {
        return filter.replace('#', '.');
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
    public void setCollectTestsOnly(boolean shouldCollectTest) {
        mCollectTestsOnly = shouldCollectTest;
    }

    /**
     * Generate a device json object from ITestDevice object.
     *
     * @param device device object
     * @throws RuntimeException
     * @throws JSONException
     */
    private JSONObject generateJsonDeviceItem(ITestDevice device) throws JSONException {
        JSONObject deviceItemObject = new JSONObject();
        deviceItemObject.put(SERIAL, device.getSerialNumber());
        try {
            deviceItemObject.put("product_type", device.getProductType());
            deviceItemObject.put("product_variant", device.getProductVariant());
            deviceItemObject.put("build_alias", device.getBuildAlias());
            deviceItemObject.put("build_id", device.getBuildId());
            deviceItemObject.put("build_flavor", device.getBuildFlavor());
        } catch (DeviceNotAvailableException e) {
            CLog.e("Device %s not available.", device.getSerialNumber());
            throw new RuntimeException("Failed to get device information");
        }
        return deviceItemObject;
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("deprecation")
    @Override
    public void run(ITestInvocationListener listener)
            throws IllegalArgumentException, DeviceNotAvailableException {
        if (mDevice == null) {
            throw new DeviceNotAvailableException("Device has not been set.");
        }

        if (mBuildInfo == null) {
            throw new RuntimeException("BuildInfo has not been set.");
        }

        if (mTestCasePath == null) {
            if (!mBinaryTestSource.isEmpty()) {
                String template;
                switch (mBinaryTestType) {
                    case BINARY_TEST_TYPE_GTEST:
                        template = TEMPLATE_GTEST_BINARY_TEST_PATH;
                        break;
                    case BINARY_TEST_TYPE_HAL_HIDL_GTEST:
                        template = TEMPLATE_HAL_HIDL_GTEST_PATH;
                        break;
                    case BINARY_TEST_TYPE_HOST_BINARY_TEST:
                        template = TEMPLATE_HOST_BINARY_TEST_PATH;
                        break;
                    default:
                        template = TEMPLATE_BINARY_TEST_PATH;
                }
                CLog.i("Using default test case template at %s.", template);
                setTestCasePath(template);
                if (mEnableCoverage && !mGlobalCoverage) {
                    CLog.e("Only global coverage is supported for test type %s.", mBinaryTestType);
                    throw new RuntimeException("Failed to produce VTS runner test config");
                }
            } else if (mBinaryTestType.equals(BINARY_TEST_TYPE_HAL_HIDL_REPLAY_TEST)) {
                setTestCasePath(TEMPLATE_HAL_HIDL_REPLAY_TEST_PATH);
            } else if (mBinaryTestType.equals(BINARY_TEST_TYPE_LLVMFUZZER)) {
                // Fuzz test don't need test-case-path.
                setTestCasePath(TEMPLATE_LLVMFUZZER_TEST_PATH);
            } else if (!mMoblyTestModule.isEmpty()) {
                setTestCasePath(TEMPLATE_MOBLY_TEST_PATH);
            } else {
                throw new IllegalArgumentException("test-case-path is not set.");
            }
        }

        doRunTest(listener);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    /**
     * Populate a jsonObject with default fields.
     * This method uses deepMergeJsonObjects method from JsonUtil to merge the default config file with the target
     * config file. Field already defined in target config file will not be overwritten. Also, JSONArray will not be
     * deep merged.
     *
     * @param jsonObject the target json object to populate
     * @param testCaseDataDir data file path
     * @throws IOException
     * @throws JSONException
     */
    private void populateDefaultJsonFields(JSONObject jsonObject, String testCaseDataDir)
            throws IOException, JSONException {
        CLog.i("Populating default fields to json object from %s", DEFAULT_TESTCASE_CONFIG_PATH);
        String content = FileUtil.readStringFromFile(new File(mTestCaseDataDir, DEFAULT_TESTCASE_CONFIG_PATH));
        JSONObject defaultJsonObject = new JSONObject(content);

        JsonUtil.deepMergeJsonObjects(jsonObject, defaultJsonObject);
    }

    /**
     * This method reads the provided VTS runner test json config, adds or updates some of its
     * fields (e.g., to add build info and device serial IDs), and returns the updated json object.
     * This method calls populateDefaultJsonFields to populate the config JSONObject if the config file is missing
     * or some required fields is missing from the JSONObject. If test name is not specified, this method
     * will use the config file's file name file without extension as test name. If config file is missing, this method
     * will use the test case's class name as test name.
     *
     * @param log_path the path of a directory to store the VTS runner logs.
     * @return the updated JSONObject as the new test config.
     */
    protected void updateVtsRunnerTestConfig(JSONObject jsonObject)
            throws IOException, JSONException, RuntimeException {
        configReader = new VtsVendorConfigFileUtil();
        if (configReader.LoadVendorConfig(mBuildInfo)) {
            JSONObject vendorConfigJson = configReader.GetVendorConfigJson();
            if (vendorConfigJson != null) {
                JsonUtil.deepMergeJsonObjects(jsonObject, vendorConfigJson);
            }
        }

        CLog.i("Load original test config %s %s", mTestCaseDataDir, mTestConfigPath);
        String content = null;

        if (mTestConfigPath != null) {
            content = FileUtil.readStringFromFile(
                    new File(Paths.get(mTestCaseDataDir, mTestConfigPath).toString()));
            CLog.i("Loaded original test config %s", content);
            if (content != null) {
                JsonUtil.deepMergeJsonObjects(jsonObject, new JSONObject(content));
            }
        }

        populateDefaultJsonFields(jsonObject, mTestCaseDataDir);
        CLog.i("Built a Json object using the loaded original test config");

        JSONArray deviceArray = new JSONArray();

        boolean coverageBuild = false;
        boolean sancovBuild = false;

        boolean first_device = true;
        for (ITestDevice device : mInvocationContext.getDevices()) {
            JSONObject deviceJson = generateJsonDeviceItem(device);
            try {
                String coverageProperty = device.getProperty(COVERAGE_PROPERTY);
                boolean enable_coverage_for_device =
                        coverageProperty != null && coverageProperty.equals("1");
                if (first_device) {
                    coverageBuild = enable_coverage_for_device;
                    first_device = false;
                } else {
                    if (coverageBuild && (!enable_coverage_for_device)) {
                        CLog.e("Device %s is not coverage build while others are.",
                                device.getSerialNumber());
                        throw new RuntimeException("Device build not the same.");
                    }
                }
            } catch (DeviceNotAvailableException e) {
                CLog.e("Device %s not available.", device.getSerialNumber());
                throw new RuntimeException("Failed to get device information");
            }

            File sancovDir =
                    mBuildInfo.getFile(VtsCoveragePreparer.getSancovResourceDirKey(device));
            if (sancovDir != null) {
                deviceJson.put("sancov_resources_path", sancovDir.getAbsolutePath());
                sancovBuild = true;
            }
            File gcovDir = mBuildInfo.getFile(VtsCoveragePreparer.getGcovResourceDirKey(device));
            if (gcovDir != null) {
                deviceJson.put("gcov_resources_path", gcovDir.getAbsolutePath());
                coverageBuild = true;
            }
            deviceArray.put(deviceJson);
        }

        JSONArray testBedArray = (JSONArray) jsonObject.get(TEST_BED);
        if (testBedArray.length() == 0) {
            JSONObject testBedItemObject = new JSONObject();
            String testName;
            if (mTestModuleName != null) {
                testName = mTestModuleName;
            } else {
                CLog.w("--test-module-name not set (not recommended); deriving automatically");
                if (mTestConfigPath != null) {
                    testName = new File(mTestConfigPath).getName();
                    testName = testName.replace(CONFIG_FILE_EXTENSION, "");
                } else if (mTestCasePath != null) {
                    testName = new File(mTestCasePath).getName();
                } else {
                    throw new RuntimeException(
                        "Failed to derive test module name; use --test-module-name option");
                }
            }
            CLog.logAndDisplay(LogLevel.INFO, "Setting test name as %s", testName);
            testBedItemObject.put(NAME, testName);
            testBedItemObject.put(ANDROIDDEVICE, deviceArray);
            testBedArray.put(testBedItemObject);
        } else if (testBedArray.length() == 1) {
            JSONObject testBedItemObject = (JSONObject) testBedArray.get(0);
            JSONArray androidDeviceArray = (JSONArray) testBedItemObject.get(ANDROIDDEVICE);
            int length;
            length = (androidDeviceArray.length() > deviceArray.length())
                    ? androidDeviceArray.length()
                    : deviceArray.length();
            for (int index = 0; index < length; index++) {
                if (index < androidDeviceArray.length()) {
                    if (index < deviceArray.length()) {
                        JsonUtil.deepMergeJsonObjects((JSONObject) androidDeviceArray.get(index),
                                (JSONObject) deviceArray.get(index));
                    }
                } else if (index < deviceArray.length()) {
                    androidDeviceArray.put(index, deviceArray.get(index));
                }
            }
        } else {
            CLog.e("Multi-device not yet supported: %d devices requested",
                    testBedArray.length());
            throw new RuntimeException("Failed to produce VTS runner test config");
        }
        jsonObject.put(DATA_FILE_PATH, mTestCaseDataDir);
        CLog.i("Added %s = %s to the Json object", DATA_FILE_PATH, mTestCaseDataDir);

        JSONObject build = new JSONObject();
        build.put(BUILD_ID, mBuildInfo.getBuildId());
        build.put(BUILD_TARGET, mBuildInfo.getBuildTargetName());
        jsonObject.put(BUILD, build);
        CLog.i("Added %s to the Json object", BUILD);

        JSONObject suite = new JSONObject();
        suite.put(NAME, mBuildInfo.getTestTag());
        suite.put(INCLUDE_FILTER, new JSONArray(mIncludeFilters));
        CLog.i("Added include filter to test suite: %s", mIncludeFilters);
        suite.put(EXCLUDE_FILTER, new JSONArray(mExcludeFilters));
        CLog.i("Added exclude filter to test suite: %s", mExcludeFilters);

        String coverageReportPath = mBuildInfo.getBuildAttributes().get("coverage_report_path");
        if (coverageReportPath != null) {
            jsonObject.put(OUTPUT_COVERAGE_REPORT, true);
            CLog.i("Added %s to the Json object", OUTPUT_COVERAGE_REPORT);
            jsonObject.put(COVERAGE_REPORT_PATH, coverageReportPath);
            CLog.i("Added %s to the Json object", COVERAGE_REPORT_PATH);
        }

        if (mExcludeOverInclude) {
            jsonObject.put(EXCLUDE_OVER_INCLUDE, mExcludeOverInclude);
            CLog.i("Added %s to the Json object", EXCLUDE_OVER_INCLUDE);
        }

        jsonObject.put(TEST_SUITE, suite);
        CLog.i("Added %s to the Json object", TEST_SUITE);

        jsonObject.put(TEST_MAX_TIMEOUT, mTestTimeout);
        CLog.i("Added %s to the Json object: %d", TEST_MAX_TIMEOUT, mTestTimeout);

        if (!mLogSeverity.isEmpty()) {
            String logSeverity = mLogSeverity.toUpperCase();
            ArrayList<String> severityList =
                    new ArrayList<String>(Arrays.asList("ERROR", "WARNING", "INFO", "DEBUG"));
            if (!severityList.contains(logSeverity)) {
                CLog.w("Unsupported log severity %s, use default log_severity:INFO instead.",
                        logSeverity);
                logSeverity = "INFO";
            }
            jsonObject.put(LOG_SEVERITY, logSeverity);
            CLog.i("Added %s to the Json object: %s", LOG_SEVERITY, logSeverity);
        }

        if (mAbi != null) {
            jsonObject.put(ABI_NAME, mAbi.getName());
            CLog.i("Added %s to the Json object", ABI_NAME);
            jsonObject.put(ABI_BITNESS, mAbi.getBitness());
            CLog.i("Added %s to the Json object", ABI_BITNESS);
        }

        if (mSkipOn32BitAbi) {
            jsonObject.put(SKIP_ON_32BIT_ABI, mSkipOn32BitAbi);
            CLog.i("Added %s to the Json object", SKIP_ON_32BIT_ABI);
        }

        if (mSkipOn64BitAbi) {
            jsonObject.put(SKIP_ON_64BIT_ABI, mSkipOn64BitAbi);
            CLog.i("Added %s to the Json object", SKIP_ON_64BIT_ABI);
        } else if (mRun32bBitOn64BitAbi) {
            jsonObject.put(RUN_32BIT_ON_64BIT_ABI, mRun32bBitOn64BitAbi);
            CLog.i("Added %s to the Json object", RUN_32BIT_ON_64BIT_ABI);
        }

        if (mSkipIfThermalThrottling) {
            jsonObject.put(SKIP_IF_THERMAL_THROTTLING, mSkipIfThermalThrottling);
            CLog.i("Added %s to the Json object", SKIP_IF_THERMAL_THROTTLING);
        }

        jsonObject.put(DISABLE_CPU_FREQUENCY_SCALING, mDisableCpuFrequencyScaling);
        CLog.i("Added %s to the Json object, value: %s", DISABLE_CPU_FREQUENCY_SCALING,
                mDisableCpuFrequencyScaling);

        if (!mBinaryTestSource.isEmpty()) {
            jsonObject.put(BINARY_TEST_SOURCE, new JSONArray(mBinaryTestSource));
            CLog.i("Added %s to the Json object", BINARY_TEST_SOURCE);
        }

        if (!mBinaryTestWorkingDirectory.isEmpty()) {
            jsonObject.put(BINARY_TEST_WORKING_DIRECTORY,
                    new JSONArray(mBinaryTestWorkingDirectory));
            CLog.i("Added %s to the Json object", BINARY_TEST_WORKING_DIRECTORY);
        }

        if (!mBinaryTestEnvp.isEmpty()) {
            jsonObject.put(BINARY_TEST_ENVP, new JSONArray(mBinaryTestEnvp));
            CLog.i("Added %s to the Json object", BINARY_TEST_ENVP);
        }

        if (!mBinaryTestArgs.isEmpty()) {
            jsonObject.put(BINARY_TEST_ARGS, new JSONArray(mBinaryTestArgs));
            CLog.i("Added %s to the Json object", BINARY_TEST_ARGS);
        }

        if (!mBinaryTestLdLibraryPath.isEmpty()) {
            jsonObject.put(BINARY_TEST_LD_LIBRARY_PATH,
                    new JSONArray(mBinaryTestLdLibraryPath));
            CLog.i("Added %s to the Json object", BINARY_TEST_LD_LIBRARY_PATH);
        }

        if (mBugReportOnFailure) {
            jsonObject.put(BUG_REPORT_ON_FAILURE, mBugReportOnFailure);
            CLog.i("Added %s to the Json object", BUG_REPORT_ON_FAILURE);
        }

        if (!mLogcatOnFailure) {
            jsonObject.put(LOGCAT_ON_FAILURE, mLogcatOnFailure);
            CLog.i("Added %s to the Json object", LOGCAT_ON_FAILURE);
        }

        if (mEnableProfiling) {
            jsonObject.put(ENABLE_PROFILING, mEnableProfiling);
            CLog.i("Added %s to the Json object", ENABLE_PROFILING);
        }

        if (mSaveTraceFileRemote) {
            jsonObject.put(SAVE_TRACE_FIEL_REMOTE, mSaveTraceFileRemote);
            CLog.i("Added %s to the Json object", SAVE_TRACE_FIEL_REMOTE);
        }

        if (mEnableSystrace) {
            jsonObject.put(ENABLE_SYSTRACE, mEnableSystrace);
            CLog.i("Added %s to the Json object", ENABLE_SYSTRACE);
        }

        if (mEnableCoverage) {
            jsonObject.put(GLOBAL_COVERAGE, mGlobalCoverage);
            if (!mExcludeCoveragePath.isEmpty()) {
                jsonObject.put(EXCLUDE_COVERAGE_PATH, new JSONArray(mExcludeCoveragePath));
                CLog.i("Added %s to the Json object", EXCLUDE_COVERAGE_PATH);
            }
            if (coverageBuild) {
                jsonObject.put(ENABLE_COVERAGE, mEnableCoverage);
                CLog.i("Added %s to the Json object", ENABLE_COVERAGE);
            } else {
                CLog.i("Device build has coverage disabled");
            }
        }

        if (mEnableSancov) {
            if (sancovBuild) {
                jsonObject.put(ENABLE_SANCOV, mEnableSancov);
                CLog.i("Added %s to the Json object", ENABLE_SANCOV);
            } else {
                CLog.i("Device build has sancov disabled");
            }
        }

        if (mPreconditionHwBinderServiceName != null) {
            jsonObject.put(PRECONDITION_HWBINDER_SERVICE, mPreconditionHwBinderServiceName);
            CLog.i("Added %s to the Json object", PRECONDITION_HWBINDER_SERVICE);
        }

        if (mPreconditionFeature != null) {
            jsonObject.put(PRECONDITION_FEATURE, mPreconditionFeature);
            CLog.i("Added %s to the Json object", PRECONDITION_FEATURE);
        }

        if (!mPreconditionFilePathPrefix.isEmpty()) {
            jsonObject.put(
                    PRECONDITION_FILE_PATH_PREFIX, new JSONArray(mPreconditionFilePathPrefix));
            CLog.i("Added %s to the Json object", PRECONDITION_FILE_PATH_PREFIX);
        }

        if (mPreconditionFirstApiLevel != 0) {
            jsonObject.put(PRECONDITION_FIRST_API_LEVEL, mPreconditionFirstApiLevel);
            CLog.i("Added %s to the Json object", PRECONDITION_FIRST_API_LEVEL);
        }

        if (mPreconditionLshal != null) {
            jsonObject.put(PRECONDITION_LSHAL, mPreconditionLshal);
            CLog.i("Added %s to the Json object", PRECONDITION_LSHAL);
        }

        if (mPreconditionVintf != null) {
            jsonObject.put(PRECONDITION_VINTF, mPreconditionVintf);
            CLog.i("Added %s to the Json object", PRECONDITION_VINTF);
        }

        if (mPreconditionSysProp != null) {
            jsonObject.put(PRECONDITION_SYSPROP, mPreconditionSysProp);
            CLog.i("Added %s to the Json object", PRECONDITION_SYSPROP);
        }

        if (!mBinaryTestProfilingLibraryPath.isEmpty()) {
            jsonObject.put(BINARY_TEST_PROFILING_LIBRARY_PATH,
                    new JSONArray(mBinaryTestProfilingLibraryPath));
            CLog.i("Added %s to the Json object", BINARY_TEST_PROFILING_LIBRARY_PATH);
        }

        if (mBinaryTestDisableFramework) {
            jsonObject.put(BINARY_TEST_DISABLE_FRAMEWORK, mBinaryTestDisableFramework);
            CLog.i("Added %s to the Json object", BINARY_TEST_DISABLE_FRAMEWORK);
        }

        if (mBinaryTestStopNativeServers) {
            jsonObject.put(BINARY_TEST_STOP_NATIVE_SERVERS, mBinaryTestStopNativeServers);
            CLog.i("Added %s to the Json object", BINARY_TEST_STOP_NATIVE_SERVERS);
        }

        if (!mNativeServerProcessName.isEmpty()) {
            jsonObject.put(NATIVE_SERVER_PROCESS_NAME, new JSONArray(mNativeServerProcessName));
            CLog.i("Added %s to the Json object", NATIVE_SERVER_PROCESS_NAME);
        }

        if (!mHalHidlReplayTestTracePaths.isEmpty()) {
            jsonObject.put(HAL_HIDL_REPLAY_TEST_TRACE_PATHS,
                    new JSONArray(mHalHidlReplayTestTracePaths));
            CLog.i("Added %s to the Json object", HAL_HIDL_REPLAY_TEST_TRACE_PATHS);
        }

        if (mHalHidlPackageName != null) {
            jsonObject.put(HAL_HIDL_PACKAGE_NAME, mHalHidlPackageName);
            CLog.i("Added %s to the Json object", SYSTRACE_PROCESS_NAME);
        }

        if (mSystraceProcessName != null) {
            jsonObject.put(SYSTRACE_PROCESS_NAME, mSystraceProcessName);
            CLog.i("Added %s to the Json object", SYSTRACE_PROCESS_NAME);
        }

        if (mPassthroughMode) {
            jsonObject.put(PASSTHROUGH_MODE, mPassthroughMode);
            CLog.i("Added %s to the Json object", PASSTHROUGH_MODE);
        }

        if (mCollectTestsOnly) {
            jsonObject.put(COLLECT_TESTS_ONLY, mCollectTestsOnly);
            CLog.i("Added %s to the Json object", COLLECT_TESTS_ONLY);
        }

        if (mGtestBatchMode) {
            jsonObject.put(GTEST_BATCH_MODE, mGtestBatchMode);
            CLog.i("Added %s to the Json object", GTEST_BATCH_MODE);
        }

        if (mLtpNumberOfThreads >= 0) {
            jsonObject.put(LTP_NUMBER_OF_THREADS, mLtpNumberOfThreads);
            CLog.i("Added %s to the Json object", LTP_NUMBER_OF_THREADS);
        }

        if (mRunAsVtsSelfTest) {
            jsonObject.put(RUN_AS_VTS_SELF_TEST, mRunAsVtsSelfTest);
            CLog.i("Added %s to the Json object", RUN_AS_VTS_SELF_TEST);
        }

        if ("vts".equals(mBuildInfo.getTestTag())) {
            jsonObject.put(RUN_AS_COMPLIANCE_TEST, true);
            CLog.i("Added %s to the Json object", RUN_AS_COMPLIANCE_TEST);
        }

        if (!mMoblyTestModule.isEmpty()) {
            jsonObject.put(MOBLY_TEST_MODULE, new JSONArray(mMoblyTestModule));
            CLog.i("Added %s to the Json object", MOBLY_TEST_MODULE);
        }
    }

    /**
     * Log a test module execution status to device logcat.
     *
     * @param status
     * @return true if succesful, false otherwise
     */
    private boolean printToDeviceLogcatAboutTestModuleStatus(String status) {
        try {
            mDevice.executeShellCommand(String.format(
                    "log -p i -t \"VTS\" \"[Test Module] %s %s\"", mTestModuleName, status));
        } catch (DeviceNotAvailableException e) {
            CLog.w("Device unavailable while trying to write a message to logcat.");
            return false;
        }
        return true;
    }

    private boolean AddTestModuleKeys(String test_module_name, long test_module_timestamp) {
        if (test_module_name.length() == 0 || test_module_timestamp == -1) {
            CLog.e(String.format("Test module keys (%s,%d) are invalid.", test_module_name,
                    test_module_timestamp));
            return false;
        }
        File reportFile = mBuildInfo.getFile(TEST_PLAN_REPORT_FILE);

        try (FileWriter fw = new FileWriter(reportFile.getAbsoluteFile(), true);
                BufferedWriter bw = new BufferedWriter(fw); PrintWriter out = new PrintWriter(bw)) {
            out.println(String.format("%s %s", test_module_name, test_module_timestamp));
        } catch (IOException e) {
            CLog.e(String.format(
                    "Can't write to the test plan result file, %s", TEST_PLAN_REPORT_FILE));
            return false;
        }
        return true;
    }

    /**
     * This method prepares a command for the test and runs the python file as
     * given in the arguments.
     *
     * @param listener
     * @throws RuntimeException
     * @throws IllegalArgumentException
     */
    private void doRunTest(ITestLifeCycleReceiver listener)
            throws RuntimeException, IllegalArgumentException {
        CLog.i("Device serial number: " + mDevice.getSerialNumber());

        setTestCaseDataDir();

        JSONObject jsonObject = new JSONObject();
        File vtsRunnerLogDir = null;
        try {
            vtsRunnerLogDir = FileUtil.createTempDir("vts-runner-log");
            updateVtsRunnerTestConfig(jsonObject);

            jsonObject.put(LOG_PATH,  vtsRunnerLogDir.getAbsolutePath());
            CLog.i("Added %s to the Json object", LOG_PATH);
        } catch(IOException e) {
            throw new RuntimeException("Failed to read test config json file");
        } catch(JSONException e) {
            throw new RuntimeException("Failed to build updated test config json data");
        }

        CLog.i("config json: %s", jsonObject.toString());

        String jsonFilePath = null;
        try {
            File tmpFile = FileUtil.createTempFile(
                    mBuildInfo.getTestTag() + "-config-" + mBuildInfo.getDeviceSerial(), ".json");
            jsonFilePath = tmpFile.getAbsolutePath();
            CLog.i("config json file path: %s", jsonFilePath);
            FileWriter fw = new FileWriter(jsonFilePath);
            fw.write(jsonObject.toString());
            fw.close();
        } catch(IOException e) {
            throw new RuntimeException("Failed to create device config json file");
        }

        VtsPythonRunnerHelper vtsPythonRunnerHelper = createVtsPythonRunnerHelper();
        vtsPythonRunnerHelper.setPythonVersion(mPythonVersion);
        if (mPythonBin == null){
            mPythonBin = vtsPythonRunnerHelper.getPythonBinary();
        }

        String[] baseOpts = {
                mPythonBin,
        };
        String[] testModule = new String[2];
        String[] cmd;
        if (mTestCasePathType != null && mTestCasePathType.toLowerCase().equals("file")) {
            testModule[0] = mTestCasePath;
            if (!mTestCasePath.endsWith(".py")) {
                testModule[0] += ".py";
            }
        } else {
            baseOpts = new String[2];
            baseOpts[0] = mPythonBin;
            baseOpts[1] = "-m";
            testModule[0] = mTestCasePath.replace("/", ".");
        }
        testModule[1] = jsonFilePath;
        cmd = ArrayUtil.buildArray(baseOpts, testModule);

        printToDeviceLogcatAboutTestModuleStatus("BEGIN");

        CommandResult commandResult = new CommandResult();
        String interruptMessage =
                vtsPythonRunnerHelper.runPythonRunner(cmd, commandResult, mTestTimeout);

        if (commandResult != null) {
            CommandStatus commandStatus = commandResult.getStatus();
            if (commandStatus != CommandStatus.SUCCESS
                && commandStatus != CommandStatus.TIMED_OUT) {
                CLog.e("Python process failed");
                CLog.e("Python path: %s", vtsPythonRunnerHelper.getPythonPath());
                CLog.e("Command stdout: " + commandResult.getStdout());
                CLog.e("Command stderr: " + commandResult.getStderr());
                CLog.e("Command status: " + commandStatus);
                CLog.e("Python log: ");
                printVtsLogs(vtsRunnerLogDir);
                printToDeviceLogcatAboutTestModuleStatus("ERROR");
                throw new RuntimeException("Failed to run VTS test");
            }
            printToDeviceLogcatAboutTestModuleStatus("END");
        }

        VtsMultiDeviceTestResultParser parser =
                new VtsMultiDeviceTestResultParser(listener, mRunName);

        if (mUseStdoutLogs) {
            if (commandResult.getStdout() == null) {
                CLog.e("The std:out is null for CommandResult.");
                throw new RuntimeException("The std:out is null for CommandResult.");
            }
            parser.processNewLines(commandResult.getStdout().split("\n"));
        } else {
            // parse from test_run_summary.json instead of stdout
            String jsonData = null;
            JSONObject object = null;
            File testRunSummary = getFileTestRunSummary(vtsRunnerLogDir);
            if (testRunSummary == null) {
                CLog.e("Couldn't locate the file : " + TEST_RUN_SUMMARY_FILE_NAME);
            } else {
                try {
                    jsonData = FileUtil.readStringFromFile(testRunSummary);
                    CLog.i("Test Result Summary: %s", jsonData);
                    object = new JSONObject(jsonData);
                } catch (IOException e) {
                    CLog.e("Error occurred in parsing Json file : %s", testRunSummary.toPath());
                } catch (JSONException e) {
                    CLog.e("Error occurred in parsing Json String : %s", jsonData);
                }
                if (object == null) {
                    CLog.e("Json object is null.");
                    throw new RuntimeException("Json object is null.");
                }
                parser.processJsonFile(object);

                try {
                    JSONObject planObject = object.getJSONObject(TESTMODULE);
                    String test_module_name = planObject.getString("Name");
                    long test_module_timestamp = planObject.getLong("Timestamp");
                    AddTestModuleKeys(test_module_name, test_module_timestamp);
                } catch (JSONException e) {
                    CLog.d("Key '%s' not found in result json summary", TESTMODULE);
                }
            }
        }
        printVtsLogs(vtsRunnerLogDir);

        File reportMsg = FileUtil.findFile(vtsRunnerLogDir, REPORT_MESSAGE_FILE_NAME);
        CLog.i("Report message path: %s", reportMsg);

        if (reportMsg == null) {
            CLog.e("Cannot find report message proto file.");
        } else if (reportMsg.length() > 0) {
            CLog.i("Uploading report message. File size: %s", reportMsg.length());
            VtsDashboardUtil dashboardUtil = new VtsDashboardUtil(configReader);
            dashboardUtil.Upload(reportMsg.getAbsolutePath());
        } else {
            CLog.i("Result uploading is not enabled.");
        }

        FileUtil.recursiveDelete(vtsRunnerLogDir);
        CLog.i("Deleted the runner log dir, %s.", vtsRunnerLogDir);
        if (jsonFilePath != null) {
          FileUtil.deleteFile(new File(jsonFilePath));
          CLog.i("Deleted the runner json config file, %s.", jsonFilePath);
        }

        if (interruptMessage != null) {
            throw new RunInterruptedException(interruptMessage);
        }
    }

    /**
     * This method return the file test_run_summary.json which is then used to parse logs.
     *
     * @param logDir : The file that needs to be converted
     * @return the file named test_run_summary.json
     * @throws IllegalArgumentException
     */
    private File getFileTestRunSummary(File logDir) throws IllegalArgumentException {
        File[] children;
        if (logDir == null) {
            throw new IllegalArgumentException("Argument logDir is null.");
        }
        children = logDir.listFiles();
        if (children != null) {
            for (File child : children) {
                if (!child.isDirectory()) {
                    if (child.getName().equals(TEST_RUN_SUMMARY_FILE_NAME)) {
                        return child;
                    }
                } else {
                    File file = getFileTestRunSummary(child);
                    if (file != null) {
                        return file;
                    }
                }
            }
        }
        return null;
    }

    /**
     * The method prints all VTS runner log files
     *
     * @param logDir the File instance of the base log dir.
     */
    private void printVtsLogs(File logDir) {
        File[] children;
        if (logDir == null) {
            CLog.e("Scan VTS log dir: null\n");
            return;
        }
        CLog.i("Scan VTS log dir %s\n", logDir.getAbsolutePath());
        children = logDir.listFiles();
        if (children != null) {
            for (File child : children) {
                if (child.isDirectory()) {
                    if (!child.getName().equals("temp")) {
                        // temp in python log directory is for temp files produced by test module
                        // and thus should not be included in log printout
                        printVtsLogs(child);
                    }
                } else {
                    CLog.i("VTS log file %s\n", child.getAbsolutePath());
                    try {
                        if (child.getName().startsWith("vts_agent") &&
                                child.getName().endsWith(".log")) {
                            CLog.i("Content: %s\n", FileUtil.readStringFromFile(child));
                        } else {
                            CLog.i("skip %s\n", child.getName());
                        }
                    } catch (IOException e) {
                        CLog.e("I/O error\n");
                    }
                }
            }
        }
    }

    /**
     * Creates VtsPythonRunnerHelper.
     */
    protected VtsPythonRunnerHelper createVtsPythonRunnerHelper() {
        return new VtsPythonRunnerHelper(mBuildInfo);
    }

    /**
     * Set the path for android-vts/testcases/ which keeps the VTS python code under vts.
     */
    private void setTestCaseDataDir() {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuildInfo);
        File testDir = null;
        try {
            testDir = buildHelper.getTestsDir();
        } catch (FileNotFoundException e) {
            /* pass */
        }
        if (testDir != null) {
            mTestCaseDataDir = testDir.getAbsolutePath();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setAbi(IAbi abi){
        mAbi = abi;
    }
}
