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

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.ITestInvocationListener;
import java.util.Arrays;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

/** Unit tests for {@link BugreportzMetricCollector}. */
@RunWith(JUnit4.class)
public class BugreportzMetricCollectorTest {
    @Mock IInvocationContext mContext;

    @Mock ITestDevice mTestDevice;

    @Mock ITestInvocationListener mForwarder;

    @Spy BugreportzMetricCollector mBugreportzMetricCollector;

    @Before
    public void setup() throws Exception {
        MockitoAnnotations.initMocks(this);

        doReturn(Arrays.asList(mTestDevice)).when(mContext).getDevices();

        when(mTestDevice.logBugreport(anyString(), any(ITestInvocationListener.class)))
                .thenReturn(true);

        mBugreportzMetricCollector.init(mContext, mForwarder);

        when(mBugreportzMetricCollector.getFileSuffix()).thenReturn("1");
    }

    /** Tests successful collection of bugreport. */
    @Test
    public void testCollect_success() throws Exception {
        when(mTestDevice.logBugreport("bugreportz-1", mForwarder)).thenReturn(true);

        DeviceMetricData runData = new DeviceMetricData(mContext);

        mBugreportzMetricCollector.collect(mTestDevice, runData);

        verify(mTestDevice).logBugreport(eq("bugreport-1"), eq(mForwarder));
    }
}
