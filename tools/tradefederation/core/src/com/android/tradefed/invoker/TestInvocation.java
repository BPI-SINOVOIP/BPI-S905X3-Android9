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
package com.android.tradefed.invoker;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.BuildRetrievalError;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.command.CommandRunner.ExitCode;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.guice.InvocationScope;
import com.android.tradefed.invoker.sandbox.SandboxedInvocationExecution;
import com.android.tradefed.invoker.shard.ShardBuildCloner;
import com.android.tradefed.log.ILeveledLogOutput;
import com.android.tradefed.log.ILogRegistry;
import com.android.tradefed.log.LogRegistry;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogSaverResultForwarder;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.sandbox.SandboxInvocationRunner;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.DeviceFailedToBootError;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IResumableTest;
import com.android.tradefed.testtype.IRetriableTest;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunInterruptedException;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.TimeUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map.Entry;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

/**
 * Default implementation of {@link ITestInvocation}.
 * <p/>
 * Loads major objects based on {@link IConfiguration}
 *   - retrieves build
 *   - prepares target
 *   - runs tests
 *   - reports results
 */
public class TestInvocation implements ITestInvocation {

    /** Key of the command line args attributes */
    public static final String COMMAND_ARGS_KEY = "command_line_args";

    /**
     * Format of the key in {@link IBuildInfo} to log the battery level for each step of the
     * invocation. (Setup, test, tear down).
     */
    private static final String BATTERY_ATTRIBUTE_FORMAT_KEY = "%s-battery-%s";

    static final String TRADEFED_LOG_NAME = "host_log";
    static final String DEVICE_LOG_NAME_PREFIX = "device_logcat_";
    static final String EMULATOR_LOG_NAME_PREFIX = "emulator_log_";
    static final String BUILD_ERROR_BUGREPORT_NAME = "build_error_bugreport";
    static final String DEVICE_UNRESPONSIVE_BUGREPORT_NAME = "device_unresponsive_bugreport";
    static final String INVOCATION_ENDED_BUGREPORT_NAME = "invocation_ended_bugreport";
    static final String TARGET_SETUP_ERROR_BUGREPORT_NAME = "target_setup_error_bugreport";
    static final String BATT_TAG = "[battery level]";

    public enum Stage {
        ERROR("error"),
        SETUP("setup"),
        TEST("test"),
        TEARDOWN("teardown");

        private final String mName;

        Stage(String name) {
            mName = name;
        }

        public String getName() {
            return mName;
        }
    }

    private String mStatus = "(not invoked)";
    private boolean mStopRequested = false;

    /**
     * A {@link ResultForwarder} for forwarding resumed invocations.
     * <p/>
     * It filters the invocationStarted event for the resumed invocation, and sums the invocation
     * elapsed time
     */
    private static class ResumeResultForwarder extends ResultForwarder {

        long mCurrentElapsedTime;

        /**
         * @param listeners
         */
        public ResumeResultForwarder(List<ITestInvocationListener> listeners,
                long currentElapsedTime) {
            super(listeners);
            mCurrentElapsedTime = currentElapsedTime;
        }

        @Override
        public void invocationStarted(IInvocationContext context) {
            // ignore
        }

        @Override
        public void invocationEnded(long newElapsedTime) {
            super.invocationEnded(mCurrentElapsedTime + newElapsedTime);
        }
    }

    /**
     * Display a log message informing the user of a invocation being started.
     *
     * @param context the {@link IInvocationContext}
     * @param config the {@link IConfiguration}
     */
    private void logStartInvocation(IInvocationContext context, IConfiguration config) {
        String shardSuffix = "";
        if (config.getCommandOptions().getShardIndex() != null) {
            shardSuffix =
                    String.format(
                            " (shard %d of %d)",
                            config.getCommandOptions().getShardIndex() + 1,
                            config.getCommandOptions().getShardCount());
        }
        StringBuilder buildInfos = new StringBuilder();
        StringBuilder msg = new StringBuilder("Starting invocation for '");
        msg.append(context.getTestTag());
        msg.append("' with ");
        for (Entry<ITestDevice, IBuildInfo> entry : context.getDeviceBuildMap().entrySet()) {
            msg.append("'[ ");
            msg.append(entry.getValue().toString());
            buildInfos.append(entry.getValue().toString());
            msg.append(" on device '");
            msg.append(entry.getKey().getSerialNumber());
            msg.append("'] ");
        }
        msg.append(shardSuffix);
        CLog.logAndDisplay(LogLevel.INFO, msg.toString());
        mStatus = String.format("running %s on build(s) '%s'", context.getTestTag(),
                buildInfos.toString()) + shardSuffix;
    }

