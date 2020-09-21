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
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.util.Collections;
import java.util.HashMap;

/** Unit tests for {@link ProcessMaxMemoryCollector}. */
@RunWith(JUnit4.class)
public class ProcessMaxMemoryCollectorTest {

    private ProcessMaxMemoryCollector mCollector;
    private IInvocationContext mContext;
    private ITestDevice mDevice;
    private ITestInvocationListener mListener;

    private static final String TEST_INPUT =
            "time,28506638,177086152\n"
                    + "4,938,system_server,11,22,N/A,44,0,0,N/A,0,0,0,N/A,0,27613,14013,176602,"
                    + "218228,0,0,122860,122860,1512,1412,5740,8664,0,0,154924,154924,27568,"
                    + "13972,11916,53456,0,0,123008,123008,0,0,0,0,0,0,0,0,Dalvik Other,3662,0,"
                    + "104,0,3660,0,0,0,Stack,1576,0,8,0,1576,0,0,0,Cursor,0,0,0,0,0,0,0,0,"
                    + "Ashmem,156,0,20,0,148,0,0,0,Gfx dev,100,0,48,0,76,0,0,0,Other dev,116,0,"
                    + "164,0,0,96,0,0,.so mmap,7500,2680,3984,21864,904,2680,0,0,.jar mmap,0,0,0,"
                    + "0,0,0,0,0,.apk mmap,72398,71448,0,11736,0,71448,0,0,.ttf mmap,0,0,0,0,0,0,"
                    + "0,0,.dex mmap,76874,46000,0,83644,40,46000,0,0,.oat mmap,8127,2684,64,"
                    + "26652,0,2684,0,0,.art mmap,1991,48,972,10004,1544,48,0,0,Other mmap,137,0,"
                    + "44,1024,4,52,0,0,EGL mtrack,0,0,0,0,0,0,0,0,GL mtrack,111,222,333,444,555,"
                    + "666,777,888,";

    @Before
    public void setup() throws Exception {
        mCollector = new ProcessMaxMemoryCollector();
        mContext = mock(IInvocationContext.class);
        mDevice = mock(ITestDevice.class);
        when(mContext.getDevices()).thenReturn(Collections.singletonList(mDevice));
        mListener = mock(ITestInvocationListener.class);
        mCollector.init(mContext, mListener);
        OptionSetter setter = new OptionSetter(mCollector);
        setter.setOptionValue("memory-usage-process-name", "system_server");
    }

    @Test
    public void testCollector() throws Exception {
        when(mDevice.executeShellCommand(Mockito.eq("dumpsys meminfo --checkin system_server")))
                .thenReturn(TEST_INPUT);

        DeviceMetricData data = new DeviceMetricData(mContext);
        mCollector.onStart(data);
        mCollector.collect(mDevice, data);
        mCollector.onEnd(data);

        verify(mDevice).executeShellCommand(Mockito.eq("dumpsys meminfo --checkin system_server"));

        HashMap<String, Metric> results = new HashMap<>();
        data.addToMetrics(results);
        assertEquals(218228, results.get("MAX_PSS#system_server").getMeasurements().getSingleInt());
        assertEquals(53456, results.get("MAX_USS#system_server").getMeasurements().getSingleInt());
    }

    @Test
    public void testCollectorNoProcess() throws Exception {
        when(mDevice.executeShellCommand(Mockito.eq("dumpsys meminfo --checkin system_server")))
                .thenReturn("No process found for: system_server");

        DeviceMetricData data = new DeviceMetricData(mContext);
        mCollector.onStart(data);
        mCollector.collect(mDevice, data);
        mCollector.onEnd(data);

        verify(mDevice).executeShellCommand(Mockito.eq("dumpsys meminfo --checkin system_server"));

        HashMap<String, Metric> results = new HashMap<>();
        data.addToMetrics(results);
        assertTrue(results.isEmpty());
    }
}
