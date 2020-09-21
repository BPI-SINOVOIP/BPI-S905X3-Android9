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
package com.android.tradefed.invoker;

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.BuildInfoKey.BuildInfoFileKey;
import com.android.tradefed.build.BuildRetrievalError;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IBuildInfo.BuildInfoProperties;
import com.android.tradefed.build.IBuildProvider;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.build.IDeviceBuildProvider;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IDeviceConfiguration;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.device.metric.IMetricCollectorReceiver;
import com.android.tradefed.invoker.TestInvocation.Stage;
import com.android.tradefed.invoker.shard.IShardHelper;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ITestLoggerReceiver;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.suite.checker.ISystemStatusCheckerReceiver;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.IHostCleaner;
import com.android.tradefed.targetprep.ITargetCleaner;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.SystemUtil;
import com.android.tradefed.util.SystemUtil.EnvVariable;
import com.android.tradefed.util.TimeUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.ListIterator;

/**
 * Class that describes all the invocation steps: build download, target_prep, run tests, clean up.
 * Can be extended to override the default behavior of some steps. Order of the steps is driven by
 * {@link TestInvocation}.
 */
public class InvocationExecution implements IInvocationExecution {

    @Override
    public boolean fetchBuild(
            IInvocationContext context,
            IConfiguration config,
            IRescheduler rescheduler,
            ITestInvocationListener listener)
            throws DeviceNotAvailableException, BuildRetrievalError {
        String currentDeviceName = null;
        try {
            updateInvocationContext(context, config);
            // TODO: evaluate fetching build in parallel
            for (String deviceName : context.getDeviceConfigNames()) {
                currentDeviceName = deviceName;
                IBuildInfo info = null;
                ITestDevice device = context.getDevice(deviceName);
                IDeviceConfiguration deviceConfig = config.getDeviceConfigByName(deviceName);
                IBuildProvider provider = deviceConfig.getBuildProvider();
                // Inject the context to the provider if it can receive it
                if (provider instanceof IInvocationContextReceiver) {
                    ((IInvocationContextReceiver) provider).setInvocationContext(context);
                }
                // Get the build
                if (provider instanceof IDeviceBuildProvider) {
                    // Download a device build if the provider can handle it.
                    info = ((IDeviceBuildProvider) provider).getBuild(device);
                } else {
                    info = provider.getBuild();
                }
                if (info != null) {
                    info.setDeviceSerial(device.getSerialNumber());
                    context.addDeviceBuildInfo(deviceName, info);
                    device.setRecovery(deviceConfig.getDeviceRecovery());
                } else {
                    CLog.logAndDisplay(
                            LogLevel.WARN,
                            "No build found to test for device: %s",
                            device.getSerialNumber());
                    return false;
                }
                // TODO: remove build update when reporting is done on context
                updateBuild(info, config);
            }
        } catch (BuildRetrievalError e) {
            CLog.e(e);
            if (currentDeviceName != null) {
                context.addDeviceBuildInfo(currentDeviceName, e.getBuildInfo());
                updateInvocationContext(context, config);
            }
            throw e;
        }
        return true;
    }

    @Override
    public void cleanUpBuilds(IInvocationContext context, IConfiguration config) {
        // Ensure build infos are always cleaned up at the end of invocation.
        for (String cleanUpDevice : context.getDeviceConfigNames()) {
            if (context.getBuildInfo(cleanUpDevice) != null) {
                try {
                    config.getDeviceConfigByName(cleanUpDevice)
                            .getBuildProvider()
                            .cleanUp(context.getBuildInfo(cleanUpDevice));
                } catch (RuntimeException e) {
                    // We catch an simply log exception in cleanUp to avoid missing any final
                    // step of the invocation.
                    CLog.e(e);
                }
            }
        }
    }

    @Override
    public boolean shardConfig(
            IConfiguration config, IInvocationContext context, IRescheduler rescheduler) {
        return createShardHelper().shardConfig(config, context, rescheduler);
    }

    /** Create an return the {@link IShardHelper} to be used. */
    @VisibleForTesting
    protected IShardHelper createShardHelper() {
        return GlobalConfiguration.getInstance().getShardingStrategy();
    }

