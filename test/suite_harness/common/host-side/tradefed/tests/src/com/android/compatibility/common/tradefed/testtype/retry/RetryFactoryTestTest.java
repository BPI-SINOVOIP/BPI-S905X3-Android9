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
package com.android.compatibility.common.tradefed.testtype.retry;

import static org.junit.Assert.assertEquals;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.tradefed.testtype.suite.CompatibilityTestSuite;
import com.android.compatibility.common.tradefed.util.RetryFilterHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.ConfigurationDef;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.suite.checker.ISystemStatusChecker;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.StubTest;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;

/**
 * Unit tests for {@link RetryFactoryTest}.
 */
@RunWith(JUnit4.class)
public class RetryFactoryTestTest {

    private RetryFactoryTest mFactory;
    private ITestInvocationListener mMockListener;
    private RetryFilterHelper mSpyFilter;

    private List<ISystemStatusChecker> mCheckers;
    private IBuildInfo mMockInfo;
    private ITestDevice mMockDevice;
    private IConfiguration mMockMainConfiguration;
    private IInvocationContext mMockContext;

    /**
     * A {@link CompatibilityTestSuite} that does not run anything.
     */
    @OptionClass(alias = "compatibility")
    public static class VoidCompatibilityTest extends CompatibilityTestSuite {
        @Override
        public LinkedHashMap<String, IConfiguration> loadTests() {
            return new LinkedHashMap<>();
        }

        @Override
        public Collection<IRemoteTest> split(int shardCountHint) {
            List<IRemoteTest> tests = new ArrayList<>();
            for (int i = 0; i < shardCountHint; i++) {
                tests.add(new StubTest());
            }
            return tests;
        }
    }

    @OptionClass(alias = "compatibility")
    public static class TestCompatibilityTestSuite extends CompatibilityTestSuite {
        @Override
        public LinkedHashMap<String, IConfiguration> loadTests() {
            LinkedHashMap<String, IConfiguration> tests = new LinkedHashMap<>();
            IConfiguration config = new Configuration("test", "test");
            config.setTest(new StubTest());
            tests.put("module1", config);
            return tests;
        }
    }

    @Before
    public void setUp() {
        mMockMainConfiguration = new Configuration("mockMain", "mockMain");
        mCheckers = new ArrayList<>();
        mMockInfo = EasyMock.createMock(IDeviceBuildInfo.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockContext = new InvocationContext();
        mMockContext.addAllocatedDevice(ConfigurationDef.DEFAULT_DEVICE_NAME, mMockDevice);
        mMockContext.addDeviceBuildInfo(ConfigurationDef.DEFAULT_DEVICE_NAME, mMockInfo);

        mSpyFilter = new RetryFilterHelper() {
            @Override
            public void validateBuildFingerprint(ITestDevice device)
                    throws DeviceNotAvailableException {
                // do nothing
            }
            @Override
            public void setCommandLineOptionsFor(Object obj) {
                // do nothing
            }
            @Override
            public void populateFiltersBySubPlan() {
                // do nothing
            }
        };
        mFactory = new RetryFactoryTest() {
            @Override
            protected RetryFilterHelper createFilterHelper(CompatibilityBuildHelper buildHelper) {
                return mSpyFilter;
            }
            @Override
            CompatibilityTestSuite createTest() {
                return new VoidCompatibilityTest();
            }
        };
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
    }

    /**
     * Tests that the CompatibilityTest created can receive all the options without throwing.
     */
    @Test
    public void testRetry_receiveOption() throws Exception {
        OptionSetter setter = new OptionSetter(mFactory);
        setter.setOptionValue("retry", "10599");
        setter.setOptionValue("test-arg", "abcd");
        EasyMock.replay(mMockListener);
        mFactory.run(mMockListener);
        EasyMock.verify(mMockListener);
    }

    /**
     * Assert that the {@link RetryFactoryTest#split(int)} calls the
     * {@link CompatibilityTestSuite#split(int)} after applying all the filters.
     */
    @Test
    public void testRetry_split() throws Exception {
        EasyMock.replay(mMockListener);
        Collection<IRemoteTest> res = mFactory.split(2);
        assertEquals(2, res.size());
        EasyMock.verify(mMockListener);
    }

    /**
     * This test is meant to validate more end-to-end that the retry can create the runner, and
     * running it works properly for the main use case.
     */
    @Test
    public void testValidation() throws Exception {
        mFactory = new RetryFactoryTest() {
            @Override
            protected RetryFilterHelper createFilterHelper(CompatibilityBuildHelper buildHelper) {
                return mSpyFilter;
            }
            @Override
            CompatibilityTestSuite createTest() {
                return new TestCompatibilityTestSuite();
            }
        };
        mFactory.setBuild(mMockInfo);
        mFactory.setDevice(mMockDevice);
        mFactory.setSystemStatusChecker(mCheckers);
        mFactory.setConfiguration(mMockMainConfiguration);
        mFactory.setInvocationContext(mMockContext);

        mMockListener.testModuleStarted(EasyMock.anyObject());
        mMockListener.testRunStarted("module1", 0);
        mMockListener.testRunEnded(EasyMock.anyLong(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testModuleEnded();

        EasyMock.replay(mMockListener, mMockInfo, mMockDevice);
        mFactory.run(mMockListener);
        EasyMock.verify(mMockListener, mMockInfo, mMockDevice);
    }
}
