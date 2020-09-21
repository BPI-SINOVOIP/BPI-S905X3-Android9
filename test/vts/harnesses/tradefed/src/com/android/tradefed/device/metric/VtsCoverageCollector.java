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

package com.android.tradefed.device.metric;

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.targetprep.VtsCoveragePreparer;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.RunInterruptedException;
import com.android.tradefed.util.VtsPythonRunnerHelper;

import java.io.File;
import java.util.Map;
/**
 * A {@link IMetricCollector} that enables vts coverage measurement during a
 * test and generates the coverage report after the test run.
 */
@OptionClass(alias = "vts-coverage-collector")
public class VtsCoverageCollector extends BaseDeviceMetricCollector {
    static final long BASE_TIMEOUT = 1000 * 60 * 20; // timeout for fetching artifacts

    private VtsPythonRunnerHelper mPythonRunnerHelper = null;
    @Override
    public void onTestRunStart(DeviceMetricData testData) {
        for (ITestDevice device : getDevices()) {
            try {
                device.executeShellCommand("rm -rf /data/misc/trace/*");
            } catch (DeviceNotAvailableException e) {
                CLog.e("Failed to cleanup existing traces: " + e.toString());
            }
        }
    }

    @Override
    public void onTestRunEnd(DeviceMetricData testData,
            final Map<String, Metric> currentTestCaseMetrics) throws RunInterruptedException {
        String moduleName = getRunName().replace(' ', '_');
        CLog.i("Test module name: " + moduleName);
        IBuildInfo buildInfo = getBuildInfos().get(0);
        if (buildInfo == null) {
            CLog.e("Could not get build Info.");
            return;
        }
        if (mPythonRunnerHelper == null) {
            mPythonRunnerHelper = new VtsPythonRunnerHelper(buildInfo);
        }
        for (ITestDevice device : getDevices()) {
            String serial = device.getSerialNumber();
            if (serial == null) {
                CLog.e("Could not get device serial number.");
                return;
            }
            String gcovDirPath = getGcoveResrouceDir(buildInfo, device);
            if (gcovDirPath == null) {
                CLog.e("Could not get gcov resource dir path.");
                return;
            }
            String coverage_report_dir = buildInfo.getBuildAttributes().get("coverage_report_path");
            if (coverage_report_dir == null) {
                CLog.e("Must specify the directory to store the coverage report.");
                return;
            }
            File local_coverage_report_dir = new File(coverage_report_dir, moduleName);

            String pythonBin = mPythonRunnerHelper.getPythonBinary();
            String cmdString = pythonBin
                    + " -m vts.utils.python.coverage.coverage_utils get_coverage --serial " + serial
                    + " --gcov_rescource_path " + gcovDirPath + " --report_path "
                    + local_coverage_report_dir.getAbsolutePath() + " --report_prefix "
                    + moduleName;
            String[] cmd = cmdString.split("\\s+");
            CommandResult commandResult = new CommandResult();
            String interruptMessage =
                    mPythonRunnerHelper.runPythonRunner(cmd, commandResult, BASE_TIMEOUT);
            if (commandResult == null || commandResult.getStatus() != CommandStatus.SUCCESS) {
                CLog.e("Could not get coverage data.");
            }
            if (interruptMessage != null) {
                throw new RunInterruptedException(interruptMessage);
            }
        }
    }

    @VisibleForTesting
    String getGcoveResrouceDir(IBuildInfo buildInfo, ITestDevice device) {
        File gcovDir = buildInfo.getFile(VtsCoveragePreparer.getGcovResourceDirKey(device));
        if (gcovDir != null) {
            return gcovDir.getAbsolutePath();
        }
        return null;
    }

    @VisibleForTesting
    void setPythonRunnerHelper(VtsPythonRunnerHelper pythonRunnerHelper) {
        mPythonRunnerHelper = pythonRunnerHelper;
    }
}
