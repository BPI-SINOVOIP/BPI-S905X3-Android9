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
package com.android.tradefed.command.remote;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.verify;

import com.android.tradefed.command.ICommandScheduler;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.device.FreeDeviceState;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.RunUtil;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mockito;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

/** Unit tests for {@link RemoteManager}. */
public class RemoteManagerTest {

    private static final long SHORT_WAIT_TIME_MS = 200;

    private RemoteManager mRemoteManager;
    private ICommandScheduler mMockScheduler;
    private IDeviceManager mMockDeviceManager;
    private DeviceTracker mMockDeviceTracker;

    @Before
    public void setUp() {
        mMockScheduler = Mockito.mock(ICommandScheduler.class);
        mMockDeviceManager = Mockito.mock(IDeviceManager.class);
        mMockDeviceTracker = Mockito.mock(DeviceTracker.class);
        mRemoteManager =
                new RemoteManager(mMockDeviceManager, mMockScheduler) {
                    @Override
                    DeviceTracker getDeviceTracker() {
                        return mMockDeviceTracker;
                    }
                };
    }

    /**
     * Test inputing an invalid string (non-json) to the RemoteManager. It should be correctly
     * rejected.
     */
    @Test
    public void testProcessClientOperations_invalidAction() throws IOException {
        String buf = "test\n";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        assertEquals(
                "{\"error\":\"Failed to handle remote command: "
                        + "com.android.tradefed.command.remote.RemoteException: "
                        + "Value test of type java.lang.String cannot be converted to JSONObject\"}\n",
                out.toString());
    }

    /**
     * Test sending a start handover command on a port and verify the command scheduler is notified.
     */
    @Test
    public void testProcessClientOperations_initHandover() throws IOException {
        String buf = "{version=\"8\", type=\"START_HANDOVER\", port=\"5555\"}";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        // ack is received without error.
        assertEquals("{}\n", out.toString());
        // wait a little bit to let the postOperation thread to run.
        RunUtil.getDefault().sleep(SHORT_WAIT_TIME_MS);
        // handover was sent to the scheduler.
        verify(mMockScheduler).handoverShutdown(5555);
    }

    /** Test when sending a ADD_COMMAND that is added to the CommandScheduler */
    @Test
    public void testProcessClientOperations_addCommand() throws Exception {
        doReturn(true).when(mMockScheduler).addCommand(Mockito.any(), Mockito.anyLong());
        String buf = "{version=\"8\", type=\"ADD_COMMAND\", time=\"5\", commandArgs=[\"empty\"]}";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        // ack is received without error.
        assertEquals("{}\n", out.toString());
        verify(mMockScheduler).addCommand(Mockito.any(), Mockito.eq(5l));
    }

    /** Test when sending a ADD_COMMAND that is not added by the CommandScheduler */
    @Test
    public void testProcessClientOperations_addCommand_fail() throws Exception {
        doReturn(false).when(mMockScheduler).addCommand(Mockito.any(), Mockito.anyLong());
        String buf = "{version=\"8\", type=\"ADD_COMMAND\", time=\"5\", commandArgs=[\"empty\"]}";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        // ack is received with failed to add command.
        assertEquals("{\"error\":\"Failed to add command\"}\n", out.toString());
        verify(mMockScheduler).addCommand(Mockito.any(), Mockito.eq(5l));
    }

    /** Test when sending a ADD_COMMAND that is rejected by the CommandScheduler as not parsable. */
    @Test
    public void testProcessClientOperations_addCommand_config() throws Exception {
        doThrow(new ConfigurationException("NOT_GOOD"))
                .when(mMockScheduler)
                .addCommand(Mockito.any(), Mockito.anyLong());
        String buf = "{version=\"8\", type=\"ADD_COMMAND\", time=\"5\", commandArgs=[\"empty\"]}";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        // ack is received with configuration exception.
        assertEquals(
                "{\"error\":\"Config error: com.android.tradefed.config."
                        + "ConfigurationException: NOT_GOOD\"}\n",
                out.toString());
        verify(mMockScheduler).addCommand(Mockito.any(), Mockito.eq(5l));
    }

