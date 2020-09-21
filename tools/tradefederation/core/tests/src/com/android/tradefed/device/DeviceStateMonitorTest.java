/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.google.common.util.concurrent.SettableFuture;

import com.android.ddmlib.CollectingOutputReceiver;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.IDevice.DeviceState;
import com.android.tradefed.util.RunUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for {@link DeviceStateMonitorTest}.
 */
public class DeviceStateMonitorTest extends TestCase {
    private static final int WAIT_TIMEOUT_NOT_REACHED_MS = 500;
    private static final int WAIT_TIMEOUT_REACHED_MS = 100;
    private static final int WAIT_STATE_CHANGE_MS = 50;
    private static final int POLL_TIME_MS = 10;

    private static final String SERIAL_NUMBER = "1";
    private IDevice mMockDevice;
    private DeviceStateMonitor mMonitor;
    private IDeviceManager mMockMgr;
    private String mStubValue = "not found";

    @Override
    protected void setUp() {
        mStubValue = "not found";
        mMockMgr = EasyMock.createMock(IDeviceManager.class);
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceOnline()} when device is already online
     */
    public void testWaitForDeviceOnline_alreadyOnline() {
        assertEquals(mMockDevice, mMonitor.waitForDeviceOnline());
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceOnline()} when device becomes online
     */
    public void testWaitForDeviceOnline() {
        mMonitor.setState(TestDeviceState.NOT_AVAILABLE);
        Thread test =
                new Thread() {
                    @Override
                    public void run() {
                        RunUtil.getDefault().sleep(WAIT_STATE_CHANGE_MS);
                        mMonitor.setState(TestDeviceState.ONLINE);
                    }
                };
        test.setName(getClass().getCanonicalName() + "#testWaitForDeviceOnline");
        test.start();
        assertEquals(mMockDevice, mMonitor.waitForDeviceOnline());
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceOnline()} when device does not becomes online
     * within allowed time
     */
    public void testWaitForDeviceOnline_timeout() {
        mMonitor.setState(TestDeviceState.NOT_AVAILABLE);
        assertNull(mMonitor.waitForDeviceOnline(WAIT_TIMEOUT_REACHED_MS));
    }

    /**
     * Test {@link DeviceStateMonitor#isAdbTcp()} with a USB serial number.
     */
    public void testIsAdbTcp_usb() {
        IDevice mockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mockDevice.getSerialNumber()).andStubReturn("2345asdf");
        EasyMock.expect(mockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.replay(mockDevice);
        DeviceStateMonitor monitor = new DeviceStateMonitor(mMockMgr, mockDevice, true);
        assertFalse(monitor.isAdbTcp());
    }

