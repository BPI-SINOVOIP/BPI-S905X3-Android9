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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.fail;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import junit.framework.TestCase;

import org.easymock.Capture;
import org.easymock.EasyMock;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Unit tests for {@link DeviceTestCase}. */
@RunWith(JUnit4.class)
public class DeviceTestCaseTest {

    public static class MockTest extends DeviceTestCase {

        public void test1() {
            // test adding a metric during the test.
            addTestMetric("test", "value");
        }
        public void test2() {}
    }

    @MyAnnotation1
    public static class MockAnnotatedTest extends DeviceTestCase {

        @MyAnnotation1
        public void test1() {}
        @MyAnnotation2
        public void test2() {}
    }

    /** A test class that illustrate duplicate names but from only one real test. */
    public static class DuplicateTest extends DeviceTestCase {

        public void test1() {
            test1("");
        }

        private void test1(String arg) {
            assertTrue(arg.isEmpty());
        }

        public void test2() {}
    }

    /**
     * Simple Annotation class for testing
     */
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyAnnotation1 {
    }

    /**
     * Simple Annotation class for testing
     */
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyAnnotation2 {
    }

    public static class MockAbortTest extends DeviceTestCase {

        private static final String EXCEP_MSG = "failed";
        private static final String FAKE_SERIAL = "fakeserial";

        public void test1() throws DeviceNotAvailableException {
            throw new DeviceNotAvailableException(EXCEP_MSG, FAKE_SERIAL);
        }
    }

    /** Verify that calling run on a DeviceTestCase will run all test methods. */
    @Test
    public void testRun_suite() throws Exception {
        MockTest test = new MockTest();

        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        listener.testRunStarted(MockTest.class.getName(), 2);
        final TestDescription test1 = new TestDescription(MockTest.class.getName(), "test1");
        final TestDescription test2 = new TestDescription(MockTest.class.getName(), "test2");
        listener.testStarted(test1);
        Map<String, String> metrics = new HashMap<>();
        metrics.put("test", "value");
        listener.testEnded(test1, TfMetricProtoUtil.upgradeConvert(metrics));
        listener.testStarted(test2);
        listener.testEnded(test2, new HashMap<String, Metric>());
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);

