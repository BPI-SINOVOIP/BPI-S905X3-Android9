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

package com.android.tradefed.result;

import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IFileEntry;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Helper test component that pulls files located on a device and adds them to the test logs.
 * The component provides {@link IRemoteTest} and {@link IDeviceTest} services.
 * <p>
 * The path to files on the device is specified with upload-dir or upload-pattern options.
 * The files will be removed if clean-upload-pattern option is provided.
 */
public class LogFilesReporter implements IRemoteTest, IDeviceTest {

    @Option(name = "upload-pattern",
            description = "A path pattern of files on the device to be added to the test logs.")
    private String mUploadPattern = null;

    // TODO(mdzyuba): upload-pattern does not work at the moment. Remove upload-dir once resolved.
    @Option(name = "upload-dir",
            description = "A folder on the device to be added to the test logs.")
    private String mUploadDir = null;

    @Option(name = "clean-upload-pattern",
            description = "Clean files specified in \"upload-pattern\" after the test is done.")
    private boolean mRemoveFilesSpecifiedByUploadPattern = true;

    // A device instance.
    private ITestDevice mDevice;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (mUploadPattern != null) {
            uploadFilesOnDeviceToLogs(mUploadPattern, listener);
            if (mRemoveFilesSpecifiedByUploadPattern) {
                cleanFilesOnDevice(mUploadPattern);
            }
        }
        if (mUploadDir != null) {
            uploadFolderOnDeviceToLogs(mUploadDir, listener);
            if (mRemoveFilesSpecifiedByUploadPattern) {
                cleanFilesOnDevice(mUploadDir + "/*");
            }
        }
    }

    /**
     * Uploads files from a device to test logs.
     *
     * @param filesPattern a path pattern to files on device to be added to the logs.
     * @param listener a listener for test results from the test invocation.
     * @throws DeviceNotAvailableException in case device is unavailable.
     */
    protected void uploadFilesOnDeviceToLogs(String filesPattern, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        final DeviceFileReporter reporter = new DeviceFileReporter(getDevice(), listener);
        reporter.addPatterns(filesPattern);
        reporter.run();
    }

    /**
     * Uploads files from a device to test logs.
     *
     * @param dir a full path to a folder on device containing files to be added to the logs.
     * @param listener a listener for test results from the test invocation.
     * @throws DeviceNotAvailableException in case device is unavailable.
     */
    protected void uploadFolderOnDeviceToLogs(String dir, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        final DeviceFileReporter reporter = new DeviceFileReporter(getDevice(), listener);
        Map<String, LogDataType> uploadFilePatterns = new LinkedHashMap<String, LogDataType>();
        IFileEntry outputDir = getDevice().getFileEntry(dir);
        if (outputDir != null) {
            for (IFileEntry file : outputDir.getChildren(false)) {
                LogDataType logDataType = LogDataType.UNKNOWN;
                if (file.getName().endsWith(LogDataType.PNG.getFileExt())) {
                    logDataType = LogDataType.PNG;
                } else if (file.getName().endsWith(LogDataType.XML.getFileExt())) {
                    logDataType = LogDataType.XML;
                }
                CLog.v(String.format("Adding file %s", file.getFullPath()));
                uploadFilePatterns.put(file.getFullPath(), logDataType);
            }
            reporter.addPatterns(uploadFilePatterns);
            reporter.run();
        } else {
            CLog.w("Directory not found on device: %s", dir);
        }
    }

    /**
     * Cleans the files on the device.
     *
     * @param pattern a path pattern to files to be removed.
     * @throws DeviceNotAvailableException in case device is unavailable.
     */
    protected void cleanFilesOnDevice(String pattern) throws DeviceNotAvailableException {
        String folder = pattern.substring(0, pattern.lastIndexOf('/'));
        if (doesDirectoryExistOnDevice(folder)) {
            getDevice().executeShellCommand(String.format("rm %s", pattern));
        }
    }

    /**
     * Checks to see if a directory exists on a device.
     *
     * @param folder a full path to a directory on a device to be verified.
     * @return true if a directory exists, false otherwise.
     * @throws DeviceNotAvailableException in case device is unavailable.
     */
    protected boolean doesDirectoryExistOnDevice(String folder) throws DeviceNotAvailableException {
        IFileEntry outputDir = getDevice().getFileEntry(folder);
        if (outputDir == null) {
            CLog.w("Directory not found on device: %s", folder);
            return false;
        }
        return true;
    }
}
