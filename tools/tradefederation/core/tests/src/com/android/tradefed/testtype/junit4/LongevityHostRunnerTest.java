/*
 * Copyright (C) 2018 The Android Open Source Project
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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.HostTest;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.ISetOptionReceiver;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.JUnitCore;
import org.junit.runner.Result;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.junit.runners.Suite.SuiteClasses;

import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link LongevityHostRunner}. */
@RunWith(JUnit4.class)
public class LongevityHostRunnerTest {
    private static final String OPTION_NAME = "arbitraryOptionName";
    private static final int SET_OPTION_VALUE = 1;
    private static final int UNSET_OPTION_VALUE = 0;

    private LongevityHostRunner mHostRunner;
    private SetClassHostTest mHostTest;

    private IAbi mMockAbi;
    private IBuildInfo mMockBuildInfo;
    private IInvocationContext mMockContext;
    private ITestDevice mMockDevice;
    private ITestInvocationListener mMockListener;

    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class PassingTest {
        @Test
        public void testTrueIsTrue() {
            Assert.assertTrue(true);
        }
    }

    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class OptionsSetTest {
        @Option(name = OPTION_NAME)
        private int mOptionValue = UNSET_OPTION_VALUE;

        @Test
        public void testHasOptionSet() {
            Assert.assertEquals(mOptionValue, SET_OPTION_VALUE);
        }
    }

    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class DeviceFeaturesTest
            implements IDeviceTest,
                    IBuildReceiver,
                    IAbiReceiver,
                    ISetOptionReceiver,
                    IMultiDeviceTest,
                    IInvocationContextReceiver {
        private IAbi mAbi;
        private IBuildInfo mBuildInfo;
        private IInvocationContext mContext;
        private ITestDevice mDevice;
        private Map<ITestDevice, IBuildInfo> mDeviceInfos;

        @Test
        public void testHasFeatures() {
            Assert.assertNotNull(mAbi);
            Assert.assertNotNull(mBuildInfo);
            Assert.assertNotNull(mContext);
            Assert.assertNotNull(mDevice);
            Assert.assertNotNull(mDeviceInfos);
        }

        @Override
        public void setDevice(ITestDevice device) {
            mDevice = device;
        }

        @Override
        public ITestDevice getDevice() {
            return mDevice;
        }

        @Override
        public void setAbi(IAbi abi) {
            mAbi = abi;
        }

        @Override
        public IAbi getAbi() {
            return mAbi;
        }

        @Override
        public void setBuild(IBuildInfo buildInfo) {
            mBuildInfo = buildInfo;
        }

        @Override
        public void setInvocationContext(IInvocationContext invocationContext) {
            mContext = invocationContext;
        }

        @Override
        public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
            mDeviceInfos = deviceInfos;
        }
    }

    /** A suite composed of the {@link PassingTest}. */
    @RunWith(LongevityHostRunner.class)
    @SuiteClasses({
        PassingTest.class,
    })
    public static class PassingLongevitySuite {}

    /**
     * A suite composed of the {@link DeviceFeaturesTest}.
     *
     * <p>Note: This does not annotate a runner, but constructs it in-test.
     */
    @SuiteClasses({
        DeviceFeaturesTest.class,
    })
    public static class FeaturesLongevitySuite {}

    /**
     * A suite composed of the {@link OptionsSetTest}.
     *
     * <p>Note: This does not annotate a runner, but constructs it in-test.
     */
    @SuiteClasses({
        OptionsSetTest.class,
    })
    public static class OptionsLongevitySuite {}

    @Before
    public void setUpMocks() throws Exception {
        // Setup mocks
        mMockAbi = mock(IAbi.class);
        mMockBuildInfo = mock(IBuildInfo.class);
        mMockContext = mock(IInvocationContext.class);
        mMockDevice = mock(ITestDevice.class);
        mMockListener = mock(ITestInvocationListener.class);
    }

    public void setUpRunner(Class<?> suiteClass) throws Exception {
        mHostRunner = new LongevityHostRunner(suiteClass);
        mHostRunner.setAbi(mMockAbi);
        mHostRunner.setBuild(mMockBuildInfo);
        mHostRunner.setDevice(mMockDevice);
        mHostRunner.setDeviceInfos(new HashMap<>());
        mHostRunner.setInvocationContext(mMockContext);
    }

    /** Test that we are able to run sub-tests with features set. */
    @Test
    public void testPassingFeatures() throws Exception {
        setUpRunner(FeaturesLongevitySuite.class);
        Result result = new JUnitCore().run(mHostRunner);
        Assert.assertEquals(result.getRunCount(), 1);
        Assert.assertTrue(result.wasSuccessful());
    }

    /** Test that iterations are respected by the runner. */
    @Test
    public void testIterationsRespected() throws Exception {
        setUpRunner(FeaturesLongevitySuite.class);
        OptionSetter setter = new OptionSetter(mHostRunner);
        setter.setOptionValue(LongevityHostRunner.ITERATIONS_OPTION, "10");
        Result result = new JUnitCore().run(mHostRunner);
        Assert.assertEquals(result.getRunCount(), 10);
        Assert.assertTrue(result.wasSuccessful());
    }

    /** Test that we are able to run sub-tests with options set. */
    @Test
    public void testPassingOptions() throws Exception {
        setUpRunner(OptionsLongevitySuite.class);
        OptionSetter setter = new OptionSetter(mHostRunner);
        setter.setOptionValue("set-option", OPTION_NAME + ":" + SET_OPTION_VALUE);
        Result result = new JUnitCore().run(mHostRunner);
        Assert.assertEquals(result.getRunCount(), 1);
        Assert.assertTrue(result.wasSuccessful());
    }

    /** Test that we are able to run this test from within a host test like in config XML. */
    @Test
    public void testConfigXmlSetup() throws Exception {
        mHostTest = new SetClassHostTest();
        OptionSetter setter = new OptionSetter(mHostTest);
        // Disable pretty logging for testing
        setter.setOptionValue("enable-pretty-logs", "false");
        mHostTest.setDevice(mMockDevice);
        mHostTest.setBuild(mMockBuildInfo);
        mHostTest.publicSetClassName(PassingLongevitySuite.class.getName());
        mHostTest.run(mMockListener);
        // Verify nothing failed, but something passed.
        verify(mMockListener, never()).testFailed(any(), any());
        verify(mMockListener).testEnded(any(), (HashMap<String, Metric>) any());
    }

    public class SetClassHostTest extends HostTest {
        public void publicSetClassName(String name) {
            setClassName(name);
        }
    }
}
