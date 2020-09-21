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
package com.android.tradefed.testtype.suite;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;

import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.ConfigurationDef;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.targetprep.StubTargetPreparer;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.StubTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.LinkedHashMap;
import java.util.List;

/** Unit tests for {@link ModuleSplitter}. */
@RunWith(JUnit4.class)
public class ModuleSplitterTest {

    private static final String DEFAULT_DEVICE = ConfigurationDef.DEFAULT_DEVICE_NAME;

    /**
     * Tests that {@link ModuleSplitter#splitConfiguration(LinkedHashMap, int, boolean)} on a non
     * shardable configuration results in a matching ModuleDefinition to be created with the same
     * objects.
     */
    @Test
    public void testSplitModule_configNotShardable() throws Exception {
        LinkedHashMap<String, IConfiguration> runConfig = new LinkedHashMap<>();

        IConfiguration config = new Configuration("fakeConfig", "desc");
        config.setTargetPreparer(new StubTargetPreparer());
        StubTest test = new StubTest();
        OptionSetter setterTest = new OptionSetter(test);
        // allow StubTest to shard in 6 sub tests
        setterTest.setOptionValue("num-shards", "6");
        config.setTest(test);

        OptionSetter setter = new OptionSetter(config.getConfigurationDescription());
        setter.setOptionValue("not-shardable", "true");
        runConfig.put("module1", config);
        List<ModuleDefinition> res = ModuleSplitter.splitConfiguration(runConfig, 5, true);
        // matching 1 for 1, config to ModuleDefinition since not shardable
        assertEquals(1, res.size());
        // The original target preparer is changed since we split multiple <test> tags.
        assertNotSame(
                config.getTargetPreparers().get(0),
                res.get(0).getTargetPreparerForDevice(DEFAULT_DEVICE).get(0));
        // The original IRemoteTest is still there because we use a pool.
        assertSame(config.getTests().get(0), res.get(0).getTests().get(0));
    }

    /**
     * Tests that {@link ModuleSplitter#splitConfiguration(LinkedHashMap, int, boolean)} on a non
     * strict shardable configuration and a dynamic context results in a matching ModuleDefinitions
     * to be created for each shards since they will be sharded.
     */
    @Test
    public void testSplitModule_configNotStrictShardable_dynamic() throws Exception {
        LinkedHashMap<String, IConfiguration> runConfig = new LinkedHashMap<>();

        IConfiguration config = new Configuration("fakeConfig", "desc");
        config.setTargetPreparer(new StubTargetPreparer());
        StubTest test = new StubTest();
        OptionSetter setterTest = new OptionSetter(test);
        // allow StubTest to shard in 6 sub tests
        setterTest.setOptionValue("num-shards", "6");
        config.setTest(test);

        OptionSetter setter = new OptionSetter(config.getConfigurationDescription());
        setter.setOptionValue("not-strict-shardable", "true");
        runConfig.put("module1", config);
        List<ModuleDefinition> res = ModuleSplitter.splitConfiguration(runConfig, 5, true);
        // We are sharding since even if we are not-strict-shardable, we are in dynamic context
        assertEquals(10, res.size());
        // The original target preparer is changed since we split multiple <test> tags.
        assertNotSame(
                config.getTargetPreparers().get(0),
                res.get(0).getTargetPreparerForDevice(DEFAULT_DEVICE).get(0));
        // The original IRemoteTest is changed since it was sharded
        assertNotSame(config.getTests().get(0), res.get(0).getTests().get(0));
    }

    /**
     * Tests that {@link ModuleSplitter#splitConfiguration(LinkedHashMap, int, boolean)} on a non
     * strict shardable configuration and not dymaic results in a matching ModuleDefinition to be
     * created with the same objects, since we will not be sharding.
     */
    @Test
    public void testSplitModule_configNotStrictShardable_notDynamic() throws Exception {
        LinkedHashMap<String, IConfiguration> runConfig = new LinkedHashMap<>();

        IConfiguration config = new Configuration("fakeConfig", "desc");
        config.setTargetPreparer(new StubTargetPreparer());
        StubTest test = new StubTest();
        OptionSetter setterTest = new OptionSetter(test);
        // allow StubTest to shard in 6 sub tests
        setterTest.setOptionValue("num-shards", "6");
        config.setTest(test);

        OptionSetter setter = new OptionSetter(config.getConfigurationDescription());
        setter.setOptionValue("not-strict-shardable", "true");
        runConfig.put("module1", config);
        List<ModuleDefinition> res = ModuleSplitter.splitConfiguration(runConfig, 5, false);
        // matching 1 for 1, config to ModuleDefinition since not shardable
        assertEquals(1, res.size());
        // The original target preparer is changed since we split multiple <test> tags.
        assertNotSame(
                config.getTargetPreparers().get(0),
                res.get(0).getTargetPreparerForDevice(DEFAULT_DEVICE).get(0));
        // The original IRemoteTest is still there because we use a pool.
        assertSame(config.getTests().get(0), res.get(0).getTests().get(0));
    }

