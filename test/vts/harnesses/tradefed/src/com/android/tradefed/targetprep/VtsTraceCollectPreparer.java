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

package com.android.tradefed.targetprep;

import com.android.annotations.VisibleForTesting;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.TargetSetupError;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.NoSuchElementException;

/**
 * Preparer class for collect HAL traces during the test run.
 *
 * Currently used for collecting HAL traces for CTS tests. In test setup, pushes
 * the profiler libs to device, sets up the permissions, and calls
 * vts_profiling_configure to enable profiling for HAL services. In test
 * tear down, pulls the traces to the host, resets permissions and cleans up the
 * trace files and profiler libs on the device.
 */
@OptionClass(alias = "vts-trace-collect-preparer")
public class VtsTraceCollectPreparer implements ITargetPreparer, ITargetCleaner {
    static final String SELINUX_DISABLED = "Disabled"; // selinux disabled
    static final String SELINUX_PERMISSIVE = "Permissive"; // selinux permissive mode

    // Relative path to vts 32 bit lib directory.
    static final String VTS_LIB_DIR_32 = "DATA/lib/";
    // Relative path to vts 64 bit lib directory.
    static final String VTS_LIB_DIR_64 = "DATA/lib64/";
    // Relative path to vts binary directory.
    static final String VTS_BINARY_DIR = "DATA/bin/";
    // Path of 32 bit test libs on target device.
    static final String VTS_TMP_LIB_DIR_32 = "/data/local/tmp/32/";
    // Path of 64 bit test libs on target device.
    static final String VTS_TMP_LIB_DIR_64 = "/data/local/tmp/64/";
    // Path of vts test directory on target device.
    static final String VTS_TMP_DIR = "/data/local/tmp/";
    // Default path to store trace files locally.
    static final String LOCAL_TRACE_DIR = "vts-profiling";

    static final String VTS_PROFILER_SUFFIX = "vts.profiler.so";
    static final String VTS_LIB_PREFIX = "libvts";
    static final String PROFILING_CONFIGURE_BINARY = "vts_profiling_configure";
    static final String TRACE_PATH = "trace_path";

    private String mEnforcingState = null; // start state for selinux enforcement

    @Option(name = "local-trace-dir", description = "Local directory to store trace.")
    private String mLocalTraceDir = LOCAL_TRACE_DIR;

    /** {@inheritDoc} */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws DeviceNotAvailableException, TargetSetupError {
        CompatibilityBuildHelper buildHelper = createBuildHelper(buildInfo);
        try {
            // adb root.
            device.enableAdbRoot();
            // Push 32 bit profiler libs.
            pushProfilerLib(device, new File(buildHelper.getTestsDir(), VTS_LIB_DIR_32),
                    VTS_TMP_LIB_DIR_32);
            // Push 64 bit profiler libs.
            pushProfilerLib(device, new File(buildHelper.getTestsDir(), VTS_LIB_DIR_64),
                    VTS_TMP_LIB_DIR_64);
            // Push vts_profiling_configure
            device.pushFile(new File(buildHelper.getTestsDir(),
                                    VTS_BINARY_DIR + PROFILING_CONFIGURE_BINARY),
                    VTS_TMP_DIR + PROFILING_CONFIGURE_BINARY);
        } catch (IOException | NoSuchElementException e) {
            // Cleanup profiler libs.
            removeProfilerLib(device);
            throw new TargetSetupError("Could not push profiler.");
        }
        // Create directory to store the trace files.
        try {
            File resultDir = buildHelper.getResultDir();
            File traceDir = new File(resultDir, mLocalTraceDir);
            buildInfo.addBuildAttribute(TRACE_PATH, traceDir.getAbsolutePath());
        } catch (FileNotFoundException e) {
            CLog.e("Failed to get traceDir: " + e.getMessage());
            // Cleanup profiler libs.
            removeProfilerLib(device);
            throw new TargetSetupError("Failed to get traceDir.");
        }
        // Set selinux permissive mode.
        mEnforcingState = device.executeShellCommand("getenforce");
        if (mEnforcingState == null
                || (!mEnforcingState.equals(SELINUX_DISABLED)
                           && !mEnforcingState.equals(SELINUX_PERMISSIVE))) {
            device.executeShellCommand("setenforce " + SELINUX_PERMISSIVE);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        // Reset selinux permission mode.
        if (mEnforcingState != null && !mEnforcingState.equals(SELINUX_DISABLED)) {
            device.executeShellCommand("setenforce " + mEnforcingState);
        }
        // Cleanup profiler libs.
        removeProfilerLib(device);
    }

    /**
     * Create and return a {@link CompatibilityBuildHelper} to use during the preparer.
     */
    @VisibleForTesting
    CompatibilityBuildHelper createBuildHelper(IBuildInfo buildInfo) {
        return new CompatibilityBuildHelper(buildInfo);
    }

    /**
     * Push all the profiler libraries (with pattern *.vts-profiler.so) and VTS
     * profiling related libraries (with pattern libvts*.so) to device.
     *
     * @param device device object
     * @param profilerLibDir directory to lookup for profiler libraries.
     * @param destDirName target directory on device to push to.
     * @throws DeviceNotAvailableException
     */
    private void pushProfilerLib(ITestDevice device, File profilerLibDir, String destDirName)
            throws DeviceNotAvailableException {
        File[] files = profilerLibDir.listFiles();
        if (files == null) {
            CLog.d("No files found in %s", profilerLibDir.getAbsolutePath());
            return;
        }
        for (File f : files) {
            String fileName = f.getName();
            if (f.isFile()
                    && (fileName.endsWith(VTS_PROFILER_SUFFIX)
                               || fileName.startsWith(VTS_LIB_PREFIX))) {
                CLog.i("Pushing %s", fileName);
                device.pushFile(f, destDirName + fileName);
            }
        }
    }

    /**
     * Remove all profiler and VTS profiling libraries from device.
     *
     * @param device device object
     * @throws DeviceNotAvailableException
     */
    private void removeProfilerLib(ITestDevice device) throws DeviceNotAvailableException {
        device.executeShellCommand(String.format("rm -rf %s/*vts.profiler.so", VTS_TMP_LIB_DIR_32));
        device.executeShellCommand(String.format("rm -rf %s/*vts.profiler.so", VTS_TMP_LIB_DIR_64));
        device.executeShellCommand(String.format("rm -rf %s/libvts_*.so", VTS_TMP_LIB_DIR_32));
        device.executeShellCommand(String.format("rm -rf %s/libvts_*.so", VTS_TMP_LIB_DIR_64));
        device.executeShellCommand(String.format("rm %s/vts_profiling_configure", VTS_TMP_DIR));
    }
}
