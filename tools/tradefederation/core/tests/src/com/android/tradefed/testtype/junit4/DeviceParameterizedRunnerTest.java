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

import static org.junit.Assert.assertNotNull;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.HostTest;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import junitparams.Parameters;

/** Unit tests for {@link DeviceParameterizedRunner}. */
@RunWith(JUnit4.class)
public class DeviceParameterizedRunnerTest {

    /** Test class that uses the parameterized runner. */
    @RunWith(DeviceParameterizedRunner.class)
    public static class TestJUnitParamsClass extends BaseHostJUnit4Test {

        /** Not all test cases have to be parameterized. */
        @Test
        public void testOne() {
            assertNotNull(getDevice());
            assertNotNull(getBuild());
        }

        @Parameters(method = "getParams")
        @Test
        public void testTwo(String param) {
            assertNotNull(param);
            assertNotNull(getDevice());
            assertNotNull(getBuild());
        }

        public List<String> getParams() {
            return Arrays.asList("param1", "param2");
        }
    }

    @RunWith(DeviceParameterizedRunner.class)
    public static class TestJUnitParamsClassWithIgnored extends BaseHostJUnit4Test {

        /** Ignored parameterized method. */
        @Ignore
        @Parameters(method = "getParams")
        @Test
        public void testTwo(String param) {
            assertNotNull(param);
            assertNotNull(getDevice());
            assertNotNull(getBuild());
        }

        public List<String> getParams() {
            return Arrays.asList("param1", "param2");
        }
    }

    private HostTest mTest;
    private ITestDevice mDevice;
    private IBuildInfo mBuild;
    private ITestInvocationListener mListener;

    @Before
    public void setUp() throws Exception {
        mDevice = EasyMock.createMock(ITestDevice.class);
        mBuild = EasyMock.createMock(IBuildInfo.class);
        mListener = EasyMock.createMock(ITestInvocationListener.class);
        mTest = new HostTest();
        mTest.setDevice(mDevice);
        mTest.setBuild(mBuild);
        OptionSetter setter = new OptionSetter(mTest);
        // Disable pretty logging for testing
        setter.setOptionValue("enable-pretty-logs", "false");
    }

    /** Test running the parameterized tests. */
    @Test
    public void testRun() throws Exception {
        OptionSetter setter = new OptionSetter(mTest);
        setter.setOptionValue("class", TestJUnitParamsClass.class.getName());

        mListener.testRunStarted(TestJUnitParamsClass.class.getName(), 3);
        TestDescription test1 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testOne");
        mListener.testStarted(test1);
        mListener.testEnded(test1, new HashMap<String, Metric>());

        TestDescription test2_p1 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testTwo[0]");
        mListener.testStarted(test2_p1);
        mListener.testEnded(test2_p1, new HashMap<String, Metric>());

        TestDescription test2_2 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testTwo[1]");
        mListener.testStarted(test2_2);
        mListener.testEnded(test2_2, new HashMap<String, Metric>());

        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mDevice, mBuild, mListener);
        mTest.run(mListener);
        EasyMock.verify(mDevice, mBuild, mListener);
    }

    /** Test running the parameterized tests in collect-tests-only mode. */
    @Test
    public void testRun_collectOnly() throws Exception {
        OptionSetter setter = new OptionSetter(mTest);
        setter.setOptionValue("class", TestJUnitParamsClass.class.getName());
        mTest.setCollectTestsOnly(true);

        mListener.testRunStarted(TestJUnitParamsClass.class.getName(), 3);
        TestDescription test1 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testOne");
        mListener.testStarted(test1);
        mListener.testEnded(test1, new HashMap<String, Metric>());

        TestDescription test2_p1 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testTwo[0]");
        mListener.testStarted(test2_p1);
        mListener.testEnded(test2_p1, new HashMap<String, Metric>());

        TestDescription test2_2 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testTwo[1]");
        mListener.testStarted(test2_2);
        mListener.testEnded(test2_2, new HashMap<String, Metric>());

        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mDevice, mBuild, mListener);
        mTest.run(mListener);
        EasyMock.verify(mDevice, mBuild, mListener);
    }

    /** Test running the parameterized tests when targetting a particular parameterized method. */
    @Test
    public void testRun_method() throws Exception {
        OptionSetter setter = new OptionSetter(mTest);
        setter.setOptionValue("class", TestJUnitParamsClass.class.getName());
        setter.setOptionValue("method", "testTwo[0]");

        mListener.testRunStarted(TestJUnitParamsClass.class.getName(), 1);

        TestDescription test2_p1 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testTwo[0]");
        mListener.testStarted(test2_p1);
        mListener.testEnded(test2_p1, new HashMap<String, Metric>());

        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mDevice, mBuild, mListener);
        mTest.run(mListener);
        EasyMock.verify(mDevice, mBuild, mListener);
    }

    /** Test running the parameterized tests when targetting a particular parameterized method. */
    @Test
    public void testRun_method_collectOnly() throws Exception {
        OptionSetter setter = new OptionSetter(mTest);
        setter.setOptionValue("class", TestJUnitParamsClass.class.getName());
        setter.setOptionValue("method", "testTwo[0]");
        mTest.setCollectTestsOnly(true);

        mListener.testRunStarted(TestJUnitParamsClass.class.getName(), 1);

        TestDescription test2_p1 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testTwo[0]");
        mListener.testStarted(test2_p1);
        mListener.testEnded(test2_p1, new HashMap<String, Metric>());

        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mDevice, mBuild, mListener);
        mTest.run(mListener);
        EasyMock.verify(mDevice, mBuild, mListener);
    }

    /** Test running the parameterized tests when excluding a particular parameterized method. */
    @Test
    public void testRun_excludeFilter_paramMethod() throws Exception {
        OptionSetter setter = new OptionSetter(mTest);
        setter.setOptionValue("class", TestJUnitParamsClass.class.getName());
        mTest.addExcludeFilter(TestJUnitParamsClass.class.getName() + "#testTwo[0]");

        mListener.testRunStarted(TestJUnitParamsClass.class.getName(), 2);
        TestDescription test1 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testOne");
        mListener.testStarted(test1);
        mListener.testEnded(test1, new HashMap<String, Metric>());

        TestDescription test2_2 =
                new TestDescription(TestJUnitParamsClass.class.getName(), "testTwo[1]");
        mListener.testStarted(test2_2);
        mListener.testEnded(test2_2, new HashMap<String, Metric>());

        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mDevice, mBuild, mListener);
        mTest.run(mListener);
        EasyMock.verify(mDevice, mBuild, mListener);
    }

    /**
     * Test running the parameterized tests with an @ignore parameterized method, the base method
     * will be completely ignored.
     */
    @Test
    public void testRun_ignored() throws Exception {
        OptionSetter setter = new OptionSetter(mTest);
        setter.setOptionValue("class", TestJUnitParamsClassWithIgnored.class.getName());

        mListener.testRunStarted(TestJUnitParamsClassWithIgnored.class.getName(), 1);

        TestDescription test2_p1 =
                new TestDescription(TestJUnitParamsClassWithIgnored.class.getName(), "testTwo");
        mListener.testStarted(test2_p1);
        mListener.testIgnored(test2_p1);
        mListener.testEnded(test2_p1, new HashMap<String, Metric>());

        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mDevice, mBuild, mListener);
        mTest.run(mListener);
        EasyMock.verify(mDevice, mBuild, mListener);
    }
}
