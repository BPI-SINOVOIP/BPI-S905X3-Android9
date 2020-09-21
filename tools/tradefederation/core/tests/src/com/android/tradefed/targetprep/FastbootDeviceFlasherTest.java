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

package com.android.tradefed.targetprep;

import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.targetprep.IDeviceFlasher.UserDataFlashOption;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * Unit tests for {@link FastbootDeviceFlasher}.
 */
public class FastbootDeviceFlasherTest extends TestCase {

    /** a temp 'don't care value' string to use */
    private static final String TEST_STRING = "foo";
    private FastbootDeviceFlasher mFlasher;
    private ITestDevice mMockDevice;
    private IDeviceBuildInfo mMockBuildInfo;
    private IFlashingResourcesRetriever mMockRetriever;
    private IFlashingResourcesParser mMockParser;
    private IRunUtil mMockRunUtil;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn(TEST_STRING);
        EasyMock.expect(mMockDevice.getProductType()).andStubReturn(TEST_STRING);
        EasyMock.expect(mMockDevice.getBuildId()).andStubReturn("1");
        EasyMock.expect(mMockDevice.getBuildFlavor()).andStubReturn("test-debug");
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);
        mMockBuildInfo = new DeviceBuildInfo("0", TEST_STRING);
        mMockBuildInfo.setDeviceImageFile(new File(TEST_STRING), "0");
        mMockBuildInfo.setUserDataImageFile(new File(TEST_STRING), "0");
        mMockRetriever = EasyMock.createNiceMock(IFlashingResourcesRetriever.class);
        mMockParser = EasyMock.createNiceMock(IFlashingResourcesParser.class);
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);

        mFlasher = new FastbootDeviceFlasher() {
            @Override
            protected IFlashingResourcesParser createFlashingResourcesParser(
                    IDeviceBuildInfo localBuild, DeviceDescriptor descriptor) {
                return mMockParser;
            }
            @Override
            protected IRunUtil getRunUtil() {
                return mMockRunUtil;
            }
        };
        mFlasher.setFlashingResourcesRetriever(mMockRetriever);
        mFlasher.setUserDataFlashOption(UserDataFlashOption.RETAIN);
    }

    /**
     * Test {@link FastbootDeviceFlasher#flash(ITestDevice, IDeviceBuildInfo)}
     * when device is not available.
     */
    public void testFlash_deviceNotAvailable() throws DeviceNotAvailableException  {
       try {
            mMockDevice.rebootIntoBootloader();
            // TODO: this is fixed to two arguments - how to set to expect a variable arg amount ?
            mMockDevice.executeFastbootCommand((String)EasyMock.anyObject(),
                    (String)EasyMock.anyObject());
            EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());
            EasyMock.replay(mMockDevice);
            mFlasher.flash(mMockDevice, mMockBuildInfo);
            fail("TargetSetupError not thrown");
        } catch (TargetSetupError e) {
            // expected
        }
    }

    /**
     * Test DeviceFlasher#flash(ITestDevice, IDeviceBuildInfo)} when required board info is not
     * present.
     */
    public void testFlash_missingBoard() throws DeviceNotAvailableException  {
        mMockDevice.rebootIntoBootloader();
        EasyMock.expect(mMockParser.getRequiredBoards()).andReturn(null);
        EasyMock.replay(mMockDevice);
        try {
            mFlasher.flash(mMockDevice, mMockBuildInfo);
            fail("TargetSetupError not thrown");
        } catch (TargetSetupError e) {
            // expected
        }
    }

    /**
     * Test {@link FastbootDeviceFlasher#getImageVersion(ITestDevice, String)}
     */
    public void testGetImageVersion() throws DeviceNotAvailableException, TargetSetupError {
        CommandResult fastbootResult = new CommandResult();
        fastbootResult.setStatus(CommandStatus.SUCCESS);
        // output of getvar is on stderr for some unknown reason
        fastbootResult.setStderr("version-bootloader: 1.0.1\nfinished. total time: 0.001s");
        fastbootResult.setStdout("");
        EasyMock.expect(mMockDevice.executeFastbootCommand("getvar", "version-bootloader")).
                andReturn(fastbootResult);
        EasyMock.replay(mMockDevice, mMockRunUtil);
        String actualVersion = mFlasher.getImageVersion(mMockDevice, "bootloader");
        assertEquals("1.0.1", actualVersion);
        EasyMock.verify(mMockDevice, mMockRunUtil);
    }

    /**
     * Test {@link FastbootDeviceFlasher#getCurrentSlot(ITestDevice)} when device is in fastboot.
     */
    public void testGetCurrentSlot_fastboot() throws DeviceNotAvailableException, TargetSetupError {
        CommandResult fastbootResult = new CommandResult();
        fastbootResult.setStatus(CommandStatus.SUCCESS);
        fastbootResult.setStderr("current-slot: _a\nfinished. total time 0.001s");
        fastbootResult.setStdout("");
        EasyMock.expect(mMockDevice.getDeviceState()).andReturn(TestDeviceState.FASTBOOT);
        EasyMock.expect(mMockDevice.executeFastbootCommand("getvar", "current-slot"))
                .andReturn(fastbootResult);
        EasyMock.replay(mMockDevice, mMockRunUtil);
        String currentSlot = mFlasher.getCurrentSlot(mMockDevice);
        assertEquals("a", currentSlot);
        EasyMock.verify(mMockDevice, mMockRunUtil);
    }

    /** Test {@link FastbootDeviceFlasher#getCurrentSlot(ITestDevice)} when device is in adb. */
    public void testGetCurrentSlot_adb() throws DeviceNotAvailableException, TargetSetupError {
        String adbResult = "[ro.boot.slot_suffix]: [_b]\n";
        EasyMock.expect(mMockDevice.getDeviceState()).andReturn(TestDeviceState.ONLINE);
        EasyMock.expect(mMockDevice.executeShellCommand("getprop ro.boot.slot_suffix"))
                .andReturn(adbResult);
        EasyMock.replay(mMockDevice, mMockRunUtil);
        String currentSlot = mFlasher.getCurrentSlot(mMockDevice);
        assertEquals("b", currentSlot);
        EasyMock.verify(mMockDevice, mMockRunUtil);
    }

    /**
     * Test {@link FastbootDeviceFlasher#getCurrentSlot(ITestDevice)} when device does not support
     * A/B.
     */
    public void testGetCurrentSlot_null() throws DeviceNotAvailableException, TargetSetupError {
        String adbResult = "\n";
        EasyMock.expect(mMockDevice.getDeviceState()).andReturn(TestDeviceState.ONLINE);
        EasyMock.expect(mMockDevice.executeShellCommand("getprop ro.boot.slot_suffix"))
                .andReturn(adbResult);
        EasyMock.replay(mMockDevice, mMockRunUtil);
        String currentSlot = mFlasher.getCurrentSlot(mMockDevice);
        assertNull(currentSlot);
        EasyMock.verify(mMockDevice, mMockRunUtil);
    }

    /** Test that a fastboot command is retried if it does not output anything. */
    public void testRetryGetVersionCommand() throws DeviceNotAvailableException, TargetSetupError {
        // The first time command is tried, make it return an empty string.
        CommandResult fastbootInValidResult = new CommandResult();
        fastbootInValidResult.setStatus(CommandStatus.SUCCESS);
        // output of getvar is on stderr for some unknown reason
        fastbootInValidResult.setStderr("");
        fastbootInValidResult.setStdout("");

        // Return the correct value on second attempt.
        CommandResult fastbootValidResult = new CommandResult();
        fastbootValidResult.setStatus(CommandStatus.SUCCESS);
        fastbootValidResult.setStderr("version-baseband: 1.0.1\nfinished. total time: 0.001s");
        fastbootValidResult.setStdout("");

        EasyMock.expect(mMockDevice.executeFastbootCommand("getvar", "version-baseband")).
                andReturn(fastbootInValidResult);
        EasyMock.expect(mMockDevice.executeFastbootCommand("getvar", "version-baseband")).
                andReturn(fastbootValidResult);
        mMockRunUtil.sleep(EasyMock.anyLong());
        EasyMock.expectLastCall();

        EasyMock.replay(mMockDevice, mMockRunUtil);
        String actualVersion = mFlasher.getImageVersion(mMockDevice, "baseband");
        EasyMock.verify(mMockDevice, mMockRunUtil);
        assertEquals("1.0.1", actualVersion);
    }

    /**
     * Test that baseband can be flashed when current baseband version is empty
     */
    public void testFlashBaseband_noVersion()
            throws DeviceNotAvailableException, TargetSetupError {
        final String newBasebandVersion = "1.0.1";
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        // expect a fastboot getvar version-baseband command
        setFastbootResponseExpectations(mockDevice, "version-baseband: \n");
        setFastbootResponseExpectations(mockDevice, "version-baseband: \n");
        // expect a 'flash radio' command
        setFastbootFlashExpectations(mockDevice, "radio");
        mockDevice.rebootIntoBootloader();
        EasyMock.replay(mockDevice, mMockRunUtil);

        FastbootDeviceFlasher flasher = getFlasherWithParserData(
                String.format("require version-baseband=%s", newBasebandVersion));

        IDeviceBuildInfo build = new DeviceBuildInfo("1234", "build-name");
        build.setBasebandImage(new File("tmp"), newBasebandVersion);
        flasher.checkAndFlashBaseband(mockDevice, build);
        EasyMock.verify(mockDevice, mMockRunUtil);
    }

    /**
     * Test flashing of user data with a tests zip
     *
     * @throws TargetSetupError
     */
    public void testFlashUserData_testsZip() throws DeviceNotAvailableException, TargetSetupError {
        mFlasher.setUserDataFlashOption(UserDataFlashOption.TESTS_ZIP);

        ITestsZipInstaller mockZipInstaller = EasyMock.createMock(ITestsZipInstaller.class);
        mFlasher.setTestsZipInstaller(mockZipInstaller);
        // expect
        mockZipInstaller.pushTestsZipOntoData(EasyMock.eq(mMockDevice), EasyMock.eq(mMockBuildInfo));
        // expect
        mMockDevice.rebootUntilOnline();
        mMockDevice.rebootIntoBootloader();
        EasyMock.expect(mMockDevice.isEncryptionSupported()).andReturn(Boolean.FALSE);

        EasyMock.replay(mMockDevice, mockZipInstaller);
        mFlasher.flashUserData(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice, mockZipInstaller);
    }

    /**
     * Verify that correct fastboot command is called with WIPE data option
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError
     */
    public void testFlashUserData_wipe() throws DeviceNotAvailableException, TargetSetupError {
        mFlasher.setUserDataFlashOption(UserDataFlashOption.WIPE);
        doTestFlashWithWipe();
    }

    /**
     * Verify that correct fastboot command is called with FORCE_WIPE data option
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError
     */
    public void testFlashUserData_forceWipe() throws DeviceNotAvailableException, TargetSetupError {
        mFlasher.setUserDataFlashOption(UserDataFlashOption.FORCE_WIPE);
        doTestFlashWithWipe();
    }

    /**
     * Verify call sequence when wiping cache on devices with cache partition
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError
     */
    public void testWipeCache_exists() throws DeviceNotAvailableException, TargetSetupError {
        mFlasher.setUserDataFlashOption(UserDataFlashOption.WIPE);
        CommandResult fastbootOutput = new CommandResult();
        fastbootOutput.setStatus(CommandStatus.SUCCESS);
        fastbootOutput.setStderr("(bootloader) slot-count: not found\n" +
                "(bootloader) slot-suffixes: not found\n" +
                "(bootloader) slot-suffixes: not found\n" +
                "partition-type:cache: ext4\n" +
                "finished. total time: 0.002s\n");
        fastbootOutput.setStdout("");

        EasyMock.expect(mMockDevice.executeFastbootCommand("getvar", "partition-type:cache")).
                andReturn(fastbootOutput);
        EasyMock.expect(mMockDevice.getUseFastbootErase()).andReturn(false);
        fastbootOutput = new CommandResult();
        fastbootOutput.setStatus(CommandStatus.SUCCESS);
        fastbootOutput.setStderr("Creating filesystem with parameters:\n" +
                "    Size: 104857600\n" +
                "    Block size: 4096\n" +
                "    Blocks per group: 32768\n" +
                "    Inodes per group: 6400\n" +
                "    Inode size: 256\n" +
                "    Journal blocks: 1024\n" +
                "    Label: \n" +
                "    Blocks: 25600\n" +
                "    Block groups: 1\n" +
                "    Reserved block group size: 7\n" +
                "Created filesystem with 11/6400 inodes and 1438/25600 blocks\n" +
                "target reported max download size of 494927872 bytes\n" +
                "erasing 'cache'...\n" +
                "OKAY [  0.024s]\n" +
                "sending 'cache' (5752 KB)...\n" +
                "OKAY [  0.178s]\n" +
                "writing 'cache'...\n" +
                "OKAY [  0.107s]\n" +
                "finished. total time: 0.309s\n");
        EasyMock.expect(mMockDevice.fastbootWipePartition("cache")).andReturn(fastbootOutput);
        EasyMock.replay(mMockDevice);
        mFlasher.wipeCache(mMockDevice);
        EasyMock.verify(mMockDevice);
    }

    /**
     * Verify call sequence when wiping cache on devices without cache partition
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError
     */
    public void testWipeCache_not_exists() throws DeviceNotAvailableException, TargetSetupError {
        mFlasher.setUserDataFlashOption(UserDataFlashOption.WIPE);
        CommandResult fastbootOutput = new CommandResult();
        fastbootOutput.setStatus(CommandStatus.SUCCESS);
        fastbootOutput.setStderr("(bootloader) slot-count: not found\n" +
                "(bootloader) slot-suffixes: not found\n" +
                "(bootloader) slot-suffixes: not found\n" +
                "partition-type:cache: \n" +
                "finished. total time: 0.002s\n");
        fastbootOutput.setStdout("");

        EasyMock.expect(mMockDevice.executeFastbootCommand("getvar", "partition-type:cache")).
                andReturn(fastbootOutput);
        EasyMock.replay(mMockDevice);
        mFlasher.wipeCache(mMockDevice);
        EasyMock.verify(mMockDevice);
    }

    /**
     * Verify call sequence when wiping cache on devices without cache partition
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError
     */
    public void testWipeCache_not_exists_error()
            throws DeviceNotAvailableException, TargetSetupError {
        mFlasher.setUserDataFlashOption(UserDataFlashOption.WIPE);
        CommandResult fastbootOutput = new CommandResult();
        fastbootOutput.setStatus(CommandStatus.SUCCESS);
        fastbootOutput.setStderr("getvar:partition-type:cache FAILED (remote: unknown command)\n" +
                "finished. total time: 0.051s\n");
        fastbootOutput.setStdout("");

        EasyMock.expect(mMockDevice.executeFastbootCommand("getvar", "partition-type:cache")).
                andReturn(fastbootOutput);
        EasyMock.replay(mMockDevice);
        mFlasher.wipeCache(mMockDevice);
        EasyMock.verify(mMockDevice);
    }

    /**
     * Convenience function to set expectations for `fastboot -w` and execute test
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError
     */
    private void doTestFlashWithWipe()  throws DeviceNotAvailableException, TargetSetupError {
        CommandResult result = new CommandResult();
        result.setStatus(CommandStatus.SUCCESS);
        result.setStderr("");
        result.setStdout("");
        EasyMock.expect(mMockDevice.executeFastbootCommand(EasyMock.anyLong(),
                EasyMock.eq("-w"))).andReturn(result);

        EasyMock.replay(mMockDevice);
        mFlasher.handleUserDataFlashing(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test doing a user data with with rm
     *
     * @throws TargetSetupError
     */
    public void testFlashUserData_wipeRm() throws DeviceNotAvailableException, TargetSetupError {
        mFlasher.setUserDataFlashOption(UserDataFlashOption.WIPE_RM);

        ITestsZipInstaller mockZipInstaller = EasyMock.createMock(ITestsZipInstaller.class);
        mFlasher.setTestsZipInstaller(mockZipInstaller);
        // expect
        mockZipInstaller.deleteData(EasyMock.eq(mMockDevice));
        // expect
        mMockDevice.rebootUntilOnline();
        mMockDevice.rebootIntoBootloader();

        EasyMock.replay(mMockDevice, mockZipInstaller);
        mFlasher.flashUserData(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice, mockZipInstaller);
    }

    /**
     * Test that
     * {@link FastbootDeviceFlasher#downloadFlashingResources(ITestDevice, IDeviceBuildInfo)}
     * throws an exception when device product is null.
     */
    public void testDownloadFlashingResources_nullDeviceProduct() throws Exception {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);
        EasyMock.expect(mMockDevice.getProductType()).andReturn(null);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn(TEST_STRING);
        EasyMock.expect(mMockParser.getRequiredBoards()).andReturn(new ArrayList<String>());
        EasyMock.replay(mMockDevice, mMockParser);
        try {
            mFlasher.downloadFlashingResources(mMockDevice, mMockBuildInfo);
            fail("Should have thrown an exception");
        } catch (DeviceNotAvailableException expected) {
            assertEquals(String.format("Could not determine product type for device %s",
                    TEST_STRING), expected.getMessage());
        } finally {
            EasyMock.verify(mMockDevice, mMockParser);
        }
    }

    /**
     * Test that
     * {@link FastbootDeviceFlasher#downloadFlashingResources(ITestDevice, IDeviceBuildInfo)}
     * throws an exception when the device product is not found in required board.
     */
    public void testDownloadFlashingResources_NotFindBoard() throws Exception {
        final String boardName = "AWESOME_PRODUCT";
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);
        EasyMock.expect(mMockDevice.getProductType()).andReturn("NOT_FOUND");
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn(TEST_STRING);
        EasyMock.expect(mMockParser.getRequiredBoards()).andStubReturn(ArrayUtil.list(boardName));
        EasyMock.replay(mMockDevice, mMockParser);
        try {
            mFlasher.downloadFlashingResources(mMockDevice, mMockBuildInfo);
            fail("Should have thrown an exception");
        } catch (TargetSetupError expected) {
            assertEquals(String.format("Device %s is NOT_FOUND. Expected %s null",
                    TEST_STRING, ArrayUtil.list(boardName)), expected.getMessage());
        } finally {
            EasyMock.verify(mMockDevice, mMockParser);
        }
    }

    /**
     * Test that
     * {@link FastbootDeviceFlasher#downloadFlashingResources(ITestDevice, IDeviceBuildInfo)}
     * proceeds to the end without throwing.
     */
    public void testDownloadFlashingResources() throws Exception {
        final String boardName = "AWESOME_PRODUCT";
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);
        EasyMock.expect(mMockDevice.getProductType()).andReturn(boardName);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn(TEST_STRING);
        EasyMock.expect(mMockParser.getRequiredBoards()).andStubReturn(ArrayUtil.list(boardName));
        EasyMock.replay(mMockDevice, mMockParser);
        mFlasher.downloadFlashingResources(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice, mMockParser);
    }

    /**
     * Test that
     * {@link FastbootDeviceFlasher#checkAndFlashBootloader(ITestDevice, IDeviceBuildInfo)} returns
     * false because bootloader version is already good.
     */
    public void testCheckAndFlashBootloader_SkippingFlashing() throws Exception {
        final String version = "version 5";
        mFlasher = new FastbootDeviceFlasher() {
            @Override
            protected String getImageVersion(ITestDevice device, String imageName)
                    throws DeviceNotAvailableException, TargetSetupError {
                return version;
            }
        };
        IDeviceBuildInfo mockBuild = EasyMock.createMock(IDeviceBuildInfo.class);
        EasyMock.expect(mockBuild.getBootloaderVersion()).andStubReturn(version);
        EasyMock.replay(mMockDevice, mockBuild);
        assertFalse(mFlasher.checkAndFlashBootloader(mMockDevice, mockBuild));
        EasyMock.verify(mMockDevice, mockBuild);
    }

    /**
     * Test that
     * {@link FastbootDeviceFlasher#checkAndFlashBootloader(ITestDevice, IDeviceBuildInfo)} returns
     * true after flashing the device bootloader.
     */
    public void testCheckAndFlashBootloader() throws Exception {
        final String version = "version 5";
        mFlasher = new FastbootDeviceFlasher() {
            @Override
            protected String getImageVersion(ITestDevice device, String imageName)
                    throws DeviceNotAvailableException, TargetSetupError {
                return "version 6";
            }
        };
        IDeviceBuildInfo mockBuild = EasyMock.createMock(IDeviceBuildInfo.class);
        EasyMock.expect(mockBuild.getBootloaderVersion()).andStubReturn(version);
        File bootloaderFake = FileUtil.createTempFile("fakeBootloader", "");
        try {
            EasyMock.expect(mockBuild.getBootloaderImageFile()).andReturn(bootloaderFake);
            CommandResult res = new CommandResult(CommandStatus.SUCCESS);
            res.setStderr("flashing");
            EasyMock.expect(mMockDevice.executeFastbootCommand(EasyMock.eq("flash"),
                    EasyMock.eq("hboot"), EasyMock.eq(bootloaderFake.getAbsolutePath())))
                    .andReturn(res);
            mMockDevice.rebootIntoBootloader();
            EasyMock.expectLastCall();
            EasyMock.replay(mMockDevice, mockBuild);
            assertTrue(mFlasher.checkAndFlashBootloader(mMockDevice, mockBuild));
            EasyMock.verify(mMockDevice, mockBuild);
        } finally {
            FileUtil.deleteFile(bootloaderFake);
        }
    }

    /**
     * Test {@link FastbootDeviceFlasher#checkAndFlashSystem(ITestDevice,
     * String, String, IDeviceBuildInfo)} when there is no need to flash the system.
     */
    public void testCheckAndFlashSystem_noFlashing() throws Exception {
        final String buildId = "systemBuildId";
        final String buildFlavor = "systemBuildFlavor";
        IDeviceBuildInfo mockBuild = EasyMock.createMock(IDeviceBuildInfo.class);
        EasyMock.expect(mockBuild.getDeviceBuildId()).andReturn(buildId);
        EasyMock.expect(mockBuild.getBuildFlavor()).andReturn(buildFlavor);
        mMockDevice.rebootUntilOnline();
        EasyMock.expectLastCall();
        EasyMock.replay(mMockDevice, mockBuild);
        assertFalse(mFlasher.checkAndFlashSystem(mMockDevice, buildId, buildFlavor, mockBuild));
        EasyMock.verify(mMockDevice, mockBuild);
        assertNull("system flash status should be null when partitions are not flashed",
                mFlasher.getSystemFlashingStatus());
    }

    /**
     * Test {@link FastbootDeviceFlasher#checkAndFlashSystem(ITestDevice,
     * String, String, IDeviceBuildInfo)} when it needs to be flashed.
     */
    public void testCheckAndFlashSystem_flashing() throws Exception {
        final String buildId = "systemBuildId";
        IDeviceBuildInfo mockBuild = EasyMock.createMock(IDeviceBuildInfo.class);
        EasyMock.expect(mockBuild.getDeviceBuildId()).andReturn(buildId);
        File deviceImage = FileUtil.createTempFile("fakeDeviceImage", "");
        try {
            EasyMock.expect(mockBuild.getDeviceImageFile()).andStubReturn(deviceImage);
            CommandResult res = new CommandResult(CommandStatus.SUCCESS);
            res.setStderr("flashing");
            EasyMock.expect(mMockDevice.executeLongFastbootCommand(EasyMock.eq("update"),
                    EasyMock.eq(deviceImage.getAbsolutePath())))
                    .andReturn(res);
            EasyMock.replay(mMockDevice, mockBuild);
            assertTrue(mFlasher.checkAndFlashSystem(mMockDevice, buildId, null, mockBuild));
            EasyMock.verify(mMockDevice, mockBuild);
            assertEquals("system flashing status should be \"SUCCESS\"",
                    CommandStatus.SUCCESS, mFlasher.getSystemFlashingStatus());
        } finally {
            FileUtil.deleteFile(deviceImage);
        }
    }

    /**
     * Test {@link FastbootDeviceFlasher#checkAndFlashSystem(ITestDevice, String, String,
     * IDeviceBuildInfo)} with flash options
     */
    public void testCheckAndFlashSystem_withFlashOptions() throws Exception {
        mFlasher.setFlashOptions(Arrays.asList("--foo", " --bar"));
        final String buildId = "systemBuildId";
        IDeviceBuildInfo mockBuild = EasyMock.createMock(IDeviceBuildInfo.class);
        EasyMock.expect(mockBuild.getDeviceBuildId()).andReturn(buildId);
        File deviceImage = FileUtil.createTempFile("fakeDeviceImage", "");
        try {
            EasyMock.expect(mockBuild.getDeviceImageFile()).andStubReturn(deviceImage);
            CommandResult res = new CommandResult(CommandStatus.SUCCESS);
            res.setStderr("flashing");
            EasyMock.expect(
                            mMockDevice.executeLongFastbootCommand(
                                    EasyMock.eq("--foo"),
                                    EasyMock.eq("--bar"),
                                    EasyMock.eq("update"),
                                    EasyMock.eq(deviceImage.getAbsolutePath())))
                    .andReturn(res);
            EasyMock.replay(mMockDevice, mockBuild);
            assertTrue(mFlasher.checkAndFlashSystem(mMockDevice, buildId, null, mockBuild));
            EasyMock.verify(mMockDevice, mockBuild);
            assertEquals(
                    "system flashing status should be \"SUCCESS\"",
                    CommandStatus.SUCCESS,
                    mFlasher.getSystemFlashingStatus());
        } finally {
            FileUtil.deleteFile(deviceImage);
        }
    }

    /**
     * Test {@link FastbootDeviceFlasher#checkAndFlashSystem(ITestDevice, String, String,
     * IDeviceBuildInfo)} when it needs to be flashed but throws an exception.
     */
    public void testCheckAndFlashSystem_exception() throws Exception {
        final String buildId = "systemBuildId";
        IDeviceBuildInfo mockBuild = EasyMock.createMock(IDeviceBuildInfo.class);
        EasyMock.expect(mockBuild.getDeviceBuildId()).andReturn(buildId);
        File deviceImage = FileUtil.createTempFile("fakeDeviceImage", "");
        try {
            EasyMock.expect(mockBuild.getDeviceImageFile()).andStubReturn(deviceImage);
            CommandResult res = new CommandResult(CommandStatus.SUCCESS);
            res.setStderr("flashing");
            EasyMock.expect(mMockDevice.executeLongFastbootCommand(EasyMock.eq("update"),
                    EasyMock.eq(deviceImage.getAbsolutePath())))
                    .andThrow(new DeviceNotAvailableException());
            EasyMock.replay(mMockDevice, mockBuild);
            try {
                mFlasher.checkAndFlashSystem(mMockDevice, buildId, null, mockBuild);
                fail("Expected DeviceNotAvailableException not thrown");
            } catch (DeviceNotAvailableException dnae) {
                // expected
                EasyMock.verify(mMockDevice, mockBuild);
                assertEquals("system flashing status should be \"EXCEPTION\"",
                        CommandStatus.EXCEPTION, mFlasher.getSystemFlashingStatus());
            }
        } finally {
            FileUtil.deleteFile(deviceImage);
        }
    }

    /**
     * Set EasyMock expectations to simulate the response to some fastboot command
     *
     * @param mockDevice the EasyMock mock {@link ITestDevice} to configure
     * @param response the fastboot command response to inject
     */
    private static void setFastbootResponseExpectations(ITestDevice mockDevice, String response)
            throws DeviceNotAvailableException {
        CommandResult result = new CommandResult();
        result.setStatus(CommandStatus.SUCCESS);
        result.setStderr(response);
        result.setStdout("");
        EasyMock.expect(
                mockDevice.executeFastbootCommand((String)EasyMock.anyObject(),
                        (String)EasyMock.anyObject())).andReturn(result);
    }

    /**
     * Set EasyMock expectations to simulate the response to a fastboot flash command
     *
     * @param image the expected image name to flash
     * @param mockDevice the EasyMock mock {@link ITestDevice} to configure
     */
    private static void setFastbootFlashExpectations(ITestDevice mockDevice, String image)
            throws DeviceNotAvailableException {
        CommandResult result = new CommandResult();
        result.setStatus(CommandStatus.SUCCESS);
        result.setStderr("success");
        result.setStdout("");
        EasyMock.expect(
                mockDevice.executeLongFastbootCommand(EasyMock.eq("flash"), EasyMock.eq(image),
                        (String)EasyMock.anyObject())).andReturn(result);
    }

    private FastbootDeviceFlasher getFlasherWithParserData(final String androidInfoData) {
        FastbootDeviceFlasher flasher = new FastbootDeviceFlasher() {
            @Override
            protected IFlashingResourcesParser createFlashingResourcesParser(
                    IDeviceBuildInfo localBuild, DeviceDescriptor desc) throws TargetSetupError {
                BufferedReader reader = new BufferedReader(new StringReader(androidInfoData));
                try {
                    return new FlashingResourcesParser(reader);
                } catch (IOException e) {
                    return null;
                }
            }

            @Override
            protected void flashBootloader(ITestDevice device, File bootloaderImageFile)
                    throws DeviceNotAvailableException, TargetSetupError {
                throw new DeviceNotAvailableException("error", "fakeserial");
            }
        };
        flasher.setFlashingResourcesRetriever(EasyMock.createNiceMock(
                IFlashingResourcesRetriever.class));
        return flasher;
    }
}
