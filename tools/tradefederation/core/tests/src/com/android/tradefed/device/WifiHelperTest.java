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

import static org.junit.Assert.*;

import com.android.tradefed.util.IRunUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.List;
import java.util.concurrent.TimeUnit;

/** Unit tests for {@link WifiHelper}. */
@RunWith(JUnit4.class)
public class WifiHelperTest {

    private ITestDevice mMockDevice;

    @Before
    public void setUp() throws Exception {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.executeShellCommand(WifiHelper.CHECK_PACKAGE_CMD))
                .andReturn(String.format("versionCode=%d", WifiHelper.PACKAGE_VERSION_CODE));
    }

    // tests for reimplementation
    @Test
    public void testBuildCommand_simple() {
        final String expected = "am instrument -e method \"meth\" -w " +
                WifiHelper.FULL_INSTRUMENTATION_NAME;
        final String cmd = WifiHelper.buildWifiUtilCmd("meth");
        assertEquals(expected, cmd);
    }

    @Test
    public void testBuildCommand_oneArg() {
        final String start = "am instrument ";
        final String piece1 = "-e method \"meth\" ";
        final String piece2 = "-e id \"45\" ";
        final String end = "-w " + WifiHelper.FULL_INSTRUMENTATION_NAME;

        final String cmd = WifiHelper.buildWifiUtilCmd("meth", "id", "45");
        // Do this piecewise since Map traverse order is arbitrary
        assertTrue(cmd.startsWith(start));
        assertTrue(cmd.contains(piece1));
        assertTrue(cmd.contains(piece2));
        assertTrue(cmd.endsWith(end));
    }

    @Test
    public void testBuildCommand_withSpace() {
        final String start = "am instrument ";
        final String piece1 = "-e method \"addWpaPskNetwork\" ";
        final String piece2 = "-e ssid \"With Space\" ";
        final String piece3 = "-e psk \"also has space\" ";
        final String end = "-w " + WifiHelper.FULL_INSTRUMENTATION_NAME;

        final String cmd = WifiHelper.buildWifiUtilCmd("addWpaPskNetwork", "ssid", "With Space",
                "psk", "also has space");
        // Do this piecewise since Map traverse order is arbitrary
        assertTrue(cmd.startsWith(start));
        assertTrue(cmd.contains(piece1));
        assertTrue(cmd.contains(piece2));
        assertTrue(cmd.contains(piece3));
        assertTrue(cmd.endsWith(end));
    }

    /**
     * Test {@link WifiHelper#waitForIp(long)} that gets invalid data on first attempt, but then
     * succeeds on second.
     */
    @Test
    public void testWaitForIp_failThenPass() throws Exception {
        MockTestDeviceHelper.injectShellResponse(mMockDevice, null, 5, TimeUnit.MINUTES, 0, "",
                false);
        MockTestDeviceHelper.injectShellResponse(mMockDevice, null, 5, TimeUnit.MINUTES, 0,
                "INSTRUMENTATION_RESULT: result=1.2.3.4", false);
        EasyMock.replay(mMockDevice);
        WifiHelper wifiHelper = new WifiHelper(mMockDevice) {
            @Override
            IRunUtil getRunUtil() {
                return EasyMock.createNiceMock(IRunUtil.class);
            }
        };
        assertTrue(wifiHelper.waitForIp(10 * 60 * 1000));
        // verify that two executeCommand attempt were made
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testStartMonitor() throws Exception {
        final long interval = 30 * 1000;
        final String urlToCheck = "urlToCheck";
        String expectedCommand = WifiHelper.buildWifiUtilCmd("startMonitor",
                "interval", Long.toString(interval), "urlToCheck", urlToCheck);
        MockTestDeviceHelper.injectShellResponse(mMockDevice, expectedCommand, 5, TimeUnit.MINUTES,
                0, "INSTRUMENTATION_RESULT: result=true", false);
        EasyMock.replay(mMockDevice);
        WifiHelper wifiHelper = new WifiHelper(mMockDevice);
        assertTrue(wifiHelper.startMonitor(interval, urlToCheck));
        // verify that executeCommand attempt were made
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testStopMonitor() throws Exception {
        MockTestDeviceHelper.injectShellResponse(mMockDevice, null, 5, TimeUnit.MINUTES, 0,
                "INSTRUMENTATION_RESULT: result=1,2,3,4,", false);
        EasyMock.replay(mMockDevice);
        WifiHelper wifiHelper = new WifiHelper(mMockDevice);
        List<Long> result = wifiHelper.stopMonitor();
        assertEquals(4, result.size());
        // verify that executeCommand attempt were made
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testStopMonitor_nullResult() throws Exception {
        MockTestDeviceHelper.injectShellResponse(mMockDevice, null, 5, TimeUnit.MINUTES, 0,
                "INSTRUMENTATION_RESULT: result=null", false);
        EasyMock.replay(mMockDevice);
        WifiHelper wifiHelper = new WifiHelper(mMockDevice);
        List<Long> result = wifiHelper.stopMonitor();
        assertEquals(0, result.size());
        // verify that executeCommand attempt were made
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testEnsureDeviceSetup_alternateVersionPattern() throws Exception {
        EasyMock.reset(mMockDevice);
        EasyMock.expect(mMockDevice.executeShellCommand(WifiHelper.CHECK_PACKAGE_CMD))
                .andReturn(String.format("versionCode=%d targetSdk=7",
                        WifiHelper.PACKAGE_VERSION_CODE));
        EasyMock.replay(mMockDevice);
        new WifiHelper(mMockDevice);
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testEnsureDeviceSetup_lowerVersion() throws Exception {
        EasyMock.reset(mMockDevice);
        EasyMock.expect(mMockDevice.executeShellCommand(WifiHelper.CHECK_PACKAGE_CMD))
                .andReturn(String.format("versionCode=%d", 10));
        EasyMock.expect(mMockDevice.installPackage(EasyMock.<File>anyObject(), EasyMock.eq(true)))
                .andReturn(null);
        EasyMock.replay(mMockDevice);
        new WifiHelper(mMockDevice);
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testEnsureDeviceSetup_alternateWifiUtilAPKPath() throws Exception {
        final String apkPath = "/path/to/WifiUtil.APK";
        EasyMock.reset(mMockDevice);
        EasyMock.expect(mMockDevice.executeShellCommand(WifiHelper.CHECK_PACKAGE_CMD))
                .andReturn(String.format("versionCode=%d", WifiHelper.PACKAGE_VERSION_CODE - 1));
        EasyMock.expect(mMockDevice.installPackage(EasyMock.<File>anyObject(), EasyMock.eq(true)))
                .andReturn(null);
        EasyMock.replay(mMockDevice);
        WifiHelper wifiHelper = new WifiHelper(mMockDevice, apkPath);
        File wifiUtilApkFile = wifiHelper.getWifiUtilApkFile();
        assertEquals(wifiUtilApkFile.getPath(), apkPath);
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testEnsureDeviceSetup_deleteAPK() throws Exception {
        EasyMock.reset(mMockDevice);
        EasyMock.expect(mMockDevice.executeShellCommand(WifiHelper.CHECK_PACKAGE_CMD))
                .andReturn(String.format("versionCode=%d", WifiHelper.PACKAGE_VERSION_CODE - 1));
        EasyMock.expect(mMockDevice.installPackage(EasyMock.<File>anyObject(), EasyMock.eq(true)))
                .andReturn(null);
        EasyMock.replay(mMockDevice);
        WifiHelper wifiHelper = new WifiHelper(mMockDevice);
        File wifiUtilApkFile = wifiHelper.getWifiUtilApkFile();
        assertFalse(wifiUtilApkFile.exists());
        EasyMock.verify(mMockDevice);
    }

    /** Test that {@link WifiHelper#cleanUp()} calls uninstall on the instrumentation package. */
    @Test
    public void testCleanPackage() throws Exception {
        EasyMock.expect(mMockDevice.uninstallPackage(WifiHelper.INSTRUMENTATION_PKG))
                .andReturn(null);
        EasyMock.replay(mMockDevice);
        WifiHelper wifiHelper = new WifiHelper(mMockDevice);
        wifiHelper.cleanUp();
        // verify that executeCommand attempt were made
        EasyMock.verify(mMockDevice);
    }
}
