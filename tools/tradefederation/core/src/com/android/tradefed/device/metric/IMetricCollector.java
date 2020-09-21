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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import java.util.List;
import java.util.Map;

/**
 * This interface will be added as a decorator when reporting tests results in order to collect
 * matching metrics.
 *
 * <p>This interface cannot be used as a <result_reporter> even it extends {@link
 * ITestInvocationListener}. The configuration checking will reject it. It must be used as a
 * "metrics_collector".
 *
 * <p>Collectors are not expected to keep an internal state as they may be re-used in several
 * places. If an internal state really must be used, then it should be cleaned on {@link
 * #init(IInvocationContext, ITestInvocationListener)}.
 */
public interface IMetricCollector extends ITestInvocationListener {

    /**
     * Initialization of the collector with the current context and where to forward results. Will
     * only be called once per instance, and the collector is expected to update its internal
     * context and listener. Init will never be called during a test run always before.
     *
     * <p>Do not override unless you know what you are doing.
     *
     * @param context the {@link IInvocationContext} for the invocation in progress.
     * @param listener the {@link ITestInvocationListener} where to put results.
     * @return the new listener wrapping the original one.
     */
    public ITestInvocationListener init(
            IInvocationContext context, ITestInvocationListener listener);

    /** Returns the list of devices available in the invocation. */
    public List<ITestDevice> getDevices();

    /** Returns the list of build information available in the invocation. */
    public List<IBuildInfo> getBuildInfos();

    /** Returns the original {@link ITestInvocationListener} where we are forwarding the results. */
    public ITestInvocationListener getInvocationListener();

    /**
     * Callback when a test run is started.
     *
     * @param runData the {@link DeviceMetricData} holding the data for the run.
     */
    public void onTestRunStart(DeviceMetricData runData);

    /**
     * Callback when a test run is ended. This should be the time for clean up.
     *
     * @param runData the {@link DeviceMetricData} holding the data for the run. Will be the same
     *     object as during {@link #onTestRunStart(DeviceMetricData)}.
     * @param currentRunMetrics the current map of metrics passed to {@link #testRunEnded(long,
     *     Map)}.
     */
    public void onTestRunEnd(DeviceMetricData runData, final Map<String, Metric> currentRunMetrics);

    /**
     * Callback when a test case is started.
     *
     * @param testData the {@link DeviceMetricData} holding the data for the test case.
     */
    public void onTestStart(DeviceMetricData testData);

    /**
     * Callback when a test case is ended. This should be the time for clean up.
     *
     * @param testData the {@link DeviceMetricData} holding the data for the test case. Will be the
     *     same object as during {@link #onTestStart(DeviceMetricData)}.
     * @param currentTestCaseMetrics the current map of metrics passed to {@link
     *     #testEnded(TestDescription, Map)}.
     */
    public void onTestEnd(
            DeviceMetricData testData, final Map<String, Metric> currentTestCaseMetrics);
}
