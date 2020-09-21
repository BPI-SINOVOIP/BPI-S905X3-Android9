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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

import java.io.File;

/** Unit tests for {@link TraceMetricCollector}. */
// TODO(b/71868090): Consolidate all the individual metric collector tests into one common tests.
@RunWith(JUnit4.class)
public class TraceMetricCollectorTest {
    @Mock IInvocationContext mContext;

    @Mock ITestInvocationListener mListener;

    @Mock ITestDevice mDevice;

    @Spy TraceMetricCollector mTraceInfoMetricCollector;

    @Rule public TemporaryFolder tempFolder = new TemporaryFolder();

    @Before
    public void setup() throws Exception {
        MockitoAnnotations.initMocks(this);

        mTraceInfoMetricCollector.init(mContext, mListener);

        doNothing()
                .when(mListener)
                .testLog(anyString(), eq(LogDataType.TEXT), any(InputStreamSource.class));

        doReturn(new File("trace-1"))
                .when(mTraceInfoMetricCollector)
                .saveProcessOutput(any(ITestDevice.class), anyString(), anyString());

        doReturn(tempFolder.newFolder()).when(mTraceInfoMetricCollector).createTempDir();
    }

    @Test
    public void testCollect() throws Exception {
        DeviceMetricData runData = new DeviceMetricData(mContext);
        when(mTraceInfoMetricCollector.getFileSuffix()).thenReturn("1");

        mTraceInfoMetricCollector.collect(mDevice, runData);

        // Verify that we logged the metric file.
        verify(mListener).testLog(eq("trace-1"), eq(LogDataType.TEXT), any());
    }
}
