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

import static org.mockito.Mockito.doThrow;

import com.android.ddmlib.AdbCommandRejectedException;
import com.android.ddmlib.FileListingService.FileEntry;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.IDevice.DeviceState;
import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.SyncException;
import com.android.ddmlib.SyncException.SyncError;
import com.android.ddmlib.SyncService;
import com.android.ddmlib.SyncService.ISyncProgressMonitor;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.Bugreport;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.StreamUtil;

import com.google.common.util.concurrent.SettableFuture;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.mockito.Mockito;

import java.io.File;
import java.io.IOException;
import java.time.Clock;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for {@link NativeDevice}.
 */
public class NativeDeviceTest extends TestCase {

    private static final String MOCK_DEVICE_SERIAL = "serial";
    private static final String FAKE_NETWORK_SSID = "FakeNet";
    private static final String FAKE_NETWORK_PASSWORD ="FakePass";

    private IDevice mMockIDevice;
    private TestableAndroidNativeDevice mTestDevice;
    private IDeviceRecovery mMockRecovery;
    private IDeviceStateMonitor mMockStateMonitor;
    private IRunUtil mMockRunUtil;
    private IWifiHelper mMockWifi;
    private IDeviceMonitor mMockDvcMonitor;

    /**
     * A {@link TestDevice} that is suitable for running tests against
     */
    private class TestableAndroidNativeDevice extends NativeDevice {

        public boolean wasCalled = false;

        public TestableAndroidNativeDevice() {
            super(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        }

        @Override
        public void postBootSetup() {
            // too annoying to mock out postBootSetup actions everyone, so do nothing
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
        mMockWifi = EasyMock.createMock(IWifiHelper.class);
        mMockWifi.cleanUp();
        EasyMock.expectLastCall().anyTimes();

        // A TestDevice with a no-op recoverDevice() implementation
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public void recoverDevice() throws DeviceNotAvailableException {
                // ignore
            }

            @Override
            public IDevice getIDevice() {
                return mMockIDevice;
            }

            @Override
            IWifiHelper createWifiHelper() {
                return mMockWifi;
            }
        };
        mTestDevice.setRecovery(mMockRecovery);
        mTestDevice.setCommandTimeout(100);
        mTestDevice.setLogStartDelay(-1);
    }

    /**
     * Test return exception for package installation
     * {@link NativeDevice#installPackage(File, boolean, String...)}.
     */
    public void testInstallPackages_exception() {
        try {
            mTestDevice.installPackage(new File(""), false);
        } catch (UnsupportedOperationException onse) {
            return;
        } catch (DeviceNotAvailableException e) {
            fail("installPackage should have thrown an Unsupported exception, not dnae");
        }
        fail("installPackage should have thrown an exception");
    }

    /**
     * Test return exception for package installation
     * {@link NativeDevice#uninstallPackage(String)}.
     */
    public void testUninstallPackages_exception() {
        try {
            mTestDevice.uninstallPackage("");
        } catch (UnsupportedOperationException onse) {
            return;
        } catch (DeviceNotAvailableException e) {
            fail("uninstallPackage should have thrown an Unsupported exception, not dnae");
        }
        fail("uninstallPackageForUser should have thrown an exception");
    }

    /**
     * Test return exception for package installation
     * {@link NativeDevice#installPackage(File, boolean, boolean, String...)}.
     */
    public void testInstallPackagesBool_exception() {
        try {
            mTestDevice.installPackage(new File(""), false, false);
        } catch (UnsupportedOperationException onse) {
            return;
        } catch (DeviceNotAvailableException e) {
            fail("installPackage should have thrown an Unsupported exception, not dnae");
        }
        fail("installPackage should have thrown an exception");
    }

    /**
     * Test return exception for package installation
     * {@link NativeDevice#installPackageForUser(File, boolean, int, String...)}.
     */
    public void testInstallPackagesForUser_exception() {
        try {
            mTestDevice.installPackageForUser(new File(""), false, 0);
        } catch (UnsupportedOperationException onse) {
            return;
        } catch (DeviceNotAvailableException e) {
            fail("installPackageForUser should have thrown an Unsupported exception, not dnae");
        }
        fail("installPackageForUser should have thrown an exception");
    }

    /**
     * Test return exception for package installation
     * {@link NativeDevice#installPackageForUser(File, boolean, boolean, int, String...)}.
     */
    public void testInstallPackagesForUserWithPermission_exception() {
        try {
            mTestDevice.installPackageForUser(new File(""), false, false, 0);
        } catch (UnsupportedOperationException onse) {
            return;
        } catch (DeviceNotAvailableException e) {
            fail("installPackageForUser should have thrown an Unsupported exception, not dnae");
        }
        fail("installPackageForUser should have thrown an exception");
    }

    /**
     * Unit test for {@link NativeDevice#getInstalledPackageNames()}.
     */
    public void testGetInstalledPackageNames_exception() throws Exception {
        try {
            mTestDevice.getInstalledPackageNames();
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("getInstalledPackageNames should have thrown an exception");
    }

    /**
     * Unit test for {@link NativeDevice#getScreenshot()}.
     */
    public void testGetScreenshot_exception() throws Exception {
        try {
            mTestDevice.getScreenshot();
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("getScreenshot should have thrown an exception");
    }

    /**
     * Unit test for {@link NativeDevice#pushDir(File, String)}.
     */
    public void testPushDir_notADir() throws Exception {
        assertFalse(mTestDevice.pushDir(new File(""), ""));
    }

    /**
     * Unit test for {@link NativeDevice#pushDir(File, String)}.
     */
    public void testPushDir_childFile() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean pushFile(File localFile, String remoteFilePath)
                    throws DeviceNotAvailableException {
                return true;
            }
        };
        File testDir = FileUtil.createTempDir("pushDirTest");
        FileUtil.createTempFile("test1", ".txt", testDir);
        assertTrue(mTestDevice.pushDir(testDir, ""));
        FileUtil.recursiveDelete(testDir);
    }

