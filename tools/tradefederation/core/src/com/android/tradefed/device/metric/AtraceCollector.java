/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.ddmlib.NullOutputReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceRuntimeException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;


/**
 * A {@link IMetricCollector} that runs atrace during a test and collects the result and log
 * them to the invocation.
 */
@OptionClass(alias = "atrace")
public class AtraceCollector extends BaseDeviceMetricCollector {

    @Option(name = "categories",
            description = "the tracing categories atrace will capture")
    private List<String> mCategories = new ArrayList<>();

    @Option(
        name = "log-path",
        description = "the temporary location the trace log will be saved to on device"
    )
    private String mLogPath = "/data/local/tmp/";

    @Option(
        name = "log-filename",
        description = "the temporary location the trace log will be saved to on device"
    )
    private String mLogFilename = "atrace";

    @Option(name = "preserve-ondevice-log",
            description = "delete the trace log on the target device after the host collects it")
    private boolean mPreserveOndeviceLog = false;

    @Option(name = "compress-dump",
            description = "produce a compressed trace dump")
    private boolean mCompressDump = true;

    /* These options will arrange a post processing executable binary to be ran on the collected
     * trace.
     * E.G.
     * <option name="post-process-binary" value="/path/to/analyzer.par"/>
     * <option name="post-process-input-file-key" value="TRACE_FILE"/>
     * <option name="post-process-args" value="--input_file TRACE_FILE --arg1 --arg2"/>
     * Will be executed as
     * /path/to/analyzer.par --input_file TRACE_FILE --arg1 --arg2
     */
    @Option(
        name = "post-process-input-file-key",
        description =
                "The string that will be replaced with the absolute path to the trace file "
                        + "in post-process-args"
    )
    private String mLogProcessingTraceInput = "TRACE_FILE";

    @Option(
        name = "post-process-binary",
        description = "a self-contained binary that will be executed on the trace file"
    )
    private File mLogProcessingBinary = null;

    @Option(name = "post-process-args", description = "args for the binary")
    private List<String> mLogProcessingArgs = new ArrayList<>();

    @Option(
        name = "post-process-timeout",
        isTimeVal = true,
        description =
                "The amount of time (eg, 1m2s) that Tradefed will wait for the "
                        + "postprocessing subprocess to finish"
    )
    private long mLogProcessingTimeoutMilliseconds = 0;

    private IRunUtil mRunUtil = RunUtil.getDefault();

    protected String fullLogPath() {
        return Paths.get(mLogPath, mLogFilename + "." + getLogType().getFileExt()).toString();
    }

    protected LogDataType getLogType() {
        if (mCompressDump) {
            return LogDataType.ATRACE;
        } else {
            return LogDataType.TEXT;
        }
    }

    protected void startTracing(ITestDevice device) {
        //atrace --async_start will set a variety of sysfs entries, and then exit.
        String cmd = "atrace --async_start ";
        if (mCompressDump) {
            cmd += "-z ";
        }
        cmd += String.join(" ", mCategories);
        CollectingOutputReceiver c = new CollectingOutputReceiver();
        CLog.i("issuing command : %s to device: %s", cmd, device.getSerialNumber());

        try {
            device.executeShellCommand(cmd, c, 1, TimeUnit.SECONDS, 1);
        } catch (DeviceNotAvailableException e) {
            CLog.e("Error starting atrace:");
            CLog.e(e);
        }
        CLog.i("command output: %s", c.getOutput());
    }

    @Override
    public void onTestStart(DeviceMetricData testData) {
        if (mCategories.isEmpty()) {
            CLog.d("no categories specified to trace, not running AtraceMetricCollector");
            return;
        }

        for (ITestDevice device : getDevices()) {
            startTracing(device);
        }
    }

    protected void stopTracing(ITestDevice device) {
        CLog.i("collecting atrace log from device: %s", device.getSerialNumber());
        try {
            device.executeShellCommand(
                    "atrace --async_stop -o " + fullLogPath(),
                    new NullOutputReceiver(),
                    60,
                    TimeUnit.SECONDS,
                    1);
        } catch (DeviceNotAvailableException e) {
            CLog.e("Error stopping atrace");
            CLog.e(e);
        }
    }

    private void postProcess(File trace, String id) {
        if (mLogProcessingBinary == null
                || !mLogProcessingBinary.exists()
                || !mLogProcessingBinary.canExecute()) {
            CLog.w("No trace postprocessor specified. Skipping trace postprocessing.");
            return;
        }

        List<String> commandLine = new ArrayList<String>();
        commandLine.add(mLogProcessingBinary.getAbsolutePath());
        for (String entry : mLogProcessingArgs) {
            commandLine.add(entry.replaceAll(mLogProcessingTraceInput, trace.getAbsolutePath()));
        }

        String[] commandLineArr = new String[commandLine.size()];
        commandLine.toArray(commandLineArr);
        CommandResult result =
                mRunUtil.runTimedCmd(mLogProcessingTimeoutMilliseconds, commandLineArr);

        CLog.v(
                "Trace postprocessing status: %s\nstdout: %s\nstderr: ",
                result.getStatus(), result.getStdout(), result.getStderr());
    }

    @Override
    public void onTestEnd(
            DeviceMetricData testData, final Map<String, Metric> currentTestCaseMetrics) {

        if (mCategories.isEmpty())
            return;

        for (ITestDevice device : getDevices()) {
            try {
                stopTracing(device);

                File trace = device.pullFile(fullLogPath());
                if (trace != null) {
                    CLog.i("Log size: %s bytes", String.valueOf(trace.length()));
                    try (FileInputStreamSource streamSource = new FileInputStreamSource(trace)) {
                        testLog(
                                mLogFilename + device.getSerialNumber(),
                                getLogType(),
                                streamSource);
                    }

                    postProcess(trace, device.getSerialNumber());
                    trace.delete();
                } else {
                    throw new DeviceRuntimeException("failed to pull log: " + fullLogPath());
                }

                if (!mPreserveOndeviceLog) {
                    device.executeShellCommand("rm -f " + fullLogPath());
                }
                else {
                    CLog.w("preserving ondevice atrace log: %s", fullLogPath());
                }
            } catch (DeviceNotAvailableException | DeviceRuntimeException e) {
                CLog.e("Error retrieving atrace log! device not available:");
                CLog.e(e);
            }
        }
    }

    @VisibleForTesting
    void setRunUtil(IRunUtil util) {
        mRunUtil = util;
    }
}
