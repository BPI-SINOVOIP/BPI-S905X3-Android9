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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.command.CommandOptions;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.IRescheduler;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.testtype.StubTest;
import com.android.tradefed.testtype.suite.ITestSuite;

import org.easymock.EasyMock;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentMatcher;
import org.mockito.Mockito;

import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** Unit tests for {@link StrictShardHelper}. */
@RunWith(JUnit4.class)
public class StrictShardHelperTest {

    private StrictShardHelper mHelper;
    private IConfiguration mConfig;
    private ILogSaver mMockLogSaver;
    private IInvocationContext mContext;
    private IRescheduler mRescheduler;

    @Before
    public void setUp() {
        mHelper = new StrictShardHelper();
        mConfig = new Configuration("fake_sharding_config", "desc");
        mContext = new InvocationContext();
        mContext.addDeviceBuildInfo("default", new BuildInfo());
        mRescheduler = Mockito.mock(IRescheduler.class);
        mMockLogSaver = Mockito.mock(ILogSaver.class);
        mConfig.setLogSaver(mMockLogSaver);
    }

    /** Test sharding using Tradefed internal algorithm. */
    @Test
    public void testShardConfig_internal() throws Exception {
        CommandOptions options = new CommandOptions();
        OptionSetter setter = new OptionSetter(options);
        setter.setOptionValue("disable-strict-sharding", "true");
        setter.setOptionValue("shard-count", "5");
        mConfig.setCommandOptions(options);
        mConfig.setCommandLine(new String[] {"empty"});
        StubTest test = new StubTest();
        setter = new OptionSetter(test);
        setter.setOptionValue("num-shards", "5");
        mConfig.setTest(test);
        assertEquals(1, mConfig.getTests().size());
        assertTrue(mHelper.shardConfig(mConfig, mContext, mRescheduler));
        // Ensure that we did split 1 tests per shard rescheduled.
        Mockito.verify(mRescheduler, Mockito.times(5))
                .scheduleConfig(
                        Mockito.argThat(
                                new ArgumentMatcher<IConfiguration>() {
                                    @Override
                                    public boolean matches(IConfiguration argument) {
                                        assertEquals(1, argument.getTests().size());
                                        return true;
                                    }
                                }));
    }

    /** Test sharding using Tradefed internal algorithm. */
    @Test
    public void testShardConfig_internal_shardIndex() throws Exception {
        CommandOptions options = new CommandOptions();
        OptionSetter setter = new OptionSetter(options);
        setter.setOptionValue("disable-strict-sharding", "true");
        setter.setOptionValue("shard-count", "5");
        setter.setOptionValue("shard-index", "2");
        mConfig.setCommandOptions(options);
        mConfig.setCommandLine(new String[] {"empty"});
        StubTest test = new StubTest();
        setter = new OptionSetter(test);
        setter.setOptionValue("num-shards", "5");
        mConfig.setTest(test);
        assertEquals(1, mConfig.getTests().size());
        // We do not shard, we are relying on the current invocation to run.
        assertFalse(mHelper.shardConfig(mConfig, mContext, mRescheduler));
        // Rescheduled is NOT called because we use the current invocation to run the index.
        Mockito.verify(mRescheduler, Mockito.times(0)).scheduleConfig(Mockito.any());
        assertEquals(1, mConfig.getTests().size());
        // Original IRemoteTest was replaced by the sharded one in the configuration.
        assertNotEquals(test, mConfig.getTests().get(0));
    }

