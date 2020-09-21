/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.command.ICommandScheduler;
import com.android.tradefed.command.ICommandScheduler.IScheduledInvocationListener;
import com.android.tradefed.device.DeviceAllocationState;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.FreeDeviceState;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.easymock.EasyMock;
import org.easymock.IAnswer;

/**
 * Unit tests for {@link RemoteManager}.
 */
public class RemoteManagerFuncTest extends TestCase {

    private IDeviceManager mMockDeviceManager;
    private RemoteManager mRemoteMgr;
    private IRemoteClient mRemoteClient;
    private ICommandScheduler mMockScheduler;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockDeviceManager = EasyMock.createMock(IDeviceManager.class);
        mMockScheduler = EasyMock.createMock(ICommandScheduler.class);
        mRemoteMgr = new RemoteManager(mMockDeviceManager, mMockScheduler);
        // Extra short timeout for testing.
        mRemoteMgr.setRemoteManagerTimeout(100);
    }

    @Override
    protected void tearDown() throws Exception {
        if (mRemoteClient != null) {
            mRemoteClient.close();
        }
        if (mRemoteMgr != null) {
            mRemoteMgr.cancelAndWait();
        }
        super.tearDown();
    }

    /**
     * An integration test for client-manager interaction, that will allocate, then free a device.
     */
    public void testAllocateFree() throws Exception {
        ITestDevice device = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(device.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDeviceManager.forceAllocateDevice("serial")).andReturn(device);
        mMockDeviceManager.freeDevice(EasyMock.eq(device),
                EasyMock.eq(FreeDeviceState.AVAILABLE));

        EasyMock.replay(mMockDeviceManager, device);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        mRemoteClient.sendAllocateDevice("serial");
        mRemoteClient.sendFreeDevice("serial");
        EasyMock.verify(mMockDeviceManager);
    }

    /**
     * An integration test for client-manager interaction, that will add a command
     */
    public void testAddCommand() throws Exception {
        EasyMock.expect(mMockScheduler.addCommand(EasyMock.aryEq(new String[] {
                "arg1", "arg2"
        }), EasyMock.anyInt())).andReturn(true);

        EasyMock.replay(mMockScheduler);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        mRemoteClient.sendAddCommand(3, "arg1", "arg2");
        EasyMock.verify(mMockScheduler);
    }

    /**
     * An integration test for client-manager interaction, that will add a command file
     */
    public void testAddCommandFile() throws Exception {
        final String cmdFile = "cmd.txt";
        List<String> extraArgs = Arrays.asList("foo", "bar");
        mMockScheduler.addCommandFile(EasyMock.eq(cmdFile), EasyMock.eq(extraArgs));

        EasyMock.replay(mMockScheduler);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        mRemoteClient.sendAddCommandFile(cmdFile, extraArgs);
        EasyMock.verify(mMockScheduler);
    }

    /**
     * An integration test for client-manager interaction, that will allocate, then close the
     * connection. Verifies that closing frees all devices.
     */
    @SuppressWarnings("deprecation")
    public void testAllocateClose() throws Exception {
        ITestDevice device = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(device.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDeviceManager.forceAllocateDevice("serial")).andReturn(device);
        mMockDeviceManager.freeDevice(EasyMock.eq(device),
                EasyMock.eq(FreeDeviceState.AVAILABLE));

        EasyMock.replay(mMockDeviceManager, device);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        mRemoteClient.sendAllocateDevice("serial");
        mRemoteClient.sendClose();
        mRemoteClient.close();
        mRemoteMgr.join();
        EasyMock.verify(mMockDeviceManager);
    }

    /**
     * An integration test for client-manager interaction, that will allocate, then frees all
     * devices.
     */
    public void testAllocateFreeAll() throws Exception {
        ITestDevice device = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(device.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDeviceManager.forceAllocateDevice("serial")).andReturn(device);
        mMockDeviceManager.freeDevice(EasyMock.eq(device),
                EasyMock.eq(FreeDeviceState.AVAILABLE));

        EasyMock.replay(mMockDeviceManager, device);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        mRemoteClient.sendAllocateDevice("serial");
        mRemoteClient.sendFreeDevice("*");
        EasyMock.verify(mMockDeviceManager);
    }

    /**
     * Test attempt to free an unknown device
     */
    public void testFree_unknown() throws Exception {
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        try {
            mRemoteClient.sendFreeDevice("foo");
            fail("RemoteException not thrown");
        } catch (RemoteException e) {
            // expected
        }
    }

    /**
     * An integration test for {@link ListDevicesOp}
     */
    public void testListDevices() throws Exception {
        List<DeviceDescriptor> deviceList =
                new ArrayList<DeviceDescriptor>(2);
        deviceList.add(
            new DeviceDescriptor(
                "serial",
                false,
                DeviceAllocationState.Available,
                "tuna",
                "toro",
                "18",
                "JWR67C",
                "4")
            );
        deviceList.add(
            new DeviceDescriptor(
                "serial2",
                false,
                DeviceAllocationState.Allocated,
                "herring",
                "crespo",
                "15",
                "IMM767",
                "5")
            );
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(deviceList);
        EasyMock.replay(mMockDeviceManager);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        List<DeviceDescriptor> returnedDevices = mRemoteClient.sendListDevices();
        assertEquals(2, returnedDevices.size());
        assertEquals("serial", returnedDevices.get(0).getSerial());
        assertFalse(returnedDevices.get(0).isStubDevice());
        assertEquals(DeviceAllocationState.Available, returnedDevices.get(0).getState());
        assertEquals("tuna", returnedDevices.get(0).getProduct());
        assertEquals("toro", returnedDevices.get(0).getProductVariant());
        assertEquals("18", returnedDevices.get(0).getSdkVersion());
        assertEquals("JWR67C", returnedDevices.get(0).getBuildId());
        assertEquals("4", returnedDevices.get(0).getBatteryLevel());
        assertEquals("serial2", returnedDevices.get(1).getSerial());
        assertFalse(returnedDevices.get(1).isStubDevice());
        assertEquals(DeviceAllocationState.Allocated, returnedDevices.get(1).getState());
        assertEquals("herring", returnedDevices.get(1).getProduct());
        assertEquals("crespo", returnedDevices.get(1).getProductVariant());
        assertEquals("15", returnedDevices.get(1).getSdkVersion());
        assertEquals("IMM767", returnedDevices.get(1).getBuildId());
        assertEquals("5", returnedDevices.get(1).getBatteryLevel());
        EasyMock.verify(mMockDeviceManager);
    }

    /**
     * An integration test for normal case {@link ExecCommandOp}
     */
    public void testExecCommand() throws Exception {
        ITestDevice device = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(device.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDeviceManager.forceAllocateDevice("serial")).andReturn(device);
        mMockDeviceManager.freeDevice(EasyMock.eq(device), EasyMock.eq(FreeDeviceState.AVAILABLE));
        String[] args = new String[] {
            "instrument"
        };
        mMockScheduler.execCommand((IScheduledInvocationListener)EasyMock.anyObject(),
                EasyMock.eq(device), EasyMock.aryEq(args));

        EasyMock.replay(mMockDeviceManager, device, mMockScheduler);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        mRemoteClient.sendAllocateDevice("serial");
        mRemoteClient.sendExecCommand("serial", args);
        mRemoteClient.sendFreeDevice("serial");
        EasyMock.verify(mMockDeviceManager, mMockScheduler);
    }

    /**
     * An integration test for consecutive executes {@link ExecCommandOp}
     */
    public void testConsecutiveExecCommand() throws Exception {
        final ITestDevice device = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(device.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDeviceManager.forceAllocateDevice("serial")).andReturn(device);
        mMockDeviceManager.freeDevice(EasyMock.eq(device), EasyMock.eq(FreeDeviceState.AVAILABLE));
        String[] args = new String[] {
            "instrument"
        };
        mMockScheduler.execCommand((IScheduledInvocationListener)EasyMock.anyObject(),
                EasyMock.eq(device), EasyMock.aryEq(args));
        IAnswer<Object> commandSuccessAnswer = new IAnswer<Object>() {
            @SuppressWarnings("unchecked")
            @Override
            public Object answer() {
              ExecCommandTracker commandTracker =
                  (ExecCommandTracker) EasyMock.getCurrentArguments()[0];
              IInvocationContext nullMeta = new InvocationContext();
              nullMeta.addAllocatedDevice("device", device);
              Map<ITestDevice, FreeDeviceState> state = new HashMap<>();
              state.put(device, FreeDeviceState.UNAVAILABLE);
              commandTracker.invocationComplete(nullMeta, state);
              return null;
            }
        };
        EasyMock.expectLastCall().andAnswer(commandSuccessAnswer);

        mMockScheduler.execCommand((IScheduledInvocationListener)EasyMock.anyObject(),
            EasyMock.eq(device), EasyMock.aryEq(args));
        EasyMock.replay(mMockDeviceManager, device, mMockScheduler);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        mRemoteClient.sendAllocateDevice("serial");
        // First command succeeds right way.
        mRemoteClient.sendExecCommand("serial", args);
        // Second command will be scheduled but will not finish.
        mRemoteClient.sendExecCommand("serial", args);
        // Third command will fail since the second command is still executing.
        try {
            mRemoteClient.sendExecCommand("serial", args);
            fail("did not receive RemoteException");
        } catch (RemoteException e) {
            // expected
        }
        mRemoteClient.sendFreeDevice("serial");
        EasyMock.verify(mMockDeviceManager, mMockScheduler);
    }

    /**
     * An integration test for case where device was not allocated before {@link ExecCommandOp}
     */
    public void testExecCommand_noallocate() throws Exception {
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        try {
            mRemoteClient.sendExecCommand("serial", new String[] {"instrument"});
        } catch (RemoteException e) {
            // expected
            return;
        }
        fail("did not receive RemoteException");
    }

    /**
     * Happy-path test for a handover start
     */
    public void testHandover() throws Exception {
        final int port = 88;
        EasyMock.expect(mMockScheduler.handoverShutdown(port)).andReturn(Boolean.TRUE);

        EasyMock.replay(mMockScheduler);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int mgrPort = mRemoteMgr.getPort();
        assertTrue(mgrPort != -1);
        mRemoteClient = RemoteClient.connect(mgrPort);
        mRemoteClient.sendStartHandover(port);
        // disgusting sleep alert! TODO: change to a wait-notify thingy
        Thread.sleep(100);
        EasyMock.verify(mMockScheduler);
    }

    /**
     * Basic test for a handover init complete op
     */
    public void testHandoverInit() throws Exception {
        // expect
        mMockScheduler.handoverInitiationComplete();

        EasyMock.replay(mMockScheduler);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int mgrPort = mRemoteMgr.getPort();
        assertTrue(mgrPort != -1);
        mRemoteClient = RemoteClient.connect(mgrPort);
        mRemoteClient.sendHandoverInitComplete();
        // disgusting sleep alert! TODO: change to a wait-notify thingy
        Thread.sleep(100);
        EasyMock.verify(mMockScheduler);
    }

    /**
     * Test {@link GetLastCommandResultOp} result when device is unknown
     * @throws Exception
     */
    public void testGetLastCommandResult_unknownDevice() throws Exception {
        ICommandResultHandler mockHandler = EasyMock.createStrictMock(ICommandResultHandler.class);
        mockHandler.notAllocated();
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();

        int mgrPort = mRemoteMgr.getPort();
        assertTrue(mgrPort != -1);
        mRemoteClient = RemoteClient.connect(mgrPort);
        EasyMock.replay(mockHandler);

        mRemoteClient.sendGetLastCommandResult("foo", mockHandler);

        EasyMock.verify(mockHandler);
    }

    /**
     * Test {@link GetLastCommandResultOp} result when there is no active command
     */
    public void testGetLastCommandResult_noActiveCommand() throws Exception {
        ICommandResultHandler mockHandler = EasyMock.createStrictMock(ICommandResultHandler.class);
        mockHandler.noActiveCommand();
        ITestDevice device = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(device.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDeviceManager.forceAllocateDevice("serial")).andReturn(device);
        mMockDeviceManager.freeDevice(EasyMock.eq(device), EasyMock.eq(FreeDeviceState.AVAILABLE));
        EasyMock.replay(mMockDeviceManager, device, mockHandler);

        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int mgrPort = mRemoteMgr.getPort();
        assertTrue(mgrPort != -1);
        mRemoteClient = RemoteClient.connect(mgrPort);

        mRemoteClient.sendAllocateDevice("serial");
        mRemoteClient.sendGetLastCommandResult("serial", mockHandler);
        mRemoteClient.sendFreeDevice("serial");

        EasyMock.verify(mockHandler, mMockDeviceManager);
    }

    /**
     * Test {@link GetLastCommandResultOp} result when command is executing
     */
    public void testGetLastCommandResult_executing() throws Exception {
        ICommandResultHandler mockHandler = EasyMock.createStrictMock(ICommandResultHandler.class);
        mockHandler.stillRunning();
        ITestDevice device = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(device.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDeviceManager.forceAllocateDevice("serial")).andReturn(device);
        mMockDeviceManager.freeDevice(EasyMock.eq(device), EasyMock.eq(FreeDeviceState.AVAILABLE));
        String[] args = new String[] {
            "instrument"
        };
        mMockScheduler.execCommand((IScheduledInvocationListener)EasyMock.anyObject(),
                EasyMock.eq(device), EasyMock.aryEq(args));

        EasyMock.replay(mMockDeviceManager, device, mMockScheduler, mockHandler);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        mRemoteClient.sendAllocateDevice("serial");
        mRemoteClient.sendExecCommand("serial", args);
        mRemoteClient.sendGetLastCommandResult("serial", mockHandler);
        mRemoteClient.sendFreeDevice("serial");
        EasyMock.verify(mMockDeviceManager, mockHandler);
    }

    /**
     * Test {@link GetLastCommandResultOp} result when commmand fails due to a not available device.
     */
    @SuppressWarnings("unchecked")
    public void testGetLastCommandResult_notAvail() throws Exception {
        ICommandResultHandler mockHandler = EasyMock.createStrictMock(ICommandResultHandler.class);
        mockHandler.failure((String)EasyMock.anyObject(), EasyMock.eq(FreeDeviceState.UNAVAILABLE),
                (Map<String, String>)EasyMock.anyObject());
        final ITestDevice device = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(device.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDeviceManager.forceAllocateDevice("serial")).andReturn(device);
        // TODO: change to not available
        mMockDeviceManager.freeDevice(EasyMock.eq(device), EasyMock.eq(FreeDeviceState.AVAILABLE));
        String[] args = new String[] {
            "instrument"
        };
        mMockScheduler.execCommand((IScheduledInvocationListener)EasyMock.anyObject(),
                EasyMock.eq(device), EasyMock.aryEq(args));
        IAnswer<Void> invErrorAnswer = new IAnswer<Void>() {
            @Override
            public Void answer() throws Throwable {
                IScheduledInvocationListener listener =
                        (IScheduledInvocationListener)EasyMock.getCurrentArguments()[0];
                IInvocationContext nullMeta = new InvocationContext();
                nullMeta.addAllocatedDevice("device", device);
                nullMeta.addDeviceBuildInfo("device", new BuildInfo());
                listener.invocationStarted(nullMeta);
                listener.invocationFailed(new DeviceNotAvailableException());
                listener.invocationEnded(1);
                Map<ITestDevice, FreeDeviceState> state = new HashMap<>();
                state.put(device, FreeDeviceState.UNAVAILABLE);
                listener.invocationComplete(nullMeta, state);
                return null;
            }
        };
        EasyMock.expectLastCall().andAnswer(invErrorAnswer);

        EasyMock.replay(mMockDeviceManager, device, mMockScheduler, mockHandler);
        mRemoteMgr.connectAnyPort();
        mRemoteMgr.start();
        int port = mRemoteMgr.getPort();
        assertTrue(port != -1);
        mRemoteClient = RemoteClient.connect(port);
        mRemoteClient.sendAllocateDevice("serial");
        mRemoteClient.sendExecCommand("serial", args);
        mRemoteClient.sendGetLastCommandResult("serial", mockHandler);
        mRemoteClient.sendFreeDevice("serial");
        EasyMock.verify(mMockDeviceManager, mockHandler, mMockScheduler);
    }
}
