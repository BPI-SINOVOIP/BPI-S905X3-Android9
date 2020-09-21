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

package com.android.performance.tests;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.AaptParser;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

@OptionClass(alias = "app-install-perf")
// Test framework that measures the install time for all apk files located under a given directory.
// The test needs aapt to be in its path in order to determine the package name of the apk. The
// package name is needed to clean up after the test is done.
public class AppInstallTest implements IDeviceTest, IRemoteTest {

    @Option(name = "test-apk-dir", description = "Directory that contains the test apks.",
            importance= Option.Importance.ALWAYS)
    private String mTestApkPath;

    @Option(name = "test-label", description = "Unique test identifier label.")
    private String mTestLabel = "AppInstallPerformance";

    @Option(name = "test-start-delay",
            description = "Delay in ms to wait for before starting the install test.")
    private long mTestStartDelay = 60000;

    private ITestDevice mDevice;

    /*
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /*
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /*
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // Delay test start time to give the background processes to finish.
        if (mTestStartDelay > 0) {
            try {
                Thread.sleep(mTestStartDelay);
            } catch (InterruptedException e) {
                CLog.e("Failed to delay test: %s", e.toString());
            }
        }
        Assert.assertFalse(mTestApkPath.isEmpty());
        File apkDir = new File(mTestApkPath);
        Assert.assertTrue(apkDir.isDirectory());
        // Find all apks in directory.
        String[] files = apkDir.list();
        Map<String, String> metrics = new HashMap<String, String> ();
        try {
            for (String fileName : files) {
                if (!fileName.endsWith(".apk")) {
                    CLog.d("Skipping non-apk %s", fileName);
                    continue;
                }
                File file = new File(apkDir, fileName);
                // Install app and measure time.
                String installTime = Long.toString(installAndTime(file));
                metrics.put(fileName, installTime);
            }
        } finally {
            reportMetrics(listener, mTestLabel, metrics);
        }
    }

    /**
     * Install file and time its install time. Cleans up after itself.
     * @param packageFile apk file to install
     * @return install time in msecs.
     * @throws DeviceNotAvailableException
     */
    long installAndTime(File packageFile) throws DeviceNotAvailableException {
        AaptParser parser = AaptParser.parse(packageFile);
        String packageName = parser.getPackageName();

        String remotePath = "/data/local/tmp/" + packageFile.getName();
        if (!mDevice.pushFile(packageFile, remotePath)) {
            throw new RuntimeException("Failed to push " + packageFile.getAbsolutePath());
        }
        long start = System.currentTimeMillis();
        String output = mDevice.executeShellCommand(
                String.format("pm install -r \"%s\"", remotePath));
        long end = System.currentTimeMillis();
        if (output == null || output.indexOf("Success") == -1) {
            CLog.e("Failed to install package %s with error %s", packageFile, output);
            return -1;
        }
        mDevice.executeShellCommand(String.format("rm \"%s\"", remotePath));
        if (packageName != null) {
            CLog.d("Uninstalling: %s", packageName);
            mDevice.uninstallPackage(packageName);
        }
        return end - start;
    }

    /**
     * Report run metrics by creating an empty test run to stick them in
     *
     * @param listener the {@link ITestInvocationListener} of test results
     * @param runName the test name
     * @param metrics the {@link Map} that contains metrics for the given test
     */
    void reportMetrics(ITestInvocationListener listener, String runName,
            Map<String, String> metrics) {
        // Create an empty testRun to report the parsed runMetrics
        CLog.d("About to report metrics: %s", metrics);
        listener.testRunStarted(runName, 0);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(metrics));
    }
}
