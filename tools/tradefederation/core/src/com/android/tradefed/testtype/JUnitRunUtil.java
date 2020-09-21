/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.JUnitToInvocationResultForwarder;
import com.android.tradefed.testtype.DeviceTestResult.RuntimeDeviceNotAvailableException;

import junit.framework.Test;
import junit.framework.TestResult;

import java.util.HashMap;

/**
 * A helper class for directing a {@link IRemoteTest#run(ITestInvocationListener)} call to a
 * {@link Test#run(TestResult)} call.
 */
public class JUnitRunUtil {

    public static void runTest(ITestInvocationListener listener, Test junitTest)
            throws DeviceNotAvailableException {
        runTest(listener, junitTest, junitTest.getClass().getName());
    }

    public static void runTest(ITestInvocationListener listener, Test junitTest,
            String runName) throws DeviceNotAvailableException {
        if (junitTest.countTestCases() == 0) {
            CLog.v("Skipping empty test case %s", runName);
            return;
        }
        listener.testRunStarted(runName, junitTest.countTestCases());
        long startTime = System.currentTimeMillis();
        // forward the JUnit results to the invocation listener
        JUnitToInvocationResultForwarder resultForwarder =
                new JUnitToInvocationResultForwarder(listener);
        DeviceTestResult result = new DeviceTestResult();
        result.addListener(resultForwarder);
        try {
            junitTest.run(result);
        } catch (RuntimeDeviceNotAvailableException e) {
            listener.testRunFailed(e.getDeviceException().getMessage());
            throw e.getDeviceException();
        } finally {
            listener.testRunEnded(
                    System.currentTimeMillis() - startTime, new HashMap<String, Metric>());
        }
    }
}
