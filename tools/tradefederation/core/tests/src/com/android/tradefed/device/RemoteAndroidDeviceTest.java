/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.device;

import com.android.ddmlib.IDevice;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

/**
 * Unit tests for {@link RemoteAndroidDevice}.
 */
public class RemoteAndroidDeviceTest extends TestCase {

    private static final String MOCK_DEVICE_SERIAL = "localhost:1234";
    private IDevice mMockIDevice;
    private IDeviceStateMonitor mMockStateMonitor;
    private IRunUtil mMockRunUtil;
    private IDeviceMonitor mMockDvcMonitor;
    private IDeviceRecovery mMockRecovery;
    private RemoteAndroidDevice mTestDevice;

    /**
     * A {@link TestDevice} that is suitable for running tests against
     */
    private class TestableRemoteAndroidDevice extends RemoteAndroidDevice {
        public TestableRemoteAndroidDevice() {
            super(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        }

        @Override
        protected IRunUtil getRunUtil() {
            return mMockRunUtil;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockIDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockIDevice.getSerialNumber()).andReturn(MOCK_DEVICE_SERIAL).anyTimes();
        mMockRecovery = EasyMock.createMock(IDeviceRecovery.class);
        mMockStateMonitor = EasyMock.createMock(IDeviceStateMonitor.class);
        mMockDvcMonitor = EasyMock.createMock(IDeviceMonitor.class);
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);

        // A TestDevice with a no-op recoverDevice() implementation
        mTestDevice = new TestableRemoteAndroidDevice();
        mTestDevice.setRecovery(mMockRecovery);
    }

    /**
     * Test {@link RemoteAndroidDevice#adbTcpConnect(String, String)} in a success case.
     */
    public void testAdbConnect() {
        CommandResult adbResult = new CommandResult();
        adbResult.setStatus(CommandStatus.SUCCESS);
        adbResult.setStdout("connected to");
        CommandResult adbResultConfirmation = new CommandResult();
        adbResultConfirmation.setStatus(CommandStatus.SUCCESS);
        adbResultConfirmation.setStdout("already connected to localhost:1234");
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(),
                EasyMock.eq("adb"), EasyMock.eq("connect"), EasyMock.eq(MOCK_DEVICE_SERIAL)))
                .andReturn(adbResult);
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(),
                EasyMock.eq("adb"), EasyMock.eq("connect"), EasyMock.eq(MOCK_DEVICE_SERIAL)))
                .andReturn(adbResultConfirmation);
        EasyMock.replay(mMockRunUtil);
        assertTrue(mTestDevice.adbTcpConnect("localhost", "1234"));
    }

    /**
     * Test {@link RemoteAndroidDevice#adbTcpConnect(String, String)} in a failure case.
     */
    public void testAdbConnect_fails() {
        CommandResult adbResult = new CommandResult();
        adbResult.setStatus(CommandStatus.SUCCESS);
        adbResult.setStdout("cannot connect");
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(),
                EasyMock.eq("adb"), EasyMock.eq("connect"), EasyMock.eq(MOCK_DEVICE_SERIAL)))
                .andReturn(adbResult).times(RemoteAndroidDevice.MAX_RETRIES);
        mMockRunUtil.sleep(EasyMock.anyLong());
        EasyMock.expectLastCall().times(RemoteAndroidDevice.MAX_RETRIES);
        EasyMock.replay(mMockRunUtil);
        assertFalse(mTestDevice.adbTcpConnect("localhost", "1234"));
    }

    /**
     * Test {@link RemoteAndroidDevice#adbTcpConnect(String, String)} in a case where adb connect
     * always return connect success (never really connected so confirmation: "already connected"
     * fails.
     */
    public void testAdbConnect_fails_confirmation() {
        CommandResult adbResult = new CommandResult();
        adbResult.setStatus(CommandStatus.SUCCESS);
        adbResult.setStdout("connected to");
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(),
                EasyMock.eq("adb"), EasyMock.eq("connect"), EasyMock.eq(MOCK_DEVICE_SERIAL)))
                .andReturn(adbResult).times(RemoteAndroidDevice.MAX_RETRIES * 2);
        mMockRunUtil.sleep(EasyMock.anyLong());
        EasyMock.expectLastCall().times(RemoteAndroidDevice.MAX_RETRIES);
        EasyMock.replay(mMockRunUtil);
        assertFalse(mTestDevice.adbTcpConnect("localhost", "1234"));
    }

    /**
     * Test {@link RemoteAndroidDevice#adbTcpDisconnect(String, String)}.
     */
    public void testAdbDisconnect() {
        CommandResult adbResult = new CommandResult();
        adbResult.setStatus(CommandStatus.SUCCESS);
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(),
                EasyMock.eq("adb"), EasyMock.eq("disconnect"), EasyMock.eq(MOCK_DEVICE_SERIAL)))
                .andReturn(adbResult);
        EasyMock.replay(mMockRunUtil);
        assertTrue(mTestDevice.adbTcpDisconnect("localhost", "1234"));
    }

    /**
     * Test {@link RemoteAndroidDevice#adbTcpDisconnect(String, String)} in a failure case.
     */
    public void testAdbDisconnect_fails() {
        CommandResult adbResult = new CommandResult();
        adbResult.setStatus(CommandStatus.FAILED);
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(),
                EasyMock.eq("adb"), EasyMock.eq("disconnect"), EasyMock.eq(MOCK_DEVICE_SERIAL)))
                .andReturn(adbResult);
        EasyMock.replay(mMockRunUtil);
        assertFalse(mTestDevice.adbTcpDisconnect("localhost", "1234"));
    }

    public void testCheckSerial() {
        EasyMock.replay(mMockIDevice);
        assertEquals("localhost", mTestDevice.getHostName());
        assertEquals("1234", mTestDevice.getPortNum());
    }

    public void testCheckSerial_invalid() {
        mMockIDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockIDevice.getSerialNumber()).andReturn("wrongserial").anyTimes();
        try {
            mTestDevice.getHostName();
        } catch (RuntimeException e) {
            // expected
            return;
        }
        fail("Wrong Serial should throw a RuntimeException");
    }

    public void testGetMacAddress() {
        assertNull(mTestDevice.getMacAddress());
    }
}
