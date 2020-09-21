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
package com.android.tradefed.invoker.shard;

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IGlobalConfiguration;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.IRescheduler;
import com.android.tradefed.invoker.ShardListener;
import com.android.tradefed.invoker.ShardMasterResultForwarder;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.IShardableListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.suite.checker.ISystemStatusChecker;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.util.QuotationAwareTokenizer;
import com.android.tradefed.util.keystore.IKeyStoreClient;
import com.android.tradefed.util.keystore.KeyStoreException;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;

/** Helper class that handles creating the shards and scheduling them for an invocation. */
public class ShardHelper implements IShardHelper {

    /**
     * List of the list configuration obj that should be clone to each shard in order to avoid state
     * issues.
     */
    private static final List<String> CONFIG_OBJ_TO_CLONE = new ArrayList<>();

    static {
        CONFIG_OBJ_TO_CLONE.add(Configuration.SYSTEM_STATUS_CHECKER_TYPE_NAME);
        CONFIG_OBJ_TO_CLONE.add(Configuration.DEVICE_METRICS_COLLECTOR_TYPE_NAME);
        CONFIG_OBJ_TO_CLONE.add(Configuration.TARGET_PREPARER_TYPE_NAME);
        CONFIG_OBJ_TO_CLONE.add(Configuration.MULTI_PREPARER_TYPE_NAME);
    }

    /**
     * Attempt to shard the configuration into sub-configurations, to be re-scheduled to run on
     * multiple resources in parallel.
     *
     * <p>A successful shard action renders the current config empty, and invocation should not
     * proceed.
     *
     * @see IShardableTest
     * @see IRescheduler
     * @param config the current {@link IConfiguration}.
     * @param context the {@link IInvocationContext} holding the tests information.
     * @param rescheduler the {@link IRescheduler}
     * @return true if test was sharded. Otherwise return <code>false</code>
     */
    @Override
    public boolean shardConfig(
            IConfiguration config, IInvocationContext context, IRescheduler rescheduler) {
        List<IRemoteTest> shardableTests = new ArrayList<IRemoteTest>();
        boolean isSharded = false;
        Integer shardCount = config.getCommandOptions().getShardCount();
        for (IRemoteTest test : config.getTests()) {
            isSharded |= shardTest(shardableTests, test, shardCount, context);
        }
        if (!isSharded) {
            return false;
        }
        // shard this invocation!
        // create the TestInvocationListener that will collect results from all the shards,
        // and forward them to the original set of listeners (minus any ISharddableListeners)
        // once all shards complete
        int expectedShard = shardableTests.size();
        if (shardCount != null) {
            expectedShard = Math.min(shardCount, shardableTests.size());
        }
        ShardMasterResultForwarder resultCollector =
                new ShardMasterResultForwarder(buildMasterShardListeners(config), expectedShard);

        resultCollector.invocationStarted(context);
        synchronized (shardableTests) {
            // When shardCount is available only create 1 poller per shard
            // TODO: consider aggregating both case by picking a predefined shardCount if not
            // available (like 4) for autosharding.
            if (shardCount != null) {
                // We shuffle the tests for best results: avoid having the same module sub-tests
                // contiguously in the list.
                Collections.shuffle(shardableTests);
                int maxShard = Math.min(shardCount, shardableTests.size());
                CountDownLatch tracker = new CountDownLatch(maxShard);
                for (int i = 0; i < maxShard; i++) {
                    IConfiguration shardConfig = config.clone();
                    shardConfig.setTest(new TestsPoolPoller(shardableTests, tracker));
                    rescheduleConfig(shardConfig, config, context, rescheduler, resultCollector);
                }
            } else {
                CountDownLatch tracker = new CountDownLatch(shardableTests.size());
                for (IRemoteTest testShard : shardableTests) {
                    CLog.i("Rescheduling sharded config...");
                    IConfiguration shardConfig = config.clone();
                    if (config.getCommandOptions().shouldUseDynamicSharding()) {
                        shardConfig.setTest(new TestsPoolPoller(shardableTests, tracker));
                    } else {
                        shardConfig.setTest(testShard);
                    }
                    rescheduleConfig(shardConfig, config, context, rescheduler, resultCollector);
                }
            }
        }
        // clean up original builds
        for (String deviceName : context.getDeviceConfigNames()) {
            config.getDeviceConfigByName(deviceName)
                    .getBuildProvider()
                    .cleanUp(context.getBuildInfo(deviceName));
        }
        return true;
    }