    /**
     * Test sharding using Tradefed internal algorithm. On a non shardable IRemoteTest and getting
     * the shard 0.
     */
    @Test
    public void testShardConfig_internal_shardIndex_notShardable_shard0() throws Exception {
        CommandOptions options = new CommandOptions();
        OptionSetter setter = new OptionSetter(options);
        setter.setOptionValue("disable-strict-sharding", "true");
        setter.setOptionValue("shard-count", "5");
        setter.setOptionValue("shard-index", "0");
        mConfig.setCommandOptions(options);
        mConfig.setCommandLine(new String[] {"empty"});
        IRemoteTest test =
                new IRemoteTest() {
                    @Override
                    public void run(ITestInvocationListener listener)
                            throws DeviceNotAvailableException {
                        // do nothing.
                    }
                };
        mConfig.setTest(test);
        assertEquals(1, mConfig.getTests().size());
        // We do not shard, we are relying on the current invocation to run.
        assertFalse(mHelper.shardConfig(mConfig, mContext, mRescheduler));
        // Rescheduled is NOT called because we use the current invocation to run the index.
        Mockito.verify(mRescheduler, Mockito.times(0)).scheduleConfig(Mockito.any());
        assertEquals(1, mConfig.getTests().size());
        // Original IRemoteTest is the same since the test was not shardable
        assertSame(test, mConfig.getTests().get(0));
    }

    /**
     * Test sharding using Tradefed internal algorithm. On a non shardable IRemoteTest and getting
     * the shard 1.
     */
    @Test
    public void testShardConfig_internal_shardIndex_notShardable_shard1() throws Exception {
        CommandOptions options = new CommandOptions();
        OptionSetter setter = new OptionSetter(options);
        setter.setOptionValue("disable-strict-sharding", "true");
        setter.setOptionValue("shard-count", "5");
        setter.setOptionValue("shard-index", "1");
        mConfig.setCommandOptions(options);
        mConfig.setCommandLine(new String[] {"empty"});
        IRemoteTest test =
                new IRemoteTest() {
                    @Override
                    public void run(ITestInvocationListener listener)
                            throws DeviceNotAvailableException {
                        // do nothing.
                    }
                };
        mConfig.setTest(test);
        assertEquals(1, mConfig.getTests().size());
        // We do not shard, we are relying on the current invocation to run.
        assertFalse(mHelper.shardConfig(mConfig, mContext, mRescheduler));
        // Rescheduled is NOT called because we use the current invocation to run the index.
        Mockito.verify(mRescheduler, Mockito.times(0)).scheduleConfig(Mockito.any());
        // We have no tests to put in shard-index 1 so it's empty.
        assertEquals(0, mConfig.getTests().size());
    }

    /** Test class to simulate an ITestSuite getting split. */
    public static class SplitITestSuite extends ITestSuite {

        private String mName;
        private IRemoteTest mForceTest = null;

        public SplitITestSuite() {}

        public SplitITestSuite(String name) {
            mName = name;
        }

        public SplitITestSuite(String name, IRemoteTest test) {
            this(name);
            mForceTest = test;
        }

        @Override
        public LinkedHashMap<String, IConfiguration> loadTests() {
            LinkedHashMap<String, IConfiguration> configs = new LinkedHashMap<>();
            IConfiguration configuration = null;
            try {
                configuration =
                        ConfigurationFactory.getInstance()
                                .createConfigurationFromArgs(
                                        new String[] {"empty", "--num-shards", "2"});
                if (mForceTest != null) {
                    configuration.setTest(mForceTest);
                }
            } catch (ConfigurationException e) {
                throw new RuntimeException(e);
            }
            configs.put(mName, configuration);
            return configs;
        }
    }

    private ITestSuite createFakeSuite(String name) throws Exception {
        ITestSuite suite = new SplitITestSuite(name);
        return suite;
    }

    private List<IRemoteTest> testShard(int shardIndex) throws Exception {
        mContext.addAllocatedDevice("default", EasyMock.createMock(ITestDevice.class));
        List<IRemoteTest> test = new ArrayList<>();
        test.add(createFakeSuite("module2"));
        test.add(createFakeSuite("module1"));
        test.add(createFakeSuite("module3"));
        test.add(createFakeSuite("module1"));
        test.add(createFakeSuite("module1"));
        test.add(createFakeSuite("module2"));
        test.add(createFakeSuite("module3"));
        CommandOptions options = new CommandOptions();
        OptionSetter setter = new OptionSetter(options);
        setter.setOptionValue("disable-strict-sharding", "true");
        setter.setOptionValue("shard-count", "3");
        setter.setOptionValue("shard-index", Integer.toString(shardIndex));
        mConfig.setCommandOptions(options);
        mConfig.setCommandLine(new String[] {"empty"});
        mConfig.setTests(test);
        mHelper.shardConfig(mConfig, mContext, mRescheduler);
        return mConfig.getTests();
    }