    /**
     * Tests that {@link ModuleSplitter#splitConfiguration(LinkedHashMap, int, boolean)} on a non
     * shardable test results in a matching ModuleDefinition created with the original IRemoteTest
     * and copied ITargetPreparers.
     */
    @Test
    public void testSplitModule_testNotShardable() throws Exception {
        LinkedHashMap<String, IConfiguration> runConfig = new LinkedHashMap<>();

        IConfiguration config = new Configuration("fakeConfig", "desc");
        config.setTest(
                new IRemoteTest() {
                    @Override
                    public void run(ITestInvocationListener listener)
                            throws DeviceNotAvailableException {
                        // do nothing.
                    }
                });
        config.setTargetPreparer(new StubTargetPreparer());

        runConfig.put("module1", config);
        List<ModuleDefinition> res = ModuleSplitter.splitConfiguration(runConfig, 5, true);
        // matching 1 for 1, config to ModuleDefinition since not shardable
        assertEquals(1, res.size());
        // The original target preparer is not there, it has been copied
        assertNotSame(
                config.getTargetPreparers().get(0),
                res.get(0).getTargetPreparerForDevice(DEFAULT_DEVICE).get(0));
        assertEquals(
                config.getTargetPreparers().get(0).getClass(),
                res.get(0).getTargetPreparerForDevice(DEFAULT_DEVICE).get(0).getClass());
        // The original IRemoteTest is still there
        assertSame(config.getTests().get(0), res.get(0).getTests().get(0));
    }

    /**
     * Tests that {@link ModuleSplitter#splitConfiguration(LinkedHashMap, int, boolean)} on a
     * shardable test but did not shard (for any reasons) results in a ModuleDefition with original
     * IRemoteTest and copied ITargetPreparers.
     */
    @Test
    public void testSplitModule_testShardable_didNotShard() throws Exception {
        LinkedHashMap<String, IConfiguration> runConfig = new LinkedHashMap<>();

        IConfiguration config = new Configuration("fakeConfig", "desc");
        config.setTest(new StubTest());
        config.setTargetPreparer(new StubTargetPreparer());

        runConfig.put("module1", config);
        List<ModuleDefinition> res = ModuleSplitter.splitConfiguration(runConfig, 5, true);
        // matching 1 for 1, config to ModuleDefinition since did not shard
        assertEquals(1, res.size());
        // The original target preparer is not there, it has been copied
        assertNotSame(
                config.getTargetPreparers().get(0),
                res.get(0).getTargetPreparerForDevice(DEFAULT_DEVICE).get(0));
        assertEquals(
                config.getTargetPreparers().get(0).getClass(),
                res.get(0).getTargetPreparerForDevice(DEFAULT_DEVICE).get(0).getClass());
        // The original IRemoteTest is still there
        assertSame(config.getTests().get(0), res.get(0).getTests().get(0));
    }

    /**
     * Tests that {@link ModuleSplitter#splitConfiguration(LinkedHashMap, int, boolean)} on a
     * shardable test that shards results in one ModuleDefinition per shard-count.
     */
    @Test
    public void testSplitModule_testShardable_shard() throws Exception {
        LinkedHashMap<String, IConfiguration> runConfig = new LinkedHashMap<>();

        IConfiguration config = new Configuration("fakeConfig", "desc");
        StubTest test = new StubTest();
        OptionSetter setter = new OptionSetter(test);
        // allow StubTest to shard in 6 sub tests
        setter.setOptionValue("num-shards", "6");
        config.setTest(test);

        runConfig.put("module1", config);
        List<ModuleDefinition> res = ModuleSplitter.splitConfiguration(runConfig, 5, true);
        // matching 1 for 10 since tests sharding in 5 units times 2.
        assertEquals(10, res.size());
        // The original IRemoteTest does not exists anymore, new IRemoteTests have been created.
        for (ModuleDefinition m : res) {
            assertNotSame(config.getTests().get(0), m.getTests().get(0));
        }
    }

    /**
     * Tests that {@link ModuleSplitter#splitConfiguration(LinkedHashMap, int, boolean)} on a
     * shardable test that shards results in one ModuleDefinition per IRemoteTest generated by the
     * test because we did not allow dynamically putting tests in a pool for all modules.
     */
    @Test
    public void testSplitModule_testShardable_shard_noDynamic() throws Exception {
        LinkedHashMap<String, IConfiguration> runConfig = new LinkedHashMap<>();

        IConfiguration config = new Configuration("fakeConfig", "desc");
        StubTest test = new StubTest();
        OptionSetter setter = new OptionSetter(test);
        // allow StubTest to shard in 6 sub tests
        setter.setOptionValue("num-shards", "6");
        config.setTest(test);

        runConfig.put("module1", config);
        List<ModuleDefinition> res = ModuleSplitter.splitConfiguration(runConfig, 5, false);
        // matching 1 for 6 since tests sharding in 6 tests.
        assertEquals(6, res.size());
        // The original IRemoteTest does not exists anymore, new IRemoteTests have been created.
        for (ModuleDefinition m : res) {
            assertNotSame(config.getTests().get(0), m.getTests().get(0));
        }
    }
}