    /**
     * Performs the invocation
     *
     * @param config the {@link IConfiguration}
     * @param context the {@link IInvocationContext} to use.
     */
    private void performInvocation(
            IConfiguration config,
            IInvocationContext context,
            IInvocationExecution invocationPath,
            IRescheduler rescheduler,
            ITestInvocationListener listener)
            throws Throwable {

        boolean resumed = false;
        String bugreportName = null;
        long startTime = System.currentTimeMillis();
        long elapsedTime = -1;
        Throwable exception = null;
        Throwable tearDownException = null;
        ITestDevice badDevice = null;

        startInvocation(config, context, listener);
        // Ensure that no unexpected attributes are added afterward
        ((InvocationContext) context).lockAttributes();
        try {
            logDeviceBatteryLevel(context, "initial");
            prepareAndRun(config, context, invocationPath, listener);
        } catch (BuildError e) {
            exception = e;
            CLog.w("Build failed on device '%s'. Reason: %s", e.getDeviceDescriptor(),
                    e.toString());
            bugreportName = BUILD_ERROR_BUGREPORT_NAME;
            badDevice = context.getDeviceBySerial(e.getDeviceDescriptor().getSerial());
            if (e instanceof DeviceFailedToBootError) {
                if (badDevice == null) {
                    context.setRecoveryModeForAllDevices(RecoveryMode.NONE);
                } else {
                    badDevice.setRecoveryMode(RecoveryMode.NONE);
                }
            }
            reportFailure(e, listener, config, context, rescheduler, invocationPath);
        } catch (TargetSetupError e) {
            exception = e;
            CLog.e("Caught exception while running invocation");
            CLog.e(e);
            bugreportName = TARGET_SETUP_ERROR_BUGREPORT_NAME;
            badDevice = context.getDeviceBySerial(e.getDeviceDescriptor().getSerial());
            reportFailure(e, listener, config, context, rescheduler, invocationPath);
        } catch (DeviceNotAvailableException e) {
            exception = e;
            // log a warning here so its captured before reportLogs is called
            CLog.w("Invocation did not complete due to device %s becoming not available. " +
                    "Reason: %s", e.getSerial(), e.getMessage());
            badDevice = context.getDeviceBySerial(e.getSerial());
            if ((e instanceof DeviceUnresponsiveException) && badDevice != null
                    && TestDeviceState.ONLINE.equals(badDevice.getDeviceState())) {
                // under certain cases it might still be possible to grab a bugreport
                bugreportName = DEVICE_UNRESPONSIVE_BUGREPORT_NAME;
            }
            resumed = resume(config, context, rescheduler, System.currentTimeMillis() - startTime);
            if (!resumed) {
                reportFailure(e, listener, config, context, rescheduler, invocationPath);
            } else {
                CLog.i("Rescheduled failed invocation for resume");
            }
            // Upon reaching here after an exception, it is safe to assume that recovery
            // has already been attempted so we disable it to avoid re-entry during clean up.
            if (badDevice != null) {
                badDevice.setRecoveryMode(RecoveryMode.NONE);
            }
            throw e;
        } catch (RunInterruptedException e) {
            CLog.w("Invocation interrupted");
            reportFailure(e, listener, config, context, rescheduler, invocationPath);
        } catch (AssertionError e) {
            exception = e;
            CLog.e("Caught AssertionError while running invocation: %s", e.toString());
            CLog.e(e);
            reportFailure(e, listener, config, context, rescheduler, invocationPath);
        } catch (Throwable t) {
            exception = t;
            // log a warning here so its captured before reportLogs is called
            CLog.e("Unexpected exception when running invocation: %s", t.toString());
            CLog.e(t);
            reportFailure(t, listener, config, context, rescheduler, invocationPath);
            throw t;
        } finally {
            for (ITestDevice device : context.getDevices()) {
                reportLogs(device, listener, Stage.TEST);
            }
            getRunUtil().allowInterrupt(false);
            if (config.getCommandOptions().takeBugreportOnInvocationEnded() ||
                    config.getCommandOptions().takeBugreportzOnInvocationEnded()) {
                if (bugreportName != null) {
                    CLog.i("Bugreport to be taken for failure instead of invocation ended.");
                } else {
                    bugreportName = INVOCATION_ENDED_BUGREPORT_NAME;
                }
            }
            if (bugreportName != null) {
                if (badDevice == null) {
                    for (ITestDevice device : context.getDevices()) {
                        takeBugreport(device, listener, bugreportName);
                    }
                } else {
                    // If we have identified a faulty device only take the bugreport on it.
                    takeBugreport(badDevice, listener, bugreportName);
                }
            }
            mStatus = "tearing down";
            try {
                invocationPath.doTeardown(context, config, exception);
            } catch (Throwable e) {
                tearDownException = e;
                CLog.e("Exception when tearing down invocation: %s", tearDownException.toString());
                CLog.e(tearDownException);
                if (exception == null) {
                    // only report when the exception is new during tear down
                    reportFailure(
                            tearDownException,
                            listener,
                            config,
                            context,
                            rescheduler,
                            invocationPath);
                }
            }
            mStatus = "done running tests";
            try {
                // Clean up host.
                invocationPath.doCleanUp(context, config, exception);
                for (ITestDevice device : context.getDevices()) {
                    reportLogs(device, listener, Stage.TEARDOWN);
                }
                if (mStopRequested) {
                    CLog.e(
                            "====================================================================="
                                    + "====");
                    CLog.e(
                            "Invocation was interrupted due to TradeFed stop, results will be "
                                    + "affected.");
                    CLog.e(
                            "====================================================================="
                                    + "====");
                }
                reportHostLog(listener, config.getLogOutput());
                elapsedTime = System.currentTimeMillis() - startTime;
                if (!resumed) {
                    listener.invocationEnded(elapsedTime);
                }
            } finally {
                invocationPath.cleanUpBuilds(context, config);
            }
        }
        if (tearDownException != null) {
            // this means a DNAE or RTE has happened during teardown, need to throw
            // if there was a preceding RTE or DNAE stored in 'exception', it would have already
            // been thrown before exiting the previous try...catch...finally block
            throw tearDownException;
        }
    }