    /**
     * Total for all the _shardX test should be 14 tests (2 per modules). 6 for module1: 3 module1
     * shard * 2 4 for module2: 2 module2 shard * 2 4 for module3: 2 module3 shard * 2
     */
    @Test
    public void testMergeSuite_shard0() throws Exception {
        List<IRemoteTest> res = testShard(0);
        assertEquals(3, res.size());

        assertTrue(res.get(0) instanceof ITestSuite);
        assertEquals("module3", ((ITestSuite) res.get(0)).getDirectModule().getId());
        assertEquals(2, ((ITestSuite) res.get(0)).getDirectModule().numTests());

        assertTrue(res.get(1) instanceof ITestSuite);
        assertEquals("module1", ((ITestSuite) res.get(1)).getDirectModule().getId());
        assertEquals(1, ((ITestSuite) res.get(1)).getDirectModule().numTests());

        assertTrue(res.get(2) instanceof ITestSuite);
        assertEquals("module2", ((ITestSuite) res.get(2)).getDirectModule().getId());
        assertEquals(1, ((ITestSuite) res.get(2)).getDirectModule().numTests());
    }

    @Test
    public void testMergeSuite_shard1() throws Exception {
        List<IRemoteTest> res = testShard(1);
        assertEquals(3, res.size());

        assertTrue(res.get(0) instanceof ITestSuite);
        assertEquals("module1", ((ITestSuite) res.get(0)).getDirectModule().getId());
        assertEquals(1, ((ITestSuite) res.get(0)).getDirectModule().numTests());

        assertTrue(res.get(1) instanceof ITestSuite);
        assertEquals("module3", ((ITestSuite) res.get(1)).getDirectModule().getId());
        assertEquals(1, ((ITestSuite) res.get(1)).getDirectModule().numTests());

        assertTrue(res.get(2) instanceof ITestSuite);
        assertEquals("module2", ((ITestSuite) res.get(2)).getDirectModule().getId());
        assertEquals(2, ((ITestSuite) res.get(2)).getDirectModule().numTests());
    }

    @Test
    public void testMergeSuite_shard2() throws Exception {
        List<IRemoteTest> res = testShard(2);
        assertEquals(3, res.size());

        assertTrue(res.get(0) instanceof ITestSuite);
        assertEquals("module1", ((ITestSuite) res.get(0)).getDirectModule().getId());
        assertEquals(4, ((ITestSuite) res.get(0)).getDirectModule().numTests());

        assertTrue(res.get(1) instanceof ITestSuite);
        assertEquals("module2", ((ITestSuite) res.get(1)).getDirectModule().getId());
        assertEquals(1, ((ITestSuite) res.get(1)).getDirectModule().numTests());

        assertTrue(res.get(1) instanceof ITestSuite);
        assertEquals("module3", ((ITestSuite) res.get(2)).getDirectModule().getId());
        assertEquals(1, ((ITestSuite) res.get(2)).getDirectModule().numTests());
    }

    @Test
    public void testShardSuite() throws Exception {
        //mConfig
        mHelper.shardConfig(mConfig, mContext, mRescheduler);
    }

    /**
     * Test class to ensure that when sharding interfaces are properly called and forwarded so the
     * tests have all their information for sharding.
     */
    public static class TestInterfaceClass
            implements IShardableTest, IMultiDeviceTest, IInvocationContextReceiver {

        @Override
        public void setInvocationContext(IInvocationContext invocationContext) {
            Assert.assertNotNull(invocationContext);
        }

        @Override
        public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
            Assert.assertNotNull(deviceInfos);
        }