    public void rescheduleConfig(
            IConfiguration shardConfig,
            IConfiguration config,
            IInvocationContext context,
            IRescheduler rescheduler,
            ShardMasterResultForwarder resultCollector) {
        cloneConfigObject(config, shardConfig);
        ShardBuildCloner.cloneBuildInfos(config, shardConfig, context);

        shardConfig.setTestInvocationListeners(
                buildShardListeners(resultCollector, config.getTestInvocationListeners()));
        shardConfig.setLogOutput(config.getLogOutput().clone());
        shardConfig.setCommandOptions(config.getCommandOptions().clone());
        // use the same {@link ITargetPreparer}, {@link IDeviceRecovery} etc as original config
        rescheduler.scheduleConfig(shardConfig);
    }

    /** Returns the current global configuration. */
    @VisibleForTesting
    protected IGlobalConfiguration getGlobalConfiguration() {
        return GlobalConfiguration.getInstance();
    }

    /**
     * Helper to clone {@link ISystemStatusChecker}s from the original config to the clonedConfig.
     */
    private void cloneConfigObject(IConfiguration oriConfig, IConfiguration clonedConfig) {
        IKeyStoreClient client = null;
        try {
            client = getGlobalConfiguration().getKeyStoreFactory().createKeyStoreClient();
        } catch (KeyStoreException e) {
            throw new RuntimeException(
                    String.format(
                            "failed to load keystore client when sharding: %s", e.getMessage()),
                    e);
        }
        try {
            IConfiguration deepCopy =
                    ConfigurationFactory.getInstance()
                            .createConfigurationFromArgs(
                                    QuotationAwareTokenizer.tokenizeLine(
                                            oriConfig.getCommandLine()),
                                    null,
                                    client);
            for (String objType : CONFIG_OBJ_TO_CLONE) {
                clonedConfig.setConfigurationObjectList(
                        objType, deepCopy.getConfigurationObjectList(objType));
            }
        } catch (ConfigurationException e) {
            // should not happen
            throw new RuntimeException(
                    String.format("failed to deep copy a configuration: %s", e.getMessage()), e);
        }
    }

    /**
     * Attempt to shard given {@link IRemoteTest}.
     *
     * @param shardableTests the list of {@link IRemoteTest}s to add to
     * @param test the {@link IRemoteTest} to shard
     * @param shardCount attempted number of shard, can be null.
     * @param context the {@link IInvocationContext} of the current invocation.
     * @return <code>true</code> if test was sharded
     */
    private static boolean shardTest(
            List<IRemoteTest> shardableTests,
            IRemoteTest test,
            Integer shardCount,
            IInvocationContext context) {
        boolean isSharded = false;
        if (test instanceof IShardableTest) {
            // inject device and build since they might be required to shard.
            if (test instanceof IBuildReceiver) {
                ((IBuildReceiver) test).setBuild(context.getBuildInfos().get(0));
            }
            if (test instanceof IDeviceTest) {
                ((IDeviceTest) test).setDevice(context.getDevices().get(0));
            }
            if (test instanceof IMultiDeviceTest) {
                ((IMultiDeviceTest) test).setDeviceInfos(context.getDeviceBuildMap());
            }
            if (test instanceof IInvocationContextReceiver) {
                ((IInvocationContextReceiver) test).setInvocationContext(context);
            }

            IShardableTest shardableTest = (IShardableTest) test;
            Collection<IRemoteTest> shards = null;
            // Give the shardCount hint to tests if they need it.
            if (shardCount != null) {
                shards = shardableTest.split(shardCount);
            } else {
                shards = shardableTest.split();
            }
            if (shards != null) {
                shardableTests.addAll(shards);
                isSharded = true;
            }
        }
        if (!isSharded) {
            shardableTests.add(test);
        }
        return isSharded;
    }

    /**
     * Builds the {@link ITestInvocationListener} listeners that will collect the results from all
     * shards. Currently excludes {@link IShardableListener}s.
     */
    private static List<ITestInvocationListener> buildMasterShardListeners(IConfiguration config) {
        List<ITestInvocationListener> newListeners = new ArrayList<ITestInvocationListener>();
        for (ITestInvocationListener l : config.getTestInvocationListeners()) {
            if (!(l instanceof IShardableListener)) {
                newListeners.add(l);
            }
        }
        return newListeners;
    }

    /**
     * Builds the list of {@link ITestInvocationListener}s for each shard. Currently includes any
     * {@link IShardableListener}, plus a single listener that will forward results to the master
     * shard collector.
     */
    private static List<ITestInvocationListener> buildShardListeners(
            ITestInvocationListener resultCollector, List<ITestInvocationListener> origListeners) {
        List<ITestInvocationListener> shardListeners = new ArrayList<ITestInvocationListener>();
        for (ITestInvocationListener l : origListeners) {
            if (l instanceof IShardableListener) {
                shardListeners.add(((IShardableListener) l).clone());
            }
        }
        ShardListener origConfigListener = new ShardListener(resultCollector);
        shardListeners.add(origConfigListener);
        return shardListeners;
    }

}
