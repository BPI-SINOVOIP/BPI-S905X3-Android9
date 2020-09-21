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
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.FileUtil;
import com.google.common.truth.Truth;
import java.io.File;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

/** Unit tests for {@link DumpHeapCollector}. */
@RunWith(JUnit4.class)
public class DumpHeapCollectorTest {

    @Mock private IInvocationContext mContext;

    @Mock private ITestInvocationListener mListener;

    @Mock private ITestDevice mDevice;

    @Rule public TemporaryFolder folder = new TemporaryFolder();

    @Spy private DumpHeapCollector mDumpheapCollector;

    @Before
    public void setup() throws Exception {
        MockitoAnnotations.initMocks(this);

        when(mDevice.executeShellCommand(String.format("dumpsys meminfo -c | grep camera")))
                .thenReturn(new String("proc,native,camera,21348,800,N/A,e\n"));

        when(mDevice.executeShellCommand(String.format("dumpsys meminfo -c | grep maps")))
                .thenReturn(new String("proc,native,maps,21349,900,N/A,e\n"));

        when(mDevice.executeShellCommand("am dumpheap camera /data/local/tmp/camera_trigger.hprof"))
                .thenReturn(new String());

        doNothing()
                .when(mListener)
                .testLog(anyString(), any(LogDataType.class), any(InputStreamSource.class));
    }

    @Test
    public void testTakeDumpheap_success() throws Exception {

        File mapsDumpheap1 = folder.newFile("maps1");
        File mapsDumpheap2 = folder.newFile("maps2");

        doReturn("1").when(mDumpheapCollector).getFileSuffix();

        when(mDevice.dumpHeap("maps", "/data/local/tmp/maps_trigger_1.hprof"))
                .thenReturn(mapsDumpheap1)
                .thenReturn(mapsDumpheap2);

        String fakeDumpheapOutput =
                "proc,native,maps,21349,900,N/A,e\nproc,native,camera,21350,800,N/A,e\n";

        List<File> files =
                mDumpheapCollector.takeDumpheap(mDevice, fakeDumpheapOutput, "maps", 850L);

        Truth.assertThat(files).containsExactly(mapsDumpheap1);
    }

    @Test
    public void testCollect_success() throws Exception {
        File tempFile1 = folder.newFile();
        File tempFile2 = folder.newFile();
        when(mDevice.dumpHeap(anyString(), anyString()))
                .thenReturn(tempFile1)
                .thenReturn(tempFile2);

        OptionSetter options = new OptionSetter(mDumpheapCollector);

        Map<String, Long> dumpheapThresholds = new HashMap<>();
        dumpheapThresholds.put("camera", 700L);
        dumpheapThresholds.put("maps", 800L);

        options.setOptionValue("dumpheap-thresholds", "camera", "700");
        options.setOptionValue("dumpheap-thresholds", "maps", "800");

        mDumpheapCollector.init(mContext, mListener);

        mDumpheapCollector.collect(mDevice, null);

        ArgumentCaptor<String> dataNameCaptor = ArgumentCaptor.forClass(String.class);
        ArgumentCaptor<LogDataType> dataTypeCaptor = ArgumentCaptor.forClass(LogDataType.class);
        ArgumentCaptor<InputStreamSource> inputCaptor =
                ArgumentCaptor.forClass(InputStreamSource.class);

        verify(mListener, times(2))
                .testLog(dataNameCaptor.capture(), dataTypeCaptor.capture(), inputCaptor.capture());

        // Assert that the correct filename was sent to testLog.
        Truth.assertThat(dataNameCaptor.getAllValues())
                .containsExactlyElementsIn(
                        Arrays.asList(
                                FileUtil.getBaseName(tempFile1.getName()),
                                FileUtil.getBaseName(tempFile2.getName())));

        // Assert that the correct data type was sent to testLog.
        Truth.assertThat(dataTypeCaptor.getAllValues())
                .containsExactlyElementsIn(Arrays.asList(LogDataType.HPROF, LogDataType.HPROF));
    }

    @Test
    public void testCollectSuccess_thresholdTooHigh() throws Exception {
        File tempFile1 = folder.newFile();
        File tempFile2 = folder.newFile();
        when(mDevice.pullFile(anyString())).thenReturn(tempFile1).thenReturn(tempFile2);

        OptionSetter options = new OptionSetter(mDumpheapCollector);

        options.setOptionValue("dumpheap-thresholds", "camera", "7000");
        options.setOptionValue("dumpheap-thresholds", "maps", "8000");

        mDumpheapCollector.init(mContext, mListener);

        mDumpheapCollector.collect(mDevice, null);

        ArgumentCaptor<String> dataNameCaptor = ArgumentCaptor.forClass(String.class);
        ArgumentCaptor<LogDataType> dataTypeCaptor = ArgumentCaptor.forClass(LogDataType.class);
        ArgumentCaptor<InputStreamSource> inputCaptor =
                ArgumentCaptor.forClass(InputStreamSource.class);

        verify(mListener, times(0))
                .testLog(dataNameCaptor.capture(), dataTypeCaptor.capture(), inputCaptor.capture());
    }

    @Test
    public void testCollectNoError_processNotFound() {
        try {
            // Make the meminfo dump not contain the heap info of fake_process.
            when(mDevice.executeShellCommand(
                            String.format("dumpsys meminfo -c | grep fake_process")))
                    .thenReturn("");

            OptionSetter options = new OptionSetter(mDumpheapCollector);
            options.setOptionValue("dumpheap-thresholds", "fake_process", "7000");

            mDumpheapCollector.init(mContext, mListener);

            mDumpheapCollector.collect(mDevice, null);

            ArgumentCaptor<String> dataNameCaptor = ArgumentCaptor.forClass(String.class);
            ArgumentCaptor<LogDataType> dataTypeCaptor = ArgumentCaptor.forClass(LogDataType.class);
            ArgumentCaptor<InputStreamSource> inputCaptor =
                    ArgumentCaptor.forClass(InputStreamSource.class);

            // Verify that no testLog calls were made.
            verify(mListener, times(0))
                    .testLog(
                            dataNameCaptor.capture(),
                            dataTypeCaptor.capture(),
                            inputCaptor.capture());
        } catch (DeviceNotAvailableException e) {
            Truth.THROW_ASSERTION_ERROR.fail(e.getMessage());
        } catch (ConfigurationException e) {
            Truth.THROW_ASSERTION_ERROR.fail(e.getMessage());
        } catch (InterruptedException e) {
            Truth.THROW_ASSERTION_ERROR.fail(e.getMessage());
        }
    }
}