        @Override
        public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
            // ignore
        }

        @Override
        public Collection<IRemoteTest> split(int hintShard) {
            if (hintShard > 1) {
                List<IRemoteTest> shards = new ArrayList<IRemoteTest>(hintShard);
                for (int i = 0; i < hintShard; i++) {
                    shards.add(new TestInterfaceClass());
                }
                return shards;
            }
            return null;
        }
    }

    /** Test that no exception occurs when sharding for any possible interfaces. */
    @Test
    public void testSuite_withAllInterfaces() throws Exception {
        mContext.addAllocatedDevice("default", EasyMock.createMock(ITestDevice.class));
        IRemoteTest forceTest = new TestInterfaceClass();
        IRemoteTest test = new SplitITestSuite("suite-interface", forceTest);

        CommandOptions options = new CommandOptions();
        OptionSetter setter = new OptionSetter(options);
        setter.setOptionValue("disable-strict-sharding", "true");
        setter.setOptionValue("shard-count", "3");
        setter.setOptionValue("shard-index", Integer.toString(0));
        mConfig.setCommandOptions(options);
        mConfig.setTest(test);
        mHelper.shardConfig(mConfig, mContext, mRescheduler);

        List<IRemoteTest> res = mConfig.getTests();
        assertEquals(1, res.size());

        assertTrue(res.get(0) instanceof ITestSuite);
        assertEquals("suite-interface", ((ITestSuite) res.get(0)).getDirectModule().getId());
        assertEquals(1, ((ITestSuite) res.get(0)).getDirectModule().numTests());
    }

    /** Helper for distribution tests to simply populate a list of a given count. */
    private List<IRemoteTest> createFakeTestList(int count) {
        List<IRemoteTest> testList = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            testList.add(new StubTest());
        }
        return testList;
    }

    /**
     * The distribution tests bellow expose an issue that arised with some combination of number of
     * tests and shard-count. The number of tests allocated to each shard made us use the full list
     * of tests before reaching the last shard, resulting in some OutOfBounds exception. Logic was
     * added to detect these cases and properly handle them as well as ensuring a proper balancing.
     */

    /** Test that the special ratio 130 tests for 20 shards is properly redistributed. */
    @Test
    public void testDistribution_hightests_highcount() {
        List<IRemoteTest> testList = createFakeTestList(130);
        int shardCount = 20;
        List<List<IRemoteTest>> res = mHelper.splitTests(testList, shardCount);
        assertEquals(7, res.get(0).size());
        assertEquals(7, res.get(1).size());
        assertEquals(7, res.get(2).size());
        assertEquals(7, res.get(3).size());
        assertEquals(7, res.get(4).size());
        assertEquals(7, res.get(5).size());
        assertEquals(7, res.get(6).size());
        assertEquals(7, res.get(7).size());
        assertEquals(7, res.get(8).size());
        assertEquals(7, res.get(9).size());
        assertEquals(6, res.get(10).size());
        assertEquals(6, res.get(11).size());
        assertEquals(6, res.get(12).size());
        assertEquals(6, res.get(13).size());
        assertEquals(6, res.get(14).size());
        assertEquals(6, res.get(15).size());
        assertEquals(6, res.get(16).size());
        assertEquals(6, res.get(17).size());
        assertEquals(6, res.get(18).size());
        assertEquals(6, res.get(19).size());
    }

    /** Test that the special ratio 7 tests for 6 shards is properly redistributed. */
    @Test
    public void testDistribution_lowtests_lowcount() {
        List<IRemoteTest> testList = createFakeTestList(7);
        int shardCount = 6;
        List<List<IRemoteTest>> res = mHelper.splitTests(testList, shardCount);
        assertEquals(2, res.get(0).size());
        assertEquals(1, res.get(1).size());
        assertEquals(1, res.get(2).size());
        assertEquals(1, res.get(3).size());
        assertEquals(1, res.get(4).size());
        assertEquals(1, res.get(5).size());
    }

    /** Test that the special ratio 13 tests for 6 shards is properly redistributed. */
    @Test
    public void testDistribution_lowtests_lowcount2() {
        List<IRemoteTest> testList = createFakeTestList(13);
        int shardCount = 6;
        List<List<IRemoteTest>> res = mHelper.splitTests(testList, shardCount);
        assertEquals(3, res.get(0).size());
        assertEquals(2, res.get(1).size());
        assertEquals(2, res.get(2).size());
        assertEquals(2, res.get(3).size());
        assertEquals(2, res.get(4).size());
        assertEquals(2, res.get(5).size());
    }
}
