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

package com.android.tradefed.util;

import static org.mockito.Mockito.when;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.sl4a.FakeSocketServerHelper;
import com.android.tradefed.util.sl4a.Sl4aClient;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mockito;

import java.io.IOException;
import java.util.Set;

/** Unit tests for {@link BluetoothUtils} */
public class BluetoothUtilsTest {

    private Sl4aClient mClient;
    private Sl4aClient mSpyClient;
    private FakeSocketServerHelper mDeviceServer;
    private ITestDevice mMockDevice;
    private IRunUtil mMockRunUtil;

    @Before
    public void setUp() {
        mMockDevice = Mockito.mock(ITestDevice.class);
        mMockRunUtil = Mockito.mock(IRunUtil.class);
        mClient =
                new Sl4aClient(mMockDevice, 1234, 9998) {
                    @Override
                    protected IRunUtil getRunUtil() {
                        return mMockRunUtil;
                    }
                };
        Mockito.doNothing().when(mMockRunUtil).sleep(Mockito.anyLong());
        mSpyClient = Mockito.spy(mClient);
    }

    @After
    public void tearDown() throws IOException {
        if (mDeviceServer != null) {
            mDeviceServer.close();
        }
    }

    @Test
    public void testParseBondedDeviceInstrumentationOutput() throws Exception {
        String[] lines = {
                "INSTRUMENTATION_RESULT: result=SUCCESS",
                "INSTRUMENTATION_RESULT: device-00=11:22:33:44:55:66",
                "INSTRUMENTATION_RESULT: device-01=22:33:44:55:66:77",
                "INSTRUMENTATION_RESULT: device-02=33:44:55:66:77:88",
                "INSTRUMENTATION_CODE: -1",
        };
        Set<String> ret = BluetoothUtils.parseBondedDeviceInstrumentationOutput(lines);
        Assert.assertEquals("return set has wrong number of entries", 3, ret.size());
        Assert.assertTrue("missing mac 00", ret.contains("11:22:33:44:55:66"));
        Assert.assertTrue("missing mac 01", ret.contains("22:33:44:55:66:77"));
        Assert.assertTrue("missing mac 02", ret.contains("33:44:55:66:77:88"));
    }

    @Test
    public void testEnableBtsnoopLogging() throws DeviceNotAvailableException, IOException {
        when(mMockDevice.executeShellCommand(Mockito.anyString())).thenReturn("");
        Mockito.doReturn(true).when(mSpyClient).isSl4ARunning();
        Mockito.doNothing().when(mSpyClient).open();
        Mockito.doReturn(null).when(mSpyClient).rpcCall(BluetoothUtils.BTSNOOP_API, true);
        Assert.assertTrue(BluetoothUtils.toggleBtsnoopLogging(mSpyClient, true));
    }

    @Test
    public void testEnableBtsnoopLoggingFailed() throws DeviceNotAvailableException, IOException {
        when(mMockDevice.executeShellCommand(Mockito.anyString())).thenReturn("");
        Mockito.doReturn(true).when(mSpyClient).isSl4ARunning();
        Mockito.doNothing().when(mSpyClient).open();
        Mockito.doThrow(new IOException())
                .when(mSpyClient)
                .rpcCall(BluetoothUtils.BTSNOOP_API, true);
        Assert.assertFalse(BluetoothUtils.toggleBtsnoopLogging(mSpyClient, true));
    }

    @Test
    public void testDisableBtsnoopLogging() throws DeviceNotAvailableException, IOException {
        when(mMockDevice.executeShellCommand(Mockito.anyString())).thenReturn("");
        Mockito.doReturn(true).when(mSpyClient).isSl4ARunning();
        Mockito.doNothing().when(mSpyClient).open();
        Mockito.doReturn(null).when(mSpyClient).rpcCall(BluetoothUtils.BTSNOOP_API, false);
        Assert.assertTrue(BluetoothUtils.toggleBtsnoopLogging(mSpyClient, false));
    }

    @Test
    public void testDisableBtsnoopLoggingFailed() throws DeviceNotAvailableException, IOException {
        when(mMockDevice.executeShellCommand(Mockito.anyString())).thenReturn("");
        Mockito.doReturn(false).when(mSpyClient).isSl4ARunning();
        Mockito.doNothing().when(mSpyClient).open();
        Mockito.doReturn(null).when(mSpyClient).rpcCall(BluetoothUtils.BTSNOOP_API, false);
        Assert.assertFalse(BluetoothUtils.toggleBtsnoopLogging(mSpyClient, false));
    }

    @Test
    public void testGetBtSnoopLogFilePath() throws DeviceNotAvailableException {
        when(mMockDevice.executeShellCommand(Mockito.anyString()))
                .thenReturn("BtSnoopFileName=/data/misc/bluetooth/logs/btsnoop_hci.log");
        Assert.assertEquals(
                BluetoothUtils.getBtSnoopLogFilePath(mMockDevice),
                "/data/misc/bluetooth/logs/btsnoop_hci.log");
    }

    @Test
    public void testGetBtSnoopLogFilePathFailed() throws DeviceNotAvailableException {
        when(mMockDevice.executeShellCommand(Mockito.anyString()))
                .thenReturn("BtSnoopFileName/data/misc/bluetooth/logs/btsnoop_hci.log");
        Assert.assertNull(BluetoothUtils.getBtSnoopLogFilePath(mMockDevice));
    }

}
