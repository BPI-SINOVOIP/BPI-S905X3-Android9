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

import com.android.tradefed.config.Option;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;

import java.io.File;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

/**
 * A {@link IMetricCollector} that makes runs multiple metric collectors periodically. This is a
 * best effort scheduler. It makes the best effort to run the collectors at given intervals while
 * making sure that no two collectors are run at the same time.
 */
public class ScheduleMultipleDeviceMetricCollector extends BaseDeviceMetricCollector {
    @Option(
        name = "metric-collection-intervals",
        description = "The interval at which the collectors should run."
    )
    private Map<String, Long> mIntervalMs = new HashMap<>();

    @Option(
        name = "metric-storage-path",
        description =
                "Absolute path to a directory on host where the collected metrics will be stored."
    )
    private File mMetricStoragePath = new File(System.getProperty("java.io.tmpdir"));

    @Option(
        name = "metric-collector-command-classes",
        description =
                "Complete package name of a class which registers the commands to do the actual "
                        + "job of collection. Can be repeated."
    )
    private List<String> mMetricCollectorClasses = new ArrayList<>();

    // List of collectors to run.
    private List<ScheduledDeviceMetricCollector> mMetricCollectors = new ArrayList<>();

    // Time interval at which the commands should run.
    private Map<ScheduledDeviceMetricCollector, Long> mMetricCollectorIntervals = new HashMap<>();

    // Time when the commands to collect various metrics were last run.
    private Map<ScheduledDeviceMetricCollector, Long> mLastUpdate = new HashMap<>();

    private Timer mTimer;

    private long mScheduleRate;

    @Override
    public ITestInvocationListener init(
            IInvocationContext context, ITestInvocationListener listener) {
        super.init(context, listener);
        initMetricCollectors(context, listener);

        return this;
    }

    /** Gets an instance of all the requested metric collectors. */
    private void initMetricCollectors(
            IInvocationContext context, ITestInvocationListener listener) {
        for (String metricCollectorClass : mMetricCollectorClasses) {
            try {
                Class<?> klass = Class.forName(metricCollectorClass);

                ScheduledDeviceMetricCollector singleMetricCollector =
                        klass.asSubclass(ScheduledDeviceMetricCollector.class).newInstance();

                singleMetricCollector.init(context, listener);

                mMetricCollectors.add(singleMetricCollector);
            } catch (ClassNotFoundException | InstantiationException | IllegalAccessException e) {
                CLog.e("Class %s not found, skipping.", metricCollectorClass);
                CLog.e(e);
            }
        }
    }

    @Override
    public final void onTestRunStart(DeviceMetricData runData) {
        if (mMetricCollectorClasses.isEmpty()) {
            CLog.w("No single metric class provided. Skipping collection.");
            return;
        }

        setupCollection();

        if (mScheduleRate == 0) {
            CLog.e(
                    "Failed to get a valid interval for even one metric collector. "
                            + "Please make sure that the collectors have non-zero intervals "
                            + "specified as an argument to this class.");
            return;
        }

        // TODO(b/70394486): Investigate if ScheduledThreadPool is better suited here so that we can
        // schedule all the metrics in their own thread and create a common object which allows
        // running of only one collector at a time.
        mTimer = new Timer();

        TimerTask timerTask =
                new TimerTask() {
                    @Override
                    public void run() {
                        collect(runData);
                    }
                };

        mTimer.scheduleAtFixedRate(timerTask, 0, mScheduleRate);
    }

    /**
     * Sets up the collection process by parsing all the args, retrieving the intervals from the
     * args and initializing the last update value of each of the collectors to current time.
     */
    private void setupCollection() {
        parseAllArgs();
        for (ScheduledDeviceMetricCollector singleMetricCollector :
                mMetricCollectorIntervals.keySet()) {
            mLastUpdate.put(singleMetricCollector, System.currentTimeMillis());
        }

        mScheduleRate = gcdOfIntervals();
    }

    /**
     * Runs all the requested collectors sequentially. Dumps the output in {@code
     * mmResultsDirectory/outputDirFormat} of the collector prefixed.
     *
     * @param runData holds the filename of the metrics collected for each collector.
     */
    private void collect(DeviceMetricData runData) {
        for (ScheduledDeviceMetricCollector singleMetricCollector :
                mMetricCollectorIntervals.keySet()) {

            Long elapsedTime = System.currentTimeMillis() - mLastUpdate.get(singleMetricCollector);

            Long taskInterval = mMetricCollectorIntervals.get(singleMetricCollector);

            if (elapsedTime >= taskInterval) {
                try {
                    for (ITestDevice device : getDevices()) {
                        singleMetricCollector.collect(device, runData);
                    }
                    mLastUpdate.put(singleMetricCollector, System.currentTimeMillis());
                } catch (InterruptedException e) {
                    CLog.e("Exception during %s", singleMetricCollector.getClass());
                    CLog.e(e);
                }
            }
        }
    }

    /** Parse all the intervals provided in the command line. */
    private void parseAllArgs() {
        for (ScheduledDeviceMetricCollector metricCollector : mMetricCollectors) {
            Long value = mIntervalMs.getOrDefault(metricCollector.getTag(), 0L);

            if (value > 0) {
                mMetricCollectorIntervals.put(metricCollector, value);
            } else if (value < 0) {
                throw new IllegalArgumentException(
                        metricCollector.getClass() + " expects a non negative interval.");
            }
        }
    }

    /** Get the {@code scheduleRate} common to all tasks which is the gcd of all the intervals. */
    private Long gcdOfIntervals() {
        Collection<Long> intervals = mMetricCollectorIntervals.values();
        if (intervals.isEmpty()) {
            return 0L;
        }
        BigInteger gcdSoFar = new BigInteger(intervals.iterator().next().toString());

        for (Long interval : intervals) {
            gcdSoFar = gcdSoFar.gcd(new BigInteger(interval.toString()));
        }

        return gcdSoFar.longValue();
    }

    @Override
    public final void onTestRunEnd(
            DeviceMetricData runData, final Map<String, Metric> currentRunMetrics) {
        if (mTimer != null) {
            mTimer.cancel();
            mTimer.purge();
        }
    }
}