    /** Do setup and run the tests */
    private void prepareAndRun(
            IConfiguration config,
            IInvocationContext context,
            IInvocationExecution invocationPath,
            ITestInvocationListener listener)
            throws Throwable {
        if (config.getCommandOptions().shouldUseSandboxing()) {
            // TODO: extract in new TestInvocation type.
            // If the invocation is sandboxed run as a sandbox instead.
            SandboxInvocationRunner.prepareAndRun(config, context, listener);
            return;
        }
        getRunUtil().allowInterrupt(true);
        logDeviceBatteryLevel(context, "initial -> setup");
        invocationPath.doSetup(context, config, listener);
        logDeviceBatteryLevel(context, "setup -> test");
        invocationPath.runTests(context, config, listener);
        logDeviceBatteryLevel(context, "after test");
    }

    /**
     * Starts the invocation.
     * <p/>
     * Starts logging, and informs listeners that invocation has been started.
     *
     * @param config
     * @param context
     */
    private void startInvocation(IConfiguration config, IInvocationContext context,
            ITestInvocationListener listener) {
        logStartInvocation(context, config);
        listener.invocationStarted(context);
    }

    /**
     * Attempt to reschedule the failed invocation to resume where it left off.
     * <p/>
     * @see IResumableTest
     *
     * @param config
     * @return <code>true</code> if invocation was resumed successfully
     */
    private boolean resume(IConfiguration config, IInvocationContext context,
            IRescheduler rescheduler, long elapsedTime) {
        for (IRemoteTest test : config.getTests()) {
            if (test instanceof IResumableTest) {
                IResumableTest resumeTest = (IResumableTest)test;
                if (resumeTest.isResumable()) {
                    // resume this config if any test is resumable
                    IConfiguration resumeConfig = config.clone();
                    // reuse the same build for the resumed invocation
                    ShardBuildCloner.cloneBuildInfos(resumeConfig, resumeConfig, context);

                    // create a result forwarder, to prevent sending two invocationStarted events
                    resumeConfig.setTestInvocationListener(new ResumeResultForwarder(
                            config.getTestInvocationListeners(), elapsedTime));
                    resumeConfig.setLogOutput(config.getLogOutput().clone());
                    resumeConfig.setCommandOptions(config.getCommandOptions().clone());
                    boolean canReschedule = rescheduler.scheduleConfig(resumeConfig);
                    if (!canReschedule) {
                        CLog.i("Cannot reschedule resumed config for build. Cleaning up build.");
                        for (String deviceName : context.getDeviceConfigNames()) {
                            resumeConfig.getDeviceConfigByName(deviceName).getBuildProvider()
                                    .cleanUp(context.getBuildInfo(deviceName));
                        }
                    }
                    // FIXME: is it a bug to return from here, when we may not have completed the
                    // FIXME: config.getTests iteration?
                    return canReschedule;
                }
            }
        }
        return false;
    }

