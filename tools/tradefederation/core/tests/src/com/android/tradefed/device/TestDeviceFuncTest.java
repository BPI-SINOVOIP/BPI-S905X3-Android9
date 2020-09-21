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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.TestAppConstants;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.KeyguardControllerState;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.awt.image.BufferedImage;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import javax.imageio.ImageIO;

/**
 * Functional tests for {@link TestDevice}.
 *
 * <p>Requires a physical device to be connected.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class TestDeviceFuncTest implements IDeviceTest {

    private static final String LOG_TAG = "TestDeviceFuncTest";
    private TestDevice mTestDevice;
    private IDeviceStateMonitor mMonitor;
    /** Expect bugreports to be at least a meg. */
    private static final int MIN_BUGREPORT_BYTES = 1024 * 1024;

    @Override
    public void setDevice(ITestDevice device) {
        mTestDevice = (TestDevice) device;
    }

    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }

    @Before
    public void setUp() throws Exception {
        mMonitor = mTestDevice.getDeviceStateMonitor();
        // Ensure at set-up that the device is available.
        mTestDevice.waitForDeviceAvailable();
    }

    /** Simple testcase to ensure that the grabbing a bugreport from a real TestDevice works. */
    @Test
    public void testBugreport() throws Exception {
        InputStreamSource bugreport = mTestDevice.getBugreport();
        try {
            String data = StreamUtil.getStringFromStream(bugreport.createInputStream());
            assertTrue(
                    String.format(
                            "Expected at least %d characters; only saw %d",
                            MIN_BUGREPORT_BYTES, data.length()),
                    data.length() >= MIN_BUGREPORT_BYTES);
        } finally {
            StreamUtil.cancel(bugreport);
        }
    }

    /** Simple testcase to ensure that the grabbing a bugreportz from a real TestDevice works. */
    @Test
    public void testBugreportz() throws Exception {
        if (mTestDevice.getApiLevel() < 24) {
            CLog.i("testBugreportz() not supported by this device, skipping.");
            return;
        }
        FileInputStreamSource f = null;
        try {
            f = (FileInputStreamSource) mTestDevice.getBugreportz();
            assertNotNull(f);
            FileInputStream contents = (FileInputStream) f.createInputStream();
            assertTrue(
                    String.format(
                            "Expected at least %d characters; only saw %d",
                            MIN_BUGREPORT_BYTES, contents.available()),
                    contents.available() >= MIN_BUGREPORT_BYTES);
        } finally {
            StreamUtil.cancel(f);
            if (f != null) {
                f.cleanFile();
            }
        }
    }

    /**
     * Simple normal case test for {@link TestDevice#executeShellCommand(String)}.
     *
     * <p>Do a 'shell ls' command, and verify /data and /system are listed in result.
     */
    @Test
    public void testExecuteShellCommand() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testExecuteShellCommand");
        assertSimpleShellCommand();
    }

    /**
     * Verify that a simple {@link TestDevice#executeShellCommand(String)} command is successful.
     */
    private void assertSimpleShellCommand() throws DeviceNotAvailableException {
        final String output = mTestDevice.executeShellCommand("ls");
        assertTrue(output.contains("data"));
        assertTrue(output.contains("system"));
    }

    /** Test install and uninstall of package */
    @Test
    public void testInstallUninstall() throws IOException, DeviceNotAvailableException {
        Log.i(LOG_TAG, "testInstallUninstall");
        // use the wifi util apk
        File tmpFile = WifiHelper.extractWifiUtilApk();
        try {
            assertWifiApkInstall(tmpFile);
        } finally {
            FileUtil.deleteFile(tmpFile);
        }
    }

    /**
     * Verifies that the given wifi util apk can be installed and uninstalled successfully
     */
    void assertWifiApkInstall(File tmpFile) throws DeviceNotAvailableException {
        try {
            mTestDevice.uninstallPackage(WifiHelper.INSTRUMENTATION_PKG);
            assertFalse(mTestDevice.getInstalledPackageNames().contains(
                    WifiHelper.INSTRUMENTATION_PKG));
            assertNull(mTestDevice.installPackage(tmpFile, false));
            assertTrue(mTestDevice.getInstalledPackageNames().contains(
                    WifiHelper.INSTRUMENTATION_PKG));
            assertFalse("apk file was not cleaned up after install",
                    mTestDevice.doesFileExist(String.format("/data/local/tmp/%s",
                            tmpFile.getName())));
        } finally {
            FileUtil.deleteFile(tmpFile);
        }
    }

    /** Test install and uninstall of package with spaces in file name */
    @Test
    public void testInstallUninstall_space() throws IOException, DeviceNotAvailableException {
        Log.i(LOG_TAG, "testInstallUninstall_space");

        File tmpFile = WifiHelper.extractWifiUtilApk();
        File tmpFileSpaces = null;
        try {
            tmpFileSpaces = FileUtil.createTempFile("wifi util (3)", ".apk");
            FileUtil.copyFile(tmpFile, tmpFileSpaces);
            assertWifiApkInstall(tmpFileSpaces);
        } finally {
            FileUtil.deleteFile(tmpFile);
            FileUtil.deleteFile(tmpFileSpaces);
        }
    }

    /** Push and then pull a file from device, and verify contents are as expected. */
    @Test
    public void testPushPull_normal() throws IOException, DeviceNotAvailableException {
        Log.i(LOG_TAG, "testPushPull");
        File tmpFile = null;
        File tmpDestFile = null;
        String deviceFilePath = null;

        try {
            tmpFile = createTempTestFile(null);
            String externalStorePath = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
            assertNotNull(externalStorePath);
            deviceFilePath = String.format("%s/%s", externalStorePath, "tmp_testPushPull.txt");
            // ensure file does not already exist
            mTestDevice.executeShellCommand(String.format("rm %s", deviceFilePath));
            assertFalse(String.format("%s exists", deviceFilePath),
                    mTestDevice.doesFileExist(deviceFilePath));

            assertTrue(mTestDevice.pushFile(tmpFile, deviceFilePath));
            assertTrue(mTestDevice.doesFileExist(deviceFilePath));
            tmpDestFile = FileUtil.createTempFile("tmp", "txt");
            assertTrue(mTestDevice.pullFile(deviceFilePath, tmpDestFile));
            assertTrue(compareFiles(tmpFile, tmpDestFile));
        } finally {
            FileUtil.deleteFile(tmpFile);
            if (tmpDestFile != null) {
                tmpDestFile.delete();
            }
            if (deviceFilePath != null) {
                mTestDevice.executeShellCommand(String.format("rm %s", deviceFilePath));
            }
        }
    }

    /**
     * Push and then pull a file from device, and verify contents are as expected.
     *
     * <p>This variant of the test uses "${EXTERNAL_STORAGE}" in the pathname.
     */
    @Test
    public void testPushPull_extStorageVariable() throws IOException, DeviceNotAvailableException {
        Log.i(LOG_TAG, "testPushPull");
        File tmpFile = null;
        File tmpDestFile = null;
        File tmpDestFile2 = null;
        String deviceFilePath = null;
        final String filename = "tmp_testPushPull.txt";

        try {
            tmpFile = createTempTestFile(null);
            String externalStorePath = "${EXTERNAL_STORAGE}";
            assertNotNull(externalStorePath);
            deviceFilePath = String.format("%s/%s", externalStorePath, filename);
            // ensure file does not already exist
            mTestDevice.executeShellCommand(String.format("rm %s", deviceFilePath));
            assertFalse(String.format("%s exists", deviceFilePath),
                    mTestDevice.doesFileExist(deviceFilePath));

            assertTrue(mTestDevice.pushFile(tmpFile, deviceFilePath));
            assertTrue(mTestDevice.doesFileExist(deviceFilePath));
            tmpDestFile = FileUtil.createTempFile("tmp", "txt");
            assertTrue(mTestDevice.pullFile(deviceFilePath, tmpDestFile));
            assertTrue(compareFiles(tmpFile, tmpDestFile));

            tmpDestFile2 = mTestDevice.pullFileFromExternal(filename);
            assertNotNull(tmpDestFile2);
            assertTrue(compareFiles(tmpFile, tmpDestFile2));
        } finally {
            FileUtil.deleteFile(tmpFile);
            FileUtil.deleteFile(tmpDestFile);
            FileUtil.deleteFile(tmpDestFile2);
            if (deviceFilePath != null) {
                mTestDevice.executeShellCommand(String.format("rm %s", deviceFilePath));
            }
        }
    }

    /**
     * Test pulling a file from device that does not exist.
     *
     * <p>Expect {@link TestDevice#pullFile(String)} to return <code>false</code>
     */
    @Test
    public void testPull_noexist() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testPull_noexist");

        // make sure the root path is valid
        String externalStorePath =  mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        assertNotNull(externalStorePath);
        String deviceFilePath = String.format("%s/%s", externalStorePath, "thisfiledoesntexist");
        assertFalse(String.format("%s exists", deviceFilePath),
                mTestDevice.doesFileExist(deviceFilePath));
        assertNull(mTestDevice.pullFile(deviceFilePath));
    }

    /**
     * Test pulling a file from device into a local file that cannot be written to.
     *
     * <p>Expect {@link TestDevice#pullFile(String, File)} to return <code>false</code>
     */
    @Test
    public void testPull_nopermissions() throws IOException, DeviceNotAvailableException {
        CLog.i("testPull_nopermissions");

        // make sure the root path is valid
        String externalStorePath =  mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        assertNotNull(externalStorePath);
        String deviceFilePath = String.format("%s/%s", externalStorePath, "testPull_nopermissions");
        // first push a file so we have something to retrieve
        assertTrue(mTestDevice.pushString("test data", deviceFilePath));
        assertTrue(String.format("%s does not exist", deviceFilePath),
                mTestDevice.doesFileExist(deviceFilePath));
        File tmpFile = null;
        try {
            tmpFile = FileUtil.createTempFile("testPull_nopermissions", ".txt");
            tmpFile.setReadOnly();
            assertFalse(mTestDevice.pullFile(deviceFilePath, tmpFile));
        } finally {
            if (tmpFile != null) {
                tmpFile.setWritable(true);
                FileUtil.deleteFile(tmpFile);
            }
        }
    }

    /**
     * Test pushing a file onto device that does not exist.
     *
     * <p>Expect {@link TestDevice#pushFile(File, String)} to return <code>false</code>
     */
    @Test
    public void testPush_noexist() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testPush_noexist");

        // make sure the root path is valid
        String externalStorePath =  mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        assertNotNull(externalStorePath);
        String deviceFilePath = String.format("%s/%s", externalStorePath, "remotepath");
        assertFalse(mTestDevice.pushFile(new File("idontexist"), deviceFilePath));
    }

    private File createTempTestFile(File dir) throws IOException {
        File tmpFile = null;
        try {
            final String fileContents = "this is the test file contents";
            tmpFile = FileUtil.createTempFile("tmp", ".txt", dir);
            FileUtil.writeToFile(fileContents, tmpFile);
            return tmpFile;
        } catch (IOException e) {
            if (tmpFile != null) {
                tmpFile.delete();
            }
            throw e;
        }
    }

    /**
     * Utility method to do byte-wise content comparison of two files.
     */
    private boolean compareFiles(File file1, File file2) throws IOException {
        BufferedInputStream stream1 = null;
        BufferedInputStream stream2 = null;

        try {
            stream1 = new BufferedInputStream(new FileInputStream(file1));
            stream2 = new BufferedInputStream(new FileInputStream(file2));
            boolean eof = false;
            while (!eof) {
                int byte1 = stream1.read();
                int byte2 = stream2.read();
                if (byte1 != byte2) {
                    return false;
                }
                eof = byte1 == -1;
            }
            return true;
        } finally {
            StreamUtil.close(stream1);
            StreamUtil.close(stream2);
        }
    }

    /**
     * Make sure that we can correctly index directories that have a symlink in the middle. This
     * verifies a ddmlib bugfix which added/fixed this functionality.
     */
    @Test
    public void testListSymlinkDir() throws Exception {
        final String extStore = "/data/local";

        // Clean up after potential failed run
        mTestDevice.executeShellCommand(String.format("rm %s/testdir", extStore));
        mTestDevice.executeShellCommand(String.format("rm %s/testdir2/foo.txt", extStore));
        mTestDevice.executeShellCommand(String.format("rmdir %s/testdir2", extStore));

        try {
            assertEquals("",
                    mTestDevice.executeShellCommand(String.format("mkdir %s/testdir2",
                    extStore)));
            assertEquals("", mTestDevice.executeShellCommand(
                    String.format("touch %s/testdir2/foo.txt", extStore)));
            assertEquals("",
                    mTestDevice.executeShellCommand(String.format("ln -s %s/testdir2 %s/testdir",
                    extStore, extStore)));

            assertNotNull(mTestDevice.getFileEntry(String.format("%s/testdir/foo.txt", extStore)));
        } finally {
            mTestDevice.executeShellCommand(String.format("rm %s/testdir", extStore));
            mTestDevice.executeShellCommand(String.format("rm %s/testdir2/foo.txt", extStore));
            mTestDevice.executeShellCommand(String.format("rmdir %s/testdir2", extStore));
        }
    }

    /** Test syncing a single file using {@link TestDevice#syncFiles(File, String)}. */
    @Test
    public void testSyncFiles_normal() throws Exception {
        doTestSyncFiles(mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE));
    }

    /**
     * Test syncing a single file using {@link TestDevice#syncFiles(File, String)}.
     *
     * <p>This variant of the test uses "${EXTERNAL_STORAGE}" in the pathname.
     */
    @Test
    public void testSyncFiles_extStorageVariable() throws Exception {
        doTestSyncFiles("${EXTERNAL_STORAGE}");
    }

    /** Test syncing a single file using {@link TestDevice#syncFiles(File, String)}. */
    private void doTestSyncFiles(String externalStorePath) throws Exception {
        String expectedDeviceFilePath = null;

        // create temp dir with one temp file
        File tmpDir = FileUtil.createTempDir("tmp");
        try {
            File tmpFile = createTempTestFile(tmpDir);
            // set last modified to 10 minutes ago
            tmpFile.setLastModified(System.currentTimeMillis() - 10*60*1000);
            assertNotNull(externalStorePath);
            expectedDeviceFilePath = String.format("%s/%s/%s", externalStorePath,
                    tmpDir.getName(), tmpFile.getName());

            assertTrue(mTestDevice.syncFiles(tmpDir, externalStorePath));
            assertTrue(mTestDevice.doesFileExist(expectedDeviceFilePath));

            // get 'ls -l' attributes of file which includes timestamp
            String origTmpFileStamp = mTestDevice.executeShellCommand(String.format("ls -l %s",
                    expectedDeviceFilePath));
            // now create another file and verify that is synced
            File tmpFile2 = createTempTestFile(tmpDir);
            tmpFile2.setLastModified(System.currentTimeMillis() - 10*60*1000);
            assertTrue(mTestDevice.syncFiles(tmpDir, externalStorePath));
            String expectedDeviceFilePath2 = String.format("%s/%s/%s", externalStorePath,
                    tmpDir.getName(), tmpFile2.getName());
            assertTrue(mTestDevice.doesFileExist(expectedDeviceFilePath2));

            // verify 1st file timestamp did not change
            String unchangedTmpFileStamp = mTestDevice.executeShellCommand(String.format("ls -l %s",
                    expectedDeviceFilePath));
            assertEquals(origTmpFileStamp, unchangedTmpFileStamp);

            // now modify 1st file and verify it does change remotely
            String testString = "blah";
            FileOutputStream stream = new FileOutputStream(tmpFile);
            stream.write(testString.getBytes());
            stream.close();

            assertTrue(mTestDevice.syncFiles(tmpDir, externalStorePath));
            String tmpFileContents = mTestDevice.executeShellCommand(String.format("cat %s",
                    expectedDeviceFilePath));
            assertTrue(tmpFileContents.contains(testString));
        } finally {
            if (expectedDeviceFilePath != null && externalStorePath != null) {
                // note that expectedDeviceFilePath has externalStorePath prepended at definition
                mTestDevice.executeShellCommand(String.format("rm -r %s", expectedDeviceFilePath));
            }
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /** Test pushing a directory */
    @Test
    public void testPushDir() throws IOException, DeviceNotAvailableException {
        String expectedDeviceFilePath = null;
        String externalStorePath = null;
        File rootDir = FileUtil.createTempDir("tmp");
        // create temp dir with one temp file
        try {
            File tmpDir = FileUtil.createTempDir("tmp", rootDir);
            File tmpFile = createTempTestFile(tmpDir);
            externalStorePath = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
            assertNotNull(externalStorePath);
            expectedDeviceFilePath = String.format("%s/%s/%s", externalStorePath,
                    tmpDir.getName(), tmpFile.getName());

            assertTrue(mTestDevice.pushDir(rootDir, externalStorePath));
            assertTrue(mTestDevice.doesFileExist(expectedDeviceFilePath));

        } finally {
            if (expectedDeviceFilePath != null && externalStorePath != null) {
                mTestDevice.executeShellCommand(String.format("rm -r %s/%s", externalStorePath,
                        expectedDeviceFilePath));
            }
            FileUtil.recursiveDelete(rootDir);
        }
    }

    /**
     * Test {@link TestDevice#executeFastbootCommand(String...)} when device is in adb mode.
     *
     * <p>Expect fastboot recovery to be invoked, which will boot device back to fastboot mode and
     * command will succeed.
     */
    @Test
    public void testExecuteFastbootCommand_deviceInAdb() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testExecuteFastbootCommand_deviceInAdb");
        if (!mTestDevice.isFastbootEnabled()) {
            Log.i(LOG_TAG, "Fastboot not enabled skipping testExecuteFastbootCommand_deviceInAdb");
            return;
        }
        long origTimeout = mTestDevice.getCommandTimeout();
        try {
            assertEquals(TestDeviceState.ONLINE, mMonitor.getDeviceState());
            // reset operation timeout to small value to make test run quicker
            mTestDevice.setCommandTimeout(5*1000);
            assertEquals(CommandStatus.SUCCESS,
            mTestDevice.executeFastbootCommand("getvar", "product").getStatus());
            assertEquals(TestDeviceState.FASTBOOT, mMonitor.getDeviceState());
        } finally {
            mTestDevice.setCommandTimeout(origTimeout);
            mTestDevice.reboot();
            assertEquals(TestDeviceState.ONLINE, mMonitor.getDeviceState());
        }
    }

    /**
     * Test {@link TestDevice#executeFastbootCommand(String...)} when an invalid command is passed.
     *
     * <p>Expect the result indicate failure, and recovery not to be invoked.
     */
    @Test
    public void testExecuteFastbootCommand_badCommand() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testExecuteFastbootCommand_badCommand");
        if (!mTestDevice.isFastbootEnabled()) {
            Log.i(LOG_TAG, "Fastboot not enabled skipping testExecuteFastbootCommand_badCommand");
            return;
        }
        IDeviceRecovery origRecovery = mTestDevice.getRecovery();
        try {
            mTestDevice.rebootIntoBootloader();
            assertEquals(TestDeviceState.FASTBOOT, mMonitor.getDeviceState());
            // substitute recovery mechanism to ensure recovery is not called when bad command
            // is passed
            IDeviceRecovery mockRecovery = EasyMock.createStrictMock(IDeviceRecovery.class);
            mTestDevice.setRecovery(mockRecovery);
            EasyMock.replay(mockRecovery);
            assertEquals(CommandStatus.FAILED,
            mTestDevice.executeFastbootCommand("badcommand").getStatus());
        } finally {
            mTestDevice.setRecovery(origRecovery);
            mTestDevice.reboot();
            assertEquals(TestDeviceState.ONLINE, mMonitor.getDeviceState());
        }
    }

    /** Verify device can be rebooted into bootloader and back to adb. */
    @Test
    public void testRebootIntoBootloader() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testRebootIntoBootloader");
        if (!mTestDevice.isFastbootEnabled()) {
            Log.i(LOG_TAG, "Fastboot not enabled skipping testRebootInBootloader");
            return;
        }
        try {
            mTestDevice.rebootIntoBootloader();
            assertEquals(TestDeviceState.FASTBOOT, mMonitor.getDeviceState());
        } finally {
            mTestDevice.reboot();
            assertEquals(TestDeviceState.ONLINE, mMonitor.getDeviceState());
        }
    }

    /** Verify device can be rebooted into adb. */
    @Test
    public void testReboot() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testReboot");
        mTestDevice.reboot();
        assertEquals(TestDeviceState.ONLINE, mMonitor.getDeviceState());
        // check that device has root
        assertTrue(mTestDevice.executeShellCommand("id").contains("root"));
    }

    /** Verify device can be rebooted into adb recovery. */
    @Test
    public void testRebootIntoRecovery() throws Exception {
        Log.i(LOG_TAG, "testRebootIntoRecovery");
        if (!mTestDevice.isFastbootEnabled()) {
            Log.i(LOG_TAG, "Fastboot not enabled skipping testRebootInRecovery");
            return;
        }
        try {
            mTestDevice.rebootIntoRecovery();
            assertEquals(TestDeviceState.RECOVERY, mMonitor.getDeviceState());
        } finally {
            // Recovery is a special case to recover from, we need to call reboot on the idevice.
            RunUtil.getDefault().sleep(15 * 1000);
            getDevice().getIDevice().reboot(null);
        }
    }

    /**
     * Verify that {@link TestDevice#clearErrorDialogs()} can successfully clear an error dialog
     * from screen.
     *
     * <p>This is done by running a test app which will crash, then running another app that does UI
     * based tests.
     *
     * <p>Assumes DevTools and TradeFedUiApp are currently installed.
     */
    @Test
    public void testClearErrorDialogs_crash() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testClearErrorDialogs_crash");
        // Ensure device is in a known state, we doing extra care here otherwise it may be flaky
        int retry = 5;
        mTestDevice.disableKeyguard();
        while (!runUITests()) {
            getDevice().reboot();
            mTestDevice.waitForDeviceAvailable();
            mTestDevice.disableKeyguard();
            RunUtil.getDefault().sleep(2000);
            if (retry == 0) {
                fail("Fail to setup the device in a known state");
            }
            retry--;
        }
        // now cause a crash dialog to appear
        getDevice().executeShellCommand("am start -n " + TestAppConstants.CRASH_ACTIVITY);
        RunUtil.getDefault().sleep(2000);
        // Ensure the error dialogue is here
        assertFalse(runUITests());
        RunUtil.getDefault().sleep(2000);

        assertTrue("Clear dialogs did not clear anything", getDevice().clearErrorDialogs());
        assertTrue(runUITests());
    }

    /**
     * Verify the steps taken to disable keyguard after reboot are successfully
     *
     * <p>This is done by rebooting then run a app that does UI based tests.
     *
     * <p>Assumes DevTools and TradeFedUiApp are currently installed.
     */
    @Test
    public void testDisableKeyguard() throws DeviceNotAvailableException {
        Log.i(LOG_TAG, "testDisableKeyguard");
        getDevice().reboot();
        mTestDevice.waitForDeviceAvailable();
        RunUtil.getDefault().sleep(3000);
        KeyguardControllerState keyguard = mTestDevice.getKeyguardState();
        if (keyguard == null) {
            // If the getKeyguardState is not supported.
            assertTrue(runUITests());
        } else {
            assertFalse(keyguard.isKeyguardShowing());
        }
    }

    /** Test that TradeFed can successfully recover from the adb host daemon process being killed */
    @Test
    public void testExecuteShellCommand_adbKilled() {
        // FIXME: adb typically does not recover, and this causes rest of tests to fail
        //Log.i(LOG_TAG, "testExecuteShellCommand_adbKilled");
        //CommandResult result = RunUtil.getInstance().runTimedCmd(30*1000, "adb", "kill-server");
        //assertEquals(CommandStatus.SUCCESS, result.getStatus());
        //assertSimpleShellCommand();
    }

    /**
     * Basic test for {@link TestDevice#getScreenshot()}.
     *
     * <p>Grab a screenshot, save it to a file, and perform a cursory size check to ensure its
     * valid.
     */
    @Test
    public void testGetScreenshot() throws DeviceNotAvailableException, IOException {
        CLog.i(LOG_TAG, "testGetScreenshot");
        InputStreamSource source = getDevice().getScreenshot();
        assertNotNull(source);
        File tmpPngFile = FileUtil.createTempFile("screenshot", ".png");
        try {
            FileUtil.writeToFile(source.createInputStream(), tmpPngFile);
            CLog.i("Created file at %s", tmpPngFile.getAbsolutePath());
            // Decode the content, will return null if not an image.
            BufferedImage image = ImageIO.read(tmpPngFile);
            assertNotNull(image);
            // All our device screenshot should be bigger than 200px
            assertTrue(image.getWidth() > 200);
            assertTrue(image.getHeight() > 200);
        } finally {
            FileUtil.deleteFile(tmpPngFile);
            source.close();
        }
    }

    /**
     * Basic test for {@link TestDevice#getLogcat(int)}.
     *
     * <p>Dumps a bunch of messages to logcat, calls getLogcat(), and verifies size of capture file
     * is equal to provided data.
     */
    @Test
    public void testGetLogcat_size() throws DeviceNotAvailableException, IOException {
        CLog.i(LOG_TAG, "testGetLogcat_size");
        for (int i = 0; i < 100; i++) {
            getDevice().executeShellCommand(String.format("log testGetLogcat_size log dump %d", i));
        }
        // sleep a small amount of time to ensure last log message makes it into capture
        RunUtil.getDefault().sleep(500);
        File tmpTxtFile = FileUtil.createTempFile("logcat", ".txt");
        try (InputStreamSource source = getDevice().getLogcatDump()) {
            assertNotNull(source);
            FileUtil.writeToFile(source.createInputStream(), tmpTxtFile);
            CLog.i("Created file at %s", tmpTxtFile.getAbsolutePath());
            // Check we have at least our 100 lines.
            assertTrue("Saved text file is smaller than expected",
                    100 * 1024 <= tmpTxtFile.length());
            // ensure last log message is present in log
            String s = FileUtil.readStringFromFile(tmpTxtFile);
            assertTrue("last log message is not in captured logcat",
                    s.contains("testGetLogcat_size log dump 99"));
        } finally {
            FileUtil.deleteFile(tmpTxtFile);
        }
    }

    /**
     * Basic test for encryption if encryption is supported.
     *
     * <p>Calls {@link TestDevice#encryptDevice(boolean)}, {@link TestDevice#unlockDevice()}, and
     * {@link TestDevice#unencryptDevice()}, as well as reboots the device while the device is
     * encrypted.
     *
     * @throws DeviceNotAvailableException
     */
    @Test
    public void testEncryption() throws DeviceNotAvailableException {
        CLog.i("testEncryption");

        if (!getDevice().isEncryptionSupported()) {
            CLog.i("Encrypting userdata is not supported. Skipping test.");
            return;
        }

        assertTrue(getDevice().unencryptDevice());
        assertFalse(getDevice().isDeviceEncrypted());
        assertTrue(getDevice().encryptDevice(false));
        assertTrue(getDevice().isDeviceEncrypted());
        assertTrue(getDevice().unlockDevice());
        // TODO: decryptUserData() can be called more than once, the framework should only be
        // restarted on the first call.
        assertTrue(getDevice().unlockDevice());
        getDevice().reboot();
        assertTrue(getDevice().unencryptDevice());
        assertFalse(getDevice().isDeviceEncrypted());
    }

    /** Test that {@link TestDevice#getProperty(String)} works after a reboot. */
    @Test
    public void testGetProperty() throws Exception {
        assertNotNull(getDevice().getProperty(DeviceProperties.BOARD));
        getDevice().rebootUntilOnline();
        assertNotNull(getDevice().getProperty(DeviceProperties.BOARD));
    }

    /** Test that {@link TestDevice#getProperty(String)} works for volatile properties. */
    @Test
    public void testGetProperty_volatile() throws Exception {
        getDevice().executeShellCommand("setprop prop.test 0");
        assertEquals("0", getDevice().getProperty("prop.test"));
        getDevice().executeShellCommand("setprop prop.test 1");
        assertEquals("1", getDevice().getProperty("prop.test"));
    }

    /** Test that the recovery mechanism works in {@link TestDevice#getFileEntry(String)} */
    @Test
    public void testGetFileEntry_recovery() throws Exception {
        if (!mTestDevice.isFastbootEnabled()) {
            Log.i(LOG_TAG, "Fastboot not enabled skipping testGetFileEntry_recovery");
            return;
        }
        try {
            getDevice().rebootIntoBootloader();
            // expect recovery to kick in, and reboot device back to adb so the call works
            IFileEntry entry = getDevice().getFileEntry("/data");
            assertNotNull(entry);
        } finally {
            getDevice().reboot();
        }
    }

    /**
     * Run the test app UI tests and return true if they all pass.
     */
    private boolean runUITests() throws DeviceNotAvailableException {
        RemoteAndroidTestRunner uirunner = new RemoteAndroidTestRunner(
                TestAppConstants.UITESTAPP_PACKAGE, getDevice().getIDevice());
        CollectingTestListener uilistener = new CollectingTestListener();
        getDevice().runInstrumentationTests(uirunner, uilistener);
        return TestAppConstants.UI_TOTAL_TESTS == uilistener.getNumTestsInState(TestStatus.PASSED);
    }

    /** Test for {@link NativeDevice#setSetting(int, String, String, String)} */
    @Test
    public void testPutSettings() throws Exception {
        String initValue = mTestDevice.getSetting(0, "system", "screen_brightness");
        CLog.i("initial value was: %s", initValue);
        assertTrue(!initValue.equals("50"));
        mTestDevice.setSetting(0, "system", "screen_brightness", "50");
        String secondValue = mTestDevice.getSetting(0, "system", "screen_brightness");
        assertEquals("50", secondValue);
        // restore initial value
        mTestDevice.setSetting(0, "system", "screen_brightness", initValue);
    }
}
