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
package com.android.tradefed.util.sl4a;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.IRunUtil;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.io.IOException;

/**
 * Test class for {@link Sl4aClient}.
 */
public class Sl4aClientTest {

    private Sl4aClient mClient = null;
    private FakeSocketServerHelper mDeviceServer;
    private ITestDevice mMockDevice;
    private IRunUtil mMockRunUtil;

    @Before
    public void setUp() throws IOException {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mDeviceServer = new FakeSocketServerHelper();
        mClient = new Sl4aClient(mMockDevice, mDeviceServer.getPort(), mDeviceServer.getPort()) {
            @Override
            protected IRunUtil getRunUtil() {
                return mMockRunUtil;
            }
            @Override
            protected void startEventDispatcher() throws DeviceNotAvailableException {
                // ignored
            }
        };
    }

    @After
    public void tearDown() throws IOException {
        if (mDeviceServer != null) {
            mDeviceServer.close();
        }
    }

    /**
     * Test for {@link Sl4aClient#isSl4ARunning()} when sl4a is running.
     */
    @Test
    public void testIsSl4ARunning() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.executeShellCommand(Sl4aClient.IS_SL4A_RUNNING_CMD))
                .andReturn("system    3968   452 1127644  49448 epoll_wait   ae1217f8 S "
                        + "com.googlecode.android_scripting");
        EasyMock.expect(mMockDevice.executeShellCommand(Sl4aClient.IS_SL4A_RUNNING_CMD_OLD))
                .andReturn(
                        "system    3968   452 1127644  49448 epoll_wait   ae1217f8 S "
                                + "com.googlecode.android_scripting");
        EasyMock.replay(mMockDevice);
        Assert.assertTrue(mClient.isSl4ARunning());
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test for {@link Sl4aClient#isSl4ARunning()} when sl4a is not running.
     */
    @Test
    public void testIsSl4ARunning_notRunning() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.executeShellCommand(Sl4aClient.IS_SL4A_RUNNING_CMD))
                .andReturn("");
        EasyMock.expect(mMockDevice.executeShellCommand(Sl4aClient.IS_SL4A_RUNNING_CMD_OLD))
                .andReturn("");
        EasyMock.replay(mMockDevice);
        Assert.assertFalse(mClient.isSl4ARunning());
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test for {@link Sl4aClient#startSl4A()} when sl4a does not starts properly.
     */
    @Test
    public void testStartSl4A_notRunning() throws DeviceNotAvailableException {
        final String cmd = String.format(Sl4aClient.SL4A_LAUNCH_CMD, mDeviceServer.getPort());
        EasyMock.expect(mMockDevice.executeShellCommand(cmd))
                .andReturn("");
        EasyMock.expect(mMockDevice.executeShellCommand(Sl4aClient.IS_SL4A_RUNNING_CMD))
                .andReturn("");
        EasyMock.expect(mMockDevice.executeShellCommand(Sl4aClient.IS_SL4A_RUNNING_CMD_OLD))
                .andReturn("");
        mMockRunUtil.sleep(EasyMock.anyLong());
        EasyMock.expectLastCall();
        EasyMock.replay(mMockDevice, mMockRunUtil);
        try {
            mClient.startSl4A();
            Assert.fail("Should have thrown an exception");
        } catch (RuntimeException expected) {
            // expected
        }
        EasyMock.verify(mMockDevice, mMockRunUtil);
    }

    /**
     * Helper to set the mocks and expectation to starts SL4A.
     */
    private void setupStartExpectation() throws DeviceNotAvailableException {
        final String cmd = String.format(Sl4aClient.SL4A_LAUNCH_CMD, mDeviceServer.getPort());
        EasyMock.expect(mMockDevice.executeShellCommand(cmd))
                .andReturn("");
        EasyMock.expect(mMockDevice.executeShellCommand(Sl4aClient.IS_SL4A_RUNNING_CMD))
        .andReturn("system    3968   452 1127644  49448 epoll_wait   ae1217f8 S "
                + "com.googlecode.android_scripting");
        EasyMock.expect(mMockDevice.executeShellCommand(Sl4aClient.IS_SL4A_RUNNING_CMD_OLD))
                .andReturn(
                        "system    3968   452 1127644  49448 epoll_wait   ae1217f8 S "
                                + "com.googlecode.android_scripting");
        mMockRunUtil.sleep(EasyMock.anyLong());
        EasyMock.expectLastCall();
        EasyMock.expect(mMockDevice.executeShellCommand(Sl4aClient.STOP_SL4A_CMD)).andReturn("");
        EasyMock.expect(mMockDevice.executeAdbCommand("forward", "tcp:" + mDeviceServer.getPort(),
                "tcp:" + mDeviceServer.getPort())).andReturn("");
        EasyMock.expect(mMockDevice.executeAdbCommand("forward", "--list")).andReturn("");
        EasyMock.expect(mMockDevice.executeAdbCommand("forward", "--remove",
                "tcp:" + mDeviceServer.getPort())).andReturn("");
    }

    /**
     * Test for {@link Sl4aClient#startSl4A()} when sl4a does starts properly.
     */
    @Test
    public void testStartSl4A() throws DeviceNotAvailableException {
        mDeviceServer.start();
        setupStartExpectation();
        EasyMock.replay(mMockDevice, mMockRunUtil);
        try {
            mClient.startSl4A();
        } finally {
            mClient.close();
        }
        EasyMock.verify(mMockDevice, mMockRunUtil);
    }

    /**
     * Test for {@link Sl4aClient#rpcCall(String, Object...)} and the response parsing for a
     * boolean result.
     */
    @Test
    public void testRpcCall_booleanResponse() throws DeviceNotAvailableException, IOException {
        mDeviceServer.start();
        setupStartExpectation();
        EasyMock.replay(mMockDevice, mMockRunUtil);
        try {
            mClient.startSl4A();
            Object rep = mClient.rpcCall("getBoolean", false);
            Assert.assertEquals(true, rep);
        } finally {
            mClient.close();
        }
        EasyMock.verify(mMockDevice, mMockRunUtil);
    }

    /**
     * Test for {@link Sl4aClient#startSL4A(ITestDevice, File)} throws an exception if sl4a apk
     * provided does not exist.
     */
    @Test
    public void testCreateSl4aClient() throws Exception {
        final String fakePath = "/fake/random/path";
        EasyMock.replay(mMockDevice);
        try {
            Sl4aClient.startSL4A(mMockDevice, new File(fakePath));
            fail("Should have thrown an exception");
        } catch (RuntimeException expected) {
            assertEquals(String.format("Sl4A apk '%s' was not found.", fakePath),
                    expected.getMessage());
        }
        EasyMock.verify(mMockDevice);
    }
}
