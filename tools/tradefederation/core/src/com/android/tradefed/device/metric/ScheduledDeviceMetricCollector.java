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

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

/**
 * A {@link IMetricCollector} that allows to run a collection task periodically at a set interval.
 */
public abstract class ScheduledDeviceMetricCollector extends BaseDeviceMetricCollector {

    @Option(
        name = "fixed-schedule-rate",
        description = "Schedule the timetask as a fixed schedule rate"
    )
    private boolean mFixedScheduleRate = false;

    @Option(
        name = "interval",
        description = "the interval between two tasks being scheduled",
        isTimeVal = true
    )
    private long mIntervalMs = 60 * 1000l;

    private Timer timer;

    @Override
    public final void onTestRunStart(DeviceMetricData runData) {
        CLog.d("starting with interval = %s", mIntervalMs);
        onStart(runData);
        timer = new Timer();
        TimerTask timerTask =
                new TimerTask() {
                    @Override
                    public void run() {
                        try {
                            for (ITestDevice device : getDevices()) {
                                collect(device, runData);
                            }
                        } catch (InterruptedException e) {
                            timer.cancel();
                            Thread.currentThread().interrupt();
                            CLog.e("Interrupted exception thrown from task:");
                            CLog.e(e);
                        }
                    }
                };

        if (mFixedScheduleRate) {
            timer.scheduleAtFixedRate(timerTask, 0, mIntervalMs);
        } else {
            timer.schedule(timerTask, 0, mIntervalMs);
        }
    }

    @Override
    public final void onTestRunEnd(
            DeviceMetricData runData, final Map<String, Metric> currentRunMetrics) {
        if (timer != null) {
            timer.cancel();
            timer.purge();
        }
        onEnd(runData);
        CLog.d("finished");
    }

    /**
     * Task periodically & asynchronously run during the test running on a specific device.
     *
     * @param device the {@link ITestDevice} the metric is associated to.
     * @param runData the {@link DeviceMetricData} where to put metrics.
     * @throws InterruptedException
     */
    abstract void collect(ITestDevice device, DeviceMetricData runData) throws InterruptedException;

    /**
     * Executed when entering this collector.
     *
     * @param runData the {@link DeviceMetricData} where to put metrics.
     */
    void onStart(DeviceMetricData runData) {
        // Does nothing.
    }

    /**
     * Executed when finishing this collector.
     *
     * @param runData the {@link DeviceMetricData} where to put metrics.
     */
    void onEnd(DeviceMetricData runData) {
        // Does nothing.
    }

    /**
     * Send all the output of a process from all the devices to a file.
     *
     * <p>Please note, metric collections should not overlap.
     *
     * @throws DeviceNotAvailableException
     * @throws IOException
     */
    File saveProcessOutput(ITestDevice device, String command, String outputFileName)
            throws DeviceNotAvailableException, IOException {
        String output = device.executeShellCommand(command);

        // Create the output file and dump the output of the command to this file.
        File outputFile = new File(outputFileName);

        FileUtil.writeToFile(output, outputFile);

        return outputFile;
    }

    /**
     * Create a suffix string to be appended at the end of each metric file to keep the name unique
     * at each run.
     *
     * @return suffix string in the format year-month-date-hour-minute-seconds-milliseconds.
     */
    @VisibleForTesting
    String getFileSuffix() {
        return new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss-SSS", Locale.US).format(new Date());
    }

    /**
     * Creates temporary directory to store the metric files.
     *
     * @return {@link File} directory with 'tmp' prefixed to its name to signify that its temporary.
     * @throws IOException
     */
    @VisibleForTesting
    File createTempDir() throws IOException {
        return FileUtil.createTempDir(String.format("tmp_%s", getTag()));
    }
}
