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
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.io.File;
import java.util.HashMap;


/** Unit tests for {@link AtraceRunMetricCollector}. */
@RunWith(JUnit4.class)
public class AtraceRunMetricCollectorTest {

    private AtraceRunMetricCollector mAtraceRunMetricCollector;
    @Mock private ITestInvocationListener mMockListener;
    @Mock private ITestDevice mMockDevice;
    private IInvocationContext mContext;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = new InvocationContext();
        mContext.addAllocatedDevice("default", mMockDevice);
        mAtraceRunMetricCollector = new AtraceRunMetricCollector() {
            @Override
            public void processMetricDirectory(
                    String key, File metricDirectory, DeviceMetricData runData) {
                try (FileInputStreamSource source = new FileInputStreamSource(
                        metricDirectory)) {
                    testLog(key, LogDataType.ZIP, source);
                } finally {
                    FileUtil.deleteFile(metricDirectory);
                }
            }
        };
        mAtraceRunMetricCollector.init(mContext, mMockListener);
    }

    /** Test when no directories have been requested, nothing should be queried anywhere. */
    @Test
    public void testNoMatchingDirectory() {
        mAtraceRunMetricCollector.testRunStarted("fake", 1);
        mAtraceRunMetricCollector.testRunEnded(0, new HashMap<String, Metric>());
    }

    /**
     * Test when a directory is found matching the directory-keys, then pulled and {@link
     * AtraceRunMetricCollector#processMetricDirectory(String, File, DeviceMetricData)} is called.
     */
    @Test
    public void testPullMatchingDirectoryKey() throws Exception {

        OptionSetter setter = new OptionSetter(mAtraceRunMetricCollector);
        setter.setOptionValue("directory-keys", "sdcard/srcdirectory");
        HashMap<String, Metric> currentMetrics = new HashMap<>();
        currentMetrics.put("srcdirectory", TfMetricProtoUtil.stringToMetric("sdcard/srcdirectory"));

        Mockito.when(mMockDevice.pullDir(Mockito.eq("sdcard/srcdirectory"),
                Mockito.any(File.class))).thenReturn(true);

        mAtraceRunMetricCollector.testRunStarted("fakeRun", 5);
        mAtraceRunMetricCollector.testRunEnded(500, new HashMap<String, Metric>());

        Mockito.verify(mMockListener)
                .testLog(Mockito.eq("sdcard/srcdirectory"), Mockito.eq(LogDataType.ZIP),
                        Mockito.any());
    }

}
