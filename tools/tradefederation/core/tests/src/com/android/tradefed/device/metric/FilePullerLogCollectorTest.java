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

import static org.junit.Assert.*;

import com.android.tradefed.config.ConfigurationDef;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.easymock.Capture;
import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link FilePullerLogCollector}. */
@RunWith(JUnit4.class)
public class FilePullerLogCollectorTest {

    private FilePullerLogCollector mCollector;
    private ITestInvocationListener mMockListener;
    private IInvocationContext mContext;
    private ITestDevice mMockDevice;

    @Before
    public void setUp() throws Exception {
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mContext = new InvocationContext();
        mContext.addAllocatedDevice(ConfigurationDef.DEFAULT_DEVICE_NAME, mMockDevice);
        mCollector = new FilePullerLogCollector();
        OptionSetter setter = new OptionSetter(mCollector);
        setter.setOptionValue("pull-pattern-keys", "log.*");
    }

    /**
     * Test that if the pattern of a metric match the requested pattern we attemp to pull it as a
     * log file.
     */
    @Test
    public void testPullAndLog() throws Exception {
        ITestInvocationListener listener = mCollector.init(mContext, mMockListener);
        TestDescription test = new TestDescription("class", "test");
        Map<String, String> metrics = new HashMap<>();
        metrics.put("log1", "/data/local/tmp/log1.txt");
        metrics.put("another_metrics", "57");

        Capture<HashMap<String, Metric>> capture = new Capture<>();
        mMockListener.testStarted(test, 0L);
        EasyMock.expect(mMockDevice.pullFile("/data/local/tmp/log1.txt"))
                .andReturn(new File("file"));
        EasyMock.expect(mMockDevice.executeShellCommand("rm -f /data/local/tmp/log1.txt"))
                .andReturn("");
        mMockListener.testLog(
                EasyMock.eq("file"), EasyMock.eq(LogDataType.TEXT), EasyMock.anyObject());
        mMockListener.testEnded(EasyMock.eq(test), EasyMock.eq(50L), EasyMock.capture(capture));

        EasyMock.replay(mMockDevice, mMockListener);
        listener.testStarted(test, 0L);
        listener.testEnded(test, 50L, TfMetricProtoUtil.upgradeConvert(metrics));
        EasyMock.verify(mMockDevice, mMockListener);
        HashMap<String, Metric> metricCaptured = capture.getValue();
        assertEquals(
                "57", metricCaptured.get("another_metrics").getMeasurements().getSingleString());
        assertEquals(
                "/data/local/tmp/log1.txt",
                metricCaptured.get("log1").getMeasurements().getSingleString());
    }

    /**
     * Test that if the pattern of a metric match the requested pattern but we don't collect on test
     * cases then nothing is done.
     */
    @Test
    public void testSkipTestCollection() throws Exception {
        OptionSetter setter = new OptionSetter(mCollector);
        setter.setOptionValue("collect-on-run-ended-only", "true");
        ITestInvocationListener listener = mCollector.init(mContext, mMockListener);
        TestDescription test = new TestDescription("class", "test");
        Map<String, String> metrics = new HashMap<>();
        metrics.put("log1", "/data/local/tmp/log1.txt");
        metrics.put("another_metrics", "57");

        Capture<HashMap<String, Metric>> capture = new Capture<>();
        mMockListener.testStarted(test, 0L);
        mMockListener.testEnded(EasyMock.eq(test), EasyMock.eq(50L), EasyMock.capture(capture));

        EasyMock.replay(mMockDevice, mMockListener);
        listener.testStarted(test, 0L);
        listener.testEnded(test, 50L, TfMetricProtoUtil.upgradeConvert(metrics));
        EasyMock.verify(mMockDevice, mMockListener);
        HashMap<String, Metric> metricCaptured = capture.getValue();
        assertEquals(
                "57", metricCaptured.get("another_metrics").getMeasurements().getSingleString());
        assertEquals(
                "/data/local/tmp/log1.txt",
                metricCaptured.get("log1").getMeasurements().getSingleString());
    }
}