        test.run(listener);
        EasyMock.verify(listener);
    }

    /**
     * Verify that calling run on a {@link DeviceTestCase} will only run methods included by
     * filtering.
     */
    @Test
    public void testRun_includeFilter() throws Exception {
        MockTest test = new MockTest();
        test.addIncludeFilter("com.android.tradefed.testtype.DeviceTestCaseTest$MockTest#test1");
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        listener.testRunStarted(MockTest.class.getName(), 1);
        final TestDescription test1 = new TestDescription(MockTest.class.getName(), "test1");
        listener.testStarted(test1);
        Map<String, String> metrics = new HashMap<>();
        metrics.put("test", "value");
        listener.testEnded(test1, TfMetricProtoUtil.upgradeConvert(metrics));
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);

        test.run(listener);
        EasyMock.verify(listener);
    }

    /**
     * Verify that calling run on a {@link DeviceTestCase} will not run methods excluded by
     * filtering.
     */
    @Test
    public void testRun_excludeFilter() throws Exception {
        MockTest test = new MockTest();
        test.addExcludeFilter("com.android.tradefed.testtype.DeviceTestCaseTest$MockTest#test1");
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        listener.testRunStarted(MockTest.class.getName(), 1);
        final TestDescription test2 = new TestDescription(MockTest.class.getName(), "test2");
        listener.testStarted(test2);
        listener.testEnded(test2, new HashMap<String, Metric>());
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);

        test.run(listener);
        EasyMock.verify(listener);
    }

    /**
     * Verify that calling run on a {@link DeviceTestCase} only runs AnnotatedElements included by
     * filtering.
     */
    @Test
    public void testRun_includeAnnotationFiltering() throws Exception {
        MockAnnotatedTest test = new MockAnnotatedTest();
        test.addIncludeAnnotation(
                "com.android.tradefed.testtype.DeviceTestCaseTest$MyAnnotation1");
        test.addExcludeAnnotation("com.android.tradefed.testtype.DeviceTestCaseTest$MyAnnotation2");
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        listener.testRunStarted(MockAnnotatedTest.class.getName(), 1);
        final TestDescription test1 =
                new TestDescription(MockAnnotatedTest.class.getName(), "test1");
        listener.testStarted(test1);
        listener.testEnded(test1, new HashMap<String, Metric>());
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);

        test.run(listener);
        EasyMock.verify(listener);
    }

    /** Verify that we properly carry the annotations of the methods. */
    @Test
    public void testRun_checkAnnotation() throws Exception {
        MockAnnotatedTest test = new MockAnnotatedTest();
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        listener.testRunStarted(MockAnnotatedTest.class.getName(), 2);
        Capture<TestDescription> capture = new Capture<>();
        listener.testStarted(EasyMock.capture(capture));
        listener.testEnded(
                EasyMock.capture(capture), (HashMap<String, Metric>) EasyMock.anyObject());
        listener.testStarted(EasyMock.capture(capture));
        listener.testEnded(
                EasyMock.capture(capture), (HashMap<String, Metric>) EasyMock.anyObject());
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);

        test.run(listener);
        EasyMock.verify(listener);

        List<TestDescription> descriptions = capture.getValues();
        // Ensure we properly capture the annotations for both methods.
        for (TestDescription desc : descriptions) {
            assertFalse(desc.getAnnotations().isEmpty());
        }
    }

    /**
     * Verify that calling run on a {@link DeviceTestCase} does not run AnnotatedElements excluded
     * by filtering.
     */
    @Test
    public void testRun_excludeAnnotationFiltering() throws Exception {
        MockAnnotatedTest test = new MockAnnotatedTest();
        test.addExcludeAnnotation(
                "com.android.tradefed.testtype.DeviceTestCaseTest$MyAnnotation2");
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        listener.testRunStarted(MockAnnotatedTest.class.getName(), 1);
        final TestDescription test1 =
                new TestDescription(MockAnnotatedTest.class.getName(), "test1");
        listener.testStarted(test1);
        listener.testEnded(test1, new HashMap<String, Metric>());
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);

        test.run(listener);
        EasyMock.verify(listener);
    }

    /** Regression test to verify a single test can still be run. */
    @Test
    public void testRun_singleTest() throws DeviceNotAvailableException {
        MockTest test = new MockTest();
        test.setName("test1");

        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        listener.testRunStarted(MockTest.class.getName(), 1);
        final TestDescription test1 = new TestDescription(MockTest.class.getName(), "test1");
        listener.testStarted(test1);
        Map<String, String> metrics = new HashMap<>();
        metrics.put("test", "value");
        listener.testEnded(test1, TfMetricProtoUtil.upgradeConvert(metrics));
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);

        test.run(listener);
        EasyMock.verify(listener);
    }

    /** Verify that a device not available exception is thrown up. */
    @Test
    public void testRun_deviceNotAvail() {
        MockAbortTest test = new MockAbortTest();
        // create a mock ITestInvocationListener, because results are easier to verify
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);

        final TestDescription test1 = new TestDescription(MockAbortTest.class.getName(), "test1");
        listener.testRunStarted(MockAbortTest.class.getName(), 1);
        listener.testStarted(test1);
        listener.testFailed(EasyMock.eq(test1),
                EasyMock.contains(MockAbortTest.EXCEP_MSG));
        listener.testEnded(test1, new HashMap<String, Metric>());
        listener.testRunFailed(EasyMock.contains(MockAbortTest.EXCEP_MSG));
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);
        try {
            test.run(listener);
            fail("DeviceNotAvailableException not thrown");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
        EasyMock.verify(listener);
    }

    /**
     * Test success case for {@link DeviceTestCase#run(ITestInvocationListener)} in collector mode,
     * where test to run is a {@link TestCase}
     */
    @Test
    public void testRun_testcaseCollectMode() throws Exception {
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        MockTest test = new MockTest();
        test.setCollectTestsOnly(true);
        listener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(2));
        listener.testStarted(EasyMock.anyObject());
        listener.testEnded(EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        listener.testStarted(EasyMock.anyObject());
        listener.testEnded(EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);
        test.run(listener);
        EasyMock.verify(listener);
    }

    /**
     * Test success case for {@link DeviceTestCase#run(ITestInvocationListener)} in collector mode,
     * where test to run is a {@link TestCase}
     */
    @Test
    public void testRun_testcaseCollectMode_singleMethod() throws Exception {
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        MockTest test = new MockTest();
        test.setName("test1");
        test.setCollectTestsOnly(true);
        listener.testRunStarted((String)EasyMock.anyObject(), EasyMock.eq(1));
        listener.testStarted(EasyMock.anyObject());
        listener.testEnded(EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);
        test.run(listener);
        EasyMock.verify(listener);
    }

    /**
     * Test that when a test class has some private method with a method name we properly ignore it
     * and only consider the actual real method that can execute in the filtering.
     */
    @Test
    public void testRun_duplicateName() throws Exception {
        DuplicateTest test = new DuplicateTest();
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);

        listener.testRunStarted(DuplicateTest.class.getName(), 2);
        final TestDescription test1 = new TestDescription(DuplicateTest.class.getName(), "test1");
        final TestDescription test2 = new TestDescription(DuplicateTest.class.getName(), "test2");
        listener.testStarted(test1);
        listener.testEnded(test1, new HashMap<String, Metric>());
        listener.testStarted(test2);
        listener.testEnded(test2, new HashMap<String, Metric>());
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(listener);

        test.run(listener);
        EasyMock.verify(listener);
    }
}
