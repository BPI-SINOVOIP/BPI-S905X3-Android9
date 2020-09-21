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
import com.android.ddmlib.IShellOutputReceiver;
import com.android.tradefed.util.RunUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

/**
 * Unit Tests for {@link BackgroundDeviceAction}.
 */
public class BackgroundDeviceActionTest extends TestCase {

    private static final String MOCK_DEVICE_SERIAL = "serial";
    private static final int SHORT_WAIT_TIME_MS = 100;
    private static final int LONG_WAIT_TIME_MS = 200;
    private static final long JOIN_WAIT_TIME_MS = 5000;

    private IShellOutputReceiver mMockReceiver;
    private IDevice mMockIDevice;
    private ITestDevice mMockTestDevice;

    private BackgroundDeviceAction mBackgroundAction;

    private TestDeviceState mDeviceState = TestDeviceState.ONLINE;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDeviceState = TestDeviceState.ONLINE;
        mMockReceiver = EasyMock.createMock(IShellOutputReceiver.class);
        mMockReceiver.addOutput((byte[])EasyMock.anyObject(), EasyMock.anyInt(), EasyMock.anyInt());
        EasyMock.expectLastCall().anyTimes();
        mMockIDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockIDevice.getSerialNumber()).andReturn(MOCK_DEVICE_SERIAL).anyTimes();
        mMockTestDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockTestDevice.getSerialNumber()).andReturn(MOCK_DEVICE_SERIAL)
                .anyTimes();
        EasyMock.expect(mMockTestDevice.getIDevice()).andReturn(mMockIDevice).anyTimes();
    }

    /**
     * test {@link BackgroundDeviceAction#run()} should properly run and stop following the
     * thread life cycle
     */
    public void testBackgroundActionComplete() throws Exception {
        String action = "";
        EasyMock.expect(mMockTestDevice.getDeviceState()).andReturn(TestDeviceState.ONLINE)
                .anyTimes();
        mMockIDevice.executeShellCommand(EasyMock.eq(action), EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockTestDevice, mMockIDevice, mMockReceiver);
        mBackgroundAction =
                new BackgroundDeviceAction(action, "desc", mMockTestDevice, mMockReceiver, 0);
        mBackgroundAction.start();
        RunUtil.getDefault().sleep(SHORT_WAIT_TIME_MS);
        assertTrue(mBackgroundAction.isAlive());
        mBackgroundAction.cancel();
        mBackgroundAction.join(JOIN_WAIT_TIME_MS);
        assertFalse(mBackgroundAction.isAlive());
        mBackgroundAction.interrupt();
    }

    /**
     * test {@link BackgroundDeviceAction#run()} if shell throw an exception, thread will not
     * terminate but will go through {@link BackgroundDeviceAction#waitForDeviceRecovery(String)}
     */
    public void testBackgroundAction_shellException() throws Exception {
        String action = "";
        EasyMock.expect(mMockTestDevice.getDeviceState()).andStubReturn(mDeviceState);
        mMockIDevice.executeShellCommand(EasyMock.eq(action), EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().andThrow(new IOException()).anyTimes();
        EasyMock.replay(mMockTestDevice, mMockIDevice, mMockReceiver);
        mBackgroundAction =
                new BackgroundDeviceAction(action, "desc", mMockTestDevice, mMockReceiver, 0) {
            @Override
            protected void waitForDeviceRecovery(String exceptionType) {
                mDeviceState = TestDeviceState.NOT_AVAILABLE;
            }
            @Override
            public synchronized boolean isCancelled() {
                return super.isCancelled();
            }
        };
        mBackgroundAction.start();
        RunUtil.getDefault().sleep(LONG_WAIT_TIME_MS);
        assertTrue(mBackgroundAction.isAlive());
        mBackgroundAction.cancel();
        mBackgroundAction.join(JOIN_WAIT_TIME_MS);
        assertFalse(mBackgroundAction.isAlive());
        assertEquals(TestDeviceState.NOT_AVAILABLE, mDeviceState);
        mBackgroundAction.interrupt();
    }

    /**
     * test {@link BackgroundDeviceAction#waitForDeviceRecovery(String)} should not block if device
     * is online.
     */
    public void testwaitForDeviceRecovery_online() throws Exception {
        String action = "";
        EasyMock.expect(mMockTestDevice.getDeviceState()).andReturn(mDeviceState);
        EasyMock.replay(mMockTestDevice, mMockIDevice, mMockReceiver);
        mBackgroundAction =
                new BackgroundDeviceAction(action, "desc", mMockTestDevice, mMockReceiver, 0);
        Thread test = new Thread(new Runnable() {
            @Override
            public void run() {
                mBackgroundAction.waitForDeviceRecovery("IOException");
            }
        });
        test.setName(getClass().getCanonicalName() + "#testwaitForDeviceRecovery_online");
        test.start();
        // Specify a timeout for join, not to be stuck if broken.
        test.join(JOIN_WAIT_TIME_MS);
        assertFalse(test.isAlive());
        test.interrupt();
    }

    /**
     * test {@link BackgroundDeviceAction#waitForDeviceRecovery(String)} should  block if device
     * is offline.
     */
    public void testwaitForDeviceRecovery_blockOffline() throws Exception {
        String action = "";
        mDeviceState = TestDeviceState.NOT_AVAILABLE;
        EasyMock.expect(mMockTestDevice.getDeviceState()).andReturn(mDeviceState).anyTimes();
        EasyMock.replay(mMockTestDevice, mMockIDevice, mMockReceiver);
        mBackgroundAction =
                new BackgroundDeviceAction(action, "desc", mMockTestDevice, mMockReceiver, 0);
        Thread test = new Thread(new Runnable() {
            @Override
            public void run() {
                mBackgroundAction.waitForDeviceRecovery("IOException");
            }
        });
        test.setName(getClass().getCanonicalName() + "#testwaitForDeviceRecovery_blockOffline");
        test.start();
        // Specify a timeout for join, not to be stuck.
        test.join(LONG_WAIT_TIME_MS);
        // Thread should still be alive.
        assertTrue(test.isAlive());
        mBackgroundAction.cancel();
        test.interrupt();
    }
}
