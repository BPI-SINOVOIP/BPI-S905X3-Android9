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
package com.android.tradefed.testtype.suite;

import com.android.annotations.VisibleForTesting;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.config.IDeviceConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.device.metric.IMetricCollectorReceiver;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ITestLoggerReceiver;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.suite.checker.ISystemStatusChecker;
import com.android.tradefed.suite.checker.ISystemStatusCheckerReceiver;
import com.android.tradefed.suite.checker.StatusCheckerResult;
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IRuntimeHintProvider;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.testtype.ITestCollector;
import com.android.tradefed.util.AbiFormatter;
import com.android.tradefed.util.AbiUtils;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.TimeUtil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

/**
 * Abstract class used to run Test Suite. This class provide the base of how the Suite will be run.
 * Each implementation can define the list of tests via the {@link #loadTests()} method.
 */
public abstract class ITestSuite
        implements IRemoteTest,
                IDeviceTest,
                IMultiDeviceTest,
                IBuildReceiver,
                ISystemStatusCheckerReceiver,
                IShardableTest,
                ITestCollector,
                IInvocationContextReceiver,
                IRuntimeHintProvider,
                IMetricCollectorReceiver,
                IConfigurationReceiver {

    public static final String SKIP_SYSTEM_STATUS_CHECKER = "skip-system-status-check";
    public static final String RUNNER_WHITELIST = "runner-whitelist";
    public static final String PREPARER_WHITELIST = "preparer-whitelist";
    public static final String MODULE_CHECKER_PRE = "PreModuleChecker";
    public static final String MODULE_CHECKER_POST = "PostModuleChecker";
    public static final String ABI_OPTION = "abi";
    public static final String SKIP_HOST_ARCH_CHECK = "skip-host-arch-check";
    public static final String PRIMARY_ABI_RUN = "primary-abi-only";
    private static final String PRODUCT_CPU_ABI_KEY = "ro.product.cpu.abi";

    // Options for test failure case
    @Option(
        name = "bugreport-on-failure",
        description =
                "Take a bugreport on every test failure. Warning: This may require a lot"
                        + "of storage space of the machine running the tests."
    )
    private boolean mBugReportOnFailure = false;

    @Option(name = "logcat-on-failure",
            description = "Take a logcat snapshot on every test failure.")
    private boolean mLogcatOnFailure = false;

    @Option(name = "logcat-on-failure-size",
            description = "The max number of logcat data in bytes to capture when "
            + "--logcat-on-failure is on. Should be an amount that can comfortably fit in memory.")
    private int mMaxLogcatBytes = 500 * 1024; // 500K

    @Option(name = "screenshot-on-failure",
            description = "Take a screenshot on every test failure.")
    private boolean mScreenshotOnFailure = false;

    @Option(name = "reboot-on-failure",
            description = "Reboot the device after every test failure.")
    private boolean mRebootOnFailure = false;

    // Options for suite runner behavior
    @Option(name = "reboot-per-module", description = "Reboot the device before every module run.")
    private boolean mRebootPerModule = false;

    @Option(name = "skip-all-system-status-check",
            description = "Whether all system status check between modules should be skipped")
    private boolean mSkipAllSystemStatusCheck = false;

    @Option(
        name = SKIP_SYSTEM_STATUS_CHECKER,
        description =
                "Disable specific system status checkers."
                        + "Specify zero or more SystemStatusChecker as canonical class names. e.g. "
                        + "\"com.android.tradefed.suite.checker.KeyguardStatusChecker\" If not "
                        + "specified, all configured or whitelisted system status checkers will "
                        + "run."
    )
    private Set<String> mSystemStatusCheckBlacklist = new HashSet<>();

    @Option(
        name = "report-system-checkers",
        description = "Whether reporting system checkers as test or not."
    )
    private boolean mReportSystemChecker = false;

    @Option(
        name = "collect-tests-only",
        description =
                "Only invoke the suite to collect list of applicable test cases. All "
                        + "test run callbacks will be triggered, but test execution will not be "
                        + "actually carried out."
    )
    private boolean mCollectTestsOnly = false;

    // Abi related options
    @Option(
        name = ABI_OPTION,
        shortName = 'a',
        description = "the abi to test. For example: 'arm64-v8a'.",
        importance = Importance.IF_UNSET
    )
    private String mAbiName = null;

    @Option(
        name = SKIP_HOST_ARCH_CHECK,
        description = "Whether host architecture check should be skipped."
    )
    private boolean mSkipHostArchCheck = false;

    @Option(
        name = PRIMARY_ABI_RUN,
        description =
                "Whether to run tests with only the device primary abi. "
                        + "This is overriden by the --abi option."
    )
    private boolean mPrimaryAbiRun = false;

    @Option(
        name = "module-metadata-include-filter",
        description =
                "Include modules for execution based on matching of metadata fields: for any of "
                        + "the specified filter name and value, if a module has a metadata field "
                        + "with the same name and value, it will be included. When both module "
                        + "inclusion and exclusion rules are applied, inclusion rules will be "
                        + "evaluated first. Using this together with test filter inclusion rules "
                        + "may result in no tests to execute if the rules don't overlap."
    )
    private MultiMap<String, String> mModuleMetadataIncludeFilter = new MultiMap<>();

    @Option(
        name = "module-metadata-exclude-filter",
        description =
                "Exclude modules for execution based on matching of metadata fields: for any of "
                        + "the specified filter name and value, if a module has a metadata field "
                        + "with the same name and value, it will be excluded. When both module "
                        + "inclusion and exclusion rules are applied, inclusion rules will be "
                        + "evaluated first."
    )
    private MultiMap<String, String> mModuleMetadataExcludeFilter = new MultiMap<>();

    @Option(name = RUNNER_WHITELIST, description = "Runner class(es) that are allowed to run.")
    private Set<String> mAllowedRunners = new HashSet<>();

    @Option(
        name = PREPARER_WHITELIST,
        description =
                "Preparer class(es) that are allowed to run. This mostly usefeul for dry-runs."
    )
    private Set<String> mAllowedPreparers = new HashSet<>();

    @Option(
        name = "reboot-before-test",
        description = "Reboot the device before the test suite starts."
    )
    private boolean mRebootBeforeTest = false;

    @Option(
        name = "max-testcase-run-count",
        description =
                "If the IRemoteTest can have its testcases run multiple times, "
                        + "the max number of runs for each testcase."
    )
    private int mMaxRunLimit = 1;

    private ITestDevice mDevice;
    private IBuildInfo mBuildInfo;
    private Map<ITestDevice, IBuildInfo> mDeviceInfos;
    private List<ISystemStatusChecker> mSystemStatusCheckers;
    private IInvocationContext mContext;
    private List<IMetricCollector> mMetricCollectors;
    private IConfiguration mMainConfiguration;

    // Sharding attributes
    private boolean mIsSharded = false;
    private ModuleDefinition mDirectModule = null;
    private boolean mShouldMakeDynamicModule = true;

    /**
     * Abstract method to load the tests configuration that will be run. Each tests is defined by a
     * {@link IConfiguration} and a unique name under which it will report results.
     */
    public abstract LinkedHashMap<String, IConfiguration> loadTests();

    /**
     * Return an instance of the class implementing {@link ITestSuite}.
     */
    private ITestSuite createInstance() {
        try {
            return this.getClass().newInstance();
        } catch (InstantiationException | IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }

    private LinkedHashMap<String, IConfiguration> loadAndFilter() {
        LinkedHashMap<String, IConfiguration> runConfig = loadTests();
        if (runConfig.isEmpty()) {
            CLog.i("No config were loaded. Nothing to run.");
            return runConfig;
        }
        if (mModuleMetadataIncludeFilter.isEmpty() && mModuleMetadataExcludeFilter.isEmpty()) {
            return runConfig;
        }
        LinkedHashMap<String, IConfiguration> filteredConfig = new LinkedHashMap<>();
        for (Entry<String, IConfiguration> config : runConfig.entrySet()) {
            if (!filterByConfigMetadata(
                    config.getValue(),
                    mModuleMetadataIncludeFilter,
                    mModuleMetadataExcludeFilter)) {
                // if the module config did not pass the metadata filters, it's excluded
                // from execution.
                continue;
            }
            if (!filterByRunnerType(config.getValue(), mAllowedRunners)) {
                // if the module config did not pass the runner type filter, it's excluded from
                // execution.
                continue;
            }
            filterPreparers(config.getValue(), mAllowedPreparers);
            filteredConfig.put(config.getKey(), config.getValue());
        }
        runConfig.clear();
        return filteredConfig;
    }

    /** Helper that creates and returns the list of {@link ModuleDefinition} to be executed. */
    private List<ModuleDefinition> createExecutionList() {
        List<ModuleDefinition> runModules = new ArrayList<>();
        if (mDirectModule != null) {
            // If we are sharded and already know what to run then we just do it.
            runModules.add(mDirectModule);
            mDirectModule.setDevice(mDevice);
            mDirectModule.setDeviceInfos(mDeviceInfos);
            mDirectModule.setBuild(mBuildInfo);
            return runModules;
        }

        LinkedHashMap<String, IConfiguration> runConfig = loadAndFilter();
        if (runConfig.isEmpty()) {
            CLog.i("No config were loaded. Nothing to run.");
            return runModules;
        }

        for (Entry<String, IConfiguration> config : runConfig.entrySet()) {
            // Validate the configuration, it will throw if not valid.
            ValidateSuiteConfigHelper.validateConfig(config.getValue());
            Map<String, List<ITargetPreparer>> preparersPerDevice =
                    getPreparerPerDevice(config.getValue());
            ModuleDefinition module =
                    new ModuleDefinition(
                            config.getKey(),
                            config.getValue().getTests(),
                            preparersPerDevice,
                            config.getValue().getMultiTargetPreparers(),
                            config.getValue());
            module.setDevice(mDevice);
            module.setDeviceInfos(mDeviceInfos);
            module.setBuild(mBuildInfo);
            runModules.add(module);
        }
        // Free the map once we are done with it.
        runConfig = null;
        return runModules;
    }

    private void checkClassLoad(Set<String> classes, String type) {
        for (String c : classes) {
            try {
                Class.forName(c);
            } catch (ClassNotFoundException e) {
                ConfigurationException ex =
                        new ConfigurationException(
                                String.format(
                                        "--%s must contains valid class, %s was not found",
                                        type, c),
                                e);
                throw new RuntimeException(ex);
            }
        }
    }

    /** Create the mapping of device to its target_preparer. */
    private Map<String, List<ITargetPreparer>> getPreparerPerDevice(IConfiguration config) {
        Map<String, List<ITargetPreparer>> res = new LinkedHashMap<>();
        for (IDeviceConfiguration holder : config.getDeviceConfig()) {
            List<ITargetPreparer> preparers = new ArrayList<>();
            res.put(holder.getDeviceName(), preparers);
            preparers.addAll(holder.getTargetPreparers());
        }
        return res;
    }

    /** Generic run method for all test loaded from {@link #loadTests()}. */
    @Override
    public final void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // Load and check the module checkers, runners and preparers in black and whitelist
        checkClassLoad(mSystemStatusCheckBlacklist, SKIP_SYSTEM_STATUS_CHECKER);
        checkClassLoad(mAllowedRunners, RUNNER_WHITELIST);
        checkClassLoad(mAllowedPreparers, PREPARER_WHITELIST);

        List<ModuleDefinition> runModules = createExecutionList();
        // Check if we have something to run.
        if (runModules.isEmpty()) {
            CLog.i("No tests to be run.");
            return;
        }

        // Allow checkers to log files for easier debugging.
        for (ISystemStatusChecker checker : mSystemStatusCheckers) {
            if (checker instanceof ITestLoggerReceiver) {
                ((ITestLoggerReceiver) checker).setTestLogger(listener);
            }
        }

        // If requested reboot each device before the testing starts.
        if (mRebootBeforeTest) {
            for (ITestDevice device : mContext.getDevices()) {
                if (!(device.getIDevice() instanceof StubDevice)) {
                    CLog.d(
                            "Rebooting device '%s' before test starts as requested.",
                            device.getSerialNumber());
                    mDevice.reboot();
                }
            }
        }

        /** Setup a special listener to take actions on test failures. */
        TestFailureListener failureListener =
                new TestFailureListener(
                        mContext.getDevices(),
                        mBugReportOnFailure,
                        mLogcatOnFailure,
                        mScreenshotOnFailure,
                        mRebootOnFailure,
                        mMaxLogcatBytes);
        /** Create the list of listeners applicable at the module level. */
        List<ITestInvocationListener> moduleListeners = createModuleListeners();

        // Only print the running log if we are going to run something.
        if (runModules.get(0).hasTests()) {
            CLog.logAndDisplay(
                    LogLevel.INFO,
                    "%s running %s modules: %s",
                    mDevice.getSerialNumber(),
                    runModules.size(),
                    runModules);
        }

        /** Run all the module, make sure to reduce the list to release resources as we go. */
        try {
            while (!runModules.isEmpty()) {
                ModuleDefinition module = runModules.remove(0);
                // Before running the module we ensure it has tests at this point or skip completely
                // to avoid running SystemCheckers and preparation for nothing.
                if (module.hasTests()) {
                    continue;
                }

                try {
                    // Populate the module context with devices and builds
                    for (String deviceName : mContext.getDeviceConfigNames()) {
                        module.getModuleInvocationContext()
                                .addAllocatedDevice(deviceName, mContext.getDevice(deviceName));
                        module.getModuleInvocationContext()
                                .addDeviceBuildInfo(deviceName, mContext.getBuildInfo(deviceName));
                    }
                    listener.testModuleStarted(module.getModuleInvocationContext());
                    // Trigger module start on module level listener too
                    new ResultForwarder(moduleListeners)
                            .testModuleStarted(module.getModuleInvocationContext());

                    runSingleModule(module, listener, moduleListeners, failureListener);
                } finally {
                    // Trigger module end on module level listener too
                    new ResultForwarder(moduleListeners).testModuleEnded();
                    // clear out module invocation context since we are now done with module
                    // execution
                    listener.testModuleEnded();
                }
            }
        } catch (DeviceNotAvailableException e) {
            CLog.e(
                    "A DeviceNotAvailableException occurred, following modules did not run: %s",
                    runModules);
            for (ModuleDefinition module : runModules) {
                listener.testRunStarted(module.getId(), 0);
                listener.testRunFailed("Module did not run due to device not available.");
                listener.testRunEnded(0, new HashMap<String, Metric>());
            }
            throw e;
        }
    }

    /**
     * Returns the list of {@link ITestInvocationListener} applicable to the {@link ModuleListener}
     * level. These listeners will be re-used for each module, they will not be re-instantiated so
     * they should not assume an internal state.
     */
    protected List<ITestInvocationListener> createModuleListeners() {
        return new ArrayList<>();
    }

    /**
     * Helper method that handle running a single module logic.
     *
     * @param module The {@link ModuleDefinition} to be ran.
     * @param listener The {@link ITestInvocationListener} where to report results
     * @param moduleListeners The {@link ITestInvocationListener}s that runs at the module level.
     * @param failureListener special listener that we add to collect information on failures.
     * @throws DeviceNotAvailableException
     */
    private void runSingleModule(
            ModuleDefinition module,
            ITestInvocationListener listener,
            List<ITestInvocationListener> moduleListeners,
            TestFailureListener failureListener)
            throws DeviceNotAvailableException {
        if (mRebootPerModule) {
            if ("user".equals(mDevice.getProperty("ro.build.type"))) {
                CLog.e(
                        "reboot-per-module should only be used during development, "
                                + "this is a\" user\" build device");
            } else {
                CLog.d("Rebooting device before starting next module");
                mDevice.reboot();
            }
        }

        if (!mSkipAllSystemStatusCheck) {
            runPreModuleCheck(module.getId(), mSystemStatusCheckers, mDevice, listener);
        }
        if (mCollectTestsOnly) {
            module.setCollectTestsOnly(mCollectTestsOnly);
        }
        // Pass the run defined collectors to be used.
        module.setMetricCollectors(mMetricCollectors);
        // Pass the main invocation logSaver
        module.setLogSaver(mMainConfiguration.getLogSaver());

        // Actually run the module
        module.run(listener, moduleListeners, failureListener, mMaxRunLimit);

        if (!mSkipAllSystemStatusCheck) {
            runPostModuleCheck(module.getId(), mSystemStatusCheckers, mDevice, listener);
        }
    }

    /**
     * Helper to run the System Status checkers preExecutionChecks defined for the test and log
     * their failures.
     */
    private void runPreModuleCheck(
            String moduleName,
            List<ISystemStatusChecker> checkers,
            ITestDevice device,
            ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        long startTime = System.currentTimeMillis();
        CLog.i("Running system status checker before module execution: %s", moduleName);
        Map<String, String> failures = new LinkedHashMap<>();
        for (ISystemStatusChecker checker : checkers) {
            // Check if the status checker should be skipped.
            if (mSystemStatusCheckBlacklist.contains(checker.getClass().getName())) {
                CLog.d(
                        "%s was skipped via %s",
                        checker.getClass().getName(), SKIP_SYSTEM_STATUS_CHECKER);
                continue;
            }

            StatusCheckerResult result = checker.preExecutionCheck(device);
            if (!CheckStatus.SUCCESS.equals(result.getStatus())) {
                String errorMessage =
                        (result.getErrorMessage() == null) ? "" : result.getErrorMessage();
                failures.put(checker.getClass().getCanonicalName(), errorMessage);
                CLog.w("System status checker [%s] failed.", checker.getClass().getCanonicalName());
            }
        }
        if (!failures.isEmpty()) {
            CLog.w("There are failed system status checkers: %s capturing a bugreport",
                    failures.toString());
            try (InputStreamSource bugSource = device.getBugreport()) {
                listener.testLog(
                        String.format("bugreport-checker-pre-module-%s", moduleName),
                        LogDataType.BUGREPORT,
                        bugSource);
            }
        }

        // We report System checkers like tests.
        reportModuleCheckerResult(MODULE_CHECKER_PRE, moduleName, failures, startTime, listener);
    }

    /**
     * Helper to run the System Status checkers postExecutionCheck defined for the test and log
     * their failures.
     */
    private void runPostModuleCheck(
            String moduleName,
            List<ISystemStatusChecker> checkers,
            ITestDevice device,
            ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        long startTime = System.currentTimeMillis();
        CLog.i("Running system status checker after module execution: %s", moduleName);
        Map<String, String> failures = new LinkedHashMap<>();
        for (ISystemStatusChecker checker : checkers) {
            // Check if the status checker should be skipped.
            if (mSystemStatusCheckBlacklist.contains(checker.getClass().getName())) {
                continue;
            }

            StatusCheckerResult result = checker.postExecutionCheck(device);
            if (!CheckStatus.SUCCESS.equals(result.getStatus())) {
                String errorMessage =
                        (result.getErrorMessage() == null) ? "" : result.getErrorMessage();
                failures.put(checker.getClass().getCanonicalName(), errorMessage);
                CLog.w("System status checker [%s] failed", checker.getClass().getCanonicalName());
            }
        }
        if (!failures.isEmpty()) {
            CLog.w("There are failed system status checkers: %s capturing a bugreport",
                    failures.toString());
            try (InputStreamSource bugSource = device.getBugreport()) {
                listener.testLog(
                        String.format("bugreport-checker-post-module-%s", moduleName),
                        LogDataType.BUGREPORT,
                        bugSource);
            }
        }

        // We report System checkers like tests.
        reportModuleCheckerResult(MODULE_CHECKER_POST, moduleName, failures, startTime, listener);
    }

    /** Helper to report status checker results as test results. */
    private void reportModuleCheckerResult(
            String identifier,
            String moduleName,
            Map<String, String> failures,
            long startTime,
            ITestInvocationListener listener) {
        if (!mReportSystemChecker) {
            // do not log here, otherwise it could be very verbose.
            return;
        }
        // Avoid messing with the final test count by making them empty runs.
        listener.testRunStarted(identifier + "_" + moduleName, 0);
        if (!failures.isEmpty()) {
            listener.testRunFailed(String.format("%s failed '%s' checkers", moduleName, failures));
        }
        listener.testRunEnded(
                System.currentTimeMillis() - startTime, new HashMap<String, Metric>());
    }

    /** {@inheritDoc} */
    @Override
    public Collection<IRemoteTest> split(int shardCountHint) {
        if (shardCountHint <= 1 || mIsSharded) {
            // cannot shard or already sharded
            return null;
        }

        LinkedHashMap<String, IConfiguration> runConfig = loadAndFilter();
        if (runConfig.isEmpty()) {
            CLog.i("No config were loaded. Nothing to run.");
            return null;
        }
        injectInfo(runConfig);

        // We split individual tests on double the shardCountHint to provide better average.
        // The test pool mechanism prevent this from creating too much overhead.
        List<ModuleDefinition> splitModules =
                ModuleSplitter.splitConfiguration(
                        runConfig, shardCountHint, mShouldMakeDynamicModule);
        runConfig.clear();
        runConfig = null;
        // create an association of one ITestSuite <=> one ModuleDefinition as the smallest
        // execution unit supported.
        List<IRemoteTest> splitTests = new ArrayList<>();
        for (ModuleDefinition m : splitModules) {
            ITestSuite suite = createInstance();
            OptionCopier.copyOptionsNoThrow(this, suite);
            suite.mIsSharded = true;
            suite.mDirectModule = m;
            splitTests.add(suite);
        }
        // return the list of ITestSuite with their ModuleDefinition assigned
        return splitTests;
    }

    /**
     * Inject {@link ITestDevice} and {@link IBuildInfo} to the {@link IRemoteTest}s in the config
     * before sharding since they may be needed.
     */
    private void injectInfo(LinkedHashMap<String, IConfiguration> runConfig) {
        for (IConfiguration config : runConfig.values()) {
            for (IRemoteTest test : config.getTests()) {
                if (test instanceof IBuildReceiver) {
                    ((IBuildReceiver) test).setBuild(mBuildInfo);
                }
                if (test instanceof IDeviceTest) {
                    ((IDeviceTest) test).setDevice(mDevice);
                }
                if (test instanceof IMultiDeviceTest) {
                    ((IMultiDeviceTest) test).setDeviceInfos(mDeviceInfos);
                }
                if (test instanceof IInvocationContextReceiver) {
                    ((IInvocationContextReceiver) test).setInvocationContext(mContext);
                }
            }
        }
    }

    /** {@inheritDoc} */
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

    /** Set the value of mAbiName */
    public void setAbiName(String abiName) {
        mAbiName = abiName;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    /**
     * Implementation of {@link ITestSuite} may require the build info to load the tests.
     */
    public IBuildInfo getBuildInfo() {
        return mBuildInfo;
    }

    /** {@inheritDoc} */
    @Override
    public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
        mDeviceInfos = deviceInfos;
    }

    /** Set the value of mPrimaryAbiRun */
    public void setPrimaryAbiRun(boolean primaryAbiRun) {
        mPrimaryAbiRun = primaryAbiRun;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSystemStatusChecker(List<ISystemStatusChecker> systemCheckers) {
        mSystemStatusCheckers = systemCheckers;
    }

    /**
     * Run the test suite in collector only mode, this requires all the sub-tests to implements this
     * interface too.
     */
    @Override
    public void setCollectTestsOnly(boolean shouldCollectTest) {
        mCollectTestsOnly = shouldCollectTest;
    }

    /** {@inheritDoc} */
    @Override
    public void setMetricCollectors(List<IMetricCollector> collectors) {
        mMetricCollectors = collectors;
    }

    /**
     * When doing distributed sharding, we cannot have ModuleDefinition that shares tests in a pool
     * otherwise intra-module sharding will not work, so we allow to disable it.
     */
    public void setShouldMakeDynamicModule(boolean dynamicModule) {
        mShouldMakeDynamicModule = dynamicModule;
    }

    /** {@inheritDoc} */
    @Override
    public void setInvocationContext(IInvocationContext invocationContext) {
        mContext = invocationContext;
    }

    /** {@inheritDoc} */
    @Override
    public long getRuntimeHint() {
        if (mDirectModule != null) {
            CLog.d(
                    "    %s: %s",
                    mDirectModule.getId(),
                    TimeUtil.formatElapsedTime(mDirectModule.getRuntimeHint()));
            return mDirectModule.getRuntimeHint();
        }
        return 0l;
    }

    /** {@inheritDoc} */
    @Override
    public void setConfiguration(IConfiguration configuration) {
        mMainConfiguration = configuration;
    }

    /**
     * Returns the {@link ModuleDefinition} to be executed directly, or null if none yet (when the
     * ITestSuite has not been sharded yet).
     */
    public ModuleDefinition getDirectModule() {
        return mDirectModule;
    }

    /**
     * Gets the set of ABIs supported by both Compatibility testing {@link
     * AbiUtils#getAbisSupportedByCompatibility()} and the device under test.
     *
     * @return The set of ABIs to run the tests on
     * @throws DeviceNotAvailableException
     */
    public Set<IAbi> getAbis(ITestDevice device) throws DeviceNotAvailableException {
        Set<IAbi> abis = new LinkedHashSet<>();
        Set<String> archAbis = getAbisForBuildTargetArch();
        if (mPrimaryAbiRun) {
            if (mAbiName == null) {
                // Get the primary from the device and make it the --abi to run.
                mAbiName = device.getProperty(PRODUCT_CPU_ABI_KEY).trim();
            } else {
                CLog.d(
                        "Option --%s supersedes the option --%s, using abi: %s",
                        ABI_OPTION, PRIMARY_ABI_RUN, mAbiName);
            }
        }
        if (mAbiName != null) {
            // A particular abi was requested, it still needs to be supported by the build.
            if ((!mSkipHostArchCheck && !archAbis.contains(mAbiName))
                    || !AbiUtils.isAbiSupportedByCompatibility(mAbiName)) {
                throw new IllegalArgumentException(
                        String.format(
                                "Your tests suite hasn't been built with "
                                        + "abi '%s' support, this suite currently supports '%s'.",
                                mAbiName, archAbis));
            } else {
                abis.add(new Abi(mAbiName, AbiUtils.getBitness(mAbiName)));
                return abis;
            }
        } else {
            // Run on all abi in common between the device and suite builds.
            List<String> deviceAbis = Arrays.asList(AbiFormatter.getSupportedAbis(device, ""));
            for (String abi : deviceAbis) {
                if ((mSkipHostArchCheck || archAbis.contains(abi))
                        && AbiUtils.isAbiSupportedByCompatibility(abi)) {
                    abis.add(new Abi(abi, AbiUtils.getBitness(abi)));
                } else {
                    CLog.d(
                            "abi '%s' is supported by device but not by this suite build (%s), "
                                    + "tests will not run against it.",
                            abi, archAbis);
                }
            }
            if (abis.isEmpty()) {
                throw new IllegalArgumentException(
                        String.format(
                                "None of the abi supported by this tests suite build ('%s') are "
                                        + "supported by the device ('%s').",
                                archAbis, deviceAbis));
            }
            return abis;
        }
    }

    /** Return the abis supported by the Host build target architecture. Exposed for testing. */
    @VisibleForTesting
    protected Set<String> getAbisForBuildTargetArch() {
        // If TestSuiteInfo does not exists, the stub arch will be replaced by all possible abis.
        return AbiUtils.getAbisForArch(TestSuiteInfo.getInstance().getTargetArch());
    }

    /** Returns the abi requested with the option -a or --abi. */
    public final String getRequestedAbi() {
        return mAbiName;
    }

    /**
     * Apply the metadata filter to the config and see if the config should run.
     *
     * @param config The {@link IConfiguration} being evaluated.
     * @param include the metadata include filter
     * @param exclude the metadata exclude filter
     * @return True if the module should run, false otherwise.
     */
    @VisibleForTesting
    protected boolean filterByConfigMetadata(
            IConfiguration config,
            MultiMap<String, String> include,
            MultiMap<String, String> exclude) {
        MultiMap<String, String> metadata = config.getConfigurationDescription().getAllMetaData();
        boolean shouldInclude = false;
        for (String key : include.keySet()) {
            Set<String> filters = new HashSet<>(include.get(key));
            if (metadata.containsKey(key)) {
                filters.retainAll(metadata.get(key));
                if (!filters.isEmpty()) {
                    // inclusion filter is not empty and there's at least one matching inclusion
                    // rule so there's no need to match other inclusion rules
                    shouldInclude = true;
                    break;
                }
            }
        }
        if (!include.isEmpty() && !shouldInclude) {
            // if inclusion filter is not empty and we didn't find a match, the module will not be
            // included
            return false;
        }
        // Now evaluate exclusion rules, this ordering also means that exclusion rules may override
        // inclusion rules: a config already matched for inclusion may still be excluded if matching
        // rules exist
        for (String key : exclude.keySet()) {
            Set<String> filters = new HashSet<>(exclude.get(key));
            if (metadata.containsKey(key)) {
                filters.retainAll(metadata.get(key));
                if (!filters.isEmpty()) {
                    // we found at least one matching exclusion rules, so we are excluding this
                    // this module
                    return false;
                }
            }
        }
        // we've matched at least one inclusion rule (if there's any) AND we didn't match any of the
        // exclusion rules (if there's any)
        return true;
    }

    /**
     * Filter out the preparers that were not whitelisted. This is useful for collect-tests-only
     * where some preparers are not needed to dry run through the invocation.
     *
     * @param config the {@link IConfiguration} considered for filtering.
     * @param preparerWhiteList the current preparer whitelist.
     */
    @VisibleForTesting
    void filterPreparers(IConfiguration config, Set<String> preparerWhiteList) {
        // If no filters was provided, skip the filtering.
        if (preparerWhiteList.isEmpty()) {
            return;
        }
        List<ITargetPreparer> preparers = new ArrayList<>(config.getTargetPreparers());
        for (ITargetPreparer prep : preparers) {
            if (!preparerWhiteList.contains(prep.getClass().getName())) {
                config.getTargetPreparers().remove(prep);
            }
        }
    }

    /**
     * Apply the Runner whitelist filtering, removing any runner that was not whitelisted. If a
     * configuration has several runners, some might be removed and the config will still run.
     *
     * @param config The {@link IConfiguration} being evaluated.
     * @param allowedRunners The current runner whitelist.
     * @return True if the configuration module is allowed to run, false otherwise.
     */
    @VisibleForTesting
    protected boolean filterByRunnerType(IConfiguration config, Set<String> allowedRunners) {
        // If no filters are provided, simply run everything.
        if (allowedRunners.isEmpty()) {
            return true;
        }
        Iterator<IRemoteTest> iterator = config.getTests().iterator();
        while (iterator.hasNext()) {
            IRemoteTest test = iterator.next();
            if (!allowedRunners.contains(test.getClass().getName())) {
                CLog.d(
                        "Runner '%s' in module '%s' was skipped by the runner whitelist: '%s'.",
                        test.getClass().getName(), config.getName(), allowedRunners);
                iterator.remove();
            }
        }

        if (config.getTests().isEmpty()) {
            CLog.d("Module %s does not have any more tests, skipping it.", config.getName());
            return false;
        }
        return true;
    }
}