    /** Test when sending a ALLOCATE_DEVICE that fails to allocate the serial requested. */
    @Test
    public void testProcessClientOperations_allocateDevice_fail() throws Exception {
        String buf = "{version=\"8\", type=\"ALLOCATE_DEVICE\", serial=\"testserial\"}";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        assertEquals("{\"error\":\"Failed to allocate device testserial\"}\n", out.toString());
        verify(mMockDeviceManager).forceAllocateDevice(Mockito.eq("testserial"));
    }

    /** Test when sending a ALLOCATE_DEVICE that succeed and the DeviceTracker track the device. */
    @Test
    public void testProcessClientOperations_allocateDevice() throws Exception {
        doReturn(Mockito.mock(ITestDevice.class))
                .when(mMockDeviceManager)
                .forceAllocateDevice(Mockito.eq("testserial"));
        String buf = "{version=\"8\", type=\"ALLOCATE_DEVICE\", serial=\"testserial\"}";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        assertEquals("{}\n", out.toString());
        verify(mMockDeviceManager).forceAllocateDevice(Mockito.eq("testserial"));
        verify(mMockDeviceTracker).allocateDevice(Mockito.any());
    }

    /**
     * Test when sending a FREE_DEVICE that fail to found the allocated device in the DeviceTracker
     */
    @Test
    public void testProcessClientOperations_processFree_notFound() throws Exception {
        String buf = "{version=\"8\", type=\"FREE_DEVICE\", serial=\"testserial\"}";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        assertEquals("{\"error\":\"Could not find device to free testserial\"}\n", out.toString());
        verify(mMockDeviceTracker).freeDevice(Mockito.eq("testserial"));
        // Device was not found as allocated remotely so we do not call free.
        verify(mMockDeviceManager, Mockito.times(0)).freeDevice(Mockito.any(), Mockito.any());
    }

    /**
     * Test when sending a FREE_DEVICE and the device is properly free from device manager and
     * device tracker.
     */
    @Test
    public void testProcessClientOperations_processFree() throws Exception {
        ITestDevice stub = Mockito.mock(ITestDevice.class);
        doReturn(stub).when(mMockDeviceTracker).freeDevice(Mockito.eq("testserial"));
        String buf = "{version=\"8\", type=\"FREE_DEVICE\", serial=\"testserial\"}";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        assertEquals("{}\n", out.toString());
        verify(mMockDeviceTracker).freeDevice(Mockito.eq("testserial"));
        verify(mMockDeviceManager, Mockito.times(1))
                .freeDevice(Mockito.eq(stub), Mockito.eq(FreeDeviceState.AVAILABLE));
    }

    /**
     * Test when sending a FREE_DEVICE and passing a wildcard * as a device serial results in
     * freeing all the device allocated in DeviceTracker.
     */
    @Test
    public void testProcessClientOperations_processFree_wildcard() throws Exception {
        List<ITestDevice> listAllocated = new ArrayList<>();
        listAllocated.add(Mockito.mock(ITestDevice.class));
        listAllocated.add(Mockito.mock(ITestDevice.class));
        doReturn(listAllocated).when(mMockDeviceTracker).freeAll();

        String buf = "{version=\"8\", type=\"FREE_DEVICE\", serial=\"*\"}";
        InputStream data = new ByteArrayInputStream(buf.getBytes());
        BufferedReader in = new BufferedReader(new InputStreamReader(data));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(out);
        mRemoteManager.processClientOperations(in, pw);
        pw.flush();
        assertEquals("{}\n", out.toString());
        verify(mMockDeviceManager, Mockito.times(2))
                .freeDevice(Mockito.any(), Mockito.eq(FreeDeviceState.AVAILABLE));
    }
}
