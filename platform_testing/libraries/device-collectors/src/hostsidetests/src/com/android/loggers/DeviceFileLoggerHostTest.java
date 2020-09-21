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
package com.android.loggers;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;

import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.AndroidJUnitTest;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.Collection;

/**
 * Test the device side file logger to ensure we receive the information from the device
 * instrumentation.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class DeviceFileLoggerHostTest extends BaseHostJUnit4Test {
    private static final String TEST_APK = "CollectorDeviceLibTest.apk";
    private static final String PACKAGE_NAME = "android.device.collectors";
    private static final String AJUR_RUNNER = "android.support.test.runner.AndroidJUnitRunner";

    private static final String FILE_LOGGER = "android.device.loggers.LogFileLogger";

    private RemoteAndroidTestRunner mTestRunner;
    private IInvocationContext mContext;

    @Before
    public void setUp() throws DeviceNotAvailableException, TargetSetupError {
        installPackage(TEST_APK);
        assertTrue(isPackageInstalled(PACKAGE_NAME));
        mTestRunner =
                new RemoteAndroidTestRunner(PACKAGE_NAME, AJUR_RUNNER, getDevice().getIDevice());
        // Set the new runListener order to ensure test cases can show their metrics.
        mTestRunner.addInstrumentationArg(AndroidJUnitTest.NEW_RUN_LISTENER_ORDER_KEY, "true");
        mContext = mock(IInvocationContext.class);
        doReturn(Arrays.asList(getDevice())).when(mContext).getDevices();
        doReturn(Arrays.asList(getBuild())).when(mContext).getBuildInfos();
    }

    @Test
    public void testFileIsLogged() throws Exception {
        mTestRunner.addInstrumentationArg("listener", FILE_LOGGER);
        mTestRunner.setClassName("android.device.loggers.test.StubInstrumentationAnnotatedTest");
        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTests(mTestRunner, listener));

        Collection<TestRunResult> results = listener.getRunResults();
        assertEquals(1, results.size());
        TestRunResult result = results.iterator().next();
        assertFalse(result.isRunFailure());
        assertFalse(result.hasFailedTests());
        // Check that each test case has results with the metrics associated.
        for (TestResult res : result.getTestResults().values()) {
            assertTrue(res.getMetrics().containsKey("fake_file"));
            assertEquals("/fake/path", res.getMetrics().get("fake_file"));
        }
    }
}