    /**
     * Unit test for {@link NativeDevice#pushDir(File, String)}.
     */
    public void testPushDir_childDir() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String executeShellCommand(String cmd) throws DeviceNotAvailableException {
                return "";
            }
            @Override
            public boolean pushFile(File localFile, String remoteFilePath)
                    throws DeviceNotAvailableException {
                return false;
            }
        };
        File testDir = FileUtil.createTempDir("pushDirTest");
        File subDir = FileUtil.createTempDir("testSubDir", testDir);
        FileUtil.createTempDir("test1", subDir);
        assertTrue(mTestDevice.pushDir(testDir, ""));
        FileUtil.recursiveDelete(testDir);
    }

    /** Test {@link NativeDevice#pullDir(String, File)} when the remote directory is empty. */
    public void testPullDir_nothingToDo() throws Exception {
        final IFileEntry fakeEntry = EasyMock.createMock(IFileEntry.class);
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public IFileEntry getFileEntry(FileEntry path)
                            throws DeviceNotAvailableException {
                        return fakeEntry;
                    }

                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "drwxr-xr-x root     root    somedirectory";
                    }
                };
        File dir = FileUtil.createTempDir("tf-test");
        Collection<IFileEntry> childrens = new ArrayList<>();
        EasyMock.expect(fakeEntry.getChildren(false)).andReturn(childrens);
        // Empty list of childen
        EasyMock.replay(fakeEntry);
        try {
            boolean res = mTestDevice.pullDir("/sdcard/screenshots/", dir);
            assertTrue(res);
            assertTrue(dir.list().length == 0);
        } finally {
            FileUtil.recursiveDelete(dir);
        }
        EasyMock.verify(fakeEntry);
    }

    /**
     * Test {@link NativeDevice#pullDir(String, File)} when the remote directory has a file and a
     * directory.
     */
    public void testPullDir() throws Exception {
        final IFileEntry fakeEntry = EasyMock.createMock(IFileEntry.class);
        final IFileEntry fakeDir = EasyMock.createMock(IFileEntry.class);
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    private boolean mFirstCall = true;

                    @Override
                    public IFileEntry getFileEntry(FileEntry path)
                            throws DeviceNotAvailableException {
                        if (mFirstCall) {
                            mFirstCall = false;
                            return fakeEntry;
                        } else {
                            return fakeDir;
                        }
                    }

                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "drwxr-xr-x root     root    somedirectory";
                    }

                    @Override
                    public boolean pullFile(String remoteFilePath, File localFile)
                            throws DeviceNotAvailableException {
                        try {
                            // Just touch the file to make it appear.
                            localFile.createNewFile();
                        } catch (IOException e) {
                            throw new RuntimeException(e);
                        }
                        return true;
                    }
                };
        File dir = FileUtil.createTempDir("tf-test");
        Collection<IFileEntry> children = new ArrayList<>();
        IFileEntry fakeFile = EasyMock.createMock(IFileEntry.class);
        children.add(fakeFile);
        EasyMock.expect(fakeFile.isDirectory()).andReturn(false);
        EasyMock.expect(fakeFile.getName()).andReturn("fakeFile");
        EasyMock.expect(fakeFile.getFullPath()).andReturn("/sdcard/screenshots/fakeFile");

        children.add(fakeDir);
        EasyMock.expect(fakeDir.isDirectory()).andReturn(true);
        EasyMock.expect(fakeDir.getName()).andReturn("fakeDir");
        EasyMock.expect(fakeDir.getFullPath()).andReturn("/sdcard/screenshots/fakeDir");
        // #pullDir is being called on dir fakeDir to pull everything recursively.
        Collection<IFileEntry> fakeDirChildren = new ArrayList<>();
        EasyMock.expect(fakeDir.getChildren(false)).andReturn(fakeDirChildren);
        EasyMock.expect(fakeEntry.getChildren(false)).andReturn(children);

        EasyMock.replay(fakeEntry, fakeFile, fakeDir);
        try {
            boolean res = mTestDevice.pullDir("/sdcard/screenshots/", dir);
            assertTrue(res);
            assertEquals(2, dir.list().length);
            assertTrue(Arrays.asList(dir.list()).contains("fakeFile"));
            assertTrue(Arrays.asList(dir.list()).contains("fakeDir"));
        } finally {
            FileUtil.recursiveDelete(dir);
        }
        EasyMock.verify(fakeEntry, fakeFile, fakeDir);
    }

    /** Test pulling a directory when one of the pull fails. */
    public void testPullDir_pullFail() throws Exception {
        final IFileEntry fakeEntry = EasyMock.createMock(IFileEntry.class);
        final IFileEntry fakeDir = EasyMock.createMock(IFileEntry.class);
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    private boolean mFirstCall = true;
                    private boolean mFirstPull = true;

                    @Override
                    public IFileEntry getFileEntry(FileEntry path)
                            throws DeviceNotAvailableException {
                        if (mFirstCall) {
                            mFirstCall = false;
                            return fakeEntry;
                        } else {
                            return fakeDir;
                        }
                    }

                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "drwxr-xr-x root     root    somedirectory";
                    }

                    @Override
                    public boolean pullFile(String remoteFilePath, File localFile)
                            throws DeviceNotAvailableException {
                        if (mFirstPull) {
                            mFirstPull = false;
                            try {
                                // Just touch the file to make it appear.
                                localFile.createNewFile();
                            } catch (IOException e) {
                                throw new RuntimeException(e);
                            }
                            return true;
                        } else {
                            return false;
                        }
                    }
                };
        File dir = FileUtil.createTempDir("tf-test");
        Collection<IFileEntry> children = new ArrayList<>();
        IFileEntry fakeFile = EasyMock.createMock(IFileEntry.class);
        children.add(fakeFile);
        EasyMock.expect(fakeFile.isDirectory()).andReturn(false);
        EasyMock.expect(fakeFile.getName()).andReturn("fakeFile");
        EasyMock.expect(fakeFile.getFullPath()).andReturn("/sdcard/screenshots/fakeFile");

        children.add(fakeDir);
        EasyMock.expect(fakeDir.isDirectory()).andReturn(true);
        EasyMock.expect(fakeDir.getName()).andReturn("fakeDir");
        EasyMock.expect(fakeDir.getFullPath()).andReturn("/sdcard/screenshots/fakeDir");
        // #pullDir is being called on dir fakeDir to pull everything recursively.
        Collection<IFileEntry> fakeDirChildren = new ArrayList<>();
        IFileEntry secondLevelChildren = EasyMock.createMock(IFileEntry.class);
        fakeDirChildren.add(secondLevelChildren);
        EasyMock.expect(fakeDir.getChildren(false)).andReturn(fakeDirChildren);
        EasyMock.expect(fakeEntry.getChildren(false)).andReturn(children);

        EasyMock.expect(secondLevelChildren.isDirectory()).andReturn(false);
        EasyMock.expect(secondLevelChildren.getName()).andReturn("secondLevelChildren");
        EasyMock.expect(secondLevelChildren.getFullPath())
                .andReturn("/sdcard/screenshots/fakeDir/secondLevelChildren");

        EasyMock.replay(fakeEntry, fakeFile, fakeDir, secondLevelChildren);
        try {
            boolean res = mTestDevice.pullDir("/sdcard/screenshots/", dir);
            // If one of the pull fails, the full command is considered failed.
            assertFalse(res);
            assertEquals(2, dir.list().length);
            assertTrue(Arrays.asList(dir.list()).contains("fakeFile"));
            // The subdir was created
            assertTrue(Arrays.asList(dir.list()).contains("fakeDir"));
            // The last file failed to pull, so the dir is empty.
            assertEquals(0, new File(dir, "fakeDir").list().length);
        } finally {
            FileUtil.recursiveDelete(dir);
        }
        EasyMock.verify(fakeEntry, fakeFile, fakeDir, secondLevelChildren);
    }

    /**
     * Test that if the requested path is not a directory on the device side, we just fail directly.
     */
    public void testPullDir_invalidPath() throws Exception {
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "-rwxr-xr-x root     root    somefile";
                    }
                };
        File dir = FileUtil.createTempDir("tf-test");
        try {
            assertFalse(mTestDevice.pullDir("somefile", dir));
            assertTrue(dir.list().length == 0);
        } finally {
            FileUtil.recursiveDelete(dir);
        }
    }

    /**
     * Unit test for {@link NativeDevice#getCurrentUser()}.
     */
    public void testGetCurrentUser_exception() throws Exception {
        try {
            mTestDevice.getScreenshot();
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("getCurrentUser should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#getUserFlags(int)}.
     */
    public void testGetUserFlags_exception() throws Exception {
        try {
            mTestDevice.getUserFlags(0);
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("getUserFlags should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#getUserSerialNumber(int)}.
     */
    public void testGetUserSerialNumber_exception() throws Exception {
        try {
            mTestDevice.getUserSerialNumber(0);
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("getUserSerialNumber should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#switchUser(int)}.
     */
    public void testSwitchUser_exception() throws Exception {
        try {
            mTestDevice.switchUser(10);
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("switchUser should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#switchUser(int, long)}.
     */
    public void testSwitchUserTimeout_exception() throws Exception {
        try {
            mTestDevice.switchUser(10, 5*1000);
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("switchUser should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#stopUser(int)}.
     */
    public void testStopUser_exception() throws Exception {
        try {
            mTestDevice.stopUser(0);
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("stopUser should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#stopUser(int, boolean, boolean)}.
     */
    public void testStopUserFlags_exception() throws Exception {
        try {
            mTestDevice.stopUser(0, true, true);
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("stopUser should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#isUserRunning(int)}.
     */
    public void testIsUserIdRunning_exception() throws Exception {
        try {
            mTestDevice.isUserRunning(0);
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("stopUser should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#hasFeature(String)}.
     */
    public void testHasFeature_exception() throws Exception {
        try {
            mTestDevice.hasFeature("feature:test");
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("hasFeature should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#getSetting(String, String)}.
     */
    public void testGetSettingSystemUser_exception() throws Exception {
        try {
            mTestDevice.getSetting("global", "wifi_on");
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("getSettings should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#getSetting(int, String, String)}.
     */
    public void testGetSetting_exception() throws Exception {
        try {
            mTestDevice.getSetting(0, "global", "wifi_on");
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("getSettings should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#setSetting(String, String, String)}.
     */
    public void testSetSettingSystemUser_exception() throws Exception {
        try {
            mTestDevice.setSetting("global", "wifi_on", "0");
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("putSettings should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#setSetting(int, String, String, String)}.
     */
    public void testSetSetting_exception() throws Exception {
        try {
            mTestDevice.setSetting(0, "global", "wifi_on", "0");
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("putSettings should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#getAndroidId(int)}.
     */
    public void testGetAndroidId_exception() throws Exception {
        try {
            mTestDevice.getAndroidId(0);
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("getAndroidId should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#getAndroidIds()}.
     */
    public void testGetAndroidIds_exception() throws Exception {
        try {
            mTestDevice.getAndroidIds();
        } catch (UnsupportedOperationException onse) {
            return;
        }
        fail("getAndroidIds should have thrown an exception.");
    }

    /**
     * Unit test for {@link NativeDevice#connectToWifiNetworkIfNeeded(String, String)}.
     */
    public void testConnectToWifiNetworkIfNeeded_alreadyConnected()
            throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.checkConnectivity(mTestDevice.getOptions().getConnCheckUrl()))
                .andReturn(true);
        EasyMock.replay(mMockWifi);
        assertTrue(mTestDevice.connectToWifiNetworkIfNeeded(FAKE_NETWORK_SSID,
                FAKE_NETWORK_PASSWORD));
        EasyMock.verify(mMockWifi);
    }

    /**
     * Unit test for {@link NativeDevice#connectToWifiNetwork(String, String)}.
     */
    public void testConnectToWifiNetwork_success() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.connectToNetwork(FAKE_NETWORK_SSID, FAKE_NETWORK_PASSWORD,
                mTestDevice.getOptions().getConnCheckUrl(), false)).andReturn(true);
        Map<String, String> fakeWifiInfo = new HashMap<String, String>();
        fakeWifiInfo.put("bssid", FAKE_NETWORK_SSID);
        EasyMock.expect(mMockWifi.getWifiInfo()).andReturn(fakeWifiInfo);
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertTrue(mTestDevice.connectToWifiNetwork(FAKE_NETWORK_SSID,
                FAKE_NETWORK_PASSWORD));
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#connectToWifiNetwork(String, String)} for a failure
     * to connect case.
     */
    public void testConnectToWifiNetwork_failure() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.connectToNetwork(FAKE_NETWORK_SSID, FAKE_NETWORK_PASSWORD,
                mTestDevice.getOptions().getConnCheckUrl(), false)).andReturn(false)
                .times(mTestDevice.getOptions().getWifiAttempts());
        Map<String, String> fakeWifiInfo = new HashMap<String, String>();
        fakeWifiInfo.put("bssid", FAKE_NETWORK_SSID);
        EasyMock.expect(mMockWifi.getWifiInfo()).andReturn(fakeWifiInfo)
                .times(mTestDevice.getOptions().getWifiAttempts());
        mMockRunUtil.sleep(EasyMock.anyLong());
        EasyMock.expectLastCall().times(mTestDevice.getOptions().getWifiAttempts() - 1);
        EasyMock.replay(mMockWifi, mMockIDevice, mMockRunUtil);
        assertFalse(mTestDevice.connectToWifiNetwork(FAKE_NETWORK_SSID,
                FAKE_NETWORK_PASSWORD));
        EasyMock.verify(mMockWifi, mMockIDevice, mMockRunUtil);
    }

    /**
     * Unit test for {@link NativeDevice#connectToWifiNetwork(String, String)} for limiting the time
     * trying to connect to wifi.
     */
    public void testConnectToWifiNetwork_maxConnectTime()
            throws DeviceNotAvailableException, ConfigurationException {
        OptionSetter deviceOptionSetter = new OptionSetter(mTestDevice.getOptions());
        deviceOptionSetter.setOptionValue("max-wifi-connect-time", "10000");
        Clock mockClock = Mockito.mock(Clock.class);
        mTestDevice.setClock(mockClock);
        EasyMock.expect(
                        mMockWifi.connectToNetwork(
                                FAKE_NETWORK_SSID,
                                FAKE_NETWORK_PASSWORD,
                                mTestDevice.getOptions().getConnCheckUrl(),
                                false))
                .andReturn(false)
                .times(2);
        Mockito.when(mockClock.millis())
                .thenReturn(Long.valueOf(0), Long.valueOf(6000), Long.valueOf(12000));
        Map<String, String> fakeWifiInfo = new HashMap<String, String>();
        fakeWifiInfo.put("bssid", FAKE_NETWORK_SSID);
        EasyMock.expect(mMockWifi.getWifiInfo()).andReturn(fakeWifiInfo).times(2);

        EasyMock.replay(mMockWifi, mMockIDevice);
        assertFalse(mTestDevice.connectToWifiNetwork(FAKE_NETWORK_SSID, FAKE_NETWORK_PASSWORD));
        EasyMock.verify(mMockWifi, mMockIDevice);
        Mockito.verify(mockClock, Mockito.times(3)).millis();
    }

    /**
     * Unit test for {@link NativeDevice#connectToWifiNetwork(String, String, boolean)}.
     */
    public void testConnectToWifiNetwork_scanSsid() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.connectToNetwork(FAKE_NETWORK_SSID, FAKE_NETWORK_PASSWORD,
                mTestDevice.getOptions().getConnCheckUrl(), true)).andReturn(true);
        Map<String, String> fakeWifiInfo = new HashMap<String, String>();
        fakeWifiInfo.put("bssid", FAKE_NETWORK_SSID);
        EasyMock.expect(mMockWifi.getWifiInfo()).andReturn(fakeWifiInfo);
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertTrue(mTestDevice.connectToWifiNetwork(FAKE_NETWORK_SSID,
                FAKE_NETWORK_PASSWORD, true));
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#checkWifiConnection(String)}.
     */
    public void testCheckWifiConnection() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.isWifiEnabled()).andReturn(true);
        EasyMock.expect(mMockWifi.getSSID()).andReturn("\"" + FAKE_NETWORK_SSID + "\"");
        EasyMock.expect(mMockWifi.hasValidIp()).andReturn(true);
        EasyMock.expect(mMockWifi.checkConnectivity(mTestDevice.getOptions().getConnCheckUrl()))
                .andReturn(true);
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertTrue(mTestDevice.checkWifiConnection(FAKE_NETWORK_SSID));
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#checkWifiConnection(String)} for a failure.
     */
    public void testCheckWifiConnection_failure() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.isWifiEnabled()).andReturn(false);
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertFalse(mTestDevice.checkWifiConnection(FAKE_NETWORK_SSID));
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#isWifiEnabled()}.
     */
    public void testIsWifiEnabled() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.isWifiEnabled()).andReturn(true);
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertTrue(mTestDevice.isWifiEnabled());
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#isWifiEnabled()} with runtime exception from
     * wifihelper.
     */
    public void testIsWifiEnabled_exception() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.isWifiEnabled()).andThrow(new RuntimeException());
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertFalse(mTestDevice.isWifiEnabled());
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#disconnectFromWifi()}.
     */
    public void testDisconnectFromWifi() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.disconnectFromNetwork()).andReturn(true);
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertTrue(mTestDevice.disconnectFromWifi());
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#enableNetworkMonitor()}.
     */
    public void testEnableNetworkMonitor() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.stopMonitor()).andReturn(null);
        EasyMock.expect(mMockWifi.startMonitor(EasyMock.anyLong(),
                EasyMock.eq(mTestDevice.getOptions().getConnCheckUrl()))).andReturn(true);
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertTrue(mTestDevice.enableNetworkMonitor());
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#enableNetworkMonitor()} in case of failure.
     */
    public void testEnableNetworkMonitor_failure() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.stopMonitor()).andReturn(null);
        EasyMock.expect(mMockWifi.startMonitor(EasyMock.anyLong(),
                EasyMock.eq(mTestDevice.getOptions().getConnCheckUrl()))).andReturn(false);
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertFalse(mTestDevice.enableNetworkMonitor());
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#disableNetworkMonitor()}.
     */
    public void testDisableNetworkMonitor() throws DeviceNotAvailableException {
        List<Long> samples = new ArrayList<Long>();
        samples.add(Long.valueOf(42));
        samples.add(Long.valueOf(256));
        samples.add(Long.valueOf(-1)); // failure to connect
        EasyMock.expect(mMockWifi.stopMonitor()).andReturn(samples);
        EasyMock.replay(mMockWifi, mMockIDevice);
        assertTrue(mTestDevice.disableNetworkMonitor());
        EasyMock.verify(mMockWifi, mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#reconnectToWifiNetwork()}.
     */
    public void testReconnectToWifiNetwork() throws DeviceNotAvailableException {
        EasyMock.expect(mMockWifi.checkConnectivity(mTestDevice.getOptions().getConnCheckUrl()))
                .andReturn(false);
        EasyMock.expect(mMockWifi.checkConnectivity(mTestDevice.getOptions().getConnCheckUrl()))
                .andReturn(true);
        mMockRunUtil.sleep(EasyMock.anyLong());
        EasyMock.expectLastCall();
        EasyMock.replay(mMockWifi, mMockIDevice, mMockRunUtil);
        try {
            mTestDevice.reconnectToWifiNetwork();
        } catch (NetworkNotAvailableException nnae) {
            fail("reconnectToWifiNetwork() should not have thrown an exception.");
        } finally {
            EasyMock.verify(mMockWifi, mMockIDevice, mMockRunUtil);
        }
    }

    /**
     * Unit test for {@link NativeDevice#isHeadless()}.
     */
    public void testIsHeadless() throws DeviceNotAvailableException {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return "1\n";
            }
        };
        assertTrue(mTestDevice.isHeadless());
    }

    /**
     * Unit test for {@link NativeDevice#isHeadless()}.
     */
    public void testIsHeadless_notHeadless() throws DeviceNotAvailableException {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return null;
            }
        };
        assertFalse(mTestDevice.isHeadless());
    }

    /**
     * Unit test for {@link NativeDevice#getDeviceDate()}.
     */
    public void testGetDeviceDate() throws DeviceNotAvailableException {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String executeShellCommand(String name) throws DeviceNotAvailableException {
                return "21692641\n";
            }
        };
        assertEquals(21692641000L, mTestDevice.getDeviceDate());
    }

    /**
     * Unit test for {@link NativeDevice#logBugreport(String, ITestLogger)}.
     */
    public void testTestLogBugreport() {
        final String dataName = "test";
        final InputStreamSource stream = new ByteArrayInputStreamSource(null);
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public InputStreamSource getBugreportz() {
                        return stream;
                    }

                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return 24;
                    }
                };
        ITestLogger listener = EasyMock.createMock(ITestLogger.class);
        listener.testLog(dataName, LogDataType.BUGREPORTZ, stream);
        EasyMock.replay(listener);
        assertTrue(mTestDevice.logBugreport(dataName, listener));
        EasyMock.verify(listener);
    }

    /**
     * Unit test for {@link NativeDevice#logBugreport(String, ITestLogger)}.
     */
    public void testTestLogBugreport_oldDevice() {
        final String dataName = "test";
        final InputStreamSource stream = new ByteArrayInputStreamSource(null);
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public InputStreamSource getBugreportz() {
                        // Older device do not support bugreportz and return null
                        return null;
                    }

                    @Override
                    public InputStreamSource getBugreportInternal() {
                        return stream;
                    }

                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        // no bugreportz support
                        return 23;
                    }
                };
        ITestLogger listener = EasyMock.createMock(ITestLogger.class);
        listener.testLog(dataName, LogDataType.BUGREPORT, stream);
        EasyMock.replay(listener);
        assertTrue(mTestDevice.logBugreport(dataName, listener));
        EasyMock.verify(listener);
    }

    /**
     * Unit test for {@link NativeDevice#logBugreport(String, ITestLogger)}.
     */
    public void testTestLogBugreport_fail() {
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public InputStreamSource getBugreportz() {
                        return null;
                    }

                    @Override
                    protected InputStreamSource getBugreportInternal() {
                        return null;
                    }

                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return 23;
                    }
                };
        ITestLogger listener = EasyMock.createMock(ITestLogger.class);
        EasyMock.replay(listener);
        assertFalse(mTestDevice.logBugreport("test", listener));
        EasyMock.verify(listener);
    }

    /**
     * Unit test for {@link NativeDevice#takeBugreport()}.
     */
    public void testTakeBugreport_apiLevelFail() {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                throw new DeviceNotAvailableException();
            }
        };
        // If we can't check API level it should return null.
        assertNull(mTestDevice.takeBugreport());
    }

    /**
     * Unit test for {@link NativeDevice#takeBugreport()}.
     */
    public void testTakeBugreport_oldDevice() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 19;
            }
        };
        Bugreport report = mTestDevice.takeBugreport();
        try {
            assertNotNull(report);
            // older device report a non zipped bugreport
            assertFalse(report.isZipped());
        } finally {
            report.close();
        }
    }

    /**
     * Unit test for {@link NativeDevice#takeBugreport()}.
     */
    public void testTakeBugreport() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 24;
            }
            @Override
            protected File getBugreportzInternal() {
                try {
                    return FileUtil.createTempFile("bugreportz", ".zip");
                } catch (IOException e) {
                    return null;
                }
            }
        };
        Bugreport report = mTestDevice.takeBugreport();
        try {
            assertNotNull(report);
            assertTrue(report.isZipped());
        } finally {
            report.close();
        }
    }

    /**
     * Unit test for {@link NativeDevice#getDeviceDate()}.
     */
    public void testGetDeviceDate_wrongformat() throws DeviceNotAvailableException {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String executeShellCommand(String name) throws DeviceNotAvailableException {
                return "WRONG\n";
            }
        };
        assertEquals(0, mTestDevice.getDeviceDate());
    }

    public void testGetBugreport_deviceUnavail() throws Exception {
        final String expectedOutput = "this is the output\r\n in two lines\r\n";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public void executeShellCommand(
                    String command, IShellOutputReceiver receiver,
                    long maxTimeToOutputShellResponse, TimeUnit timeUnit, int retryAttempts)
                            throws DeviceNotAvailableException {
                String fakeRep = expectedOutput;
                receiver.addOutput(fakeRep.getBytes(), 0, fakeRep.getBytes().length);
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };

        // FIXME: this isn't actually causing a DeviceNotAvailableException to be thrown
        mMockRecovery.recoverDevice(EasyMock.eq(mMockStateMonitor), EasyMock.eq(false));
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());
        EasyMock.replay(mMockRecovery, mMockIDevice);
        assertEquals(expectedOutput, StreamUtil.getStringFromStream(
                mTestDevice.getBugreport().createInputStream()));
    }

    public void testGetBugreport_compatibility_deviceUnavail() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public void executeShellCommand(
                    String command, IShellOutputReceiver receiver,
                    long maxTimeToOutputShellResponse, TimeUnit timeUnit, int retryAttempts)
                            throws DeviceNotAvailableException {
                throw new DeviceNotAvailableException();
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 24;
            }
            @Override
            public IFileEntry getFileEntry(String path) throws DeviceNotAvailableException {
                return null;
            }
        };
        EasyMock.replay(mMockRecovery, mMockIDevice);
        assertEquals(0, mTestDevice.getBugreport().size());
        EasyMock.verify(mMockRecovery, mMockIDevice);
    }

    public void testGetBugreport_deviceUnavail_fallback() throws Exception {
        final IFileEntry fakeEntry = EasyMock.createMock(IFileEntry.class);
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public void executeShellCommand(
                    String command, IShellOutputReceiver receiver,
                    long maxTimeToOutputShellResponse, TimeUnit timeUnit, int retryAttempts)
                            throws DeviceNotAvailableException {
                throw new DeviceNotAvailableException();
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 24;
            }
            @Override
            public IFileEntry getFileEntry(String path) throws DeviceNotAvailableException {
                return fakeEntry;
            }
            @Override
            public File pullFile(String remoteFilePath) throws DeviceNotAvailableException {
                try {
                    return FileUtil.createTempFile("bugreport", ".txt");
                } catch (IOException e) {
                    return null;
                }
            }
        };
        List<IFileEntry> list = new ArrayList<>();
        list.add(fakeEntry);
        EasyMock.expect(fakeEntry.getChildren(false)).andReturn(list);
        EasyMock.expect(fakeEntry.getName()).andReturn("bugreport-NYC-2016-08-17-10-17-00.tmp");
        EasyMock.replay(fakeEntry, mMockRecovery, mMockIDevice);
        InputStreamSource res = null;
        try {
            res = mTestDevice.getBugreport();
            assertNotNull(res);
            EasyMock.verify(fakeEntry, mMockRecovery, mMockIDevice);
        } finally {
            StreamUtil.cancel(res);
        }
    }

    /**
     * Unit test for {@link NativeDevice#getBugreportz()}.
     */
    public void testGetBugreportz() throws IOException {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public void executeShellCommand(
                    String command, IShellOutputReceiver receiver,
                    long maxTimeToOutputShellResponse, TimeUnit timeUnit, int retryAttempts)
                            throws DeviceNotAvailableException {
                String fakeRep = "OK:/data/0/com.android.shell/bugreports/bugreport1970-10-27.zip";
                receiver.addOutput(fakeRep.getBytes(), 0, fakeRep.getBytes().length);
            }
            @Override
            public boolean doesFileExist(String destPath) throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean pullFile(String remoteFilePath, File localFile)
                    throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                assertEquals("rm /data/0/com.android.shell/bugreports/*", command);
                return null;
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 24;
            }
        };
        FileInputStreamSource f = null;
        try {
            f = (FileInputStreamSource) mTestDevice.getBugreportz();
            assertNotNull(f);
            assertTrue(f.createInputStream().available() == 0);
        } finally {
            StreamUtil.cancel(f);
            if (f != null) {
                f.cleanFile();
            }
        }
    }

    /**
     * Unit test for {@link NativeDevice#getBugreportz()}.
     */
    public void testGetBugreportz_fails() {
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return 24;
                    }

                    @Override
                    protected File getBugreportzInternal() {
                        return null;
                    }

                    @Override
                    public IFileEntry getFileEntry(String path) throws DeviceNotAvailableException {
                        return null;
                    }
                };
        FileInputStreamSource f = null;
        try {
            f = (FileInputStreamSource) mTestDevice.getBugreportz();
            assertNull(f);
        } finally {
            StreamUtil.cancel(f);
            if (f != null) {
                f.cleanFile();
            }
        }
    }

    /**
     * Test that we can distinguish a newer file even with Timezone on the device.
     * Seoul is GMT+9.
     */
    public void testIsNewer() throws Exception {
        TestableAndroidNativeDevice testDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public String getProperty(String name) throws DeviceNotAvailableException {
                        return "Asia/Seoul";
                    }

                    @Override
                    public long getDeviceTimeOffset(Date date) throws DeviceNotAvailableException {
                        return 0;
                    }
                };
        File localFile = FileUtil.createTempFile("timezonetest", ".txt");
        try {
            localFile.setLastModified(1470906000000l); // Thu Aug 11 09:00:00 GMT 2016
            IFileEntry remoteFile = EasyMock.createMock(IFileEntry.class);
            EasyMock.expect(remoteFile.getDate()).andReturn("2016-08-11");
            EasyMock.expect(remoteFile.getTime()).andReturn("18:00");
            EasyMock.replay(remoteFile);
            assertTrue(testDevice.isNewer(localFile, remoteFile));
            EasyMock.verify(remoteFile);
        } finally {
            FileUtil.deleteFile(localFile);
        }
    }

    /**
     * Test that we can distinguish a newer file even with Timezone on the device.
     * Seoul is GMT+9. Clock on device is inaccurate and in advance of host.
     */
    public void testIsNewer_timeOffset() throws Exception {
        TestableAndroidNativeDevice testDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public String getProperty(String name) throws DeviceNotAvailableException {
                        return "Asia/Seoul";
                    }

                    @Override
                    public long getDeviceTimeOffset(Date date) throws DeviceNotAvailableException {
                        return -15 * 60 * 1000; // Device in advance of 15m on host.
                    }
                };
        File localFile = FileUtil.createTempFile("timezonetest", ".txt");
        try {
            localFile.setLastModified(1470906000000l); // Thu, 11 Aug 2016 09:00:00 GMT
            IFileEntry remoteFile = EasyMock.createMock(IFileEntry.class);
            EasyMock.expect(remoteFile.getDate()).andReturn("2016-08-11");
            EasyMock.expect(remoteFile.getTime()).andReturn("18:15");
            EasyMock.replay(remoteFile);
            // Should sync because after time offset correction, file is older.
            assertTrue(testDevice.isNewer(localFile, remoteFile));
            EasyMock.verify(remoteFile);
        } finally {
            FileUtil.deleteFile(localFile);
        }
    }

    /**
     * Test that we can distinguish a newer file even with Timezone on the device.
     * Seoul is GMT+9.
     * Local file is set to 10min earlier than remoteFile.
     */
    public void testIsNewer_fails() throws Exception {
        TestableAndroidNativeDevice testDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public String getProperty(String name) throws DeviceNotAvailableException {
                        return "Asia/Seoul";
                    }

                    @Override
                    public long getDeviceTimeOffset(Date date) throws DeviceNotAvailableException {
                        return 0;
                    }
                };
        File localFile = FileUtil.createTempFile("timezonetest", ".txt");
        try {
            localFile.setLastModified(1470906000000l); // Thu, 11 Aug 2016 09:00:00 GMT
            IFileEntry remoteFile = EasyMock.createMock(IFileEntry.class);
            EasyMock.expect(remoteFile.getDate()).andReturn("2016-08-11");
            EasyMock.expect(remoteFile.getTime()).andReturn("18:10");
            EasyMock.replay(remoteFile);
            assertFalse(testDevice.isNewer(localFile, remoteFile));
            EasyMock.verify(remoteFile);
        } finally {
            FileUtil.deleteFile(localFile);
        }
    }

    /**
     * Unit test for {@link NativeDevice#getBuildAlias()}.
     */
    public void testGetBuildAlias() throws DeviceNotAvailableException {
        final String alias = "alias\n";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return alias;
            }
        };
        assertEquals(alias, mTestDevice.getBuildAlias());
    }

    /**
     * Unit test for {@link NativeDevice#getBuildAlias()}.
     */
    public void testGetBuildAlias_null() throws DeviceNotAvailableException {
        final String alias = null;
        final String buildId = "alias\n";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return alias;
            }
            @Override
            public String getBuildId() throws DeviceNotAvailableException {
                return buildId;
            }
        };
        assertEquals(buildId, mTestDevice.getBuildAlias());
    }

    /**
     * Unit test for {@link NativeDevice#getBuildAlias()}.
     */
    public void testGetBuildAlias_empty() throws DeviceNotAvailableException {
        final String alias = "";
        final String buildId = "alias\n";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return alias;
            }
            @Override
            public String getBuildId() throws DeviceNotAvailableException {
                return buildId;
            }
        };
        assertEquals(buildId, mTestDevice.getBuildAlias());
    }

    /**
     * Unit test for {@link NativeDevice#getBuildId()}.
     */
    public void testGetBuildId() throws DeviceNotAvailableException {
        final String buildId = "299865";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return buildId;
            }
        };
        assertEquals(buildId, mTestDevice.getBuildId());
    }

    /**
     * Unit test for {@link NativeDevice#getBuildId()}.
     */
    public void testGetBuildId_null() throws DeviceNotAvailableException {
        final String buildId = null;
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return buildId;
            }
        };
        assertEquals(IBuildInfo.UNKNOWN_BUILD_ID, mTestDevice.getBuildId());
    }

    /**
     * Unit test for {@link NativeDevice#getBuildFlavor()}.
     */
    public void testGetBuildFlavor() throws DeviceNotAvailableException {
        final String flavor = "ham-user";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return flavor;
            }
        };
        assertEquals(flavor, mTestDevice.getBuildFlavor());
    }

    /**
     * Unit test for {@link NativeDevice#getBuildFlavor()}.
     */
    public void testGetBuildFlavor_null_flavor() throws DeviceNotAvailableException {
        final String productName = "prod";
        final String buildType = "buildtype";
        String expected = String.format("%s-%s", productName, buildType);
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                if ("ro.build.flavor".equals(name)) {
                    return null;
                } else if ("ro.product.name".equals(name)) {
                    return productName;
                } else if ("ro.build.type".equals(name)) {
                    return buildType;
                } else {
                    return null;
                }
            }
        };
        assertEquals(expected, mTestDevice.getBuildFlavor());
    }

    /**
     * Unit test for {@link NativeDevice#getBuildFlavor()}.
     */
    public void testGetBuildFlavor_null() throws DeviceNotAvailableException {
        final String productName = null;
        final String buildType = "buildtype";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                if ("ro.build.flavor".equals(name)) {
                    return "";
                } else if ("ro.product.name".equals(name)) {
                    return productName;
                } else if ("ro.build.type".equals(name)) {
                    return buildType;
                } else {
                    return null;
                }
            }
        };
        assertNull(mTestDevice.getBuildFlavor());
    }

    /**
     * Unit test for {@link NativeDevice#doAdbReboot(String)}.
     */
    public void testDoAdbReboot_emulator() throws Exception {
        final String into = "bootloader";
        mMockIDevice.reboot(into);
        EasyMock.expectLastCall();
        EasyMock.replay(mMockIDevice);
        mTestDevice.doAdbReboot(into);
        EasyMock.verify(mMockIDevice);
    }

    /**
     * Unit test for {@link NativeDevice#doReboot()}.
     */
    public void testDoReboot() throws Exception {
        NativeDevice testDevice = new NativeDevice(mMockIDevice,
                mMockStateMonitor, mMockDvcMonitor) {
            @Override
            public TestDeviceState getDeviceState() {
                return TestDeviceState.ONLINE;
            }
        };
        mMockIDevice.reboot(null);
        EasyMock.expectLastCall();
        EasyMock.expect(mMockStateMonitor.waitForDeviceNotAvailable(EasyMock.anyLong()))
                .andReturn(true);
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        testDevice.doReboot();
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /**
     * Unit test for {@link NativeDevice#doReboot()}.
     */
    public void testDoReboot_skipped() throws Exception {
        NativeDevice testDevice = new NativeDevice(mMockIDevice,
                mMockStateMonitor, mMockDvcMonitor) {
            @Override
            public TestDeviceState getDeviceState() {
                mOptions = new TestDeviceOptions() {
                    @Override
                    public boolean shouldDisableReboot() {
                        return true;
                    }
                };
                return TestDeviceState.ONLINE;
            }
        };
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        testDevice.doReboot();
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /**
     * Unit test for {@link NativeDevice#doReboot()}.
     */
    public void testDoReboot_fastboot() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public TestDeviceState getDeviceState() {
                return TestDeviceState.FASTBOOT;
            }
            @Override
            public CommandResult executeFastbootCommand(String... cmdArgs)
                    throws DeviceNotAvailableException, UnsupportedOperationException {
                wasCalled = true;
                return new CommandResult();
            }
        };
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        mTestDevice.doReboot();
        assertTrue(mTestDevice.wasCalled);
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /**
     * Unit test for {@link NativeDevice#unlockDevice()} already decrypted.
     */
    public void testUnlockDevice_skipping() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean isEncryptionSupported() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean isDeviceEncrypted() throws DeviceNotAvailableException {
                return false;
            }
        };
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        assertTrue(mTestDevice.unlockDevice());
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /**
     * Unit test for {@link NativeDevice#unlockDevice()}.
     */
    public void testUnlockDevice() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean isEncryptionSupported() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean isDeviceEncrypted() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean enableAdbRoot() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "200 checkpw -1";
            }
        };
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        assertTrue(mTestDevice.unlockDevice());
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /**
     * Unit test for {@link NativeDevice#unlockDevice()}.
     */
    public void testUnlockDevice_garbageOutput() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean isEncryptionSupported() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean isDeviceEncrypted() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean enableAdbRoot() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "gdsgdgggsgdg not working";
            }
        };
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        assertFalse(mTestDevice.unlockDevice());
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /**
     * Unit test for {@link NativeDevice#unlockDevice()}.
     */
    public void testUnlockDevice_emptyOutput() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean isEncryptionSupported() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean isDeviceEncrypted() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean enableAdbRoot() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "";
            }
        };
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        assertFalse(mTestDevice.unlockDevice());
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /**
     * Unit test for {@link NativeDevice#unlockDevice()}.
     */
    public void testUnlockDevice_goodOutputPasswordEnteredCorrectly() throws Exception {
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean isEncryptionSupported() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean isDeviceEncrypted() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public boolean enableAdbRoot() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "200 encryption 0";
            }
        };
        EasyMock.expect(mMockStateMonitor.waitForDeviceAvailable()).andReturn(mMockIDevice);
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        assertTrue(mTestDevice.unlockDevice());
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /**
     * Test {NativeDevice#parseFreeSpaceFromModernOutput(String)} with a regular df output and
     * mount name of numbers.
     */
    public void testParseDfOutput_mountWithNumber() {
        String dfOutput = "Filesystem     1K-blocks   Used Available Use% Mounted on\n" +
                "/dev/fuse       31154688 100576  31054112   1% /storage/3134-3433";
        Long res = mTestDevice.parseFreeSpaceFromModernOutput(dfOutput);
        assertNotNull(res);
        assertEquals(31054112L, res.longValue());
    }

    /**
     * Test {NativeDevice#parseFreeSpaceFromModernOutput(String)} with a regular df output and
     * mount name of mix of letters and numbers.
     */
    public void testParseDfOutput() {
        String dfOutput = "Filesystem     1K-blocks   Used Available Use% Mounted on\n" +
                "/dev/fuse       31154688 100576  31054112   1% /storage/sdcard58";
        Long res = mTestDevice.parseFreeSpaceFromModernOutput(dfOutput);
        assertNotNull(res);
        assertEquals(31054112L, res.longValue());
    }

    /**
     * Test {NativeDevice#parseFreeSpaceFromModernOutput(String)} with a regular df output and
     * mount name with incorrect field.
     */
    public void testParseDfOutput_wrongMount() {
        String dfOutput = "Filesystem     1K-blocks   Used Available Use% Mounted on\n" +
                "/dev/fuse       31154688 100576  31054112   1% \test\\wrongpath";
        Long res = mTestDevice.parseFreeSpaceFromModernOutput(dfOutput);
        assertNull(res);
    }

    /**
     * Test {NativeDevice#parseFreeSpaceFromModernOutput(String)} with a regular df output and
     * a filesytem name with numbers in it.
     */
    public void testParseDfOutput_filesystemWithNumber() {
        String dfOutput = "Filesystem     1K-blocks   Used Available Use% Mounted on\n" +
                "/dev/dm-1       31154688 100576  31054112   1% /";
        Long res = mTestDevice.parseFreeSpaceFromModernOutput(dfOutput);
        assertNotNull(res);
        assertEquals(31054112L, res.longValue());
    }

    /**
     * Test that {@link NativeDevice#getDeviceTimeOffset(Date)} returns the proper offset forward
     */
    public void testGetDeviceTimeOffset() throws DeviceNotAvailableException {
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public long getDeviceDate() throws DeviceNotAvailableException {
                        return 1476958881000L;
                    }
                };
        Date date = new Date(1476958891000L);
        assertEquals(10000L, mTestDevice.getDeviceTimeOffset(date));
    }

    /**
     * Test that {@link NativeDevice#getDeviceTimeOffset(Date)}} returns the proper offset when
     * there is delay.
     */
    public void testGetDeviceTimeOffset_delay() throws DeviceNotAvailableException {
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public long getDeviceDate() throws DeviceNotAvailableException {
                        // DeviceDate is in second
                        return 1476958891000L;
                    }
                };
        // Date takes millisecond since Epoch
        Date date = new Date(1476958881000L);
        assertEquals(-10000L, mTestDevice.getDeviceTimeOffset(date));
    }

    /**
     * Test that {@link NativeDevice#setDate(Date)} returns the proper offset when
     * there is delay with api level above 24, posix format is used.
     */
    public void testSetDate() throws DeviceNotAvailableException {
        Date date = new Date(1476958881000L);
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return 24;
                    }

                    @Override
                    public long getDeviceTimeOffset(Date date) throws DeviceNotAvailableException {
                        // right above set threshold
                        return NativeDevice.MAX_HOST_DEVICE_TIME_OFFSET + 1;
                    }

                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        CLog.e("%s", command);
                        assertEquals("date -u 102010212016.21", command);
                        return command;
                    }
                };
        mTestDevice.setDate(date);
    }

    /**
     * Test that {@link NativeDevice#setDate(Date)} returns the proper offset when
     * there is delay with api level below 23, regular second format is used.
     */
    public void testSetDate_lowApi() throws DeviceNotAvailableException {
        Date date = new Date(1476958881000L);
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return 22;
                    }

                    @Override
                    public long getDeviceTimeOffset(Date date) throws DeviceNotAvailableException {
                        // right above set threshold
                        return NativeDevice.MAX_HOST_DEVICE_TIME_OFFSET + 1;
                    }

                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        CLog.e("%s", command);
                        assertEquals("date -u 1476958881", command);
                        return command;
                    }
                };
        mTestDevice.setDate(date);
    }

    /**
     * Test that {@link NativeDevice#setDate(Date)} does not attemp to set if bellow threshold.
     */
    public void testSetDate_NoAction() throws DeviceNotAvailableException {
        Date date = new Date(1476958881000L);
        mTestDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    public long getDeviceTimeOffset(Date date) throws DeviceNotAvailableException {
                        // right below set threshold
                        return NativeDevice.MAX_HOST_DEVICE_TIME_OFFSET - 1;
                    }

                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        fail("Should not be called");
                        return command;
                    }
                };
        mTestDevice.setDate(date);
    }

    /**
     * Test that {@link NativeDevice#getDeviceDescriptor()} returns the proper information.
     */
    public void testGetDeviceDescriptor() {
        final String serial = "Test";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public IDevice getIDevice() {
                return new StubDevice(serial);
            }
        };
        DeviceDescriptor desc = mTestDevice.getDeviceDescriptor();
        assertTrue(desc.isStubDevice());
        assertEquals(serial, desc.getSerial());
        assertEquals("StubDevice", desc.getDeviceClass());
    }

    /**
     * Test that {@link NativeDevice#pullFile(String, File)} returns true when the pull is
     * successful.
     */
    public void testPullFile() throws Exception {
        final String fakeRemotePath = "/test/";
        SyncService s = Mockito.mock(SyncService.class);
        EasyMock.expect(mMockIDevice.getSyncService()).andReturn(s);
        EasyMock.replay(mMockIDevice);
        File tmpFile = FileUtil.createTempFile("pull", ".test");
        try {
            boolean res = mTestDevice.pullFile(fakeRemotePath, tmpFile);
            EasyMock.verify(mMockIDevice);
            Mockito.verify(s)
                    .pullFile(
                            Mockito.eq(fakeRemotePath),
                            Mockito.eq(tmpFile.getAbsolutePath()),
                            Mockito.any(ISyncProgressMonitor.class));
            Mockito.verify(s).close();
            assertTrue(res);
        } finally {
            FileUtil.deleteFile(tmpFile);
        }
    }

    /**
     * Test that {@link NativeDevice#pullFile(String, File)} returns false when it fails to
     * pull the file.
     */
    public void testPullFile_fails() throws Exception {
        final String fakeRemotePath = "/test/";
        SyncService s = Mockito.mock(SyncService.class);
        EasyMock.expect(mMockIDevice.getSyncService()).andReturn(s);
        EasyMock.replay(mMockIDevice);
        File tmpFile = FileUtil.createTempFile("pull", ".test");
        doThrow(new SyncException(SyncError.CANCELED))
                .when(s)
                .pullFile(
                        Mockito.eq(fakeRemotePath),
                        Mockito.eq(tmpFile.getAbsolutePath()),
                        Mockito.any(ISyncProgressMonitor.class));
        try {
            boolean res = mTestDevice.pullFile(fakeRemotePath, tmpFile);
            EasyMock.verify(mMockIDevice);
            Mockito.verify(s)
                    .pullFile(
                            Mockito.eq(fakeRemotePath),
                            Mockito.eq(tmpFile.getAbsolutePath()),
                            Mockito.any(ISyncProgressMonitor.class));
            Mockito.verify(s).close();
            assertFalse(res);
        } finally {
            FileUtil.deleteFile(tmpFile);
        }
    }

    /**
     * Test that {@link NativeDevice#pullFile(String)} returns a file when succeed pulling the
     * file.
     */
    public void testPullFile_returnFileSuccess() throws Exception {
        final String fakeRemotePath = "/test/";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean pullFile(String remoteFilePath, File localFile)
                    throws DeviceNotAvailableException {
                return true;
            }
        };
        File res = mTestDevice.pullFile(fakeRemotePath);
        try {
            assertNotNull(res);
        } finally {
            FileUtil.deleteFile(res);
        }
    }

    /**
     * Test that {@link NativeDevice#pullFile(String)} returns null when failed to pull the file.
     */
    public void testPullFile_returnNull() throws Exception {
        final String fakeRemotePath = "/test/";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean pullFile(String remoteFilePath, File localFile)
                    throws DeviceNotAvailableException {
                return false;
            }
        };
        File res = mTestDevice.pullFile(fakeRemotePath);
        try {
            assertNull(res);
        } finally {
            FileUtil.deleteFile(res);
        }
    }

    /**
     * Test that {@link NativeDevice#pullFileContents(String)} returns a string when succeed pulling
     * the file.
     */
    public void testPullFileContents_returnFileSuccess() throws Exception {
        final String fakeRemotePath = "/test/";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean pullFile(String remoteFilePath, File localFile)
                    throws DeviceNotAvailableException {
                return true;
            }
        };
        String res = mTestDevice.pullFileContents(fakeRemotePath);
        assertNotNull(res);
    }

    /**
     * Test that {@link NativeDevice#pullFileContents(String)} returns null when failed to pull the
     * file.
     */
    public void testPullFileContents_returnNull() throws Exception {
        final String fakeRemotePath = "/test/";
        mTestDevice = new TestableAndroidNativeDevice() {
            @Override
            public boolean pullFile(String remoteFilePath, File localFile)
                    throws DeviceNotAvailableException {
                return false;
            }
        };
        String res = mTestDevice.pullFileContents(fakeRemotePath);
        assertNull(res);
    }

    /**
     * Test that {@link NativeDevice#pushFile(File, String)} returns true when the push is
     * successful.
     */
    public void testPushFile() throws Exception {
        final String fakeRemotePath = "/test/";
        SyncService s = Mockito.mock(SyncService.class);
        EasyMock.expect(mMockIDevice.getSyncService()).andReturn(s);
        EasyMock.replay(mMockIDevice);
        File tmpFile = FileUtil.createTempFile("push", ".test");
        try {
            boolean res = mTestDevice.pushFile(tmpFile, fakeRemotePath);
            EasyMock.verify(mMockIDevice);
            Mockito.verify(s)
                    .pushFile(
                            Mockito.eq(tmpFile.getAbsolutePath()),
                            Mockito.eq(fakeRemotePath),
                            Mockito.any(ISyncProgressMonitor.class));
            Mockito.verify(s).close();
            assertTrue(res);
        } finally {
            FileUtil.deleteFile(tmpFile);
        }
    }

    /**
     * Test that {@link NativeDevice#pushFile(File, String)} returns false when the push is
     * unsuccessful.
     */
    public void testPushFile_fails() throws Exception {
        final String fakeRemotePath = "/test/";
        SyncService s = Mockito.mock(SyncService.class);
        EasyMock.expect(mMockIDevice.getSyncService()).andReturn(s);
        EasyMock.replay(mMockIDevice);
        File tmpFile = FileUtil.createTempFile("push", ".test");
        doThrow(new SyncException(SyncError.CANCELED))
                .when(s)
                .pushFile(
                        Mockito.eq(tmpFile.getAbsolutePath()),
                        Mockito.eq(fakeRemotePath),
                        Mockito.any(ISyncProgressMonitor.class));
        try {
            boolean res = mTestDevice.pushFile(tmpFile, fakeRemotePath);
            EasyMock.verify(mMockIDevice);
            Mockito.verify(s)
                    .pushFile(
                            Mockito.eq(tmpFile.getAbsolutePath()),
                            Mockito.eq(fakeRemotePath),
                            Mockito.any(ISyncProgressMonitor.class));
            Mockito.verify(s).close();
            assertFalse(res);
        } finally {
            FileUtil.deleteFile(tmpFile);
        }
    }

    /**
     * Test validating valid MAC addresses
     */
    public void testIsMacAddress() {
        assertTrue(mTestDevice.isMacAddress("00:00:00:00:00:00"));
        assertTrue(mTestDevice.isMacAddress("00:15:E9:2B:99:3C"));
        assertTrue(mTestDevice.isMacAddress("FF:FF:FF:FF:FF:FF"));
        assertTrue(mTestDevice.isMacAddress("58:a2:b5:7d:49:24"));
    }

    /**
     * Test validating invalid MAC addresses
     */
    public void testIsMacAddress_invalidInput() {
        assertFalse(mTestDevice.isMacAddress(""));
        assertFalse(mTestDevice.isMacAddress("00-15-E9-2B-99-3C")); // Invalid delimiter
    }

    /**
     * Test querying a device MAC address
     */
    public void testGetMacAddress() throws Exception {
        String address = "58:a2:b5:7d:49:24";
        IDevice device = new StubDevice(MOCK_DEVICE_SERIAL) {
            @Override
            public void executeShellCommand(String command, IShellOutputReceiver receiver)
                    throws TimeoutException, AdbCommandRejectedException,
                    ShellCommandUnresponsiveException, IOException {
                receiver.addOutput(address.getBytes(), 0, address.length());
            }
        };
        mMockIDevice.executeShellCommand(EasyMock.anyObject(), EasyMock.anyObject());
        EasyMock.expectLastCall().andDelegateTo(device).anyTimes();
        EasyMock.replay(mMockIDevice);
        assertEquals(address, mTestDevice.getMacAddress());
        EasyMock.verify(mMockIDevice);
    }

    /**
     * Test querying a device MAC address when the device is in fastboot
     */
    public void testGetMacAddress_fastboot() throws Exception {
        mTestDevice.setDeviceState(TestDeviceState.FASTBOOT);
        // Will fail if executeShellCommand is called. Which it should not.
        assertNull(mTestDevice.getMacAddress());
    }

    /**
     * Test querying a device MAC address but failing to do so
     */
    public void testGetMacAddress_failure() throws Exception {
        IDevice device = new StubDevice(MOCK_DEVICE_SERIAL) {
            @Override
            public void executeShellCommand(String command, IShellOutputReceiver receiver)
                    throws TimeoutException, AdbCommandRejectedException,
                    ShellCommandUnresponsiveException, IOException {
                throw new IOException();
            }
        };
        mMockIDevice.executeShellCommand(EasyMock.anyObject(), EasyMock.anyObject());
        EasyMock.expectLastCall().andDelegateTo(device).anyTimes();
        EasyMock.replay(mMockIDevice);
        assertNull(mTestDevice.getMacAddress());
        EasyMock.verify(mMockIDevice);
    }

    /**
     * Test querying a device MAC address for a stub device
     */
    public void testGetMacAddress_stubDevice() throws Exception {
        String address = "58:a2:b5:7d:49:24";
        IDevice device = new StubDevice(MOCK_DEVICE_SERIAL) {
            @Override
            public void executeShellCommand(String command, IShellOutputReceiver receiver)
                    throws TimeoutException, AdbCommandRejectedException,
                    ShellCommandUnresponsiveException, IOException {
                receiver.addOutput(address.getBytes(), 0, address.length());
            }
        };
        mTestDevice.setIDevice(device);
        assertNull(mTestDevice.getMacAddress());
    }

    /** Test that a non online device return null for sim state. */
    public void testGetSimState_unavailableDevice() {
        mMockIDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockIDevice.getState()).andReturn(DeviceState.UNAUTHORIZED);
        EasyMock.expect(mMockIDevice.getSerialNumber()).andReturn("serial");
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        assertNull(mTestDevice.getSimState());
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /** Test that a non online device return null for sim operator. */
    public void testGetSimOperator_unavailableDevice() {
        mMockIDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockIDevice.getState()).andReturn(DeviceState.UNAUTHORIZED);
        EasyMock.expect(mMockIDevice.getSerialNumber()).andReturn("serial");
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        assertNull(mTestDevice.getSimOperator());
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /**
     * Test that when a {@link NativeDevice#getLogcatSince(long)} is requested a matching logcat
     * command is generated.
     */
    public void testGetLogcatSince() throws Exception {
        long date = 1512990942000L; // 2017-12-11 03:15:42.015
        EasyMock.expect(mMockIDevice.getState()).andReturn(DeviceState.ONLINE);
        SettableFuture<String> value = SettableFuture.create();
        value.set("23");
        EasyMock.expect(mMockIDevice.getSystemProperty("ro.build.version.sdk")).andReturn(value);
        mMockIDevice.executeShellCommand(
                EasyMock.eq("logcat -v threadtime -t '12-11 03:15:42.015'"), EasyMock.anyObject());
        EasyMock.replay(mMockIDevice);
        mTestDevice.getLogcatSince(date);
        EasyMock.verify(mMockIDevice);
    }

    public void testGetProductVariant() throws Exception {
        EasyMock.expect(mMockIDevice.getProperty(DeviceProperties.VARIANT)).andReturn("variant");

        EasyMock.replay(mMockIDevice);
        assertEquals("variant", mTestDevice.getProductVariant());
        EasyMock.verify(mMockIDevice);
    }

    public void testGetProductVariant_legacy() throws Exception {
        TestableAndroidNativeDevice testDevice =
                new TestableAndroidNativeDevice() {
                    @Override
                    protected String internalGetProperty(
                            String propName, String fastbootVar, String description)
                            throws DeviceNotAvailableException, UnsupportedOperationException {
                        if (DeviceProperties.VARIANT_LEGACY.equals(propName)) {
                            return "legacy";
                        }
                        return null;
                    }
                };

        assertEquals("legacy", testDevice.getProductVariant());
    }

    /** Test when {@link NativeDevice#executeShellV2Command(String)} returns a success. */
    public void testExecuteShellV2Command() throws Exception {
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.SUCCESS);
        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                100, "adb", "-s", "serial", "shell", "some", "command"))
                .andReturn(res);
        EasyMock.replay(mMockRunUtil, mMockIDevice);
        assertNotNull(mTestDevice.executeShellV2Command("some command"));
        EasyMock.verify(mMockRunUtil, mMockIDevice);
    }

    /**
     * Test when {@link NativeDevice#executeShellV2Command(String, long, TimeUnit, int)} fails and
     * repeat because of a timeout.
     */
    public void testExecuteShellV2Command_timeout() throws Exception {
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.TIMED_OUT);
        res.setStderr("timed out");
        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                200L, "adb", "-s", "serial", "shell", "some", "command"))
                .andReturn(res)
                .times(2);
        EasyMock.replay(mMockRunUtil, mMockIDevice);
        try {
            mTestDevice.executeShellV2Command("some command", 200L, TimeUnit.MILLISECONDS, 1);
            fail("Should have thrown an exception.");
        } catch (DeviceUnresponsiveException e) {
            // expected
        }
        EasyMock.verify(mMockRunUtil, mMockIDevice);
    }

    /**
     * Test when {@link NativeDevice#executeShellV2Command(String, long, TimeUnit, int)} fails and
     * output.
     */
    public void testExecuteShellV2Command_fail() throws Exception {
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.FAILED);
        res.setStderr("timed out");
        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                200L, "adb", "-s", "serial", "shell", "some", "command"))
                .andReturn(res)
                .times(1);
        EasyMock.replay(mMockRunUtil, mMockIDevice);
        CommandResult result =
                mTestDevice.executeShellV2Command("some command", 200L, TimeUnit.MILLISECONDS, 1);
        assertNotNull(result);
        // The final result is what RunUtil returned, so it contains full status information.
        assertSame(res, result);
        EasyMock.verify(mMockRunUtil, mMockIDevice);
    }
}
