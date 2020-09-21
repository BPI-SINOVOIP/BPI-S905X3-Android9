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

package com.android.framework.tests;

import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Runs a series of automated use cases and collects loaded class information in order to generate
 * a list of preloaded classes based on the input thresholds.
 */
public class PreloadedClassesTest implements IRemoteTest, IDeviceTest, IBuildReceiver {
    private static final String JUNIT_RUNNER = "android.support.test.runner.AndroidJUnitRunner";
    // Preload tool commands
    private static final String TOOL_CMD = "java -cp %s com.android.preload.Main --seq %s %s";
    private static final String SCAN_ALL_CMD = "scan-all";
    private static final String COMPUTE_CMD = "comp %d %s";
    private static final String EXPORT_CMD = "export %s";
    private static final String IMPORT_CMD = "import %s";
    // Large, common timeouts
    private static final long SCAN_TIMEOUT_MS = 5 * 60 * 1000;
    private static final long COMPUTE_TIMEOUT_MS = 60 * 1000;

    @Option(name = "package",
            description = "Instrumentation package for use case automation.",
            mandatory = true)
    private String mPackage = null;

    @Option(name = "test-case",
            description = "List of use cases to exercise from the package.",
            mandatory = true)
    private List<String> mTestCases = new ArrayList<>();

    @Option(name = "preload-tool", description = "Overridden location of the preload JAR file.")
    private String mPreloadToolJarPath = null;

    @Option(name = "threshold",
            description = "List of thresholds for computing preloaded classes.",
            mandatory = true)
    private List<String> mThresholds = new ArrayList<>();

    @Option(name = "quit-on-error",
            description = "Quits if errors are encountered anywhere in the process.",
            mandatory = false)
    private boolean mQuitOnError = false;

    private ITestDevice mDevice;
    private IBuildInfo mBuildInfo;
    private List<File> mExportFiles = new ArrayList<>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // Download preload tool, if not supplied
        if (mPreloadToolJarPath == null) {
            File preload = mBuildInfo.getFile("preload2.jar");
            if (preload != null && preload.exists()) {
                mPreloadToolJarPath = preload.getAbsolutePath();
            } else {
                CLog.e("Unable to find the preload tool.");
            }
        } else {
            CLog.v("Using alternative preload tool path, %s", mPreloadToolJarPath);
        }

        IRemoteAndroidTestRunner runner =
                new RemoteAndroidTestRunner(mPackage, JUNIT_RUNNER, getDevice().getIDevice());

        for (String testCaseIdentifier : mTestCases) {
            // Run an individual use case
            runner.addInstrumentationArg("class", testCaseIdentifier);
            getDevice().runInstrumentationTests(runner, listener);
            // Scan loaded classes and export
            File outfile = scanAndExportClasses();
            if (outfile != null) {
                mExportFiles.add(outfile);
            } else {
                String msg = String.format("Failed to find outfile after %s", testCaseIdentifier);
                if (mQuitOnError) {
                    throw new RuntimeException(msg);
                } else {
                    CLog.e(msg + ". Continuing anyway...");
                }
            }
        }

        try {
            // Consider each threshold input
            for (String thresholdStr : mThresholds) {
                int threshold = 0;
                try {
                    threshold = Integer.parseInt(thresholdStr);
                } catch (NumberFormatException e) {
                    if (mQuitOnError) {
                        throw e;
                    } else {
                        CLog.e("Failed to parse threshold: %s", thresholdStr);
                        CLog.e(e);
                        continue;
                    }
                }
                // Generate the corresponding preloaded classes
                File classes = writePreloadedClasses(threshold);
                if (classes != null) {
                    try (FileInputStreamSource stream = new FileInputStreamSource(classes)) {
                        String name = String.format("preloaded-classes-threshold-%s", thresholdStr);
                        listener.testLog(name, LogDataType.TEXT, stream);
                    }
                    // Clean up after uploading
                    FileUtil.deleteFile(classes);
                } else {
                    String msg = String.format(
                            "Failed to generate classes file for threshold, %s", thresholdStr);
                    if (mQuitOnError) {
                        throw new RuntimeException(msg);
                    } else {
                        CLog.e(msg + ". Continuing anyway...");
                    }
                }
            }
        } finally {
            // Clean up temporary export files.
            for (File f : mExportFiles) {
                FileUtil.deleteFile(f);
            }
        }
    }

    /**
     * Calls the preload tool to pull and scan heap profiles and to generate and export the list of
     * loaded Java classes.
     * @return {@link File} containing the loaded Java classes
     */
    private File scanAndExportClasses() {
        File temp = null;
        try {
            temp = FileUtil.createTempFile("scanned", ".txt");
        } catch (IOException e) {
            CLog.e("Failed while creating temp file.");
            CLog.e(e);
            return null;
        }
        // Construct the command
        String exportCmd = String.format(EXPORT_CMD, temp.getAbsolutePath());
        String actionCmd = String.format("%s %s", SCAN_ALL_CMD, exportCmd);
        String[] fullCmd = constructPreloadCommand(actionCmd);
        CommandResult result = RunUtil.getDefault().runTimedCmd(SCAN_TIMEOUT_MS, fullCmd);
        if (CommandStatus.SUCCESS.equals(result.getStatus())) {
            return temp;
        } else {
            // Clean up the temp file
            FileUtil.deleteFile(temp);
            // Log and return the failure
            CLog.e("Error scanning: %s", result.getStderr());
            return null;
        }
    }

    /**
     * Calls the preload tool to import the previously exported files and to generate the list of
     * preloaded classes based on the threshold input.
     * @return {@link File} containing the generated list of preloaded classes
     */
    private File writePreloadedClasses(int threshold) {
        File temp = null;
        try {
            temp = FileUtil.createTempFile("preloaded-classes", ".txt");
        } catch (IOException e) {
            CLog.e("Failed while creating temp file.");
            CLog.e(e);
            return null;
        }
        // Construct the command
        String actionCmd = "";
        for (File f : mExportFiles) {
            String importCmd = String.format(IMPORT_CMD, f.getAbsolutePath());
            actionCmd += importCmd + " ";
        }
        actionCmd += String.format(COMPUTE_CMD, threshold, temp.getAbsolutePath());
        String[] fullCmd = constructPreloadCommand(actionCmd);
        CommandResult result = RunUtil.getDefault().runTimedCmd(COMPUTE_TIMEOUT_MS, fullCmd);
        if (CommandStatus.SUCCESS.equals(result.getStatus())) {
            return temp;
        } else {
            // Clean up the temp file
            FileUtil.deleteFile(temp);
            // Log and return the failure
            CLog.e("Error computing classes: %s", result.getStderr());
            return null;
        }
    }

    private String[] constructPreloadCommand(String command) {
        return String.format(TOOL_CMD, mPreloadToolJarPath, getDevice().getSerialNumber(), command)
                .split(" ");
    }

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }
}
