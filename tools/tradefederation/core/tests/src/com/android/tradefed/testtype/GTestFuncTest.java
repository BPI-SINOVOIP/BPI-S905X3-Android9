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

import com.android.ddmlib.Log;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import org.easymock.EasyMock;

import java.io.IOException;
import java.util.HashMap;


/**
 * Functional tests for {@link InstrumentationTest}.
 */
public class GTestFuncTest extends DeviceTestCase {

    private static final String LOG_TAG = "GTestFuncTest";
    private GTest mGTest = null;
    private ITestInvocationListener mMockListener = null;

    // Native test app constants
    public static final String NATIVE_TESTAPP_GTEST_CLASSNAME = "TradeFedNativeAppTest";
    public static final String NATIVE_TESTAPP_MODULE_NAME = "tfnativetests";
    public static final String NATIVE_TESTAPP_GTEST_CRASH_METHOD = "testNullPointerCrash";
    public static final String NATIVE_TESTAPP_GTEST_TIMEOUT_METHOD = "testInfiniteLoop";
    public static final int NATIVE_TESTAPP_TOTAL_TESTS = 2;

    private static final String NATIVE_SAMPLETEST_MODULE_NAME = "tfnativetestsamplelibtests";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mGTest = new GTest();
        mGTest.setDevice(getDevice());
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
    }

    /**
     * Test normal run of the sample native test project (7 tests, one of which is a failure).
     */
    @SuppressWarnings("unchecked")
    public void testRun() throws DeviceNotAvailableException {
        HashMap<String, Metric> emptyMap = new HashMap<>();
        mGTest.setModuleName(NATIVE_SAMPLETEST_MODULE_NAME);
        Log.i(LOG_TAG, "testRun");
        mMockListener.testRunStarted(NATIVE_SAMPLETEST_MODULE_NAME, 7);
        String[][] allTests = {
                {"FibonacciTest", "testRecursive_One"},
                {"FibonacciTest", "testRecursive_Ten"},
                {"FibonacciTest", "testIterative_Ten"},
                {"CelciusToFarenheitTest", "testNegative"},
                {"CelciusToFarenheitTest", "testPositive"},
                {"FarenheitToCelciusTest", "testExactFail"},
                {"FarenheitToCelciusTest", "testApproximatePass"},
        };
        for (String[] test : allTests) {
            String testClass = test[0];
            String testName = test[1];
            TestDescription id = new TestDescription(testClass, testName);
            mMockListener.testStarted(id);

            if (testName.endsWith("Fail")) {
              mMockListener.testFailed(
                      EasyMock.eq(id),
                      EasyMock.isA(String.class));
            }
            mMockListener.testEnded(id, emptyMap);
        }
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mMockListener);
        mGTest.run(mMockListener);
        EasyMock.verify(mMockListener);
    }

    /**
     * Helper to run tests in the Native Test App.
     *
     * @param testId the {%link TestDescription} of the test to run
     */
    @SuppressWarnings("unchecked")
    private void doNativeTestAppRunSingleTestFailure(TestDescription testId) {
        HashMap<String, Metric> emptyMap = new HashMap<>();
        mGTest.setModuleName(NATIVE_TESTAPP_MODULE_NAME);
        mMockListener.testRunStarted(NATIVE_TESTAPP_MODULE_NAME, 1);
        mMockListener.testStarted(EasyMock.eq(testId));
        mMockListener.testFailed(EasyMock.eq(testId),
                EasyMock.isA(String.class));
        mMockListener.testEnded(EasyMock.eq(testId), EasyMock.eq(emptyMap));
        mMockListener.testRunFailed((String)EasyMock.anyObject());
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mMockListener);
    }

    /**
     * Test run scenario where test process crashes while trying to access NULL ptr.
     */
    public void testRun_testCrash() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testRun_testCrash");
        TestDescription testId =
                new TestDescription(
                        NATIVE_TESTAPP_GTEST_CLASSNAME, NATIVE_TESTAPP_GTEST_CRASH_METHOD);
        doNativeTestAppRunSingleTestFailure(testId);
        // Set GTest to only run the crash test
        mGTest.addIncludeFilter(NATIVE_TESTAPP_GTEST_CRASH_METHOD);

        mGTest.run(mMockListener);
        EasyMock.verify(mMockListener);
    }

    /**
     * Test run scenario where device reboots during test run.
     */
    public void testRun_deviceReboot() throws Exception {
        Log.i(LOG_TAG, "testRun_deviceReboot");

        TestDescription testId =
                new TestDescription(
                        NATIVE_TESTAPP_GTEST_CLASSNAME, NATIVE_TESTAPP_GTEST_TIMEOUT_METHOD);

        doNativeTestAppRunSingleTestFailure(testId);

        // Set GTest to only run the crash test
        mGTest.addIncludeFilter(NATIVE_TESTAPP_GTEST_TIMEOUT_METHOD);

        // fork off a thread to do the reboot
        Thread rebootThread = new Thread() {
            @Override
            public void run() {
                // wait for test run to begin
                try {
                    Thread.sleep(500);
                    Runtime.getRuntime().exec(
                            String.format("adb -s %s reboot", getDevice().getIDevice()
                                    .getSerialNumber()));
                } catch (InterruptedException e) {
                    Log.w(LOG_TAG, "interrupted");
                } catch (IOException e) {
                    Log.w(LOG_TAG, "IOException when rebooting");
                }
            }
        };
        rebootThread.start();
        mGTest.run(mMockListener);
        getDevice().waitForDeviceAvailable();
        EasyMock.verify(mMockListener);
    }

    /**
     * Test run scenario where test timesout.
     */
    public void testRun_timeout() throws Exception {
        Log.i(LOG_TAG, "testRun_timeout");

        TestDescription testId =
                new TestDescription(
                        NATIVE_TESTAPP_GTEST_CLASSNAME, NATIVE_TESTAPP_GTEST_TIMEOUT_METHOD);
        // set max time to a small amount to reduce this test's execution time
        mGTest.setMaxTestTimeMs(100);
        doNativeTestAppRunSingleTestFailure(testId);

        // Set GTest to only run the timeout test
        mGTest.addIncludeFilter(NATIVE_TESTAPP_GTEST_TIMEOUT_METHOD);

        mGTest.run(mMockListener);
        getDevice().waitForDeviceAvailable();
        EasyMock.verify(mMockListener);
    }
}
