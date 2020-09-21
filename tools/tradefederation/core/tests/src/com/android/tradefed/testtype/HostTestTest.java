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
package com.android.tradefed.testtype;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner.TestLogData;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner.TestMetrics;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import org.easymock.Capture;
import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Assert;
import org.junit.Assume;
import org.junit.AssumptionViolatedException;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.JUnit4;
import org.junit.runners.Suite.SuiteClasses;
import org.junit.runners.model.InitializationError;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.AnnotatedElement;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link HostTest}.
 */
@SuppressWarnings("unchecked")
public class HostTestTest extends TestCase {

    private HostTest mHostTest;
    private ITestInvocationListener mListener;
    private ITestDevice mMockDevice;
    private IBuildInfo mMockBuildInfo;

    @MyAnnotation
    @MyAnnotation3
    public static class SuccessTestCase extends TestCase {
        public SuccessTestCase() {}

        public SuccessTestCase(String name) {
            super(name);
        }

        @MyAnnotation
        public void testPass() {
        }

        @MyAnnotation
        @MyAnnotation2
        public void testPass2() {
        }
    }

    public static class TestMetricTestCase extends MetricTestCase {

        @Option(name = "test-option")
        public String testOption = null;

        @Option(name = "list-option")
        public List<String> listOption = new ArrayList<>();

        @Option(name = "map-option")
        public Map<String, String> mapOption = new HashMap<>();

        public void testPass() {
            addTestMetric("key1", "metric1");
        }

        public void testPass2() {
            addTestMetric("key2", "metric2");
            if (testOption != null) {
                addTestMetric("test-option", testOption);
            }
            if (!listOption.isEmpty()) {
                addTestMetric("list-option", listOption.toString());
            }
            if (!mapOption.isEmpty()) {
                addTestMetric("map-option", mapOption.toString());
            }
        }
    }

    public static class LogMetricTestCase extends MetricTestCase {

        public void testPass() {}

        public void testPass2() {
            addTestLog(
                    "test2_log",
                    LogDataType.TEXT,
                    new ByteArrayInputStreamSource("test_log".getBytes()));
            addTestMetric("key2", "metric2");
        }
    }

    @MyAnnotation
    public static class AnotherTestCase extends TestCase {
        public AnotherTestCase() {
        }

        public AnotherTestCase(String name) {
            super(name);
        }

        @MyAnnotation
        @MyAnnotation2
        @MyAnnotation3
        public void testPass3() {}

        @MyAnnotation
        public void testPass4() {
        }
    }

    /**
     * Test class, we have to annotate with full org.junit.Test to avoid name collision in import.
     */
    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class Junit4TestClass {

        public Junit4TestClass() {}

        @Option(name = "junit4-option")
        public boolean mOption = false;

        @Rule public TestMetrics metrics = new TestMetrics();

        @MyAnnotation
        @MyAnnotation2
        @org.junit.Test
        public void testPass5() {
            // test log through the rule.
            metrics.addTestMetric("key", "value");
        }

        @MyAnnotation
        @org.junit.Test
        public void testPass6() {
            metrics.addTestMetric("key2", "value2");
            if (mOption) {
                metrics.addTestMetric("junit4-option", "true");
            }
        }
    }

    /**
     * Test class, we have to annotate with full org.junit.Test to avoid name collision in import.
     */
    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class Junit4TestLogClass {

        public Junit4TestLogClass() {}

        @Rule public TestLogData logs = new TestLogData();

        @org.junit.Test
        public void testPass1() {
            ByteArrayInputStreamSource source = new ByteArrayInputStreamSource("test".getBytes());
            logs.addTestLog("TEST", LogDataType.TEXT, source);
            // Always cancel streams.
            StreamUtil.cancel(source);
        }

        @org.junit.Test
        public void testPass2() {
            ByteArrayInputStreamSource source = new ByteArrayInputStreamSource("test2".getBytes());
            logs.addTestLog("TEST2", LogDataType.TEXT, source);
            // Always cancel streams.
            StreamUtil.cancel(source);
        }
    }

    /**
     * Test class, we have to annotate with full org.junit.Test to avoid name collision in import.
     * And with one test marked as Ignored
     */
    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class Junit4TestClassWithIgnore implements IDeviceTest {
        private ITestDevice mDevice;

        public Junit4TestClassWithIgnore() {}

        @org.junit.Test
        public void testPass5() {}

        @Ignore
        @org.junit.Test
        public void testPass6() {}

        @Override
        public void setDevice(ITestDevice device) {
            mDevice = device;
        }

        @Override
        public ITestDevice getDevice() {
            return mDevice;
        }
    }

