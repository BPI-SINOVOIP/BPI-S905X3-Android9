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

package com.android.media.tests;

import com.android.ddmlib.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * A harness that launches AudioJitter tool and reports result.
 */
public class AudioJitterTest implements IDeviceTest, IRemoteTest {

    private static final String RUN_KEY = "audiojitter";
    private static final long TIMEOUT_MS = 5 * 60 * 1000; // 5 min
    private static final int MAX_ATTEMPTS = 3;
    private static final Map<String, String> METRICS_KEY_MAP = createMetricsKeyMap();

    private ITestDevice mDevice;

    private static final String DEVICE_TEMPORARY_DIR_PATH = "/data/local/tmp/";
    private static final String JITTER_BINARY_FILENAME = "sljitter";
    private static final String JITTER_BINARY_DEVICE_PATH =
            DEVICE_TEMPORARY_DIR_PATH + JITTER_BINARY_FILENAME;

    private static Map<String, String> createMetricsKeyMap() {
        Map<String, String> result = new HashMap<String, String>();
        result.put("min_jitter_ticks", "min_jitter_ticks");
        result.put("min_jitter_ms", "min_jitter_ms");
        result.put("min_jitter_period_id", "min_jitter_period_id");
        result.put("max_jitter_ticks", "max_jitter_ticks");
        result.put("max_jitter_ms", "max_jitter_ms");
        result.put("max_jitter_period_id", "max_jitter_period_id");
        result.put("mark_jitter_ticks", "mark_jitter_ticks");
        result.put("mark_jitter_ms", "mark_jitter_ms");
        result.put("max_cb_done_delay_ms", "max_cb_done_delay_ms");
        result.put("max_thread_delay_ms", "max_thread_delay_ms");
        result.put("max_render_delay_ms", "max_render_delay_ms");
        result.put("drift_rate", "drift_rate");
        result.put("error_ms", "error_ms");
        result.put("min_error_ms", "min_error_ms");
        result.put("max_error_ms", "max_error_ms");
        return Collections.unmodifiableMap(result);
    }

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
        TestDescription testId = new TestDescription(getClass().getCanonicalName(), RUN_KEY);
        ITestDevice device = getDevice();

        listener.testRunStarted(RUN_KEY, 0);
        listener.testStarted(testId);

        long testStartTime = System.currentTimeMillis();
        Map<String, String> metrics = new HashMap<String, String>();
        String errMsg = null;

        // start jitter and wait for process to complete
        CollectingOutputReceiver receiver = new CollectingOutputReceiver();
        device.executeShellCommand(JITTER_BINARY_DEVICE_PATH, receiver,
                TIMEOUT_MS, TimeUnit.MILLISECONDS, MAX_ATTEMPTS);
        String resultStr = receiver.getOutput();

        if (resultStr != null) {
            // parse result
            CLog.i("== Jitter result ==");
            Map<String, String> jitterResult = parseResult(resultStr);
            if (jitterResult == null) {
                errMsg = "Failed to parse Jitter result.";
            } else {
                metrics = jitterResult;
            }
        } else {
            errMsg = "Jitter result not found.";
        }

        if (errMsg != null) {
            CLog.e(errMsg);
            listener.testFailed(testId, errMsg);
            listener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(metrics));
            listener.testRunFailed(errMsg);
        } else {
            long durationMs = System.currentTimeMillis() - testStartTime;
            listener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(metrics));
            listener.testRunEnded(durationMs, TfMetricProtoUtil.upgradeConvert(metrics));
        }
    }

    /**
     * Parse Jitter result.
     *
     * @param result Jitter result output
     * @return a {@link HashMap} that contains metrics keys and results
     */
    private Map<String, String> parseResult(String result) {
        Map<String, String> resultMap = new HashMap<String, String>();
        String lines[] = result.split("\\r?\\n");
        for (String line: lines) {
            line = line.trim().replaceAll(" +", " ");
            String[] tokens = line.split(" ");
            if (tokens.length >= 2) {
                String metricName = tokens[0];
                String metricValue = tokens[1];
                if (METRICS_KEY_MAP.containsKey(metricName)) {
                    CLog.i(String.format("%s: %s", metricName, metricValue));
                    resultMap.put(METRICS_KEY_MAP.get(metricName), metricValue);
                }
            }
        }
        return resultMap;
    }
}