    @Override
    public void doSetup(
            IInvocationContext context,
            IConfiguration config,
            final ITestInvocationListener listener)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        long start = System.currentTimeMillis();
        try {
            // Before all the individual setup, make the multi-pre-target-preparer devices setup
            runMultiTargetPreparers(
                    config.getMultiPreTargetPreparers(),
                    listener,
                    context,
                    "multi pre target preparer setup");

            // TODO: evaluate doing device setup in parallel
            for (String deviceName : context.getDeviceConfigNames()) {
                ITestDevice device = context.getDevice(deviceName);
                CLog.d("Starting setup for device: '%s'", device.getSerialNumber());
                if (device instanceof ITestLoggerReceiver) {
                    ((ITestLoggerReceiver) context.getDevice(deviceName)).setTestLogger(listener);
                }
                if (!config.getCommandOptions().shouldSkipPreDeviceSetup()) {
                    device.preInvocationSetup(context.getBuildInfo(deviceName));
                }
                for (ITargetPreparer preparer :
                        config.getDeviceConfigByName(deviceName).getTargetPreparers()) {
                    // do not call the preparer if it was disabled
                    if (preparer.isDisabled()) {
                        CLog.d("%s has been disabled. skipping.", preparer);
                        continue;
                    }
                    if (preparer instanceof ITestLoggerReceiver) {
                        ((ITestLoggerReceiver) preparer).setTestLogger(listener);
                    }
                    CLog.d(
                            "starting preparer '%s' on device: '%s'",
                            preparer, device.getSerialNumber());
                    preparer.setUp(device, context.getBuildInfo(deviceName));
                    CLog.d(
                            "done with preparer '%s' on device: '%s'",
                            preparer, device.getSerialNumber());
                }
                CLog.d("Done with setup of device: '%s'", device.getSerialNumber());
            }
            // After all the individual setup, make the multi-devices setup
            runMultiTargetPreparers(
                    config.getMultiTargetPreparers(),
                    listener,
                    context,
                    "multi target preparer setup");

        } finally {
            // Note: These metrics are handled in a try in case of a kernel reset or device issue.
            // Setup timing metric. It does not include flashing time on boot tests.
            long setupDuration = System.currentTimeMillis() - start;
            context.addInvocationTimingMetric(IInvocationContext.TimingEvent.SETUP, setupDuration);
            CLog.d("Setup duration: %s'", TimeUtil.formatElapsedTime(setupDuration));
            // Upload the setup logcat after setup is complete.
            for (String deviceName : context.getDeviceConfigNames()) {
                reportLogs(context.getDevice(deviceName), listener, Stage.SETUP);
            }
        }
    }

    /** Runs the {@link IMultiTargetPreparer} specified. */
    private void runMultiTargetPreparers(
            List<IMultiTargetPreparer> multiPreparers,
            ITestLogger logger,
            IInvocationContext context,
            String description)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        for (IMultiTargetPreparer multiPreparer : multiPreparers) {
            // do not call the preparer if it was disabled
            if (multiPreparer.isDisabled()) {
                CLog.d("%s has been disabled. skipping.", multiPreparer);
                continue;
            }
            if (multiPreparer instanceof ITestLoggerReceiver) {
                ((ITestLoggerReceiver) multiPreparer).setTestLogger(logger);
            }
            CLog.d("Starting %s '%s'", description, multiPreparer);
            multiPreparer.setUp(context);
            CLog.d("done with %s '%s'", description, multiPreparer);
        }
    }

    /** Runs the {@link IMultiTargetPreparer} specified tearDown. */
    private void runMultiTargetPreparersTearDown(
            List<IMultiTargetPreparer> multiPreparers,
            IInvocationContext context,
            Throwable throwable,
            String description)
            throws DeviceNotAvailableException {
        ListIterator<IMultiTargetPreparer> iterator =
                multiPreparers.listIterator(multiPreparers.size());
        while (iterator.hasPrevious()) {
            IMultiTargetPreparer multipreparer = iterator.previous();
            if (multipreparer.isDisabled() || multipreparer.isTearDownDisabled()) {
                CLog.d("%s has been disabled. skipping.", multipreparer);
                continue;
            }
            CLog.d("Starting %s '%s'", description, multipreparer);
            multipreparer.tearDown(context, throwable);
            CLog.d("Done with %s '%s'", description, multipreparer);
        }
    }

    @Override
    public void doTeardown(IInvocationContext context, IConfiguration config, Throwable exception)
            throws Throwable {
        Throwable throwable = null;

        List<IMultiTargetPreparer> multiPreparers = config.getMultiTargetPreparers();
        runMultiTargetPreparersTearDown(
                multiPreparers, context, throwable, "multi target preparer teardown");

        // Clear wifi settings, to prevent wifi errors from interfering with teardown process.
        for (String deviceName : context.getDeviceConfigNames()) {
            ITestDevice device = context.getDevice(deviceName);
            device.clearLastConnectedWifiNetwork();
            List<ITargetPreparer> preparers =
                    config.getDeviceConfigByName(deviceName).getTargetPreparers();
            ListIterator<ITargetPreparer> itr = preparers.listIterator(preparers.size());
            while (itr.hasPrevious()) {
                ITargetPreparer preparer = itr.previous();
                if (preparer instanceof ITargetCleaner) {
                    ITargetCleaner cleaner = (ITargetCleaner) preparer;
                    // do not call the cleaner if it was disabled
                    if (cleaner.isDisabled() || cleaner.isTearDownDisabled()) {
                        CLog.d("%s has been disabled. skipping.", cleaner);
                        continue;
                    }
                    try {
                        CLog.d(
                                "starting tearDown '%s' on device: '%s'",
                                preparer, device.getSerialNumber());
                        cleaner.tearDown(device, context.getBuildInfo(deviceName), exception);
                        CLog.d(
                                "done with tearDown '%s' on device: '%s'",
                                preparer, device.getSerialNumber());
                    } catch (Throwable e) {
                        // We catch it and rethrow later to allow each targetprep to be attempted.
                        // Only the first one will be thrown but all should be logged.
                        CLog.e("Deferring throw for:");
                        CLog.e(e);
                        if (throwable == null) {
                            throwable = e;
                        }
                    }
                }
            }
            // Extra tear down step for the device
            if (!config.getCommandOptions().shouldSkipPreDeviceSetup()) {
                device.postInvocationTearDown();
            }
        }

        // After all, run the multi_pre_target_preparer tearDown.
        List<IMultiTargetPreparer> multiPrePreparers = config.getMultiPreTargetPreparers();
        runMultiTargetPreparersTearDown(
                multiPrePreparers, context, throwable, "multi pre target preparer teardown");

        if (throwable != null) {
            throw throwable;
        }
    }

    @Override
    public void doCleanUp(IInvocationContext context, IConfiguration config, Throwable exception) {
        for (String deviceName : context.getDeviceConfigNames()) {
            List<ITargetPreparer> preparers =
                    config.getDeviceConfigByName(deviceName).getTargetPreparers();
            ListIterator<ITargetPreparer> itr = preparers.listIterator(preparers.size());
            while (itr.hasPrevious()) {
                ITargetPreparer preparer = itr.previous();
                if (preparer instanceof IHostCleaner) {
                    IHostCleaner cleaner = (IHostCleaner) preparer;
                    if (preparer.isDisabled() || preparer.isTearDownDisabled()) {
                        CLog.d("%s has been disabled. skipping.", cleaner);
                        continue;
                    }
                    cleaner.cleanUp(context.getBuildInfo(deviceName), exception);
                }
            }
        }
    }

    @Override
    public void runTests(
            IInvocationContext context, IConfiguration config, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        for (IRemoteTest test : config.getTests()) {
            // For compatibility of those receivers, they are assumed to be single device alloc.
            if (test instanceof IDeviceTest) {
                ((IDeviceTest) test).setDevice(context.getDevices().get(0));
            }
            if (test instanceof IBuildReceiver) {
                ((IBuildReceiver) test).setBuild(context.getBuildInfo(context.getDevices().get(0)));
            }
            if (test instanceof ISystemStatusCheckerReceiver) {
                ((ISystemStatusCheckerReceiver) test)
                        .setSystemStatusChecker(config.getSystemStatusCheckers());
            }

            // TODO: consider adding receivers for only the list of ITestDevice and IBuildInfo.
            if (test instanceof IMultiDeviceTest) {
                ((IMultiDeviceTest) test).setDeviceInfos(context.getDeviceBuildMap());
            }
            if (test instanceof IInvocationContextReceiver) {
                ((IInvocationContextReceiver) test).setInvocationContext(context);
            }

            // We clone the collectors for each IRemoteTest to ensure no state conflicts.
            List<IMetricCollector> clonedCollectors = cloneCollectors(config.getMetricCollectors());
            if (test instanceof IMetricCollectorReceiver) {
                ((IMetricCollectorReceiver) test).setMetricCollectors(clonedCollectors);
                // If test can receive collectors then let it handle the how to set them up
                test.run(listener);
            } else {
                // Wrap collectors in each other and collection will be sequential, do this in the
                // loop to ensure they are always initialized against the right context.
                ITestInvocationListener listenerWithCollectors = listener;
                for (IMetricCollector collector : clonedCollectors) {
                    listenerWithCollectors = collector.init(context, listenerWithCollectors);
                }
                test.run(listenerWithCollectors);
            }
        }
    }

    @Override
    public boolean resetBuildAndReschedule(
            Throwable exception,
            ITestInvocationListener listener,
            IConfiguration config,
            IInvocationContext context) {
        if (!(exception instanceof BuildError) && !(exception.getCause() instanceof BuildError)) {
            for (String deviceName : context.getDeviceConfigNames()) {
                config.getDeviceConfigByName(deviceName)
                        .getBuildProvider()
                        .buildNotTested(context.getBuildInfo(deviceName));
            }
            return true;
        }
        return false;
    }

    /**
     * Helper to clone {@link IMetricCollector}s in order for each {@link IRemoteTest} to get a
     * different instance, and avoid internal state and multi-init issues.
     */
    private List<IMetricCollector> cloneCollectors(List<IMetricCollector> originalCollectors) {
        List<IMetricCollector> cloneList = new ArrayList<>();
        for (IMetricCollector collector : originalCollectors) {
            try {
                // TF object should all have a constructore with no args, so this should be safe.
                IMetricCollector clone = collector.getClass().newInstance();
                OptionCopier.copyOptionsNoThrow(collector, clone);
                cloneList.add(clone);
            } catch (InstantiationException | IllegalAccessException e) {
                throw new RuntimeException(e);
            }
        }
        return cloneList;
    }

    private void reportLogs(ITestDevice device, ITestInvocationListener listener, Stage stage) {
        if (device == null) {
            return;
        }
        // non stub device
        if (!(device.getIDevice() instanceof StubDevice)) {
            try (InputStreamSource logcatSource = device.getLogcat()) {
                device.clearLogcat();
                String name = TestInvocation.getDeviceLogName(stage);
                listener.testLog(name, LogDataType.LOGCAT, logcatSource);
            }
        }
        // emulator logs
        if (device.getIDevice() != null && device.getIDevice().isEmulator()) {
            try (InputStreamSource emulatorOutput = device.getEmulatorOutput()) {
                // TODO: Clear the emulator log
                String name = TestInvocation.getEmulatorLogName(stage);
                listener.testLog(name, LogDataType.TEXT, emulatorOutput);
            }
        }
    }

    /**
     * Update the {@link IInvocationContext} with additional info from the {@link IConfiguration}.
     *
     * @param context the {@link IInvocationContext}
     * @param config the {@link IConfiguration}
     */
    private void updateInvocationContext(IInvocationContext context, IConfiguration config) {
        // TODO: Once reporting on context is done, only set context attributes
        if (config.getCommandLine() != null) {
            // TODO: obfuscate the password if any.
            context.addInvocationAttribute(
                    TestInvocation.COMMAND_ARGS_KEY, config.getCommandLine());
        }
        if (config.getCommandOptions().getShardCount() != null) {
            context.addInvocationAttribute(
                    "shard_count", config.getCommandOptions().getShardCount().toString());
        }
        if (config.getCommandOptions().getShardIndex() != null) {
            context.addInvocationAttribute(
                    "shard_index", config.getCommandOptions().getShardIndex().toString());
        }
        context.setTestTag(getTestTag(config));
    }

    /** Helper to create the test tag from the configuration. */
    private String getTestTag(IConfiguration config) {
        String testTag = config.getCommandOptions().getTestTag();
        if (config.getCommandOptions().getTestTagSuffix() != null) {
            testTag =
                    String.format("%s-%s", testTag, config.getCommandOptions().getTestTagSuffix());
        }
        return testTag;
    }

    /**
     * Update the {@link IBuildInfo} with additional info from the {@link IConfiguration}.
     *
     * @param info the {@link IBuildInfo}
     * @param config the {@link IConfiguration}
     */
    private void updateBuild(IBuildInfo info, IConfiguration config) {
        if (config.getCommandLine() != null) {
            // TODO: obfuscate the password if any.
            info.addBuildAttribute(TestInvocation.COMMAND_ARGS_KEY, config.getCommandLine());
        }
        if (config.getCommandOptions().getShardCount() != null) {
            info.addBuildAttribute(
                    "shard_count", config.getCommandOptions().getShardCount().toString());
        }
        if (config.getCommandOptions().getShardIndex() != null) {
            info.addBuildAttribute(
                    "shard_index", config.getCommandOptions().getShardIndex().toString());
        }
        // TODO: update all the configs to only use test-tag from CommandOption and not build
        // providers.
        // When CommandOption is set, it overrides any test-tag from build_providers
        if (!"stub".equals(config.getCommandOptions().getTestTag())) {
            info.setTestTag(getTestTag(config));
        } else if (info.getTestTag() == null || info.getTestTag().isEmpty()) {
            // We ensure that that a default test-tag is always available.
            info.setTestTag("stub");
        } else {
            CLog.w(
                    "Using the test-tag from the build_provider. Consider updating your config to"
                            + " have no alias/namespace in front of test-tag.");
        }

        if (info.getProperties().contains(BuildInfoProperties.DO_NOT_LINK_TESTS_DIR)) {
            CLog.d("Skip linking external directory as FileProperty was set.");
            return;
        }
        // Load environment tests dir.
        if (info instanceof IDeviceBuildInfo) {
            File testsDir = ((IDeviceBuildInfo) info).getTestsDir();
            if (testsDir != null && testsDir.exists()) {
                handleLinkingExternalDirs(
                        (IDeviceBuildInfo) info,
                        testsDir,
                        EnvVariable.ANDROID_TARGET_OUT_TESTCASES,
                        BuildInfoFileKey.TARGET_LINKED_DIR.getFileKey());
                handleLinkingExternalDirs(
                        (IDeviceBuildInfo) info,
                        testsDir,
                        EnvVariable.ANDROID_HOST_OUT_TESTCASES,
                        BuildInfoFileKey.HOST_LINKED_DIR.getFileKey());
            }
        }
    }

    private void handleLinkingExternalDirs(
            IDeviceBuildInfo info, File testsDir, EnvVariable var, String baseName) {
        File externalDir = getExternalTestCasesDirs(var);
        if (externalDir == null) {
            return;
        }
        try {
            // Avoid conflict by creating a randomized name for the arriving symlink file.
            File subDir = FileUtil.createTempDir(baseName, testsDir);
            subDir.delete();
            FileUtil.symlinkFile(externalDir, subDir);
            // Tag the dir in the build info to be possibly cleaned.
            info.setFile(
                    baseName,
                    subDir,
                    /** version */
                    "v1");
            // Ensure we always delete the linking, no matter how the JVM exits.
            subDir.deleteOnExit();
        } catch (IOException e) {
            CLog.e("Failed to load external test dir %s. Ignoring it.", externalDir);
            CLog.e(e);
        }
    }

    /** Returns the external directory coming from the environment. */
    @VisibleForTesting
    File getExternalTestCasesDirs(EnvVariable envVar) {
        return SystemUtil.getExternalTestCasesDir(envVar);
    }
}
