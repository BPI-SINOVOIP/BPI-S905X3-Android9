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

package com.android.tradefed.testtype.junit4;

import static org.junit.Assert.assertTrue;

import com.android.annotations.VisibleForTesting;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log.LogLevel;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.result.ddmlib.DefaultRemoteAndroidTestRunner;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.targetprep.suite.SuiteApkInstaller;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;

import org.junit.After;
import org.junit.Assume;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Base test class for running host JUnit4 style tests. This class provides support to install, run
 * and clean up instrumentation tests from the host side. This class is multi-devices compatible.
 * Should be the single source of truth to run instrumentation tests from host side in order to
 * avoid duplicated utility and base class.
 */
public abstract class BaseHostJUnit4Test
        implements IAbiReceiver, IBuildReceiver, IDeviceTest, IInvocationContextReceiver {

    static final String AJUR_RUNNER = "android.support.test.runner.AndroidJUnitRunner";
    static final long DEFAULT_TEST_TIMEOUT_MS = 10 * 60 * 1000L;
    private static final long DEFAULT_MAX_TIMEOUT_TO_OUTPUT_MS = 10 * 60 * 1000L; //10min

    private ITestDevice mDevice;
    private IBuildInfo mBuild;
    private IAbi mAbi;
    private IInvocationContext mContext;
    private Map<SuiteApkInstaller, ITestDevice> mInstallers = new LinkedHashMap<>();
    private TestRunResult mLatestInstruRes;

    @Override
    public final void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public final ITestDevice getDevice() {
        return mDevice;
    }

    @Override
    public final void setBuild(IBuildInfo buildInfo) {
        mBuild = buildInfo;
    }

    public final IBuildInfo getBuild() {
        return mBuild;
    }

    @Override
    public final void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public final IAbi getAbi() {
        return mAbi;
    }

    @Override
    public final void setInvocationContext(IInvocationContext invocationContext) {
        mContext = invocationContext;
    }

    public final List<ITestDevice> getListDevices() {
        return mContext.getDevices();
    }

    /**
     * Automatic tear down for all the apk installed. This will uninstall all the apk from the
     * device they where installed on.
     */
    @After
    public final void autoTearDown() throws DeviceNotAvailableException {
        mLatestInstruRes = null;
        for (SuiteApkInstaller installer : mInstallers.keySet()) {
            ITestDevice device = mInstallers.get(installer);
            installer.tearDown(device, mContext.getBuildInfo(device), null);
        }
        mInstallers.clear();
    }

    // ------------------------- Utility APIs provided for tests -------------------------

    /**
     * Install an apk given its name on the device. Apk will be auto-cleaned.
     *
     * @param apkFileName The name of the apk file.
     * @param options extra options given to the install command
     */
    public final void installPackage(String apkFileName, String... options)
            throws DeviceNotAvailableException, TargetSetupError {
        installPackage(getDevice(), apkFileName, options);
    }

    /**
     * Install an apk given its name on a given device. Apk will be auto-cleaned.
     *
     * @param device the {@link ITestDevice} on which to install the apk.
     * @param apkFileName The name of the apk file.
     * @param options extra options given to the install command
     */
    public final void installPackage(ITestDevice device, String apkFileName, String... options)
            throws DeviceNotAvailableException, TargetSetupError {
        SuiteApkInstaller installer = createSuiteApkInstaller();
        // Force the apk clean up
        installer.setCleanApk(true);
        // Store the preparer for cleanup
        mInstallers.put(installer, device);
        installer.addTestFileName(apkFileName);
        installer.setAbi(getAbi());
        for (String option : options) {
            installer.addInstallArg(option);
        }
        installer.setUp(device, mContext.getBuildInfo(device));
    }

    /**
     * Install an apk given its name for a specific user.
     *
     * @param apkFileName The name of the apk file.
     * @param grantPermission whether to pass the grant permission flag when installing the apk.
     * @param userId the user id of the user where to install the apk.
     * @param options extra options given to the install command
     */
    public final void installPackageAsUser(
            String apkFileName, boolean grantPermission, int userId, String... options)
            throws DeviceNotAvailableException, TargetSetupError {
        installPackageAsUser(getDevice(), apkFileName, grantPermission, userId, options);
    }

    /**
     * Install an apk given its name for a specific user on a given device.
     *
     * @param device the {@link ITestDevice} on which to install the apk.
     * @param apkFileName The name of the apk file.
     * @param grantPermission whether to pass the grant permission flag when installing the apk.
     * @param userId the user id of the user where to install the apk.
     * @param options extra options given to the install command
     */
    public final void installPackageAsUser(
            ITestDevice device,
            String apkFileName,
            boolean grantPermission,
            int userId,
            String... options)
            throws DeviceNotAvailableException, TargetSetupError {
        SuiteApkInstaller installer = createSuiteApkInstaller();
        // Force the apk clean up
        installer.setCleanApk(true);
        // Store the preparer for cleanup
        mInstallers.put(installer, device);
        installer.addTestFileName(apkFileName);
        installer.setUserId(userId);
        installer.setShouldGrantPermission(grantPermission);
        installer.setAbi(getAbi());
        for (String option : options) {
            installer.addInstallArg(option);
        }
        installer.setUp(device, mContext.getBuildInfo(device));
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @return True if it succeed without failure. False otherwise.
     */
    public final boolean runDeviceTests(String pkgName, String testClassName)
            throws DeviceNotAvailableException {
        return runDeviceTests(getDevice(), pkgName, testClassName, null);
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @param testTimeoutMs the timeout in millisecond to be applied to each test case.
     * @return True if it succeed without failure. False otherwise.
     */
    public final boolean runDeviceTests(String pkgName, String testClassName, Long testTimeoutMs)
            throws DeviceNotAvailableException {
        return runDeviceTests(
                getDevice(),
                AJUR_RUNNER,
                pkgName,
                testClassName,
                null,
                null,
                testTimeoutMs,
                DEFAULT_MAX_TIMEOUT_TO_OUTPUT_MS,
                0L,
                true,
                false);
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @param userId the id of the user to run the test against. can be null.
     * @param testTimeoutMs the timeout in millisecond to be applied to each test case.
     * @return True if it succeed without failure. False otherwise.
     */
    public final boolean runDeviceTests(
            String pkgName, String testClassName, Integer userId, Long testTimeoutMs)
            throws DeviceNotAvailableException {
        return runDeviceTests(
                getDevice(),
                AJUR_RUNNER,
                pkgName,
                testClassName,
                null,
                userId,
                testTimeoutMs,
                DEFAULT_MAX_TIMEOUT_TO_OUTPUT_MS,
                0L,
                true,
                false);
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @param testMethodName the name of the test method in the class to be run.
     * @return True if it succeed without failure. False otherwise.
     */
    public final boolean runDeviceTests(String pkgName, String testClassName, String testMethodName)
            throws DeviceNotAvailableException {
        return runDeviceTests(getDevice(), pkgName, testClassName, testMethodName);
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param runner the instrumentation runner to be used.
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @param testMethodName the name of the test method in the class to be run.
     * @return True if it succeed without failure. False otherwise.
     */
    public final boolean runDeviceTests(
            String runner, String pkgName, String testClassName, String testMethodName)
            throws DeviceNotAvailableException {
        return runDeviceTests(
                getDevice(),
                runner,
                pkgName,
                testClassName,
                testMethodName,
                null,
                DEFAULT_TEST_TIMEOUT_MS,
                DEFAULT_MAX_TIMEOUT_TO_OUTPUT_MS,
                0L,
                true,
                false);
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param device the device agaisnt which to run the instrumentation.
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @param testMethodName the name of the test method in the class to be run.
     * @return True if it succeed without failure. False otherwise.
     */
    public final boolean runDeviceTests(
            ITestDevice device, String pkgName, String testClassName, String testMethodName)
            throws DeviceNotAvailableException {
        return runDeviceTests(
                device,
                AJUR_RUNNER,
                pkgName,
                testClassName,
                testMethodName,
                null,
                DEFAULT_TEST_TIMEOUT_MS,
                DEFAULT_MAX_TIMEOUT_TO_OUTPUT_MS,
                0L,
                true,
                false);
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param device the device agaisnt which to run the instrumentation.
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @param testMethodName the name of the test method in the class to be run.
     * @param testTimeoutMs the timeout in millisecond to be applied to each test case.
     * @return True if it succeed without failure. False otherwise.
     */
    public final boolean runDeviceTests(
            ITestDevice device,
            String pkgName,
            String testClassName,
            String testMethodName,
            Long testTimeoutMs)
            throws DeviceNotAvailableException {
        return runDeviceTests(
                device,
                AJUR_RUNNER,
                pkgName,
                testClassName,
                testMethodName,
                null,
                testTimeoutMs,
                DEFAULT_MAX_TIMEOUT_TO_OUTPUT_MS,
                0L,
                true,
                false);
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param device the device agaisnt which to run the instrumentation.
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @param testMethodName the name of the test method in the class to be run.
     * @param userId the id of the user to run the test against. can be null.
     * @param testTimeoutMs the timeout in millisecond to be applied to each test case.
     * @return True if it succeed without failure. False otherwise.
     */
    public final boolean runDeviceTests(
            ITestDevice device,
            String pkgName,
            String testClassName,
            String testMethodName,
            Integer userId,
            Long testTimeoutMs)
            throws DeviceNotAvailableException {
        return runDeviceTests(
                device,
                AJUR_RUNNER,
                pkgName,
                testClassName,
                testMethodName,
                userId,
                testTimeoutMs,
                DEFAULT_MAX_TIMEOUT_TO_OUTPUT_MS,
                0L,
                true,
                false);
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param device the device agaisnt which to run the instrumentation.
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @param testMethodName the name of the test method in the class to be run.
     * @param testTimeoutMs the timeout in millisecond to be applied to each test case.
     * @param maxTimeToOutputMs the max timeout the test has to start outputting something.
     * @param maxInstrumentationTimeoutMs the max timeout the full instrumentation has to complete.
     * @return True if it succeed without failure. False otherwise.
     */
    public final boolean runDeviceTests(
            ITestDevice device,
            String pkgName,
            String testClassName,
            String testMethodName,
            Long testTimeoutMs,
            Long maxTimeToOutputMs,
            Long maxInstrumentationTimeoutMs)
            throws DeviceNotAvailableException {
        return runDeviceTests(
                device,
                AJUR_RUNNER,
                pkgName,
                testClassName,
                testMethodName,
                null,
                testTimeoutMs,
                maxTimeToOutputMs,
                maxInstrumentationTimeoutMs,
                true,
                false);
    }

    /**
     * Runs the instrumentation base on the information in {@link DeviceTestRunOptions}.
     *
     * @param options the {@link DeviceTestRunOptions} driving the instrumentation setup.
     * @return True if it succeeded without failure. False otherwise.
     * @throws DeviceNotAvailableException
     */
    public final boolean runDeviceTests(DeviceTestRunOptions options)
            throws DeviceNotAvailableException {
        return runDeviceTests(
                options.getDevice() == null ? getDevice() : options.getDevice(),
                options.getRunner(),
                options.getPackageName(),
                options.getTestClassName(),
                options.getTestMethodName(),
                options.getUserId(),
                options.getTestTimeoutMs(),
                options.getMaxTimeToOutputMs(),
                options.getMaxInstrumentationTimeoutMs(),
                options.shouldCheckResults(),
                options.isHiddenApiCheckDisabled());
    }

    /**
     * Method to run an installed instrumentation package. Use {@link #getLastDeviceRunResults()}
     * right after to get the details of results.
     *
     * @param device the device agaisnt which to run the instrumentation.
     * @param pkgName the name of the package to run.
     * @param testClassName the name of the test class to run.
     * @param testMethodName the name of the test method in the class to be run.
     * @param userId the id of the user to run the test against. can be null.
     * @param testTimeoutMs the timeout in millisecond to be applied to each test case.
     * @param maxTimeToOutputMs the max timeout the test has to start outputting something.
     * @param maxInstrumentationTimeoutMs the max timeout the full instrumentation has to complete.
     * @param checkResults whether or not the results are checked for crashes.
     * @param isHiddenApiCheckDisabled whether or not we should disable the hidden api check.
     * @return True if it succeeded without failure. False otherwise.
     */
    public final boolean runDeviceTests(
            ITestDevice device,
            String runner,
            String pkgName,
            String testClassName,
            String testMethodName,
            Integer userId,
            Long testTimeoutMs,
            Long maxTimeToOutputMs,
            Long maxInstrumentationTimeoutMs,
            boolean checkResults,
            boolean isHiddenApiCheckDisabled)
            throws DeviceNotAvailableException {
        TestRunResult runResult =
                doRunTests(
                        device,
                        runner,
                        pkgName,
                        testClassName,
                        testMethodName,
                        userId,
                        testTimeoutMs,
                        maxTimeToOutputMs,
                        maxInstrumentationTimeoutMs,
                        isHiddenApiCheckDisabled);
        mLatestInstruRes = runResult;
        printTestResult(runResult);
        if (checkResults) {
            if (runResult.isRunFailure()) {
                throw new AssertionError(
                        "Failed to successfully run device tests for "
                                + runResult.getName()
                                + ": "
                                + runResult.getRunFailureMessage());
            }
            if (runResult.getNumTests() == 0) {
                throw new AssertionError("No tests were run on the device");
            }
            if (runResult.hasFailedTests()) {
                // build a meaningful error message
                StringBuilder errorBuilder = new StringBuilder("on-device tests failed:\n");
                for (Map.Entry<TestDescription, TestResult> resultEntry :
                        runResult.getTestResults().entrySet()) {
                    if (!TestStatus.PASSED.equals(resultEntry.getValue().getStatus())) {
                        errorBuilder.append(resultEntry.getKey().toString());
                        errorBuilder.append(":\n");
                        errorBuilder.append(resultEntry.getValue().getStackTrace());
                    }
                }
                throw new AssertionError(errorBuilder.toString());
            }
            // Assume not all tests have skipped (and rethrow AssumptionViolatedException if so)
            Assume.assumeTrue(
                    runResult.getNumTests()
                            != runResult.getNumTestsInState(TestStatus.ASSUMPTION_FAILURE));
        }
        return !runResult.hasFailedTests() && runResult.getNumTests() > 0;
    }

    /**
     * Returns the {@link TestRunResult} resulting from the latest runDeviceTests that ran. Or null
     * if no results available.
     */
    public final TestRunResult getLastDeviceRunResults() {
        return mLatestInstruRes;
    }

    /** Helper method to run tests and return the listener that collected the results. */
    private TestRunResult doRunTests(
            ITestDevice device,
            String runner,
            String pkgName,
            String testClassName,
            String testMethodName,
            Integer userId,
            Long testTimeoutMs,
            Long maxTimeToOutputMs,
            Long maxInstrumentationTimeoutMs,
            boolean isHiddenApiCheckDisabled)
            throws DeviceNotAvailableException {
        RemoteAndroidTestRunner testRunner = createTestRunner(pkgName, runner, device.getIDevice());
        String runOptions = "";
        // hidden-api-checks flag only exists in P and after.
        if (isHiddenApiCheckDisabled && (device.getApiLevel() >= 28)) {
            runOptions += "--no-hidden-api-checks ";
        }
        if (getAbi() != null) {
            runOptions += String.format("--abi %s", getAbi().getName());
        }
        // Set the run options if any.
        if (!runOptions.isEmpty()) {
            testRunner.setRunOptions(runOptions);
        }

        if (testClassName != null && testMethodName != null) {
            testRunner.setMethodName(testClassName, testMethodName);
        } else if (testClassName != null) {
            testRunner.setClassName(testClassName);
        }

        if (testTimeoutMs != null) {
            testRunner.addInstrumentationArg("timeout_msec", Long.toString(testTimeoutMs));
        } else {
            testRunner.addInstrumentationArg(
                    "timeout_msec", Long.toString(DEFAULT_TEST_TIMEOUT_MS));
        }
        if (maxTimeToOutputMs != null) {
            testRunner.setMaxTimeToOutputResponse(maxTimeToOutputMs, TimeUnit.MILLISECONDS);
        }
        if (maxInstrumentationTimeoutMs != null) {
            testRunner.setMaxTimeout(maxInstrumentationTimeoutMs, TimeUnit.MILLISECONDS);
        }

        CollectingTestListener listener = createListener();
        if (userId == null) {
            assertTrue(device.runInstrumentationTests(testRunner, listener));
        } else {
            assertTrue(device.runInstrumentationTestsAsUser(testRunner, userId, listener));
        }
        return listener.getCurrentRunResults();
    }

    @VisibleForTesting
    RemoteAndroidTestRunner createTestRunner(
            String packageName, String runnerName, IDevice device) {
        return new DefaultRemoteAndroidTestRunner(packageName, runnerName, device);
    }

    @VisibleForTesting
    CollectingTestListener createListener() {
        return new CollectingTestListener();
    }

    private void printTestResult(TestRunResult runResult) {
        for (Map.Entry<TestDescription, TestResult> testEntry :
                runResult.getTestResults().entrySet()) {
            TestResult testResult = testEntry.getValue();
            TestStatus testStatus = testResult.getStatus();
            CLog.logAndDisplay(LogLevel.INFO, "Test " + testEntry.getKey() + ": " + testStatus);
            if (!TestStatus.PASSED.equals(testStatus)
                    && !TestStatus.ASSUMPTION_FAILURE.equals(testStatus)) {
                CLog.logAndDisplay(LogLevel.WARN, testResult.getStackTrace());
            }
        }
    }

    /**
     * Uninstalls a package on the device.
     *
     * @param pkgName the Android package to uninstall
     * @return a {@link String} with an error code, or <code>null</code> if success
     */
    public final String uninstallPackage(String pkgName) throws DeviceNotAvailableException {
        return getDevice().uninstallPackage(pkgName);
    }

    /**
     * Uninstalls a package on the device
     *
     * @param device the device that should uninstall the package.
     * @param pkgName the Android package to uninstall
     * @return a {@link String} with an error code, or <code>null</code> if success
     */
    public final String uninstallPackage(ITestDevice device, String pkgName)
            throws DeviceNotAvailableException {
        return device.uninstallPackage(pkgName);
    }

    /**
     * Checks if a package of a given name is installed on the device
     *
     * @param pkg the name of the package
     * @return true if the package is found on the device
     */
    public final boolean isPackageInstalled(String pkg) throws DeviceNotAvailableException {
        return isPackageInstalled(getDevice(), pkg);
    }

    /**
     * Checks if a package of a given name is installed on the device
     *
     * @param device the device that should uninstall the package.
     * @param pkg the name of the package
     * @return true if the package is found on the device
     */
    public final boolean isPackageInstalled(ITestDevice device, String pkg)
            throws DeviceNotAvailableException {
        for (String installedPackage : device.getInstalledPackageNames()) {
            if (pkg.equals(installedPackage)) {
                return true;
            }
        }
        return false;
    }

    public boolean hasDeviceFeature(String feature) throws DeviceNotAvailableException {
        return getDevice().hasFeature("feature:" + feature);
    }

    @VisibleForTesting
    SuiteApkInstaller createSuiteApkInstaller() {
        return new SuiteApkInstaller();
    }
}
