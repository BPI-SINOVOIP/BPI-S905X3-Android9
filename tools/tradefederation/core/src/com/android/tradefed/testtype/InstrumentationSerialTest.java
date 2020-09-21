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

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestRunResult;

import java.util.Collection;
import java.util.HashMap;

/**
 * A Test that runs a set of instrumentation tests by running one adb command for per test.
 */
class InstrumentationSerialTest implements IRemoteTest {

    /** number of attempts to make if test fails to run */
    static final int FAILED_RUN_TEST_ATTEMPTS = 2;

    /** the set of tests to run */
    private final Collection<TestDescription> mTests;

    private final InstrumentationTest mInstrumentationTest;

    /**
     * Creates a {@link InstrumentationSerialTest}.
     *
     * @param instrumentationTest {@link InstrumentationTest} used to configure this class
     * @param testsToRun a {@link Collection} of tests to run. Note this {@link Collection} will be
     *     used as is (ie a reference to the testsToRun object will be kept).
     */
    InstrumentationSerialTest(
            InstrumentationTest instrumentationTest, Collection<TestDescription> testsToRun)
            throws ConfigurationException {
        // reuse the InstrumentationTest class to perform actual test run
        mInstrumentationTest = createInstrumentationTest(instrumentationTest);
        // keep local copy of tests to be run
        mTests = testsToRun;
    }

    /**
     * Create and initialize new instance of {@link InstrumentationTest}. Exposed for unit testing.
     *
     * @param instrumentationTest  {@link InstrumentationTest} used to configure this class
     * @return  the newly created {@link InstrumentationTest}
     */
    InstrumentationTest createInstrumentationTest(InstrumentationTest instrumentationTest)
            throws ConfigurationException {
        InstrumentationTest runner = new InstrumentationTest();
        OptionCopier.copyOptions(instrumentationTest, runner);
        runner.setDevice(instrumentationTest.getDevice());
        runner.setForceAbi(instrumentationTest.getForceAbi());
        // ensure testFile is not used.
        runner.setReRunUsingTestFile(false);
        // no need to rerun when executing tests one by one
        runner.setRerunMode(false);
        return runner;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(final ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (mInstrumentationTest.getDevice() == null) {
            throw new IllegalArgumentException("Device has not been set");
        }
        // reuse the InstrumentationTest class to perform actual test run
        try {
            for (TestDescription testToRun : mTests) {
                InstrumentationTest runner = createInstrumentationTest(mInstrumentationTest);
                runner.setClassName(testToRun.getClassName());
                runner.setMethodName(testToRun.getTestName());
                runTest(runner, listener, testToRun);
            }
        } catch (ConfigurationException e) {
            CLog.e("Failed to create new InstrumentationTest: %s", e.getMessage());
        }
    }

    private void runTest(
            InstrumentationTest runner, ITestInvocationListener listener, TestDescription testToRun)
            throws DeviceNotAvailableException {
        // use a listener filter, to track if the test failed to run
        CollectingTestListener trackingListener = new CollectingTestListener();
        for (int i=1; i <= FAILED_RUN_TEST_ATTEMPTS; i++) {
            runner.run(new ResultForwarder(trackingListener, listener));
            if (trackingListener.getCurrentRunResults().getTestResults().containsKey(testToRun)) {
                return;
            }
            CLog.w(
                    "Expected test %s did not run on attempt %d of %d",
                    testToRun, i, FAILED_RUN_TEST_ATTEMPTS);
        }
        markTestAsFailed(testToRun, trackingListener.getCurrentRunResults(), listener);
    }

    private void markTestAsFailed(
            TestDescription test, TestRunResult testRun, ITestInvocationListener listener) {
        listener.testRunStarted(testRun.getName(), 1);
        listener.testStarted(test);

        String message =
                testRun.isRunFailure()
                        ? testRun.getRunFailureMessage()
                        : "The test was not initialized by the test runner.";
        listener.testFailed(
                test, String.format("Test failed to run. Test run failed due to : %s", message));

        if (testRun.isRunFailure()) {
            listener.testRunFailed(message);
        }
        listener.testEnded(test, new HashMap<String, Metric>());
        listener.testRunEnded(0, new HashMap<String, Metric>());
    }
}
