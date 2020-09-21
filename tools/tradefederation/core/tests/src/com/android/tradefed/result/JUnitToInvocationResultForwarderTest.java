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
package com.android.tradefed.result;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.testtype.DeviceTestCase;

import junit.framework.AssertionFailedError;
import junit.framework.TestCase;

import org.easymock.Capture;
import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.lang.annotation.ElementType;
import java.lang.annotation.Inherited;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.util.HashMap;

/** Unit tests for {@link JUnitToInvocationResultForwarder}. */
@RunWith(JUnit4.class)
public class JUnitToInvocationResultForwarderTest {

    private ITestInvocationListener mListener;
    private JUnitToInvocationResultForwarder mForwarder;

    @Before
    public void setUp() throws Exception {
        mListener = EasyMock.createMock(ITestInvocationListener.class);
        mForwarder = new JUnitToInvocationResultForwarder(mListener);
    }

    /** Inherited test annotation to ensure we properly collected it. */
    @Target(ElementType.METHOD)
    @Inherited
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyCustomAnnotation {}

    /** Base test class with a test method annotated with an inherited annotation. */
    public class BaseTestClass extends TestCase {
        @MyCustomAnnotation
        public void testbaseWithAnnotation() {}
    }

    /** Extension of the base test class. */
    public class InheritingClass extends BaseTestClass {}

    /**
     * Test method for {@link JUnitToInvocationResultForwarder#addFailure(junit.framework.Test,
     * AssertionFailedError)}.
     */
    @Test
    public void testAddFailure() {
        final AssertionFailedError a = new AssertionFailedError();
        mListener.testFailed(
                EasyMock.eq(new TestDescription(DeviceTestCase.class.getName(), "testAddFailure")),
                (String) EasyMock.anyObject());
        EasyMock.replay(mListener);
        DeviceTestCase test = new DeviceTestCase();
        test.setName("testAddFailure");
        mForwarder.addFailure(test, a);
        EasyMock.verify(mListener);
    }

    /** Test method for {@link JUnitToInvocationResultForwarder#endTest(junit.framework.Test)}. */
    @Test
    public void testEndTest() {
        HashMap<String, Metric> emptyMap = new HashMap<>();
        mListener.testEnded(
                EasyMock.eq(new TestDescription(DeviceTestCase.class.getName(), "testEndTest")),
                EasyMock.eq(emptyMap));
        DeviceTestCase test = new DeviceTestCase();
        test.setName("testEndTest");
        EasyMock.replay(mListener);
        mForwarder.endTest(test);
        EasyMock.verify(mListener);
    }

    /** Test method for {@link JUnitToInvocationResultForwarder#startTest(junit.framework.Test)}. */
    @Test
    public void testStartTest() {
        mListener.testStarted(
                EasyMock.eq(new TestDescription(DeviceTestCase.class.getName(), "testStartTest")));
        DeviceTestCase test = new DeviceTestCase();
        test.setName("testStartTest");
        EasyMock.replay(mListener);
        mForwarder.startTest(test);
        EasyMock.verify(mListener);
    }

    /**
     * Test method for {@link JUnitToInvocationResultForwarder#startTest(junit.framework.Test)} when
     * the test method is inherited.
     */
    @Test
    public void testStartTest_annotations() {
        Capture<TestDescription> capture = new Capture<>();
        mListener.testStarted(EasyMock.capture(capture));
        InheritingClass test = new InheritingClass();
        test.setName("testbaseWithAnnotation");
        EasyMock.replay(mListener);
        mForwarder.startTest(test);
        EasyMock.verify(mListener);
        TestDescription desc = capture.getValue();
        assertEquals(InheritingClass.class.getName(), desc.getClassName());
        assertEquals("testbaseWithAnnotation", desc.getTestName());
        assertEquals(1, desc.getAnnotations().size());
        // MyCustomAnnotation is inherited
        assertTrue(desc.getAnnotations().iterator().next() instanceof MyCustomAnnotation);
    }
}
