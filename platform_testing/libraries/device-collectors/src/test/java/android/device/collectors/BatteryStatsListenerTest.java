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
package android.device.collectors;

import android.app.Instrumentation;
import android.device.collectors.util.SendToInstrumentation;
import android.os.Bundle;
import android.support.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.io.File;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

/**
 * Android Unit tests for {@link BatteryStatsListener}.
 *
 * To run:
 * atest CollectorDeviceLibTest:android.device.collectors.BatteryStatsListenerTest
 */
@RunWith(AndroidJUnit4.class)
public class BatteryStatsListenerTest {

    private File mLogDir;
    private File mLogFile;
    private Description mRunDesc;
    private Description mTestDesc;
    private BatteryStatsListener mListener;

    @Mock
    private Instrumentation mInstrumentation;


    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLogDir = new File("tmp/");
        mLogFile = new File("unique_log_file.log");
        mRunDesc = Description.createSuiteDescription("run");
        mTestDesc = Description.createTestDescription("run", "test");
    }

    @After
    public void tearDown() {
        if (mLogFile != null) {
            mLogFile.delete();
        }
        if (mLogDir != null) {
            mLogDir.delete();
        }
    }

    private BatteryStatsListener initListener(Bundle b) {
        BatteryStatsListener listener = spy(new BatteryStatsListener(b));
        listener.setInstrumentation(mInstrumentation);
        doReturn(new byte[0]).when(listener).executeCommandBlocking(anyString());
        doReturn(mLogDir).when(listener).createAndEmptyDirectory(anyString());
        doReturn(mLogFile).when(listener).dumpBatteryStats(anyString());
        doReturn(true).when(listener).resetBatteryStats();
        return listener;
    }

    @Test
    public void testTestRunCollector() throws Exception {
        Bundle b = new Bundle();
        b.putString(BatteryStatsListener.KEY_PER_RUN, "true");
        mListener = initListener(b);

        // Test run start behavior
        mListener.testRunStarted(mRunDesc);
        verify(mListener).createAndEmptyDirectory(BatteryStatsListener.DEFAULT_DIR);
        verify(mListener, times(1)).resetBatteryStats();

        // Test test start behavior
        mListener.testStarted(mTestDesc);
        verify(mListener, times(1)).resetBatteryStats();

        // Test test end behavior
        mListener.testFinished(mTestDesc);
        verify(mListener, times(1)).resetBatteryStats();
        verify(mListener, times(0)).dumpBatteryStats(anyString());

        // Test run end behavior
        mListener.testRunFinished(new Result());
        verify(mListener, times(1)).resetBatteryStats();
        verify(mListener, times(1)).dumpBatteryStats(anyString());

        Bundle resultBundle = new Bundle();
        mListener.instrumentationRunFinished(System.out, resultBundle, new Result());

        int protoFileCount = 0;
        for(String key : resultBundle.keySet()) {
            if (key.contains(mLogFile.getName())) protoFileCount++;
        }
        assertEquals(1, protoFileCount);
    }

    @Test
    public void testTestRunSaveToSpecifiedDirCollector() throws Exception {
        Bundle b = new Bundle();
        b.putString(BatteryStatsListener.KEY_PER_RUN, "true");
        b.putString(BatteryStatsListener.KEY_FORMAT, "file:unique/bs/dir");
        mListener = initListener(b);

        // Test run start behavior
        mListener.testRunStarted(mRunDesc);
        verify(mListener).createAndEmptyDirectory("unique/bs/dir");
    }

    @Test
    public void testTestRunToBytesCollector() throws Exception {
        Bundle b = new Bundle();
        b.putString(BatteryStatsListener.KEY_FORMAT, BatteryStatsListener.OPTION_BYTE);
        mListener = initListener(b);
        final int numTestCase = 5;

        // Test run start behavior
        mListener.testRunStarted(mRunDesc);
        verify(mListener, times(0)).createAndEmptyDirectory(BatteryStatsListener.DEFAULT_DIR);
        verify(mListener, times(0)).resetBatteryStats();

        for (int i = 1; i <= numTestCase; i++) {
            // Test test start behavior
            mListener.testStarted(mTestDesc);
            verify(mListener, times(i)).resetBatteryStats();

            // Test test end behavior
            mListener.testFinished(mTestDesc);
            verify(mListener, times(i)).resetBatteryStats();
            verify(mListener, times(i)).executeCommandBlocking(BatteryStatsListener.CMD_DUMPSYS);
        }

        // Test run end behavior
        mListener.testRunFinished(new Result());
        verify(mListener, times(numTestCase)).resetBatteryStats();
        verify(mListener, times(numTestCase)).executeCommandBlocking(BatteryStatsListener.CMD_DUMPSYS);

        Bundle resultBundle = new Bundle();
        mListener.instrumentationRunFinished(System.out, resultBundle, new Result());

        ArgumentCaptor<Bundle> capture = ArgumentCaptor.forClass(Bundle.class);
        Mockito.verify(mInstrumentation, times(numTestCase))
                .sendStatus(Mockito.eq(
                        SendToInstrumentation.INST_STATUS_IN_PROGRESS), capture.capture());
        List<Bundle> capturedBundle = capture.getAllValues();
        assertEquals(numTestCase, capturedBundle.size());

        int protoCount = 0;
        for(Bundle bundle:capturedBundle){
            for(String key : bundle.keySet()) {
                if (key.contains("bytes")) protoCount++;
            }
        }
        assertEquals(numTestCase, protoCount);
    }

    @Test
    public void testTestCaseCollector() throws Exception {
        Bundle b = new Bundle();
        mListener = initListener(b);
        final int numTestCase = 5;

        // Test run start behavior
        mListener.testRunStarted(mRunDesc);
        verify(mListener).createAndEmptyDirectory(BatteryStatsListener.DEFAULT_DIR);
        verify(mListener, times(0)).resetBatteryStats();

        for (int i = 1; i <= numTestCase; i++) {
            // Test test start behavior
            mListener.testStarted(mTestDesc);
            verify(mListener, times(i)).resetBatteryStats();

            // Test test end behavior
            mListener.testFinished(mTestDesc);
            verify(mListener, times(i)).resetBatteryStats();
            verify(mListener, times(i)).dumpBatteryStats(anyString());
        }

        // Test run end behavior
        mListener.testRunFinished(new Result());
        verify(mListener, times(numTestCase)).resetBatteryStats();
        verify(mListener, times(numTestCase)).dumpBatteryStats(anyString());

        Bundle resultBundle = new Bundle();
        mListener.instrumentationRunFinished(System.out, resultBundle, new Result());

        ArgumentCaptor<Bundle> capture = ArgumentCaptor.forClass(Bundle.class);
        Mockito.verify(mInstrumentation, times(numTestCase))
                .sendStatus(Mockito.eq(
                        SendToInstrumentation.INST_STATUS_IN_PROGRESS), capture.capture());
        List<Bundle> capturedBundle = capture.getAllValues();
        assertEquals(numTestCase, capturedBundle.size());

        int protoFileCount = 0;
        for(Bundle bundle:capturedBundle){
            for(String key : bundle.keySet()) {
                if (key.contains(mLogFile.getName())) protoFileCount++;
            }
        }
        assertEquals(numTestCase, protoFileCount);
    }
}