    /**
     * Test class that run a test throwing an {@link AssumptionViolatedException} which should be
     * handled as the testAssumptionFailure.
     */
    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class JUnit4TestClassAssume {

        @org.junit.Test
        public void testPass5() {
            Assume.assumeTrue(false);
        }
    }

    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class JUnit4TestClassMultiException {

        @org.junit.Test
        public void testPass5() {
            Assume.assumeTrue(false);
        }

        @After
        public void tearDown() {
            Assert.assertTrue(false);
        }
    }

    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class Junit4TestClassMulti implements IMultiDeviceTest {
        private Map<ITestDevice, IBuildInfo> mDeviceMap;

        public Junit4TestClassMulti() {}

        @org.junit.Test
        public void testPass5() {
            assertNotNull(mDeviceMap);
        }

        @Override
        public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
            mDeviceMap = deviceInfos;
        }
    }

    @RunWith(DeviceSuite.class)
    @SuiteClasses({
        Junit4TestClass.class,
        SuccessTestCase.class,
    })
    public class Junit4SuiteClass {
    }

    /**
     * JUnit4 runner that implements {@link ISetOptionReceiver} but does not actually have the
     * set-option.
     */
    public static class InvalidJunit4Runner extends BlockJUnit4ClassRunner
            implements ISetOptionReceiver {
        public InvalidJunit4Runner(Class<?> klass) throws InitializationError {
            super(klass);
        }
    }

    @RunWith(InvalidJunit4Runner.class)
    public static class Junit4RegularClass {
        @Option(name = "option")
        private String mOption = null;

        @org.junit.Test
        public void testPass() {}
    }

    /**
     * Malformed on purpose test class.
     */
    public static class Junit4MalformedTestClass {
        public Junit4MalformedTestClass() {
        }

        @Before
        protected void setUp() {
            // @Before should be on a public method.
        }

        @org.junit.Test
        public void testPass() {
        }
    }

    /**
     * Simple Annotation class for testing
     */
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyAnnotation {
    }

    /**
     * Simple Annotation class for testing
     */
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyAnnotation2 {
    }

    /** Simple Annotation class for testing */
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyAnnotation3 {}

    public static class SuccessTestSuite extends TestSuite {
        public SuccessTestSuite() {
            super(SuccessTestCase.class);
        }
    }

    public static class SuccessHierarchySuite extends TestSuite {
        public SuccessHierarchySuite() {
            super();
            addTestSuite(SuccessTestCase.class);
        }
    }

    public static class SuccessDeviceTest extends DeviceTestCase {

        @Option(name = "option")
        public String mOption = null;

        public SuccessDeviceTest() {
            super();
        }

        public void testPass() {
            assertNotNull(getDevice());
            if (mOption != null) {
                addTestMetric("option", mOption);
            }
        }
    }

    @MyAnnotation
    public static class SuccessDeviceTest2 extends DeviceTestCase {
        public SuccessDeviceTest2() {
            super();
        }

        @MyAnnotation3
        public void testPass1() {
            assertNotNull(getDevice());
        }

        public void testPass2() {
            assertNotNull(getDevice());
        }
    }

    @MyAnnotation
    public static class InheritedDeviceTest3 extends SuccessDeviceTest2 {
        public InheritedDeviceTest3() {
            super();
        }

        @Override
        public void testPass1() {
            super.testPass1();
        }

        @MyAnnotation3
        public void testPass3() {}
    }

    public static class TestRemoteNotCollector implements IDeviceTest, IRemoteTest {
        @Override
        public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {}

        @Override
        public void setDevice(ITestDevice device) {}

        @Override
        public ITestDevice getDevice() {
            return null;
        }
    }

    /** Non-public class; should fail to load. */
    private static class PrivateTest extends TestCase {
        @SuppressWarnings("unused")
        public void testPrivate() {
        }
    }

    /** class without default constructor; should fail to load */
    public static class NoConstructorTest extends TestCase {
        public NoConstructorTest(String name) {
            super(name);
        }
        public void testNoConstructor() {
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mHostTest = new HostTest();
        mListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockBuildInfo = EasyMock.createMock(IBuildInfo.class);
        mHostTest.setDevice(mMockDevice);
        mHostTest.setBuild(mMockBuildInfo);
        OptionSetter setter = new OptionSetter(mHostTest);
        // Disable pretty logging for testing
        setter.setOptionValue("enable-pretty-logs", "false");
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link TestCase}.
     */
    public void testRun_testcase() throws Exception {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        TestDescription test2 = new TestDescription(SuccessTestCase.class.getName(), "testPass2");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link MetricTestCase}.
     */
    public void testRun_MetricTestCase() throws Exception {
        mHostTest.setClassName(TestMetricTestCase.class.getName());
        TestDescription test1 = new TestDescription(TestMetricTestCase.class.getName(), "testPass");
        TestDescription test2 =
                new TestDescription(TestMetricTestCase.class.getName(), "testPass2");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        // test1 should only have its metrics
        Map<String, String> metric1 = new HashMap<>();
        metric1.put("key1", "metric1");
        mListener.testEnded(test1, TfMetricProtoUtil.upgradeConvert(metric1));
        // test2 should only have its metrics
        mListener.testStarted(EasyMock.eq(test2));
        Map<String, String> metric2 = new HashMap<>();
        metric2.put("key2", "metric2");
        mListener.testEnded(test2, TfMetricProtoUtil.upgradeConvert(metric2));
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test a case where a test use {@link MetricTestCase#addTestLog(String, LogDataType,
     * InputStreamSource)} in order to log data for all the reporters to know about.
     */
    public void testRun_LogMetricTestCase() throws Exception {
        mHostTest.setClassName(LogMetricTestCase.class.getName());
        TestDescription test1 = new TestDescription(LogMetricTestCase.class.getName(), "testPass");
        TestDescription test2 = new TestDescription(LogMetricTestCase.class.getName(), "testPass2");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        // test1 should only have its metrics
        mListener.testEnded(test1, new HashMap<String, Metric>());
        // test2 should only have its metrics
        mListener.testStarted(EasyMock.eq(test2));
        Map<String, String> metric2 = new HashMap<>();
        metric2.put("key2", "metric2");
        mListener.testLog(
                EasyMock.eq("test2_log"), EasyMock.eq(LogDataType.TEXT), EasyMock.anyObject());
        mListener.testEnded(test2, TfMetricProtoUtil.upgradeConvert(metric2));
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link MetricTestCase} and where an option is set to get extra metrics.
     */
    public void testRun_MetricTestCase_withOption() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("set-option", "test-option:test");
        // List option can take several values.
        setter.setOptionValue("set-option", "list-option:test1");
        setter.setOptionValue("set-option", "list-option:test2");
        // Map option
        setter.setOptionValue("set-option", "map-option:key=value");
        mHostTest.setClassName(TestMetricTestCase.class.getName());
        TestDescription test1 = new TestDescription(TestMetricTestCase.class.getName(), "testPass");
        TestDescription test2 =
                new TestDescription(TestMetricTestCase.class.getName(), "testPass2");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        // test1 should only have its metrics
        Map<String, String> metric1 = new HashMap<>();
        metric1.put("key1", "metric1");
        mListener.testEnded(test1, TfMetricProtoUtil.upgradeConvert(metric1));
        // test2 should only have its metrics
        mListener.testStarted(EasyMock.eq(test2));
        Map<String, String> metric2 = new HashMap<>();
        metric2.put("key2", "metric2");
        metric2.put("test-option", "test");
        metric2.put("list-option", "[test1, test2]");
        metric2.put("map-option", "{key=value}");
        mListener.testEnded(test2, TfMetricProtoUtil.upgradeConvert(metric2));
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link TestSuite}.
     */
    public void testRun_testSuite() throws Exception {
        mHostTest.setClassName(SuccessTestSuite.class.getName());
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        TestDescription test2 = new TestDescription(SuccessTestCase.class.getName(), "testPass2");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * hierarchy of {@link TestSuite}s.
     */
    public void testRun_testHierarchySuite() throws Exception {
        mHostTest.setClassName(SuccessHierarchySuite.class.getName());
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        TestDescription test2 = new TestDescription(SuccessTestCase.class.getName(), "testPass2");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link TestCase} and methodName is set.
     */
    public void testRun_testMethod() throws Exception {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        mHostTest.setMethodName("testPass");
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, where className is not set.
     */
    public void testRun_missingClass() throws Exception {
        try {
            mHostTest.run(mListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for an invalid class.
     */
    public void testRun_invalidClass() throws Exception {
        try {
            mHostTest.setClassName("foo");
            mHostTest.run(mListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)},
     * for a valid class that is not a {@link Test}.
     */
    public void testRun_notTestClass() throws Exception {
        try {
            mHostTest.setClassName(String.class.getName());
            mHostTest.run(mListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)},
     * for a private class.
     */
    public void testRun_privateClass() throws Exception {
        try {
            mHostTest.setClassName(PrivateTest.class.getName());
            mHostTest.run(mListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)},
     * for a test class with no default constructor.
     */
    public void testRun_noConstructorClass() throws Exception {
        try {
            mHostTest.setClassName(NoConstructorTest.class.getName());
            mHostTest.run(mListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for multiple test classes.
     */
    public void testRun_multipleClass() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", SuccessTestCase.class.getName());
        setter.setOptionValue("class", AnotherTestCase.class.getName());
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        TestDescription test2 = new TestDescription(SuccessTestCase.class.getName(), "testPass2");
        TestDescription test3 = new TestDescription(AnotherTestCase.class.getName(), "testPass3");
        TestDescription test4 = new TestDescription(AnotherTestCase.class.getName(), "testPass4");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test3));
        mListener.testEnded(EasyMock.eq(test3), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test4));
        mListener.testEnded(EasyMock.eq(test4), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)},
     * for multiple test classes with a method name.
     */
    public void testRun_multipleClassAndMethodName() throws Exception {
        try {
            OptionSetter setter = new OptionSetter(mHostTest);
            setter.setOptionValue("class", SuccessTestCase.class.getName());
            setter.setOptionValue("class", AnotherTestCase.class.getName());
            mHostTest.setMethodName("testPass3");
            mHostTest.run(mListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for a {@link IDeviceTest}.
     */
    public void testRun_deviceTest() throws Exception {
        final ITestDevice device = EasyMock.createMock(ITestDevice.class);
        mHostTest.setClassName(SuccessDeviceTest.class.getName());
        mHostTest.setDevice(device);
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("set-option", "option:value");

        TestDescription test1 = new TestDescription(SuccessDeviceTest.class.getName(), "testPass");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        Map<String, String> expected = new HashMap<>();
        expected.put("option", "value");
        mListener.testEnded(
                EasyMock.eq(test1), EasyMock.eq(TfMetricProtoUtil.upgradeConvert(expected)));
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)},
     * for a {@link IDeviceTest} where no device has been provided.
     */
    public void testRun_missingDevice() throws Exception {
        mHostTest.setClassName(SuccessDeviceTest.class.getName());
        mHostTest.setDevice(null);
        try {
            mHostTest.run(mListener);
            fail("expected IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test for {@link HostTest#countTestCases()}
     */
    public void testCountTestCases() throws Exception {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        assertEquals("Incorrect test case count", 2, mHostTest.countTestCases());
    }

    /**
     * Test for {@link HostTest#countTestCases()} with filtering on JUnit4 tests
     */
    public void testCountTestCasesJUnit4WithFiltering() throws Exception {
        mHostTest.setClassName(Junit4TestClass.class.getName());
        mHostTest.addIncludeFilter(
                "com.android.tradefed.testtype.HostTestTest$Junit4TestClass#testPass5");
        assertEquals("Incorrect test case count", 1, mHostTest.countTestCases());
    }

    /**
     * Test for {@link HostTest#countTestCases()}, if JUnit4 test class is malformed it will
     * count as 1 in the total number of tests.
     */
    public void testCountTestCasesJUnit4Malformed() throws Exception {
        mHostTest.setClassName(Junit4MalformedTestClass.class.getName());
        assertEquals("Incorrect test case count", 1, mHostTest.countTestCases());
    }

    /**
     * Test for {@link HostTest#countTestCases()} with filtering on JUnit4 tests and no test
     * remain.
     */
    public void testCountTestCasesJUnit4WithFiltering_no_more_tests() throws Exception {
        mHostTest.setClassName(Junit4TestClass.class.getName());
        mHostTest.addExcludeFilter(
                "com.android.tradefed.testtype.HostTestTest$Junit4TestClass#testPass5");
        mHostTest.addExcludeFilter(
                "com.android.tradefed.testtype.HostTestTest$Junit4TestClass#testPass6");
        assertEquals("Incorrect test case count", 0, mHostTest.countTestCases());
    }

    /**
     * Test for {@link HostTest#countTestCases()} with tests of varying JUnit versions
     */
    public void testCountTestCasesJUnitVersionMixed() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", SuccessTestCase.class.getName()); // 2 tests
        setter.setOptionValue("class", Junit4TestClass.class.getName()); // 2 tests
        setter.setOptionValue("class", Junit4SuiteClass.class.getName()); // 4 tests
        assertEquals("Incorrect test case count", 8, mHostTest.countTestCases());
    }

    /**
     * Test for {@link HostTest#countTestCases()} with filtering on tests of varying JUnit versions
     */
    public void testCountTestCasesJUnitVersionMixedWithFiltering() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", SuccessTestCase.class.getName()); // 2 tests
        setter.setOptionValue("class", Junit4TestClass.class.getName()); // 2 tests
        mHostTest.addIncludeFilter(
                "com.android.tradefed.testtype.HostTestTest$SuccessTestCase#testPass");
        mHostTest.addIncludeFilter(
                "com.android.tradefed.testtype.HostTestTest$Junit4TestClass#testPass5");
        assertEquals("Incorrect test case count", 2, mHostTest.countTestCases());
    }

    /**
     * Test for {@link HostTest#countTestCases()} with annotation filtering
     */
    public void testCountTestCasesAnnotationFiltering() throws Exception {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation2");
        assertEquals("Incorrect test case count", 1, mHostTest.countTestCases());
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link TestCase} with annotation filtering.
     */
    public void testRun_testcaseAnnotationFiltering() throws Exception {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        TestDescription test2 = new TestDescription(SuccessTestCase.class.getName(), "testPass2");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link TestCase} with notAnnotationFiltering
     */
    public void testRun_testcaseNotAnnotationFiltering() throws Exception {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation2");
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        // Only test1 will run, test2 should be filtered out.
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link TestCase} with both annotation filtering.
     */
    public void testRun_testcaseBothAnnotationFiltering() throws Exception {
        mHostTest.setClassName(AnotherTestCase.class.getName());
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation2");
        TestDescription test4 = new TestDescription(AnotherTestCase.class.getName(), "testPass4");
        // Only a test with MyAnnotation and Without MyAnnotation2 will run. Here testPass4
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test4));
        mListener.testEnded(EasyMock.eq(test4), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link TestCase} with multiple include annotation, test must contains them all.
     */
    public void testRun_testcaseMultiInclude() throws Exception {
        mHostTest.setClassName(AnotherTestCase.class.getName());
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation2");
        TestDescription test3 = new TestDescription(AnotherTestCase.class.getName(), "testPass3");
        // Only a test with MyAnnotation and with MyAnnotation2 will run. Here testPass3
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test3));
        mListener.testEnded(EasyMock.eq(test3), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#shouldTestRun(AnnotatedElement)}, where a class is
     * properly annotated to run.
     */
    public void testRun_shouldTestRun_Success() throws Exception {
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        assertTrue(mHostTest.shouldTestRun(SuccessTestCase.class));
    }

    /**
     * Test success case for {@link HostTest#shouldTestRun(AnnotatedElement)}, where a class is
     * properly annotated to run with multiple annotation expected.
     */
    public void testRun_shouldTestRunMulti_Success() throws Exception {
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation2");
        assertTrue(mHostTest.shouldTestRun(AnotherTestCase.class));
    }

    /**
     * Test case for {@link HostTest#shouldTestRun(AnnotatedElement)}, where a class is
     * properly annotated to be filtered.
     */
    public void testRun_shouldNotRun() throws Exception {
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        assertFalse(mHostTest.shouldTestRun(SuccessTestCase.class));
    }

    /**
     * Test case for {@link HostTest#shouldTestRun(AnnotatedElement)}, where a class is
     * properly annotated to be filtered because one of its two annotations is part of the exclude.
     */
    public void testRun_shouldNotRunMulti() throws Exception {
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        assertFalse(mHostTest.shouldTestRun(SuccessTestCase.class));
        mHostTest = new HostTest();
        // If only the other annotation is excluded.
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation3");
        assertFalse(mHostTest.shouldTestRun(SuccessTestCase.class));
    }

    /**
     * Test success case for {@link HostTest#shouldTestRun(AnnotatedElement)}, where a class is
     * annotated with a different annotation from the exclude filter.
     */
    public void testRun_shouldRun_exclude() throws Exception {
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation2");
        assertTrue(mHostTest.shouldTestRun(SuccessTestCase.class));
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where test to run is a
     * {@link TestCase} with annotation filtering.
     */
    public void testRun_testcaseCollectMode() throws Exception {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        mHostTest.setCollectTestsOnly(true);
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted((TestDescription) EasyMock.anyObject());
        mListener.testEnded(
                (TestDescription) EasyMock.anyObject(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted((TestDescription) EasyMock.anyObject());
        mListener.testEnded(
                (TestDescription) EasyMock.anyObject(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where the
     * {@link IRemoteTest} does not implements {@link ITestCollector}
     */
    public void testRun_testcaseCollectMode_IRemotedevice() throws Exception {
        final ITestDevice device = EasyMock.createMock(ITestDevice.class);
        mHostTest.setClassName(TestRemoteNotCollector.class.getName());
        mHostTest.setDevice(device);
        mHostTest.setCollectTestsOnly(true);
        EasyMock.replay(mListener);
        try {
            mHostTest.run(mListener);
        } catch (IllegalArgumentException expected) {
            EasyMock.verify(mListener);
            return;
        }
        fail("HostTest run() should have thrown an exception.");
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for test with Junit4 style.
     */
    public void testRun_junit4style() throws Exception {
        mHostTest.setClassName(Junit4TestClass.class.getName());
        TestDescription test1 = new TestDescription(Junit4TestClass.class.getName(), "testPass5");
        TestDescription test2 = new TestDescription(Junit4TestClass.class.getName(), "testPass6");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        Map<String, String> metrics = new HashMap<>();
        metrics.put("key", "value");
        mListener.testEnded(test1, TfMetricProtoUtil.upgradeConvert(metrics));
        mListener.testStarted(EasyMock.eq(test2));
        // test cases do not share metrics.
        Map<String, String> metrics2 = new HashMap<>();
        metrics2.put("key2", "value2");
        mListener.testEnded(
                EasyMock.eq(test2), EasyMock.eq(TfMetricProtoUtil.upgradeConvert(metrics2)));
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for test with Junit4 style and
     * handling of @Ignored.
     */
    public void testRun_junit4style_ignored() throws Exception {
        mHostTest.setClassName(Junit4TestClassWithIgnore.class.getName());
        TestDescription test1 =
                new TestDescription(Junit4TestClassWithIgnore.class.getName(), "testPass5");
        TestDescription test2 =
                new TestDescription(Junit4TestClassWithIgnore.class.getName(), "testPass6");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testIgnored(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for test with Junit4 style and
     * handling of Assume.
     */
    public void testRun_junit4style_assumeFailure() throws Exception {
        mHostTest.setClassName(JUnit4TestClassAssume.class.getName());
        TestDescription test1 =
                new TestDescription(JUnit4TestClassAssume.class.getName(), "testPass5");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testAssumptionFailure(EasyMock.eq(test1), EasyMock.anyObject());
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for test with Junit4 style and
     * handling of Multiple exception one from @Test one from @After. Junit replay both as failure.
     */
    public void testRun_junit4style_multiException() throws Exception {
        mListener = EasyMock.createStrictMock(ITestInvocationListener.class);
        mHostTest.setClassName(JUnit4TestClassMultiException.class.getName());
        TestDescription test1 =
                new TestDescription(JUnit4TestClassMultiException.class.getName(), "testPass5");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testFailed(
                EasyMock.eq(test1),
                EasyMock.contains("MultipleFailureException: There were 2 errors:"));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for test with Junit4 style properly
     * pass to the test the {@link IMultiDeviceTest} information.
     */
    public void testRun_junit4style_multiDevice() throws Exception {
        mHostTest.setClassName(Junit4TestClassMulti.class.getName());
        mHostTest.setDeviceInfos(new HashMap<>());
        TestDescription test1 =
                new TestDescription(Junit4TestClassMulti.class.getName(), "testPass5");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for test with Junit4 style and with
     * method filtering. Only run the expected method.
     */
    public void testRun_junit4_withMethodFilter() throws Exception {
        mHostTest.setClassName(Junit4TestClass.class.getName());
        TestDescription test2 = new TestDescription(Junit4TestClass.class.getName(), "testPass6");
        mHostTest.setMethodName("testPass6");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for a mix of test junit3 and 4
     */
    public void testRun_junit_version_mix() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", SuccessTestCase.class.getName());
        setter.setOptionValue("class", Junit4TestClass.class.getName());
        runMixJunitTest(mHostTest, 2, 2);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for a mix of test junit3 and 4 in
     * collect only mode
     */
    public void testRun_junit_version_mix_collect() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", SuccessTestCase.class.getName());
        setter.setOptionValue("class", Junit4TestClass.class.getName());
        setter.setOptionValue("collect-tests-only", "true");
        runMixJunitTest(mHostTest, 2, 2);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for a mix of test junit3 and 4 in
     * a Junit 4 suite class.
     */
    public void testRun_junit_suite_mix() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        runMixJunitTest(mHostTest, 4, 1);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for a mix of test junit3 and 4 in
     * a Junit 4 suite class, in collect only mode.
     */
    public void testRun_junit_suite_mix_collect() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("collect-tests-only", "true");
        runMixJunitTest(mHostTest, 4, 1);
    }

    /**
     * Helper for test option variation and avoid repeating the same setup
     */
    private void runMixJunitTest(HostTest hostTest, int expectedTest, int expectedRun)
            throws Exception {
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        TestDescription test2 = new TestDescription(SuccessTestCase.class.getName(), "testPass2");
        TestDescription test3 = new TestDescription(Junit4TestClass.class.getName(), "testPass5");
        TestDescription test4 = new TestDescription(Junit4TestClass.class.getName(), "testPass6");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(expectedTest));
        EasyMock.expectLastCall().times(expectedRun);
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test3));
        mListener.testEnded(EasyMock.eq(test3), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test4));
        mListener.testEnded(EasyMock.eq(test4), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(expectedRun);
        EasyMock.replay(mListener);
        hostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)} with a filtering and
     * junit 4 handling.
     */
    public void testRun_testcase_Junit4TestNotAnnotationFiltering() throws Exception {
        mHostTest.setClassName(Junit4TestClass.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation2");
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("set-option", "junit4-option:true");
        TestDescription test1 = new TestDescription(Junit4TestClass.class.getName(), "testPass6");
        // Only test1 will run, test2 should be filtered out.
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        Map<String, String> metrics = new HashMap<>();
        metrics.put("key2", "value2");
        // If the option was correctly set, this metric should be true.
        metrics.put("junit4-option", "true");
        mListener.testEnded(
                EasyMock.eq(test1), EasyMock.eq(TfMetricProtoUtil.upgradeConvert(metrics)));
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test success case for {@link HostTest#run(ITestInvocationListener)}, where filtering is
     * applied and results in 0 tests to run.
     */
    public void testRun_testcase_Junit4Test_filtering_no_more_tests() throws Exception {
        mHostTest.setClassName(Junit4TestClass.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(0));
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test that in case the class attempted to be ran is malformed we bubble up the test failure.
     */
    public void testRun_Junit4Test_malformed() throws Exception {
        mHostTest.setClassName(Junit4MalformedTestClass.class.getName());
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(1));
        Capture<TestDescription> captured = new Capture<>();
        mListener.testStarted(EasyMock.capture(captured));
        mListener.testFailed((TestDescription) EasyMock.anyObject(), (String) EasyMock.anyObject());
        mListener.testEnded(
                (TestDescription) EasyMock.anyObject(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        assertEquals(Junit4MalformedTestClass.class.getName(), captured.getValue().getClassName());
        assertEquals("initializationError", captured.getValue().getTestName());
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for a mix of test junit3 and 4 in
     * a Junit 4 suite class, and filtering is applied.
     */
    public void testRun_junit_suite_mix_filtering() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        runMixJunitTestWithFilter(mHostTest);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for a mix of test junit3 and 4 in
     * a Junit 4 suite class, and filtering is applied, in collect mode
     */
    public void testRun_junit_suite_mix_filtering_collect() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("collect-tests-only", "true");
        runMixJunitTestWithFilter(mHostTest);
    }

    /**
     * Helper for test option variation and avoid repeating the same setup
     */
    private void runMixJunitTestWithFilter(HostTest hostTest) throws Exception {
        hostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation2");
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        TestDescription test4 = new TestDescription(Junit4TestClass.class.getName(), "testPass6");
        mListener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(2));
        EasyMock.expectLastCall().times(1);
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test4));
        mListener.testEnded(EasyMock.eq(test4), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        EasyMock.replay(mListener);
        hostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#split(int)} making sure each test type is properly handled and added
     * with a container or directly.
     */
    public void testRun_junit_suite_split() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        mHostTest.setDevice(mMockDevice);
        mHostTest.setBuild(mMockBuildInfo);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        setter.setOptionValue("class", TestRemoteNotCollector.class.getName());
        List<IRemoteTest> list = (ArrayList<IRemoteTest>) mHostTest.split(1);
        // split by class; numShards parameter should be ignored
        assertEquals(3, list.size());
        assertEquals("com.android.tradefed.testtype.HostTest",
                list.get(0).getClass().getName());
        assertEquals("com.android.tradefed.testtype.HostTest",
                list.get(1).getClass().getName());
        assertEquals("com.android.tradefed.testtype.HostTest",
                list.get(2).getClass().getName());

        // We expect all the test from the JUnit4 suite to run under the original suite classname
        // not under the container class name.
        mListener.testRunStarted(
                EasyMock.eq("com.android.tradefed.testtype.HostTestTest$Junit4SuiteClass"),
                EasyMock.eq(4));
        TestDescription test1 = new TestDescription(Junit4TestClass.class.getName(), "testPass5");
        TestDescription test2 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        TestDescription test3 = new TestDescription(SuccessTestCase.class.getName(), "testPass2");
        TestDescription test4 = new TestDescription(Junit4TestClass.class.getName(), "testPass6");
        mListener.testStarted(test1);
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test3));
        mListener.testEnded(EasyMock.eq(test3), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test4));
        mListener.testEnded(EasyMock.eq(test4), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        // Run the JUnit4 Container
        ((IBuildReceiver)list.get(0)).setBuild(mMockBuildInfo);
        ((IDeviceTest)list.get(0)).setDevice(mMockDevice);
        list.get(0).run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Similar to {@link #testRun_junit_suite_split()} but with shard-unit set to method
     */
    public void testRun_junit_suite_split_by_method() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        mHostTest.setDevice(mMockDevice);
        mHostTest.setBuild(mMockBuildInfo);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        setter.setOptionValue("class", TestRemoteNotCollector.class.getName());
        setter.setOptionValue("shard-unit", "method");
        final Class<?>[] expectedTestCaseClasses = new Class<?>[] {
            Junit4TestClass.class,
            Junit4TestClass.class,
            SuccessTestCase.class,
            SuccessTestCase.class,
            SuccessTestSuite.class,
            SuccessTestSuite.class,
            TestRemoteNotCollector.class,
        };
        List<IRemoteTest> list = (ArrayList<IRemoteTest>) mHostTest.split(expectedTestCaseClasses.length);
        assertEquals(expectedTestCaseClasses.length, list.size());
        for (int i = 0; i < expectedTestCaseClasses.length; i++) {
            IRemoteTest shard = list.get(i);
            assertTrue(HostTest.class.isInstance(shard));
            HostTest hostTest = (HostTest)shard;
            assertEquals(1, hostTest.getClasses().size());
            assertEquals(1, hostTest.countTestCases());
            assertEquals(expectedTestCaseClasses[i], hostTest.getClasses().get(0));
        }

        // We expect all the test from the JUnit4 suite to run under the original suite classname
        // not under the container class name.
        TestDescription test = new TestDescription(Junit4TestClass.class.getName(), "testPass5");
        mListener.testRunStarted(test.getClassName(), 1);
        mListener.testStarted(test);
        mListener.testEnded(EasyMock.eq(test), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        // Run the JUnit4 Container
        ((IBuildReceiver)list.get(0)).setBuild(mMockBuildInfo);
        ((IDeviceTest)list.get(0)).setDevice(mMockDevice);
        list.get(0).run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#split(int)} when no class is specified throws an exception
     */
    public void testSplit_noClass() throws Exception {
        try {
            mHostTest.split(1);
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException e) {
            assertEquals("Missing Test class name", e.getMessage());
        }
    }

    /**
     * Test for {@link HostTest#split(int)} when multiple classes are specified with a method option
     * too throws an exception
     */
    public void testSplit_methodAndMultipleClass() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        mHostTest.setMethodName("testPass2");
        try {
            mHostTest.split(1);
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException e) {
            assertEquals("Method name given with multiple test classes", e.getMessage());
        }
    }

    /**
     * Test for {@link HostTest#split(int)} when a single class is specified, no splitting can occur
     * and it returns null.
     */
    public void testSplit_singleClass() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        mHostTest.setMethodName("testPass2");
        assertNull(mHostTest.split(1));
    }

    /**
     * Test for {@link HostTest#getTestShard(int, int)} with one shard per classes, the runtime hint
     * is also split across tests based on number of tests.
     */
    public void testGetTestStrictShardable() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        setter.setOptionValue("class", TestRemoteNotCollector.class.getName());
        setter.setOptionValue("runtime-hint", "2m");
        IRemoteTest shard0 = mHostTest.getTestShard(3, 0);
        assertTrue(shard0 instanceof HostTest);
        assertEquals(1, ((HostTest)shard0).getClasses().size());
        assertEquals("com.android.tradefed.testtype.HostTestTest$Junit4SuiteClass",
                ((HostTest)shard0).getClasses().get(0).getName());
        // This shard contains 4 out of 7 tests: (4/7) * 2 minutes
        assertEquals(68571, ((HostTest) shard0).getRuntimeHint());

        IRemoteTest shard1 = mHostTest.getTestShard(3, 1);
        assertTrue(shard1 instanceof HostTest);
        assertEquals(1, ((HostTest)shard1).getClasses().size());
        assertEquals("com.android.tradefed.testtype.HostTestTest$SuccessTestSuite",
                ((HostTest)shard1).getClasses().get(0).getName());
        // This shard contains 2 out of 7 tests: (2/7) * 2 minutes
        assertEquals(34285, ((HostTest) shard1).getRuntimeHint());

        IRemoteTest shard2 = mHostTest.getTestShard(3, 2);
        assertTrue(shard2 instanceof HostTest);
        assertEquals(1, ((HostTest)shard2).getClasses().size());
        assertEquals("com.android.tradefed.testtype.HostTestTest$TestRemoteNotCollector",
                ((HostTest)shard2).getClasses().get(0).getName());
        // This shard contains 1 out of 7 tests: (1/7) * 2 minutes
        assertEquals(17142, ((HostTest) shard2).getRuntimeHint());
    }

    /**
     * Similar to {@link #testGetTestStrictShardable()} but with shard-unit set to method
     */
    public void testGetTestStrictShardable_shardUnit_method() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        setter.setOptionValue("class", TestRemoteNotCollector.class.getName());
        setter.setOptionValue("runtime-hint", "2m");
        setter.setOptionValue("shard-unit", "method");
        final int numShards = mHostTest.countTestCases();
        final long runtimeHint = 2 * 60 * 1000; // 2 minutes in microseconds
        final Class<?>[] expectedTestCaseClasses = new Class<?>[] {
            Junit4TestClass.class,
            Junit4TestClass.class,
            SuccessTestCase.class,
            SuccessTestCase.class,
            SuccessTestSuite.class,
            SuccessTestSuite.class,
            TestRemoteNotCollector.class,
        };
        assertEquals(expectedTestCaseClasses.length, numShards);
        for (int i = 0; i < numShards; i++) {
            IRemoteTest shard = mHostTest.getTestShard(numShards, i);
            assertTrue(shard instanceof HostTest);
            HostTest hostTest = (HostTest)shard;
            assertEquals(1, hostTest.getClasses().size());
            assertEquals(1, hostTest.countTestCases());
            assertEquals(expectedTestCaseClasses[i], hostTest.getClasses().get(0));
            assertEquals(runtimeHint / numShards, hostTest.getRuntimeHint());
        }
    }

    /**
     * Test for {@link HostTest#getTestShard(int, int)} when more shard than classes are requested,
     * the empty shard will have no test (StubTest).
     */
    public void testGetTestStrictShardable_tooManyShards() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        IRemoteTest shard0 = mHostTest.getTestShard(4, 0);
        assertTrue(shard0 instanceof HostTest);
        assertEquals(1, ((HostTest)shard0).getClasses().size());
        assertEquals("com.android.tradefed.testtype.HostTestTest$Junit4SuiteClass",
                ((HostTest)shard0).getClasses().get(0).getName());

        IRemoteTest shard1 = mHostTest.getTestShard(4, 1);
        assertTrue(shard1 instanceof HostTest);
        assertEquals(1, ((HostTest)shard1).getClasses().size());
        assertEquals("com.android.tradefed.testtype.HostTestTest$SuccessTestSuite",
                ((HostTest)shard1).getClasses().get(0).getName());

        IRemoteTest shard2 = mHostTest.getTestShard(4, 2);
        assertTrue(shard2 instanceof HostTest);
        assertEquals(0, ((HostTest)shard2).getClasses().size());
        IRemoteTest shard3 = mHostTest.getTestShard(4, 3);
        assertTrue(shard3 instanceof HostTest);
        assertEquals(0, ((HostTest)shard3).getClasses().size());
        // empty shard that can run and be skipped without reporting anything
        ITestInvocationListener mockListener = EasyMock.createMock(ITestInvocationListener.class);
        EasyMock.replay(mockListener);
        shard3.run(mockListener);
        EasyMock.verify(mockListener);
    }

    /**
     * Similar to {@link #testGetTestStrictShardable_tooManyShards()} but with shard-unit
     * set to method
     */
    public void testGetTestStrictShardable_tooManyShards_shardUnit_method() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        setter.setOptionValue("shard-unit", "method");
        int numTestCases = mHostTest.countTestCases();
        final int numShards = numTestCases + 1;
        final Class<?>[] expectedTestCaseClasses = new Class<?>[] {
            Junit4TestClass.class,
            Junit4TestClass.class,
            SuccessTestCase.class,
            SuccessTestCase.class,
            SuccessTestSuite.class,
            SuccessTestSuite.class,
        };
        assertEquals(expectedTestCaseClasses.length, numTestCases);
        for (int i = 0; i < numTestCases ; i++) {
            IRemoteTest shard = mHostTest.getTestShard(numShards, i);
            assertTrue(shard instanceof HostTest);
            HostTest hostTest = (HostTest)shard;
            assertEquals(1, hostTest.getClasses().size());
            assertEquals(1, hostTest.countTestCases());
            assertEquals(expectedTestCaseClasses[i], hostTest.getClasses().get(0));
        }
        IRemoteTest lastShard = mHostTest.getTestShard(numShards, numTestCases);
        assertTrue(lastShard instanceof HostTest);
        assertEquals(0, ((HostTest)lastShard).getClasses().size());
        assertEquals(0, ((HostTest)lastShard).countTestCases());
        // empty shard that can run and be skipped without reporting anything
        ITestInvocationListener mockListener = EasyMock.createMock(ITestInvocationListener.class);
        EasyMock.replay(mockListener);
        lastShard.run(mockListener);
        EasyMock.verify(mockListener);
    }

    /**
     * Test for {@link HostTest#getTestShard(int, int)} with one shard per classes.
     */
    public void testGetTestStrictShardable_wrapping() throws Exception {
        final ITestDevice device = EasyMock.createMock(ITestDevice.class);
        mHostTest.setDevice(device);
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        setter.setOptionValue("class", TestRemoteNotCollector.class.getName());
        setter.setOptionValue("class", SuccessHierarchySuite.class.getName());
        setter.setOptionValue("class", SuccessDeviceTest.class.getName());
        IRemoteTest shard0 = mHostTest.getTestShard(3, 0);
        assertTrue(shard0 instanceof HostTest);
        assertEquals(2, ((HostTest)shard0).getClasses().size());
        assertEquals("com.android.tradefed.testtype.HostTestTest$Junit4SuiteClass",
                ((HostTest)shard0).getClasses().get(0).getName());
        assertEquals("com.android.tradefed.testtype.HostTestTest$SuccessHierarchySuite",
                ((HostTest)shard0).getClasses().get(1).getName());

        IRemoteTest shard1 = mHostTest.getTestShard(3, 1);
        assertTrue(shard1 instanceof HostTest);
        assertEquals(2, ((HostTest)shard1).getClasses().size());
        assertEquals("com.android.tradefed.testtype.HostTestTest$SuccessTestSuite",
                ((HostTest)shard1).getClasses().get(0).getName());
        assertEquals("com.android.tradefed.testtype.HostTestTest$SuccessDeviceTest",
                ((HostTest)shard1).getClasses().get(1).getName());

        IRemoteTest shard2 = mHostTest.getTestShard(3, 2);
        assertTrue(shard2 instanceof HostTest);
        assertEquals(1, ((HostTest)shard2).getClasses().size());
        assertEquals("com.android.tradefed.testtype.HostTestTest$TestRemoteNotCollector",
                ((HostTest)shard2).getClasses().get(0).getName());
    }

    /**
     * Similar to {@link #testGetTestStrictShardable_wrapping()} but with shard-unit set to method
     */
    public void testGetTestStrictShardable_wrapping_shardUnit_method() throws Exception {
        testGetTestShardable_wrapping_shardUnit_method(true);
    }

    /**
     * Similar to {@link #testGetTestStrictShardable_wrapping_shardUnit_method()}
     * but test {@link IShardableTest} interface
     */
    public void testGetTestShardable_wrapping_shardUnit_method() throws Exception {
        testGetTestShardable_wrapping_shardUnit_method(false);
    }

    /**
     * Shard by method and verify that each shard contains the expected classes
     *
     * @param strict test {@link IStrictShardableTest} interface if true,
     * {@link IShardableTest} if false
     * @throws Exception
     */
    private void testGetTestShardable_wrapping_shardUnit_method(boolean strict) throws Exception {
        final ITestDevice device = EasyMock.createMock(ITestDevice.class);
        mHostTest.setDevice(device);
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4SuiteClass.class.getName());
        setter.setOptionValue("class", SuccessTestSuite.class.getName());
        setter.setOptionValue("class", TestRemoteNotCollector.class.getName());
        setter.setOptionValue("class", SuccessHierarchySuite.class.getName());
        setter.setOptionValue("class", SuccessDeviceTest.class.getName());
        setter.setOptionValue("runtime-hint", "2m");
        setter.setOptionValue("shard-unit", "method");
        final Class<?>[] expectedTestCaseClasses = new Class<?>[] {
            Junit4TestClass.class,
            SuccessTestCase.class,
            TestRemoteNotCollector.class,
            SuccessDeviceTest.class,
            Junit4TestClass.class,
            SuccessTestSuite.class,
            SuccessHierarchySuite.class,
            SuccessTestCase.class,
            SuccessTestSuite.class,
            SuccessHierarchySuite.class,
        };
        final int numShards = 3;
        final long runtimeHint = 2 * 60 * 1000; // 2 minutes in microseconds
        int numTestCases = mHostTest.countTestCases();
        assertEquals(expectedTestCaseClasses.length, numTestCases);
        for (int i = 0, j = 0; i < numShards ; i++) {
            IRemoteTest shard;
            if (strict) {
                shard = mHostTest.getTestShard(numShards, i);
            } else {
                shard = new ArrayList<>(mHostTest.split(numShards)).get(i);
            }
            assertTrue(shard instanceof HostTest);
            HostTest hostTest = (HostTest)shard;
            int q = numTestCases / numShards;
            int r = numTestCases % numShards;
            int n = q + (i < r ? 1 : 0);
            assertEquals(n, hostTest.countTestCases());
            assertEquals(n, hostTest.getClasses().size());
            assertEquals(runtimeHint * n / numTestCases, hostTest.getRuntimeHint());
            for (int k = 0; k < n; k++) {
                assertEquals(expectedTestCaseClasses[j++], hostTest.getClasses().get(k));
            }
        }
    }

    /** An annotation on the class exclude it. All the method of the class should be excluded. */
    public void testClassAnnotation_excludeAll() throws Exception {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation3");
        assertEquals(0, mHostTest.countTestCases());
        // nothing run.
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /** An annotation on the class include it. We include all the method inside it. */
    public void testClassAnnotation_includeAll() throws Exception {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation3");
        assertEquals(2, mHostTest.countTestCases());
        TestDescription test1 = new TestDescription(SuccessTestCase.class.getName(), "testPass");
        TestDescription test2 = new TestDescription(SuccessTestCase.class.getName(), "testPass2");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * An annotation on the method (no annotation on class) exclude it. This method does not run.
     */
    public void testMethodAnnotation_excludeAll() throws Exception {
        mHostTest.setClassName(AnotherTestCase.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation3");
        assertEquals(1, mHostTest.countTestCases());
        TestDescription test1 = new TestDescription(AnotherTestCase.class.getName(), "testPass4");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /** An annotation on the method (no annotation on class) include it. Only this method run. */
    public void testMethodAnnotation_includeAll() throws Exception {
        mHostTest.setClassName(AnotherTestCase.class.getName());
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation3");
        assertEquals(1, mHostTest.countTestCases());
        TestDescription test1 = new TestDescription(AnotherTestCase.class.getName(), "testPass3");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Check that a method annotation in a {@link DeviceTestCase} is properly included with an
     * include filter during collect-tests-only
     */
    public void testMethodAnnotation_includeAll_collect() throws Exception {
        mHostTest.setCollectTestsOnly(true);
        mHostTest.setClassName(SuccessDeviceTest2.class.getName());
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation3");
        assertEquals(1, mHostTest.countTestCases());
        TestDescription test1 =
                new TestDescription(SuccessDeviceTest2.class.getName(), "testPass1");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test that a method annotated and overridden is not included because the child method is not
     * annotated (annotation are not inherited).
     */
    public void testMethodAnnotation_inherited() throws Exception {
        mHostTest.setCollectTestsOnly(true);
        mHostTest.setClassName(InheritedDeviceTest3.class.getName());
        mHostTest.addIncludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation3");
        assertEquals(1, mHostTest.countTestCases());
        TestDescription test1 =
                new TestDescription(InheritedDeviceTest3.class.getName(), "testPass3");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test that a method annotated and overridden is not excluded if the child method does not have
     * the annotation.
     */
    public void testMethodAnnotation_inherited_exclude() throws Exception {
        mHostTest.setCollectTestsOnly(true);
        mHostTest.setClassName(InheritedDeviceTest3.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation3");
        assertEquals(2, mHostTest.countTestCases());
        TestDescription test1 =
                new TestDescription(InheritedDeviceTest3.class.getName(), "testPass1");
        TestDescription test2 =
                new TestDescription(InheritedDeviceTest3.class.getName(), "testPass2");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testEnded(EasyMock.eq(test1), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testStarted(EasyMock.eq(test2));
        mListener.testEnded(EasyMock.eq(test2), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /** Check that a {@link DeviceTestCase} is properly excluded when the class is excluded. */
    public void testDeviceTestCase_excludeClass() throws Exception {
        mHostTest.setClassName(SuccessDeviceTest2.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        assertEquals(0, mHostTest.countTestCases());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Check that a {@link DeviceTestCase} is properly excluded when the class is excluded in
     * collect-tests-only mode (yielding the same result as above).
     */
    public void testDeviceTestCase_excludeClass_collect() throws Exception {
        mHostTest.setCollectTestsOnly(true);
        mHostTest.setClassName(SuccessDeviceTest2.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        assertEquals(0, mHostTest.countTestCases());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test that {@link HostTest#getTestShard(int, int)} returns a {@link IRemoteTest} with no
     * runtime and no tests when filtered out of all its tests.
     */
    public void testShardAndUpdateRuntime() {
        mHostTest.setClassName(SuccessTestCase.class.getName());
        mHostTest.addExcludeAnnotation("com.android.tradefed.testtype.HostTestTest$MyAnnotation");
        HostTest test = (HostTest) mHostTest.getTestShard(2, 0);
        assertEquals(0l, test.getRuntimeHint());
        assertEquals(0, test.countTestCases());
    }

    /**
     * {@link HostTest#getTestShard(int, int)} returns a {@link IRemoteTest} with no runtime and no
     * tests when filtered out of all its tests with a exclude-filter.
     */
    public void testShardAndUpdateRuntime_filters() {
        mHostTest.setClassName(SuccessDeviceTest.class.getName());
        mHostTest.addExcludeFilter(
                "com.android.tradefed.testtype.HostTestTest$SuccessDeviceTest#testPass");
        HostTest test = (HostTest) mHostTest.getTestShard(2, 0);
        assertEquals(0l, test.getRuntimeHint());
        assertEquals(0, test.countTestCases());
    }

    /**
     * Test for {@link HostTest#split(int)} when the exclude-filter is set, it should be carried over
     * to shards.
     */
    public void testSplit_withExclude() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", SuccessTestCase.class.getName());
        setter.setOptionValue("class", AnotherTestCase.class.getName());
        mHostTest.addExcludeFilter(
                "com.android.tradefed.testtype.HostTestTest$SuccessTestCase#testPass");
        Collection<IRemoteTest> res = mHostTest.split(1);
        // split by class; numShards parameter should be ignored
        assertEquals(2, res.size());

        // only one tests in the SuccessTestCase because it's been filtered out.
        mListener.testRunStarted(
                EasyMock.eq("com.android.tradefed.testtype.HostTestTest$SuccessTestCase"),
                EasyMock.eq(1));
        TestDescription tid2 =
                new TestDescription(
                        "com.android.tradefed.testtype.HostTestTest$SuccessTestCase", "testPass2");
        mListener.testStarted(tid2);
        mListener.testEnded(tid2, new HashMap<String, Metric>());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        mListener.testRunStarted(
                EasyMock.eq("com.android.tradefed.testtype.HostTestTest$AnotherTestCase"),
                EasyMock.eq(2));
        TestDescription tid3 =
                new TestDescription(
                        "com.android.tradefed.testtype.HostTestTest$AnotherTestCase", "testPass3");
        mListener.testStarted(tid3);
        mListener.testEnded(tid3, new HashMap<String, Metric>());
        TestDescription tid4 =
                new TestDescription(
                        "com.android.tradefed.testtype.HostTestTest$AnotherTestCase", "testPass4");
        mListener.testStarted(tid4);
        mListener.testEnded(tid4, new HashMap<String, Metric>());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mListener, mMockDevice);
        for (IRemoteTest test : res) {
            assertTrue(test instanceof HostTest);
            ((HostTest) test).setDevice(mMockDevice);
            test.run(mListener);
        }
        EasyMock.verify(mListener, mMockDevice);
    }

    /**
     * Test that when the 'set-option' format is not respected, an exception is thrown. Only one '='
     * is allowed in the value.
     */
    public void testRun_setOption_invalid() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        // Map option with invalid format
        setter.setOptionValue("set-option", "map-option:key=value=2");
        mHostTest.setClassName(TestMetricTestCase.class.getName());
        EasyMock.replay(mListener);
        try {
            mHostTest.run(mListener);
            fail("Should have thrown an exception.");
        } catch (RuntimeException expected) {
            // expected
        }
        EasyMock.verify(mListener);
    }

    /**
     * Test that when a JUnit runner implements {@link ISetOptionReceiver} we attempt to pass it the
     * hostTest set-option.
     */
    public void testSetOption_regularJUnit4_fail() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        // Map option with invalid format
        setter.setOptionValue("set-option", "option:value");
        mHostTest.setClassName(Junit4RegularClass.class.getName());
        mListener.testRunStarted(
                EasyMock.eq("com.android.tradefed.testtype.HostTestTest$Junit4RegularClass"),
                EasyMock.eq(1));
        EasyMock.replay(mListener);
        try {
            mHostTest.run(mListener);
            fail("Should have thrown an exception.");
        } catch (RuntimeException expected) {
            // expected
        }
        EasyMock.verify(mListener);
    }

    /**
     * Test for {@link HostTest#run(ITestInvocationListener)}, for test with Junit4 style that log
     * some data.
     */
    public void testRun_junit4style_log() throws Exception {
        mHostTest.setClassName(Junit4TestLogClass.class.getName());
        TestDescription test1 =
                new TestDescription(Junit4TestLogClass.class.getName(), "testPass1");
        TestDescription test2 =
                new TestDescription(Junit4TestLogClass.class.getName(), "testPass2");
        mListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.eq(2));
        mListener.testStarted(EasyMock.eq(test1));
        mListener.testLog(EasyMock.eq("TEST"), EasyMock.eq(LogDataType.TEXT), EasyMock.anyObject());
        mListener.testEnded(test1, new HashMap<String, Metric>());
        mListener.testStarted(EasyMock.eq(test2));
        // test cases do not share logs, only the second test logs are seen.
        mListener.testLog(
                EasyMock.eq("TEST2"), EasyMock.eq(LogDataType.TEXT), EasyMock.anyObject());
        mListener.testEnded(test2, new HashMap<String, Metric>());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mListener);
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Similar to {@link #testSplit_withExclude()} but with shard-unit set to method
     */
    public void testSplit_excludeTestCase_shardUnit_method() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", SuccessTestCase.class.getName());
        setter.setOptionValue("class", AnotherTestCase.class.getName());

        // only one tests in the SuccessTestCase because it's been filtered out.
        TestDescription tid2 = new TestDescription(SuccessTestCase.class.getName(), "testPass2");
        TestDescription tid3 = new TestDescription(AnotherTestCase.class.getName(), "testPass3");
        TestDescription tid4 = new TestDescription(AnotherTestCase.class.getName(), "testPass4");
        testSplit_excludeFilter_shardUnit_Method(
                SuccessTestCase.class.getName() + "#testPass",
                new TestDescription[] {tid2, tid3, tid4});
    }

    /**
     * Similar to {@link #testSplit_excludeTestCase_shardUnit_method()} but exclude class
     */
    public void testSplit_excludeTestClass_shardUnit_method() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", SuccessTestCase.class.getName());
        setter.setOptionValue("class", AnotherTestCase.class.getName());

        TestDescription tid3 = new TestDescription(AnotherTestCase.class.getName(), "testPass3");
        TestDescription tid4 = new TestDescription(AnotherTestCase.class.getName(), "testPass4");
        testSplit_excludeFilter_shardUnit_Method(
                SuccessTestCase.class.getName(), new TestDescription[] {tid3, tid4});
    }

    private void testSplit_excludeFilter_shardUnit_Method(
            String excludeFilter, TestDescription[] expectedTids)
            throws DeviceNotAvailableException, ConfigurationException {
        mHostTest.addExcludeFilter(excludeFilter);
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("shard-unit", "method");

        Collection<IRemoteTest> res = mHostTest.split(expectedTids.length);
        assertEquals(expectedTids.length, res.size());

        for (TestDescription tid : expectedTids) {
            mListener.testRunStarted(tid.getClassName(), 1);
            mListener.testStarted(tid);
            mListener.testEnded(tid, new HashMap<String, Metric>());
            mListener.testRunEnded(
                    EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        }

        EasyMock.replay(mListener, mMockDevice);
        for (IRemoteTest test : res) {
            assertTrue(test instanceof HostTest);
            ((HostTest) test).setDevice(mMockDevice);
            test.run(mListener);
        }
        EasyMock.verify(mListener, mMockDevice);
    }

    /** JUnit 4 class that throws within its @BeforeClass */
    @RunWith(JUnit4.class)
    public static class JUnit4FailedBeforeClass {
        @BeforeClass
        public static void beforeClass() {
            throw new RuntimeException();
        }

        @org.junit.Test
        public void test1() {}
    }

    /**
     * Test that when an exception is thrown from within @BeforeClass, we correctly report a failure
     * since we cannot run each individual test.
     */
    public void testRun_junit4ExceptionBeforeClass() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", JUnit4FailedBeforeClass.class.getName());
        setter.setOptionValue("class", Junit4TestClass.class.getName());
        // First class fail with the run failure
        mListener.testRunStarted(EasyMock.anyObject(), EasyMock.eq(1));
        mListener.testRunFailed(EasyMock.contains("Failed with trace:"));
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        // Second class run properly
        mListener.testRunStarted(EasyMock.anyObject(), EasyMock.eq(2));
        TestDescription tid2 = new TestDescription(Junit4TestClass.class.getName(), "testPass5");
        mListener.testStarted(EasyMock.eq(tid2));
        mListener.testEnded(EasyMock.eq(tid2), (HashMap<String, Metric>) EasyMock.anyObject());
        TestDescription tid3 = new TestDescription(Junit4TestClass.class.getName(), "testPass6");
        mListener.testStarted(EasyMock.eq(tid3));
        mListener.testEnded(EasyMock.eq(tid3), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mListener);
        assertEquals(3, mHostTest.countTestCases());
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /** JUnit4 class that throws within its @Before */
    @RunWith(JUnit4.class)
    public static class JUnit4FailedBefore {
        @Before
        public void before() {
            throw new RuntimeException();
        }

        @org.junit.Test
        public void test1() {}
    }

    /**
     * Test that when an exception is thrown within @Before, the test are reported and failed with
     * the exception.
     */
    public void testRun_junit4ExceptionBefore() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", JUnit4FailedBefore.class.getName());
        setter.setOptionValue("class", Junit4TestClass.class.getName());
        // First class has a test failure because of the @Before
        mListener.testRunStarted(EasyMock.anyObject(), EasyMock.eq(1));
        TestDescription tid = new TestDescription(JUnit4FailedBefore.class.getName(), "test1");
        mListener.testStarted(EasyMock.eq(tid));
        mListener.testFailed(EasyMock.eq(tid), EasyMock.anyObject());
        mListener.testEnded(EasyMock.eq(tid), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        // Second class run properly
        mListener.testRunStarted(EasyMock.anyObject(), EasyMock.eq(2));
        TestDescription tid2 = new TestDescription(Junit4TestClass.class.getName(), "testPass5");
        mListener.testStarted(EasyMock.eq(tid2));
        mListener.testEnded(EasyMock.eq(tid2), (HashMap<String, Metric>) EasyMock.anyObject());
        TestDescription tid3 = new TestDescription(Junit4TestClass.class.getName(), "testPass6");
        mListener.testStarted(EasyMock.eq(tid3));
        mListener.testEnded(EasyMock.eq(tid3), (HashMap<String, Metric>) EasyMock.anyObject());
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mListener);
        assertEquals(3, mHostTest.countTestCases());
        mHostTest.run(mListener);
        EasyMock.verify(mListener);
    }

    /**
     * Test that when all tests are filtered out, we properly shard them with 0 runtime, and they
     * will be completely skipped during execution.
     */
    public void testSplit_withFilter() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", Junit4TestClass.class.getName());
        setter.setOptionValue("class", AnotherTestCase.class.getName());
        // Filter everything out
        mHostTest.addExcludeFilter(Junit4TestClass.class.getName());
        mHostTest.addExcludeFilter(AnotherTestCase.class.getName());

        Collection<IRemoteTest> tests = mHostTest.split(6);
        assertEquals(2, tests.size());
        for (IRemoteTest test : tests) {
            assertTrue(test instanceof HostTest);
            assertEquals(0L, ((HostTest) test).getRuntimeHint());
            assertEquals(0, ((HostTest) test).countTestCases());
        }
    }
}