    private void reportFailure(
            Throwable exception,
            ITestInvocationListener listener,
            IConfiguration config,
            IInvocationContext context,
            IRescheduler rescheduler,
            IInvocationExecution invocationPath) {
        // Always report the failure
        listener.invocationFailed(exception);
        // Reset the build (if necessary) and decide if we should reschedule the configuration.
        boolean shouldReschedule =
                invocationPath.resetBuildAndReschedule(exception, listener, config, context);
        if (shouldReschedule) {
            rescheduleTest(config, rescheduler);
        }
    }

    private void rescheduleTest(IConfiguration config, IRescheduler rescheduler) {
        for (IRemoteTest test : config.getTests()) {
            if (!config.getCommandOptions().isLoopMode() && test instanceof IRetriableTest &&
                    ((IRetriableTest) test).isRetriable()) {
                rescheduler.rescheduleCommand();
                return;
            }
        }
    }

    private void reportLogs(ITestDevice device, ITestInvocationListener listener, Stage stage) {
        if (device == null) {
            return;
        }
        // non stub device
        if (!(device.getIDevice() instanceof StubDevice)) {
            try (InputStreamSource logcatSource = device.getLogcat()) {
                device.clearLogcat();
                String name = getDeviceLogName(stage);
                listener.testLog(name, LogDataType.LOGCAT, logcatSource);
            }
        }
        // emulator logs
        if (device.getIDevice() != null && device.getIDevice().isEmulator()) {
            try (InputStreamSource emulatorOutput = device.getEmulatorOutput()) {
                // TODO: Clear the emulator log
                String name = getEmulatorLogName(stage);
                listener.testLog(name, LogDataType.TEXT, emulatorOutput);
            }

        }
    }

    private void reportHostLog(ITestInvocationListener listener, ILeveledLogOutput logger) {
        try (InputStreamSource globalLogSource = logger.getLog()) {
            listener.testLog(TRADEFED_LOG_NAME, LogDataType.TEXT, globalLogSource);
        }
        // once tradefed log is reported, all further log calls for this invocation can get lost
        // unregister logger so future log calls get directed to the tradefed global log
        getLogRegistry().unregisterLogger();
        logger.closeLog();
    }

    private void takeBugreport(
            ITestDevice device, ITestInvocationListener listener, String bugreportName) {
        if (device == null) {
            return;
        }
        if (device.getIDevice() instanceof StubDevice) {
            return;
        }
        // logBugreport will report a regular bugreport if bugreportz is not supported.
        boolean res =
                device.logBugreport(
                        String.format("%s_%s", bugreportName, device.getSerialNumber()), listener);
        if (!res) {
            CLog.w("Error when collecting bugreport for device '%s'", device.getSerialNumber());
        }
    }

