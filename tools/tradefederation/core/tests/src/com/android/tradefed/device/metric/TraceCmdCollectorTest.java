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
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link TraceCmdCollector}, */
@RunWith(JUnit4.class)
public final class TraceCmdCollectorTest {
    private ITestDevice mMockDevice;
    private TraceCmdCollector mTraceCmd;
    private OptionSetter mOptionSetter;
    private ITestInvocationListener mMockTestLogger;
    private IInvocationContext mMockInvocationContext;
    private String mDefaultLogPath = "/data/local/tmp/atrace.dat";
    private String mTraceCmdPath = "/data/local/tmp/trace-cmd";
    private String mSerialNo = "12349876";
    private String mCategories = "freq batterystats";
    private String mTraceCmdOptions = "-e chamomille -e chrysanthemum";

    @Before
    public void setUp() throws Exception {
        mMockDevice = EasyMock.createNiceMock(ITestDevice.class);
        mMockTestLogger = EasyMock.createMock(ITestInvocationListener.class);
        mMockInvocationContext = EasyMock.createNiceMock(IInvocationContext.class);

        mTraceCmd = new TraceCmdCollector();
        mOptionSetter = new OptionSetter(mTraceCmd);
        mOptionSetter.setOptionValue("categories", mCategories);

        EasyMock.expect(mMockInvocationContext.getDevices())
                .andStubReturn(Arrays.asList(mMockDevice));
        EasyMock.replay(mMockInvocationContext);
        mTraceCmd.init(mMockInvocationContext, mMockTestLogger);
    }

    /**
     * Test {@link TraceCmdCollector#onTestStart(DeviceMetricData)} to see if trace collection
     * started with trace-cmd and atrace correctly.
     *
     * <p>Expect that atrace and trace-cmd commands were both issued with their appropriate
     * arguments.
     */
    @Test
    public void testStartsAtraceAndTraceCmdOptions() throws Exception {
        String expectedCLI =
                "nohup "
                        + mTraceCmdPath
                        + " record -o "
                        + mDefaultLogPath
                        + " "
                        + mTraceCmdOptions
                        + " > /dev/null 2>&1 &";
        mMockDevice.executeShellCommand(
                EasyMock.eq("atrace --async_start -z " + mCategories),
                EasyMock.anyObject(),
                EasyMock.eq(1L),
                EasyMock.anyObject(),
                EasyMock.eq(1));
        EasyMock.expectLastCall().times(1);

        mMockDevice.executeShellCommand(
                EasyMock.eq("chmod +x " + mTraceCmdPath),
                EasyMock.anyObject(),
                EasyMock.eq(1L),
                EasyMock.anyObject(),
                EasyMock.eq(1));
        EasyMock.expectLastCall().times(1);
        EasyMock.replay(mMockDevice);

        mOptionSetter.setOptionValue("trace-cmd-binary", mTraceCmdPath);
        mOptionSetter.setOptionValue("trace-cmd-recording-args", mTraceCmdOptions);

        mTraceCmd.onTestStart(new DeviceMetricData(mMockInvocationContext));
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test {@link TraceCmdCollector#onTestStart(DeviceMetricData)} to see if trace collection
     * failure is handled correctly.
     *
     * <p>Expect that atrace continues on if trace-cmd cant be found on the device.
     */
    @Test
    public void testStartsAtraceAndTraceCmdFails() throws Exception {
        mMockDevice.executeShellCommand(
                EasyMock.eq("atrace --async_start -z " + mCategories),
                EasyMock.anyObject(),
                EasyMock.eq(1L),
                EasyMock.anyObject(),
                EasyMock.eq(1));
        EasyMock.expectLastCall().times(1);
        mMockDevice.executeShellCommand(
                (String) EasyMock.anyObject(),
                EasyMock.anyObject(),
                EasyMock.eq(1L),
                EasyMock.anyObject(),
                EasyMock.eq(1));
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());

        EasyMock.replay(mMockDevice);

        mOptionSetter.setOptionValue("trace-cmd-binary", mTraceCmdPath);

        mTraceCmd.onTestStart(new DeviceMetricData(mMockInvocationContext));
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test {@link TraceCmdCollector#onTestEnd(DeviceMetricData, Map)} to see if trace-cmd
     * collection stopped correctly.
     *
     * <p>Expect that trace-cmd was stopped, the trace file was pulled from device to host and the
     * trace file removed from device.
     */
    @Test
    public void testStopsTraceCmdDuringTearDown() throws Exception {
        //wait won't work, as trace-cmd was ran with nohup in a different session.
        mMockDevice.executeShellCommand(
                EasyMock.eq(
                        "for PID in $(pidof trace-cmd); "
                                + "do while kill -s sigint $PID; do sleep 0.3; done; done;"),
                EasyMock.anyObject(),
                EasyMock.eq(60L),
                EasyMock.anyObject(),
                EasyMock.eq(1));
        EasyMock.expectLastCall().times(1);
        EasyMock.expect(mMockDevice.pullFile(EasyMock.eq(mDefaultLogPath)))
                .andReturn(new File("/tmp/potato"))
                .once();

        EasyMock.replay(mMockDevice);
        mOptionSetter.setOptionValue("trace-cmd-binary", "trc");
        mTraceCmd.onTestEnd(
                new DeviceMetricData(mMockInvocationContext), new HashMap<String, Metric>());
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test {@link TraceCmdCollector#onTestEnd(DeviceMetricData, Map)} to see that it uploads its
     * file correctly when using trace-cmd.
     *
     * <p>Expect that the log is uploaded with the proper filename and LogDataType.
     */
    @Test
    public void testUploadslogWithRawKernelBuffer() throws Exception {
        EasyMock.expect(mMockDevice.pullFile((String) EasyMock.anyObject()))
                .andStubReturn(new File("/tmp/potato"));
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn(mSerialNo);
        mMockTestLogger.testLog(
                EasyMock.eq("atrace" + mSerialNo),
                EasyMock.eq(LogDataType.KERNEL_TRACE),
                EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        EasyMock.replay(mMockDevice, mMockTestLogger);

        mOptionSetter.setOptionValue("trace-cmd-binary", "trace-cmd");
        mTraceCmd.onTestEnd(
                new DeviceMetricData(mMockInvocationContext), new HashMap<String, Metric>());

        EasyMock.verify(mMockTestLogger);
    }
}