    /**
     * Test {@link DeviceStateMonitor#isAdbTcp()} with a TCP serial number.
     */
    public void testIsAdbTcp_tcp() {
        IDevice mockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mockDevice.getSerialNumber()).andStubReturn("192.168.1.1:5555");
        EasyMock.expect(mockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.replay(mockDevice);
        DeviceStateMonitor monitor = new DeviceStateMonitor(mMockMgr, mockDevice, true);
        assertTrue(monitor.isAdbTcp());
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceNotAvailable(long)} when device is already
     * offline
     */
    public void testWaitForDeviceOffline_alreadyOffline() {
        mMonitor.setState(TestDeviceState.NOT_AVAILABLE);
        boolean res = mMonitor.waitForDeviceNotAvailable(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceNotAvailable(long)} when device becomes
     * offline
     */
    public void testWaitForDeviceOffline() {
        mMonitor.setState(TestDeviceState.ONLINE);
        Thread test =
                new Thread() {
                    @Override
                    public void run() {
                        RunUtil.getDefault().sleep(WAIT_STATE_CHANGE_MS);
                        mMonitor.setState(TestDeviceState.NOT_AVAILABLE);
                    }
                };
        test.setName(getClass().getCanonicalName() + "#testWaitForDeviceOffline");
        test.start();
        boolean res = mMonitor.waitForDeviceNotAvailable(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceNotAvailable(long)} when device doesn't become
     * offline
     */
    public void testWaitForDeviceOffline_timeout() {
        mMonitor.setState(TestDeviceState.ONLINE);
        boolean res = mMonitor.waitForDeviceNotAvailable(WAIT_TIMEOUT_REACHED_MS);
        assertFalse(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceShell(long)} when shell is available.
     */
    public void testWaitForShellAvailable() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return "/system/bin/adb";
                    }
                };
            }
        };
        boolean res = mMonitor.waitForDeviceShell(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceShell(long)} when shell become available.
     */
    public void testWaitForShell_becomeAvailable() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return mStubValue;
                    }
                };
            }
            @Override
            protected long getCheckPollTime() {
                return POLL_TIME_MS;
            }
        };
        Thread test =
                new Thread() {
                    @Override
                    public void run() {
                        RunUtil.getDefault().sleep(WAIT_STATE_CHANGE_MS);
                        mStubValue = "/system/bin/adb";
                    }
                };
        test.setName(getClass().getCanonicalName() + "#testWaitForShell_becomeAvailable");
        test.start();
        boolean res = mMonitor.waitForDeviceShell(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceShell(long)} when shell never become available.
     */
    public void testWaitForShell_timeout() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return mStubValue;
                    }
                };
            }
            @Override
            protected long getCheckPollTime() {
                return POLL_TIME_MS;
            }
        };
        boolean res = mMonitor.waitForDeviceShell(WAIT_TIMEOUT_REACHED_MS);
        assertFalse(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForBootComplete(long)} when boot is already complete.
     */
    public void testWaitForBootComplete() throws Exception {
        IDevice mFakeDevice = new StubDevice("serial") {
            @Override
            public Future<String> getSystemProperty(String name) {
                SettableFuture<String> f = SettableFuture.create();
                f.set("1");
                return f;
            }
        };
        mMonitor = new DeviceStateMonitor(mMockMgr, mFakeDevice, true);
        boolean res = mMonitor.waitForBootComplete(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForBootComplete(long)} when boot complete status change.
     */
    public void testWaitForBoot_becomeComplete() throws Exception {
        mStubValue = "0";
        IDevice mFakeDevice = new StubDevice("serial") {
            @Override
            public Future<String> getSystemProperty(String name) {
                SettableFuture<String> f = SettableFuture.create();
                f.set(mStubValue);
                return f;
            }
        };
        mMonitor = new DeviceStateMonitor(mMockMgr, mFakeDevice, true) {
            @Override
            protected long getCheckPollTime() {
                return POLL_TIME_MS;
            }
        };
        Thread test =
                new Thread() {
                    @Override
                    public void run() {
                        RunUtil.getDefault().sleep(WAIT_STATE_CHANGE_MS);
                        mStubValue = "1";
                    }
                };
        test.setName(getClass().getCanonicalName() + "#testWaitForBoot_becomeComplete");
        test.start();
        boolean res = mMonitor.waitForBootComplete(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForBootComplete(long)} when boot complete timeout.
     */
    public void testWaitForBoot_timeout() throws Exception {
        mStubValue = "0";
        IDevice mFakeDevice = new StubDevice("serial") {
            @Override
            public Future<String> getSystemProperty(String name) {
                SettableFuture<String> f = SettableFuture.create();
                f.set(mStubValue);
                return f;
            }
        };
        mMonitor = new DeviceStateMonitor(mMockMgr, mFakeDevice, true) {
            @Override
            protected long getCheckPollTime() {
                return POLL_TIME_MS;
            }
        };
        boolean res = mMonitor.waitForBootComplete(WAIT_TIMEOUT_REACHED_MS);
        assertFalse(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForPmResponsive(long)} when package manager is already
     * responsive.
     */
    public void testWaitForPmResponsive() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return "package:com.android.awesomeclass";
                    }
                };
            }
        };
        boolean res = mMonitor.waitForPmResponsive(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForPmResponsive(long)} when package manager becomes
     * responsive
     */
    public void testWaitForPm_becomeResponsive() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return mStubValue;
                    }
                };
            }
            @Override
            protected long getCheckPollTime() {
                return POLL_TIME_MS;
            }
        };
        Thread test =
                new Thread() {
                    @Override
                    public void run() {
                        RunUtil.getDefault().sleep(WAIT_STATE_CHANGE_MS);
                        mStubValue = "package:com.android.awesomeclass";
                    }
                };
        test.setName(getClass().getCanonicalName() + "#testWaitForPm_becomeResponsive");
        test.start();
        boolean res = mMonitor.waitForPmResponsive(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForPmResponsive(long)} when package manager check timeout
     * before becoming responsive.
     */
    public void testWaitForPm_timeout() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return mStubValue;
                    }
                };
            }
            @Override
            protected long getCheckPollTime() {
                return POLL_TIME_MS;
            }
        };
        boolean res = mMonitor.waitForPmResponsive(WAIT_TIMEOUT_REACHED_MS);
        assertFalse(res);
    }

    /**
     * Test {@link DeviceStateMonitor#getMountPoint(String)} return the cached mount point.
     */
    public void testgetMountPoint() throws Exception {
        String expectedMountPoint = "NOT NULL";
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        EasyMock.expect(mMockDevice.getMountPoint((String) EasyMock.anyObject()))
                .andStubReturn(expectedMountPoint);
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true);
        assertEquals(expectedMountPoint, mMonitor.getMountPoint(""));
    }

    /**
     * Test {@link DeviceStateMonitor#getMountPoint(String)} return the mount point that is not
     * cached.
     */
    public void testgetMountPoint_nonCached() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        EasyMock.expect(mMockDevice.getMountPoint((String) EasyMock.anyObject()))
                .andStubReturn(null);
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject());
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return "NONCACHED";
                    }
                };
            }
        };
        assertEquals("NONCACHED", mMonitor.getMountPoint(""));
    }

    /**
     * Test {@link DeviceStateMonitor#waitForStoreMount(long)} when mount point is already mounted
     */
    public void testWaitForStoreMount() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return "number 10 one";
                    }
                };
            }
            @Override
            protected long getCurrentTime() {
                return 10;
            }
            @Override
            public String getMountPoint(String mountName) {
                return "";
            }
        };
        boolean res = mMonitor.waitForStoreMount(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForStoreMount(long)} when mount point return permission
     * denied should return directly false.
     */
    public void testWaitForStoreMount_PermDenied() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return "/system/bin/sh: cat: /sdcard/1459376318045: Permission denied";
                    }
                };
            }
            @Override
            protected long getCurrentTime() {
                return 10;
            }
            @Override
            public String getMountPoint(String mountName) {
                return "";
            }
        };
        boolean res = mMonitor.waitForStoreMount(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertFalse(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForStoreMount(long)} when mount point become available
     */
    public void testWaitForStoreMount_becomeAvailable() throws Exception {
        mStubValue = null;
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return "number 10 one";
                    }
                };
            }
            @Override
            protected long getCurrentTime() {
                return 10;
            }
            @Override
            protected long getCheckPollTime() {
                return POLL_TIME_MS;
            }
            @Override
            public String getMountPoint(String mountName) {
                return mStubValue;
            }
        };
        Thread test =
                new Thread() {
                    @Override
                    public void run() {
                        RunUtil.getDefault().sleep(WAIT_STATE_CHANGE_MS);
                        mStubValue = "NOT NULL";
                    }
                };
        test.setName(getClass().getCanonicalName() + "#testWaitForStoreMount_becomeAvailable");
        test.start();
        boolean res = mMonitor.waitForStoreMount(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForStoreMount(long)} when mount point is available and
     * the output of the check (string in the file) become valid.
     */
    public void testWaitForStoreMount_outputBecomeValid() throws Exception {
        mStubValue = "INVALID";
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected CollectingOutputReceiver createOutputReceiver() {
                return new CollectingOutputReceiver() {
                    @Override
                    public String getOutput() {
                        return mStubValue;
                    }
                };
            }
            @Override
            protected long getCurrentTime() {
                return 10;
            }
            @Override
            protected long getCheckPollTime() {
                return POLL_TIME_MS;
            }
            @Override
            public String getMountPoint(String mountName) {
                return "NOT NULL";
            }
        };
        Thread test =
                new Thread() {
                    @Override
                    public void run() {
                        RunUtil.getDefault().sleep(WAIT_STATE_CHANGE_MS);
                        mStubValue = "number 10 one";
                    }
                };
        test.setName(getClass().getCanonicalName() + "#testWaitForStoreMount_outputBecomeValid");
        test.start();
        boolean res = mMonitor.waitForStoreMount(WAIT_TIMEOUT_NOT_REACHED_MS);
        assertTrue(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForStoreMount(long)} when mount point check timeout
     */
    public void testWaitForStoreMount_timeout() throws Exception {
        mStubValue = null;
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        mMockDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall().anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            protected long getCheckPollTime() {
                return POLL_TIME_MS;
            }
            @Override
            public String getMountPoint(String mountName) {
                return mStubValue;
            }
        };
        boolean res = mMonitor.waitForStoreMount(WAIT_TIMEOUT_REACHED_MS);
        assertFalse(res);
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceAvailable(long)} wait for device available
     * when device is already available.
     */
    public void testWaitForDeviceAvailable() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE).anyTimes();
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            public IDevice waitForDeviceOnline(long waitTime) {
                return mMockDevice;
            }
            @Override
            public boolean waitForBootComplete(long waitTime) {
                return true;
            }
            @Override
            protected boolean waitForPmResponsive(long waitTime) {
                return true;
            }
            @Override
            protected boolean waitForStoreMount(long waitTime) {
                return true;
            }
        };
        assertEquals(mMockDevice, mMonitor.waitForDeviceAvailable(WAIT_TIMEOUT_NOT_REACHED_MS));
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceAvailable(long)} when device is not online.
     */
    public void testWaitForDeviceAvailable_notOnline() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE).anyTimes();
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            public IDevice waitForDeviceOnline(long waitTime) {
                return null;
            }
        };
        assertNull(mMonitor.waitForDeviceAvailable(WAIT_TIMEOUT_NOT_REACHED_MS));
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceAvailable(long)} when device boot is not
     * complete.
     */
    public void testWaitForDeviceAvailable_notBootComplete() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE).anyTimes();
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            public IDevice waitForDeviceOnline(long waitTime) {
                return mMockDevice;
            }
            @Override
            public boolean waitForBootComplete(long waitTime) {
                return false;
            }
        };
        assertNull(mMonitor.waitForDeviceAvailable(WAIT_TIMEOUT_REACHED_MS));
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceAvailable(long)} when pm is not responsive.
     */
    public void testWaitForDeviceAvailable_pmNotResponsive() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE).anyTimes();
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            public IDevice waitForDeviceOnline(long waitTime) {
                return mMockDevice;
            }
            @Override
            public boolean waitForBootComplete(long waitTime) {
                return true;
            }
            @Override
            protected boolean waitForPmResponsive(long waitTime) {
                return false;
            }
        };
        assertNull(mMonitor.waitForDeviceAvailable(WAIT_TIMEOUT_REACHED_MS));
    }

    /**
     * Test {@link DeviceStateMonitor#waitForDeviceAvailable(long)} when mount point is not ready
     */
    public void testWaitForDeviceAvailable_notMounted() throws Exception {
        mMockDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getState()).andReturn(DeviceState.ONLINE).anyTimes();
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn(SERIAL_NUMBER).anyTimes();
        EasyMock.replay(mMockDevice);
        mMonitor = new DeviceStateMonitor(mMockMgr, mMockDevice, true) {
            @Override
            public IDevice waitForDeviceOnline(long waitTime) {
                return mMockDevice;
            }
            @Override
            public boolean waitForBootComplete(long waitTime) {
                return true;
            }
            @Override
            protected boolean waitForPmResponsive(long waitTime) {
                return true;
            }
            @Override
            protected boolean waitForStoreMount(long waitTime) {
                return false;
            }
        };
        assertNull(mMonitor.waitForDeviceAvailable(WAIT_TIMEOUT_REACHED_MS));
    }
}