    /**
     * Gets the {@link ILogRegistry} to use.
     * <p/>
     * Exposed for unit testing.
     */
    ILogRegistry getLogRegistry() {
        return LogRegistry.getLogRegistry();
    }

    /**
     * Utility method to fetch the default {@link IRunUtil} singleton
     * <p />
     * Exposed for unit testing.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    @Override
    public String toString() {
        return mStatus;
    }

    /**
     * Log the battery level of each device in the invocation.
     *
     * @param context the {@link IInvocationContext} of the invocation.
     * @param event a {@link String} describing the context of the logging (initial, setup, etc.).
     */
    @VisibleForTesting
    void logDeviceBatteryLevel(IInvocationContext context, String event) {
        for (ITestDevice testDevice : context.getDevices()) {
            if (testDevice == null) {
                continue;
            }
            IDevice device = testDevice.getIDevice();
            if (device == null || device instanceof StubDevice) {
                continue;
            }
            try {
                Integer batteryLevel = device.getBattery(500, TimeUnit.MILLISECONDS).get();
                CLog.v("%s - %s - %d%%", BATT_TAG, event, batteryLevel);
                context.getBuildInfo(testDevice)
                        .addBuildAttribute(
                                String.format(
                                        BATTERY_ATTRIBUTE_FORMAT_KEY,
                                        testDevice.getSerialNumber(),
                                        event),
                                batteryLevel.toString());
                continue;
            } catch (InterruptedException | ExecutionException e) {
                // fall through
            }

            CLog.v("Failed to get battery level for %s", testDevice.getSerialNumber());
        }
    }

    /**
     * Invoke {@link IInvocationExecution#fetchBuild(IInvocationContext, IConfiguration,
     * IRescheduler, ITestInvocationListener)} and handles the output as well as failures.
     *
     * @param context the {@link IInvocationContext} of the invocation.
     * @param config the {@link IConfiguration} of this test run.
     * @param rescheduler the {@link IRescheduler}, for rescheduling portions of the invocation for
     *     execution on another resource(s)
     * @param listener the {@link ITestInvocation} to report build download failures.
     * @param invocationPath the {@link IInvocationExecution} driving the invocation.
     * @return True if we successfully downloaded the build, false otherwise.
     * @throws DeviceNotAvailableException
     */
    private boolean invokeFetchBuild(
            IInvocationContext context,
            IConfiguration config,
            IRescheduler rescheduler,
            ITestInvocationListener listener,
            IInvocationExecution invocationPath)
            throws DeviceNotAvailableException {
        try {
            boolean res = invocationPath.fetchBuild(context, config, rescheduler, listener);
            if (!res) {
                mStatus = "(no build to test)";
                rescheduleTest(config, rescheduler);
                // Set the exit code to error
                setExitCode(ExitCode.NO_BUILD, new BuildRetrievalError("No build found to test."));
                return false;
            }
            return res;
        } catch (BuildRetrievalError e) {
            // report an empty invocation, so this error is sent to listeners
            startInvocation(config, context, listener);
            // don't want to use #reportFailure, since that will call buildNotTested
            listener.invocationFailed(e);
            for (ITestDevice device : context.getDevices()) {
                reportLogs(device, listener, Stage.ERROR);
            }
            reportHostLog(listener, config.getLogOutput());
            listener.invocationEnded(0);
            return false;
        }
    }

