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
import com.android.ddmlib.IDevice.DeviceState;
import com.android.tradefed.util.IRunUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.concurrent.TimeUnit;

/**
 * Unit Tests for {@link ManagedTestDeviceFactory}
 */
public class ManagedTestDeviceFactoryTest extends TestCase {

    ManagedTestDeviceFactory mFactory;
    IDeviceManager mMockDeviceManager;
    IDeviceMonitor mMockDeviceMonitor;
    IRunUtil mMockRunUtil;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockDeviceManager = EasyMock.createMock(IDeviceManager.class);
        mMockDeviceMonitor = EasyMock.createMock(IDeviceMonitor.class);
        mFactory = new ManagedTestDeviceFactory(true, mMockDeviceManager, mMockDeviceMonitor) ;
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
    }

    public void testIsSerialTcpDevice() {
        String input = "127.0.0.1:5555";
        assertTrue(mFactory.isTcpDeviceSerial(input));
    }

    public void testIsSerialTcpDevice_localhost() {
        String input = "localhost:54014";
        assertTrue(mFactory.isTcpDeviceSerial(input));
    }

    public void testIsSerialTcpDevice_notTcp() {
        String input = "00bf84d7d084cc84";
        assertFalse(mFactory.isTcpDeviceSerial(input));
    }

    public void testIsSerialTcpDevice_malformedPort() {
        String input = "127.0.0.1:999989";
        assertFalse(mFactory.isTcpDeviceSerial(input));
    }

    public void testIsSerialTcpDevice_nohost() {
        String input = ":5555";
        assertFalse(mFactory.isTcpDeviceSerial(input));
    }

    /**
     * Test that {@link ManagedTestDeviceFactory#checkFrameworkSupport(IDevice)} is true when the
     * device returns a proper 'pm' path.
     */
    public void testFrameworkAvailable() throws Exception {
        final CollectingOutputReceiver cor = new CollectingOutputReceiver();
        mFactory = new ManagedTestDeviceFactory(true, mMockDeviceManager, mMockDeviceMonitor) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                String response = ManagedTestDeviceFactory.EXPECTED_RES + "\n";
                cor.addOutput(response.getBytes(), 0, response.length());
                return cor;
            }
        };
        IDevice mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        String expectedCmd = String.format(ManagedTestDeviceFactory.CHECK_PM_CMD,
                ManagedTestDeviceFactory.EXPECTED_RES);
        mMockDevice.executeShellCommand(EasyMock.eq(expectedCmd), EasyMock.eq(cor),
                EasyMock.anyLong(), EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall();
        EasyMock.replay(mMockDevice);
        assertTrue(mFactory.checkFrameworkSupport(mMockDevice));
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that {@link ManagedTestDeviceFactory#checkFrameworkSupport(IDevice)} is false when the
     * device does not have the 'pm' binary.
     */
    public void testFrameworkNotAvailable() throws Exception {
        final CollectingOutputReceiver cor = new CollectingOutputReceiver();
        mFactory = new ManagedTestDeviceFactory(true, mMockDeviceManager, mMockDeviceMonitor) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                String response = "ls: /system/bin/pm: No such file or directory\n";
                cor.addOutput(response.getBytes(), 0, response.length());
                return cor;
            }
        };
        IDevice mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        String expectedCmd = String.format(ManagedTestDeviceFactory.CHECK_PM_CMD,
                ManagedTestDeviceFactory.EXPECTED_RES);
        mMockDevice.executeShellCommand(EasyMock.eq(expectedCmd), EasyMock.eq(cor),
                EasyMock.anyLong(), EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall();
        EasyMock.replay(mMockDevice);
        assertFalse(mFactory.checkFrameworkSupport(mMockDevice));
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that {@link ManagedTestDeviceFactory#checkFrameworkSupport(IDevice)} is retrying
     * because device doesn't return a proper answer. It should return True for default value.
     */
    public void testCheckFramework_emptyReturns() throws Exception {
        final CollectingOutputReceiver cor = new CollectingOutputReceiver();
        mFactory = new ManagedTestDeviceFactory(true, mMockDeviceManager, mMockDeviceMonitor) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                String response = "";
                cor.addOutput(response.getBytes(), 0, response.length());
                return cor;
            }
            @Override
            protected IRunUtil getRunUtil() {
                return mMockRunUtil;
            }
        };
        mMockRunUtil.sleep(500);
        EasyMock.expectLastCall().times(ManagedTestDeviceFactory.FRAMEWORK_CHECK_MAX_RETRY);
        IDevice mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn("SERIAL");
        EasyMock.expect(mMockDevice.getState()).andStubReturn(DeviceState.ONLINE);
        String expectedCmd = String.format(ManagedTestDeviceFactory.CHECK_PM_CMD,
                ManagedTestDeviceFactory.EXPECTED_RES);
        mMockDevice.executeShellCommand(EasyMock.eq(expectedCmd), EasyMock.eq(cor),
                EasyMock.anyLong(), EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().times(ManagedTestDeviceFactory.FRAMEWORK_CHECK_MAX_RETRY);
        EasyMock.replay(mMockDevice, mMockRunUtil);
        assertTrue(mFactory.checkFrameworkSupport(mMockDevice));
        EasyMock.verify(mMockDevice, mMockRunUtil);
    }
}
