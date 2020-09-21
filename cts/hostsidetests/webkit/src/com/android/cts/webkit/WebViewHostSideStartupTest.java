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
package com.android.cts.webkit;

import android.platform.test.annotations.AppModeFull;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.DeviceTestCase;

import java.util.Collection;

public class WebViewHostSideStartupTest extends DeviceTestCase {
    private static final String RUNNER = "android.support.test.runner.AndroidJUnitRunner";

    private static final String DEVICE_WEBVIEW_STARTUP_PKG = "com.android.cts.webkit";
    private static final String DEVICE_WEBVIEW_STARTUP_TEST_CLASS = "WebViewDeviceSideStartupTest";
    @Override
    protected void setUp() throws Exception {
        super.setUp();
    }

    @AppModeFull(reason = "Instant apps cannot open TCP sockets.")
    public void testCookieManager() throws DeviceNotAvailableException {
        assertDeviceTestPasses("testCookieManagerBlockingUiThread");
    }

    public void testWebViewVersionApiOnUiThread() throws DeviceNotAvailableException {
        assertDeviceTestPasses("testGetCurrentWebViewPackageOnUiThread");
    }

    public void testWebViewVersionApi() throws DeviceNotAvailableException {
        assertDeviceTestPasses("testGetCurrentWebViewPackage");
    }

    public void testStrictMode() throws DeviceNotAvailableException {
        assertDeviceTestPasses("testStrictModeNotViolatedOnStartup");
    }

    public void testGetWebViewLooperOnUiThread() throws DeviceNotAvailableException {
        assertDeviceTestPasses("testGetWebViewLooperOnUiThread");
    }

    public void testGetWebViewLooperFromUiThread() throws DeviceNotAvailableException {
        assertDeviceTestPasses("testGetWebViewLooperCreatedOnUiThreadFromInstrThread");
    }

    public void testGetWebViewLooperCreatedOnBackgroundThreadFromInstThread()
            throws DeviceNotAvailableException {
        assertDeviceTestPasses("testGetWebViewLooperCreatedOnBackgroundThreadFromInstThread");
    }

    private void assertDeviceTestPasses(String testMethodName) throws DeviceNotAvailableException {
        TestRunResult testRunResult = runSingleDeviceTest(DEVICE_WEBVIEW_STARTUP_PKG,
                                                 DEVICE_WEBVIEW_STARTUP_TEST_CLASS,
                                                 testMethodName);
        assertTrue(testRunResult.isRunComplete());

        Collection<TestResult> testResults = testRunResult.getTestResults().values();
        // We're only running a single test.
        assertEquals(1, testResults.size());
        TestResult testResult = testResults.toArray(new TestResult[1])[0];
        assertTrue(testResult.getStackTrace(), TestStatus.PASSED.equals(testResult.getStatus()));
    }

    private TestRunResult runSingleDeviceTest(String packageName, String testClassName,
            String testMethodName) throws DeviceNotAvailableException {
        testClassName = packageName + "." + testClassName;

        RemoteAndroidTestRunner testRunner = new RemoteAndroidTestRunner(
                packageName, RUNNER, getDevice().getIDevice());
        testRunner.setMethodName(testClassName, testMethodName);

        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTests(testRunner, listener));

        return listener.getCurrentRunResults();
    }
}
