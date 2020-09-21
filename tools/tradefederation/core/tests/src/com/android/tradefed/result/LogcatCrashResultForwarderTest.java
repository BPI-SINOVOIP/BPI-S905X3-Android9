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
package com.android.tradefed.result;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.HashMap;

/** Unit tests for {@link LogcatCrashResultForwarder}. */
@RunWith(JUnit4.class)
public class LogcatCrashResultForwarderTest {
    private LogcatCrashResultForwarder mReporter;
    private ITestInvocationListener mMockListener;
    private ITestDevice mMockDevice;

    @Before
    public void setUp() {
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
    }

    /** Test if a crash is detected but no crash is found in the logcat. */
    @Test
    @SuppressWarnings("MustBeClosedChecker")
    public void testCaptureTestCrash_noCrashInLogcat() {
        mReporter = new LogcatCrashResultForwarder(mMockDevice, mMockListener);
        TestDescription test = new TestDescription("com.class", "test");

        mMockListener.testStarted(test, 0L);
        EasyMock.expect(mMockDevice.getLogcatSince(0L))
                .andReturn(new ByteArrayInputStreamSource("".getBytes()));
        mMockListener.testFailed(test, "instrumentation failed. reason: 'Process crashed.'");
        mMockListener.testEnded(test, 5L, new HashMap<String, Metric>());

        EasyMock.replay(mMockListener, mMockDevice);
        mReporter.testStarted(test, 0L);
        mReporter.testFailed(test, "instrumentation failed. reason: 'Process crashed.'");
        mReporter.testEnded(test, 5L, new HashMap<String, Metric>());
        EasyMock.verify(mMockListener, mMockDevice);
    }

    /**
     * Test that if a crash is detected and found in the logcat, we add the extracted information to
     * the failure.
     */
    @Test
    @SuppressWarnings("MustBeClosedChecker")
    public void testCaptureTestCrash_oneCrashingLogcat() {
        mReporter = new LogcatCrashResultForwarder(mMockDevice, mMockListener);
        TestDescription test = new TestDescription("com.class", "test");

        mMockListener.testStarted(test, 0L);
        String logcat =
                "03-20 09:57:36.709 11 11 E AndroidRuntime: FATAL EXCEPTION: Thread-2\n"
                        + "03-20 09:57:36.709 11 11 E AndroidRuntime: Process: android.gesture.cts"
                        + ", PID: 11034\n"
                        + "03-20 09:57:36.709 11 11 E AndroidRuntime: java.lang.RuntimeException:"
                        + " Runtime\n"
                        + "03-20 09:57:36.709 11 11 E AndroidRuntime:    at android.GestureTest$1"
                        + ".run(GestureTest.java:52)\n"
                        + "03-20 09:57:36.709 11 11 E AndroidRuntime:    at java.lang.Thread.run"
                        + "(Thread.java:764)\n"
                        + "03-20 09:57:36.711 11 11 I TestRunner: started: testGetStrokesCount"
                        + "(android.gesture.cts.GestureTest)\n";

        EasyMock.expect(mMockDevice.getLogcatSince(0L))
                .andReturn(new ByteArrayInputStreamSource(logcat.getBytes()));
        // Some crash was added to the failure
        mMockListener.testFailed(
                EasyMock.eq(test),
                EasyMock.contains(
                        "instrumentation failed. reason: 'Process crashed.'"
                                + "\nCrash Message:Runtime"));
        mMockListener.testEnded(test, 5L, new HashMap<String, Metric>());
        // If a run failure follows, expect it to contain the additional stack too.
        mMockListener.testRunFailed(
                EasyMock.contains("Something went wrong.\nCrash Message:Runtime"));

        EasyMock.replay(mMockListener, mMockDevice);
        mReporter.testStarted(test, 0L);
        mReporter.testFailed(test, "instrumentation failed. reason: 'Process crashed.'");
        mReporter.testEnded(test, 5L, new HashMap<String, Metric>());
        mReporter.testRunFailed("Something went wrong.");
        EasyMock.verify(mMockListener, mMockDevice);
    }

    /**
     * Test that if a crash is note detected at testFailed but later found at testRunFailed, we
     * still add the extracted information to the failure.
     */
    @Test
    @SuppressWarnings("MustBeClosedChecker")
    public void testCaptureTestCrash_oneCrashingLogcatAfterTestEnded() {
        mReporter = new LogcatCrashResultForwarder(mMockDevice, mMockListener);
        TestDescription test = new TestDescription("com.class", "test");

        mMockListener.testStarted(test, 0L);
        String logcat =
                "03-20 09:57:36.709 11 11 E AndroidRuntime: FATAL EXCEPTION: Thread-2\n"
                        + "03-20 09:57:36.709 11 11 E AndroidRuntime: Process: android.gesture.cts"
                        + ", PID: 11034\n"
                        + "03-20 09:57:36.709 11 11 E AndroidRuntime: java.lang.RuntimeException:"
                        + " Runtime\n"
                        + "03-20 09:57:36.709 11 11 E AndroidRuntime:    at android.GestureTest$1"
                        + ".run(GestureTest.java:52)\n"
                        + "03-20 09:57:36.709 11 11 E AndroidRuntime:    at java.lang.Thread.run"
                        + "(Thread.java:764)\n"
                        + "03-20 09:57:36.711 11 11 I TestRunner: started: testGetStrokesCount"
                        + "(android.gesture.cts.GestureTest)\n";

        EasyMock.expect(mMockDevice.getLogcatSince(0L))
                .andReturn(new ByteArrayInputStreamSource(logcat.getBytes()));
        // No crash added at the point of testFailed.
        mMockListener.testFailed(test, "Something went wrong.");
        mMockListener.testEnded(test, 5L, new HashMap<String, Metric>());
        // If a run failure comes with a crash detected, expect it to contain the additional stack.
        mMockListener.testRunFailed(
                EasyMock.contains(
                        "instrumentation failed. reason: 'Process crashed.'"
                                + "\nCrash Message:Runtime"));

        EasyMock.replay(mMockListener, mMockDevice);
        mReporter.testStarted(test, 0L);
        mReporter.testFailed(test, "Something went wrong.");
        mReporter.testEnded(test, 5L, new HashMap<String, Metric>());
        mReporter.testRunFailed("instrumentation failed. reason: 'Process crashed.'");
        EasyMock.verify(mMockListener, mMockDevice);
    }
}
