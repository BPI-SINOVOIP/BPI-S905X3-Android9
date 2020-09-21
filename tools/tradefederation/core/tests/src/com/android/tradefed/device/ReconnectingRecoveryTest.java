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
package com.android.tradefed.device;

import com.android.ddmlib.IDevice;
import com.android.tradefed.util.IRunUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

/**
 * Unit tests for {@link ReconnectingRecovery}.
 */
public class ReconnectingRecoveryTest extends TestCase {

    private static final String SERIAL = "serial";
    private IDevice mMockDevice;
    private IDeviceStateMonitor mMockMonitor;
    private IRunUtil mMockRunUtil;
    private ReconnectingRecovery mRecovery;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mRecovery = new ReconnectingRecovery() {
            @Override
            protected IRunUtil getRunUtil() {
                return mMockRunUtil;
            }
        };
        mMockMonitor = EasyMock.createMock(IDeviceStateMonitor.class);
        EasyMock.expect(mMockMonitor.getSerialNumber()).andStubReturn(SERIAL);
        mMockDevice = EasyMock.createMock(IDevice.class);
    }

    /**
     * Test {@link ReconnectingRecovery#recoverDevice(IDeviceStateMonitor, boolean)}
     * when device is actually recoverable upon the first attempt.
     */
    public final void testRecoverDevice_successOnFirstTry() throws DeviceNotAvailableException {
        expectInitialDisconnectConnectAttempt();
        EasyMock.expect(mMockMonitor.waitForDeviceOnline()).andReturn(mMockDevice);
        EasyMock.expect(mMockMonitor.waitForDeviceShell(EasyMock.anyLong())).andReturn(true);
        EasyMock.expect(mMockMonitor.waitForDeviceAvailable()).andReturn(mMockDevice);
        replayMocks();
        mRecovery.recoverDevice(mMockMonitor, false);
        verifyMocks();
    }

    /**
     * Test {@link ReconnectingRecovery#recoverDevice(IDeviceStateMonitor, boolean)}
     * when device is actually recoverable, but not on the first attempt.
     */
    public final void testRecoverDevice_successRetrying() throws DeviceNotAvailableException {
        expectInitialDisconnectConnectAttempt();
        // fail 1st attempt
        EasyMock.expect(mMockMonitor.waitForDeviceOnline()).andReturn(null);
        // then it should retry at least once
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), connectCommand())).andReturn(
                null);
        EasyMock.expect(mMockMonitor.waitForDeviceOnline()).andReturn(mMockDevice);
        EasyMock.expect(mMockMonitor.waitForDeviceShell(EasyMock.anyLong())).andReturn(true);
        EasyMock.expect(mMockMonitor.waitForDeviceAvailable()).andReturn(mMockDevice);
        replayMocks();
        mRecovery.recoverDevice(mMockMonitor, false);
        verifyMocks();
    }

    /**
     * Test {@link ReconnectingRecovery#recoverDevice(IDeviceStateMonitor, boolean)}
     * when device is actually irrecoverable.
     */
    public final void testRecoverDevice_failure() throws DeviceNotAvailableException {
        expectInitialDisconnectConnectAttempt();
        EasyMock.expect(mMockMonitor.waitForDeviceOnline()).andStubReturn(null);
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), connectCommand()))
                .andStubReturn(null);
        EasyMock.expect(mMockMonitor.waitForDeviceShell(EasyMock.anyLong())).andReturn(true);
        EasyMock.expect(mMockMonitor.waitForDeviceAvailable()).andReturn(null);
        replayMocks();
        try {
            mRecovery.recoverDevice(mMockMonitor, false);
            fail("DeviceUnresponsiveException not thrown");
        } catch (DeviceUnresponsiveException e) {
            assertTrue(true);
        }
        verifyMocks();
    }

    private void expectInitialDisconnectConnectAttempt() {
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), disconnectCommand()))
                .andStubReturn(null);
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), connectCommand()))
                .andStubReturn(null);
    }

    public final void testRecoverDeviceBootloader_notImplemented()
            throws DeviceNotAvailableException {
        replayMocks();
        try {
            mRecovery.recoverDeviceBootloader(mMockMonitor);
            fail("should have thrown an UnsupportedOperationException");
        } catch (java.lang.UnsupportedOperationException e) {
            // expected
        }
        verifyMocks();
    }

    private String[] disconnectCommand() {
        return new String[] { EasyMock.eq("adb"), EasyMock.eq("disconnect"), EasyMock.eq(SERIAL) };
    }

    private String[] connectCommand() {
        return new String[] { EasyMock.eq("adb"), EasyMock.eq("connect"), EasyMock.eq(SERIAL) };
    }

    /**
     * Verify all the mock objects
     */
    private void verifyMocks() {
        EasyMock.verify(mMockRunUtil, mMockMonitor, mMockDevice);
    }

    /**
     * Switch all the mock objects to replay mode
     */
    private void replayMocks() {
        EasyMock.replay(mMockRunUtil, mMockMonitor, mMockDevice);
    }
}
