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
package com.android.tradefed.testtype;

import static org.junit.Assert.assertEquals;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner.TestMetrics;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.junit.runners.Suite.SuiteClasses;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.HashMap;
import java.util.Map;

/** Unit Tests for {@link DeviceSuite}. */
@RunWith(JUnit4.class)
public class DeviceSuiteTest {

    // We use HostTest as a runner for JUnit4 Suite
    private HostTest mHostTest;
    private ITestDevice mMockDevice;
    private ITestInvocationListener mListener;
    private IBuildInfo mMockBuildInfo;
    private IAbi mMockAbi;

    @Before
    public void setUp() throws Exception {
        mHostTest = new HostTest();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockBuildInfo = EasyMock.createMock(IBuildInfo.class);
        mMockAbi = EasyMock.createMock(IAbi.class);
        mHostTest.setDevice(mMockDevice);
        mHostTest.setBuild(mMockBuildInfo);
        mHostTest.setAbi(mMockAbi);
        OptionSetter setter = new OptionSetter(mHostTest);
        // Disable pretty logging for testing
        setter.setOptionValue("enable-pretty-logs", "false");
    }

    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class Junit4DeviceTestclass implements IDeviceTest, IAbiReceiver,
            IBuildReceiver {
        public static ITestDevice sDevice;
        public static IBuildInfo sBuildInfo;
        public static IAbi sAbi;

        @Rule public TestMetrics metrics = new TestMetrics();

        @Option(name = "option")
        private String mOption = null;

        public Junit4DeviceTestclass() {
            sDevice = null;
            sBuildInfo = null;
            sAbi = null;
        }

        @Test
        @MyAnnotation1
        public void testPass1() {
            if (mOption != null) {
                metrics.addTestMetric("option", mOption);
            }
        }

        @Test
        public void testPass2() {}

        @Override
        public void setDevice(ITestDevice device) {
            sDevice = device;
        }

        @Override
        public ITestDevice getDevice() {
            return sDevice;
        }

        @Override
        public void setBuild(IBuildInfo buildInfo) {
            sBuildInfo = buildInfo;
        }

        @Override
        public void setAbi(IAbi abi) {
            sAbi = abi;
        }
    }

    @RunWith(DeviceSuite.class)
    @SuiteClasses({
        Junit4DeviceTestclass.class,
    })
    public class Junit4DeviceSuite {
    }

    /** JUnit3 test class */
    public static class JUnit3DeviceTestCase extends DeviceTestCase
            implements IBuildReceiver, IAbiReceiver {
        private IBuildInfo mBuild;
        private IAbi mAbi;

        public void testOne() {
            assertNotNull(getDevice());
            assertNotNull(mBuild);
            assertNotNull(mAbi);
        }

        @Override
        public void setBuild(IBuildInfo buildInfo) {
            mBuild = buildInfo;
        }

        @Override
        public void setAbi(IAbi abi) {
            mAbi = abi;
        }
    }

    /** JUnit4 style suite that contains a JUnit3 class. */
    @RunWith(DeviceSuite.class)
    @SuiteClasses({
        JUnit3DeviceTestCase.class,
    })
    public class JUnit4SuiteWithJunit3 {}

    /** Simple Annotation class for testing */
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyAnnotation1 {}

    @Test
    public void testRunDeviceSuite() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4DeviceSuite.class.getName());
        mListener.testRunStarted(
                EasyMock.eq("com.android.tradefed.testtype.DeviceSuiteTest$Junit4DeviceSuite"),
                EasyMock.eq(2));
        TestDescription test1 =
                new TestDescription(Junit4DeviceTestclass.class.getName(), "testPass1");
        TestDescription test2 =
                new TestDescription(Junit4DeviceTestclass.class.getName(), "testPass2");
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), EasyMock.eq(new HashMap<String, Metric>()));
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), EasyMock.eq(new HashMap<String, Metric>()));
        mListener.testRunEnded(EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        EasyMock.replay(mListener, mMockDevice);
        mHostTest.run(mListener);
        EasyMock.verify(mListener, mMockDevice);
        // Verify that all setters were called on Test class inside suite
        assertEquals(mMockDevice, Junit4DeviceTestclass.sDevice);
        assertEquals(mMockBuildInfo, Junit4DeviceTestclass.sBuildInfo);
        assertEquals(mMockAbi, Junit4DeviceTestclass.sAbi);
    }

    /** Test the run with filtering to include only one annotation. */
    @Test
    public void testRun_withFiltering() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4DeviceSuite.class.getName());
        mHostTest.addIncludeAnnotation(
                "com.android.tradefed.testtype.DeviceSuiteTest$MyAnnotation1");
        assertEquals(1, mHostTest.countTestCases());
        mListener.testRunStarted(
                EasyMock.eq("com.android.tradefed.testtype.DeviceSuiteTest$Junit4DeviceSuite"),
                EasyMock.eq(1));
        TestDescription test1 =
                new TestDescription(Junit4DeviceTestclass.class.getName(), "testPass1");
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), EasyMock.eq(new HashMap<String, Metric>()));
        mListener.testRunEnded(EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        EasyMock.replay(mListener, mMockDevice);
        mHostTest.run(mListener);
        EasyMock.verify(mListener, mMockDevice);
    }

    /** Tests that options are piped from Suite to the sub-runners. */
    @Test
    public void testRun_withOption() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4DeviceSuite.class.getName());
        setter.setOptionValue("set-option", "option:value_test");
        mListener.testRunStarted(
                EasyMock.eq("com.android.tradefed.testtype.DeviceSuiteTest$Junit4DeviceSuite"),
                EasyMock.eq(2));
        TestDescription test1 =
                new TestDescription(Junit4DeviceTestclass.class.getName(), "testPass1");
        TestDescription test2 =
                new TestDescription(Junit4DeviceTestclass.class.getName(), "testPass2");
        mListener.testStarted(EasyMock.eq(test1));
        Map<String, String> expected = new HashMap<>();
        expected.put("option", "value_test");
        mListener.testEnded(
                EasyMock.eq(test1), EasyMock.eq(TfMetricProtoUtil.upgradeConvert(expected)));
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), EasyMock.eq(new HashMap<String, Metric>()));
        mListener.testRunEnded(EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        EasyMock.replay(mListener, mMockDevice);
        mHostTest.run(mListener);
        EasyMock.verify(mListener, mMockDevice);
    }

    /** Test that a JUnit3 class inside our JUnit4 suite can receive the usual values. */
    @Test
    public void testRunDeviceSuite_junit3() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", JUnit4SuiteWithJunit3.class.getName());
        mListener.testRunStarted(
                EasyMock.eq("com.android.tradefed.testtype.DeviceSuiteTest$JUnit4SuiteWithJunit3"),
                EasyMock.eq(1));
        TestDescription test1 =
                new TestDescription(JUnit3DeviceTestCase.class.getName(), "testOne");
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), EasyMock.eq(new HashMap<String, Metric>()));
        mListener.testRunEnded(EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        EasyMock.replay(mListener, mMockDevice);
        mHostTest.run(mListener);
        EasyMock.verify(mListener, mMockDevice);
    }
}
