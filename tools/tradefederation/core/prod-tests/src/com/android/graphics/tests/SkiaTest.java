/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.graphics.tests;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IFileEntry;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.util.HashMap;

/**
 *  Test for running Skia native tests.
 *
 *  The test is not necessarily Skia specific, but it provides
 *  functionality that allows native Skia tests to be run.
 *
 *  Includes options to specify the Skia test app to run (inside
 *  nativetest directory), flags to pass to the test app, and a file
 *  to retrieve off the device after the test completes. (Skia test
 *  apps record their results to a json file, so retrieving this file
 *  allows us to view the results so long as the app completed.)
 */
@OptionClass(alias = "skia_native_tests")
public class SkiaTest implements IRemoteTest, IDeviceTest {
    private ITestDevice mDevice;

    static final String DEFAULT_NATIVETEST_PATH = "/data/nativetest";

    @Option(name = "native-test-device-path",
      description = "The path on the device where native tests are located.")
    private String mNativeTestDevicePath = DEFAULT_NATIVETEST_PATH;

    @Option(name = "skia-flags",
        description = "Flags to pass to the skia program.")
    private String mFlags = "";

    @Option(name = "skia-app",
        description = "Skia program to run.",
        mandatory = true)
    private String mSkiaApp = "";

    @Option(name = "skia-json",
        description = "Full path on device for json output file.")
    private File mOutputFile = null;

    @Option(name = "skia-pngs",
        description = "Directory on device for holding png results for retrieval.")
    private File mPngDir = null;

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (mDevice == null) {
            throw new IllegalArgumentException("Device has not been set");
        }

        listener.testRunStarted(mSkiaApp, 1);
        long start = System.currentTimeMillis();

        // Native Skia tests are in nativeTestDirectory/mSkiaApp/mSkiaApp.
        String fullPath = mNativeTestDevicePath + "/"
                + mSkiaApp + "/" + mSkiaApp;
        IFileEntry app = mDevice.getFileEntry(fullPath);
        TestDescription testId = new TestDescription(mSkiaApp, "testFileExists");
        listener.testStarted(testId);
        if (app == null) {
            CLog.w("Could not find test %s in %s!", fullPath, mDevice.getSerialNumber());
            listener.testFailed(testId, "Device does not have " + fullPath);
            listener.testEnded(testId, new HashMap<String, Metric>());
        } else {
            // The test for detecting the file has ended.
            listener.testEnded(testId, new HashMap<String, Metric>());
            prepareDevice();
            runTest(app);
            retrieveFiles(mSkiaApp, listener);
        }

        listener.testRunEnded(System.currentTimeMillis() - start, new HashMap<String, Metric>());
    }

    /**
     *  Emulates running mkdirs on an ITestDevice.
     *
     *  Creates the directory named by dir *on device*, recursively creating missing parent
     *  directories if necessary.
     *
     *  @param dir Directory to create.
     */
    private void mkdirs(File dir) throws DeviceNotAvailableException {
        if (dir == null || mDevice.doesFileExist(dir.getPath())) {
            return;
        }

        String dirName = dir.getPath();
        CLog.v("creating folder '%s'", dirName);
        mDevice.executeShellCommand("mkdir -p " + dirName);
    }

    /**
     *  Do pre-test setup on the device.
     *
     *  Setup involves ensuring necessary directories exist and removing old
     *  test result files.
     */
    private void prepareDevice() throws DeviceNotAvailableException {
        if (mOutputFile != null) {
            String path = mOutputFile.getPath();
            if (mDevice.doesFileExist(path)) {
                // Delete the file. We don't want to think this file from an
                // earlier run represents this one.
                CLog.v("Removing old file " + path);
                mDevice.executeShellCommand("rm " + path);
            } else {
                // Ensure its containing folder exists.
                mkdirs(mOutputFile.getParentFile());
            }
        }

        if (mPngDir != null) {
            String pngPath = mPngDir.getPath();
            if (mDevice.doesFileExist(pngPath)) {
                // Empty the old directory
                mDevice.executeShellCommand("rm -rf " + pngPath + "/*");
            } else {
                mkdirs(mPngDir);
            }
        }
    }

    /**
     * Retrieve a file from the device and upload it to the listener.
     *
     * <p>Each file for uploading is considered its own test, so we can track whether or not
     * uploading succeeded.
     *
     * @param remoteFile File on the device.
     * @param testIdClass String to be passed to TestDescription's constructor as className.
     * @param testIdMethod String passed to TestDescription's constructor as testName.
     * @param listener Listener for reporting test failure/success and uploading files.
     * @param type LogDataType of the file being uploaded.
     */
    private void retrieveAndUploadFile(
            File remoteFile,
            String testIdClass,
            String testIdMethod,
            ITestInvocationListener listener,
            LogDataType type)
            throws DeviceNotAvailableException {
        String remotePath = remoteFile.getPath();
        CLog.v("adb pull %s (using pullFile)", remotePath);
        File localFile = mDevice.pullFile(remotePath);

        TestDescription testId = new TestDescription(testIdClass, testIdMethod);
        listener.testStarted(testId);
        if (localFile == null) {
            listener.testFailed(testId, "Failed to pull " + remotePath);
        } else {
            CLog.v("pulled result file to " + localFile.getPath());
            try (FileInputStreamSource source = new FileInputStreamSource(localFile)) {
                // Use the original name, for clarity.
                listener.testLog(remoteFile.getName(), type, source);
            }
            if (!localFile.delete()) {
                CLog.w("Failed to delete temporary file %s", localFile.getPath());
            }
        }
        listener.testEnded(testId, new HashMap<String, Metric>());
    }

    /**
     *  Retrieve files from the device.
     *
     *  Report to the listener whether retrieving the files succeeded.
     *
     *  @param appName Name of the app.
     *  @param listener Listener for reporting results of file retrieval.
     */
    private void retrieveFiles(String appName,
            ITestInvocationListener listener) throws DeviceNotAvailableException {
        // FIXME: This could be achieved with DeviceFileReporter. Blocked on b/18408206.
        if (mOutputFile != null) {
            retrieveAndUploadFile(mOutputFile, appName, "outputJson", listener, LogDataType.TEXT);
        }

        if (mPngDir != null) {
            String pngDir = mPngDir.getPath();
            IFileEntry remotePngDir = mDevice.getFileEntry(pngDir);
            for (IFileEntry pngFile : remotePngDir.getChildren(false)) {
                if (pngFile.getName().endsWith("png")) {
                    retrieveAndUploadFile(new File(pngFile.getFullPath()),
                            "PngRetrieval", pngFile.getName(), listener, LogDataType.PNG);
                }
            }
        }
    }

    /**
     *  Run a test on a device.
     *
     *  @param app Test app to run.
     */
    private void runTest(IFileEntry app) throws DeviceNotAvailableException {
        String fullPath = app.getFullEscapedPath();
        // force file to be executable
        mDevice.executeShellCommand(String.format("chmod 755 %s", fullPath));

        // The device will not immediately capture logs in response to
        // startLogcat. Instead, it delays 5 * 1000ms. See TestDevice.java
        // mLogStartDelay. To ensure we see all the logs, sleep by the same
        // amount.
        mDevice.startLogcat();
        RunUtil.getDefault().sleep(5 * 1000);

        String cmd = fullPath + " " + mFlags;
        CLog.v("Running '%s' on %s", cmd, mDevice.getSerialNumber());

        mDevice.executeShellCommand("stop");
        mDevice.executeShellCommand(cmd);
        mDevice.executeShellCommand("start");
    }
}