    /** {@inheritDoc} */
    @Override
    public void invoke(
            IInvocationContext context,
            IConfiguration config,
            IRescheduler rescheduler,
            ITestInvocationListener... extraListeners)
            throws DeviceNotAvailableException, Throwable {
        List<ITestInvocationListener> allListeners =
                new ArrayList<>(config.getTestInvocationListeners().size() + extraListeners.length);
        allListeners.addAll(config.getTestInvocationListeners());
        allListeners.addAll(Arrays.asList(extraListeners));
        ITestInvocationListener listener =
                new LogSaverResultForwarder(config.getLogSaver(), allListeners);
        IInvocationExecution invocationPath =
                createInvocationExec(config.getConfigurationDescription().shouldUseSandbox());

        // Create the Guice scope
        InvocationScope scope = getInvocationScope();
        scope.enter();
        // Seed our TF objects to the Guice scope
        scope.seedConfiguration(config);
        try {
            mStatus = "fetching build";
            config.getLogOutput().init();
            getLogRegistry().registerLogger(config.getLogOutput());
            for (String deviceName : context.getDeviceConfigNames()) {
                context.getDevice(deviceName).clearLastConnectedWifiNetwork();
                context.getDevice(deviceName)
                        .setOptions(config.getDeviceConfigByName(deviceName).getDeviceOptions());
                if (config.getDeviceConfigByName(deviceName)
                        .getDeviceOptions()
                        .isLogcatCaptureEnabled()) {
                    if (!(context.getDevice(deviceName).getIDevice() instanceof StubDevice)) {
                        context.getDevice(deviceName).startLogcat();
                    }
                }
            }

            String cmdLineArgs = config.getCommandLine();
            if (cmdLineArgs != null) {
                CLog.i("Invocation was started with cmd: %s", cmdLineArgs);
            }

            long start = System.currentTimeMillis();
            boolean providerSuccess =
                    invokeFetchBuild(context, config, rescheduler, listener, invocationPath);
            long fetchBuildDuration = System.currentTimeMillis() - start;
            context.addInvocationTimingMetric(IInvocationContext.TimingEvent.FETCH_BUILD,
                    fetchBuildDuration);
            CLog.d("Fetch build duration: %s", TimeUtil.formatElapsedTime(fetchBuildDuration));
            if (!providerSuccess) {
                return;
            }

            mStatus = "sharding";
            boolean sharding = invocationPath.shardConfig(config, context, rescheduler);
            if (sharding) {
                CLog.i("Invocation for %s has been sharded, rescheduling", context.getSerials());
                return;
            }

            if (config.getTests() == null || config.getTests().isEmpty()) {
                CLog.e("No tests to run");
                return;
            }

            performInvocation(config, context, invocationPath, rescheduler, listener);
            setExitCode(ExitCode.NO_ERROR, null);
        } catch (IOException e) {
            CLog.e(e);
        } finally {
            scope.exit();
            // Ensure build infos are always cleaned up at the end of invocation.
            invocationPath.cleanUpBuilds(context, config);

            // ensure we always deregister the logger
            for (String deviceName : context.getDeviceConfigNames()) {
                if (!(context.getDevice(deviceName).getIDevice() instanceof StubDevice)) {
                    context.getDevice(deviceName).stopLogcat();
                }
            }
            // save remaining logs contents to global log
            getLogRegistry().dumpToGlobalLog(config.getLogOutput());
            // Ensure log is unregistered and closed
            getLogRegistry().unregisterLogger();
            config.getLogOutput().closeLog();
        }
    }

    /** Returns the current {@link InvocationScope}. */
    @VisibleForTesting
    InvocationScope getInvocationScope() {
        return InvocationScope.getDefault();
    }

    /**
     * Helper to set the exit code. Exposed for testing.
     */
    protected void setExitCode(ExitCode code, Throwable stack) {
        GlobalConfiguration.getInstance().getCommandScheduler()
                .setLastInvocationExitCode(code, stack);
    }

    public static String getDeviceLogName(Stage stage) {
        return DEVICE_LOG_NAME_PREFIX + stage.getName();
    }

    public static String getEmulatorLogName(Stage stage) {
        return EMULATOR_LOG_NAME_PREFIX + stage.getName();
    }

    @Override
    public void notifyInvocationStopped() {
        mStopRequested = true;
    }

    /**
     * Create the invocation path that should be followed.
     *
     * @param isSandboxed If we are currently running in the sandbox, then a special path is
     *     applied.
     * @return The {@link IInvocationExecution} describing the invocation.
     */
    public IInvocationExecution createInvocationExec(boolean isSandboxed) {
        if (isSandboxed) {
            return new SandboxedInvocationExecution();
        }
        return new InvocationExecution();
    }
}
