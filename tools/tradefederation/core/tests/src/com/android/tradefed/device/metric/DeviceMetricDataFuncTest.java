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

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Measurements;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

/** Functional tests for {@link DeviceMetricData} * */
@RunWith(JUnit4.class)
public class DeviceMetricDataFuncTest {

    private IInvocationContext mContext;

    @Before
    public void setUp() {
        mContext = new InvocationContext();
        ITestDevice device = mock(ITestDevice.class);
        mContext.addAllocatedDevice("device1", device);
    }

    @Test
    public void testAddToMetricsMultiThreaded_success()
            throws InterruptedException, ExecutionException {
        // Incrementing threadCounts in steps and then testing makes sure that there is no
        // flakyness, sticking to one value of threadCount will cause flakyness.
        for (int threadCount = 10; threadCount <= 200; threadCount += 10) {
            testAddToMetricsMultiThreaded(threadCount);
        }
    }

    private void testAddToMetricsMultiThreaded(int threadCount)
            throws InterruptedException, ExecutionException {
        // Create the object to test.
        DeviceMetricData deviceMetricData = new DeviceMetricData(mContext);

        // Create a callable wrapper of DeviceMetricData#addStringMetric and
        // DeviceMetricData#addToMetrics which will add a metric and then try to retrieve it.
        Callable<HashMap<String, Metric>> task =
                new Callable<HashMap<String, Metric>>() {

                    @Override
                    public HashMap<String, Metric> call() throws Exception {
                        deviceMetricData.addMetric(
                                UUID.randomUUID().toString(),
                                Metric.newBuilder()
                                        .setMeasurements(
                                                Measurements.newBuilder()
                                                        .setSingleString("value")
                                                        .build()));
                        HashMap<String, Metric> data = new HashMap<>();
                        deviceMetricData.addToMetrics(data);
                        return data;
                    }
                };
        // Create a copy of this callable for every thread.
        List<Callable<HashMap<String, Metric>>> tasks = Collections.nCopies(threadCount, task);

        // Create a thread pool to execute the tasks.
        ExecutorService executorService = Executors.newFixedThreadPool(threadCount);

        // Invoke the tasks. The call to ExecutorService#invokeAll blocks until all the threads are
        // done.
        List<Future<HashMap<String, Metric>>> futures = executorService.invokeAll(tasks);

        // Store the results from all the tasks in a common data structure.
        HashMap<String, Metric> metricsData = new HashMap<String, Metric>(futures.size());
        for (Future<HashMap<String, Metric>> future : futures) {
            metricsData.putAll(future.get());
        }

        // assert that the number of metrics out is equal to number of metrics in.
        assertEquals(threadCount, metricsData.size());

        // discard all the threads.
        executorService.shutdownNow();
    }
}
