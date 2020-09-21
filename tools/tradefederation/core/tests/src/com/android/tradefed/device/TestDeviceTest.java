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

import com.android.ddmlib.AdbCommandRejectedException;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.IDevice.DeviceState;
import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.InstallReceiver;
import com.android.ddmlib.RawImage;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.TimeoutException;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice.MountPointInfo;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.host.HostOptions;
import com.android.tradefed.host.IHostOptions;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ITestLifeCycleReceiver;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.KeyguardControllerState;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.ZipUtil2;

import com.google.common.util.concurrent.SettableFuture;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.easymock.IAnswer;
import org.easymock.IExpectationSetters;
import org.junit.Assert;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for {@link TestDevice}.
 */
public class TestDeviceTest extends TestCase {

    private static final String MOCK_DEVICE_SERIAL = "serial";
    // For getCurrentUser, the min api should be 24. We make the stub return 23, the logic should
    // increment it by one.
    private static final int MIN_API_LEVEL_GET_CURRENT_USER = 23;
    private static final int MIN_API_LEVEL_STOP_USER = 22;
    private static final String RAWIMAGE_RESOURCE = "/testdata/rawImage.zip";
    private IDevice mMockIDevice;
    private IShellOutputReceiver mMockReceiver;
    private TestDevice mTestDevice;
    private TestDevice mRecoveryTestDevice;
    private TestDevice mNoFastbootTestDevice;
    private IDeviceRecovery mMockRecovery;
    private IDeviceStateMonitor mMockStateMonitor;
    private IRunUtil mMockRunUtil;
    private IWifiHelper mMockWifi;
    private IDeviceMonitor mMockDvcMonitor;

    /**
     * A {@link TestDevice} that is suitable for running tests against
     */
    private class TestableTestDevice extends TestDevice {
        public TestableTestDevice() {
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

        @Override
        void doReboot() throws DeviceNotAvailableException, UnsupportedOperationException {
        }

        @Override
        IHostOptions getHostOptions() {
            // Avoid issue with GlobalConfiguration
            return new HostOptions();
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
        mMockReceiver = EasyMock.createMock(IShellOutputReceiver.class);
        mMockRecovery = EasyMock.createMock(IDeviceRecovery.class);
        mMockStateMonitor = EasyMock.createMock(IDeviceStateMonitor.class);
        mMockDvcMonitor = EasyMock.createMock(IDeviceMonitor.class);
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mMockWifi = EasyMock.createMock(IWifiHelper.class);

        // A TestDevice with a no-op recoverDevice() implementation
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public void recoverDevice() throws DeviceNotAvailableException {
                        // ignore
                    }

                    @Override
                    IWifiHelper createWifiHelper() throws DeviceNotAvailableException {
                        return mMockWifi;
                    }
                };
        mTestDevice.setRecovery(mMockRecovery);
        mTestDevice.setCommandTimeout(100);
        mTestDevice.setLogStartDelay(-1);

        // TestDevice with intact recoverDevice()
        mRecoveryTestDevice = new TestableTestDevice();
        mRecoveryTestDevice.setRecovery(mMockRecovery);
        mRecoveryTestDevice.setCommandTimeout(100);
        mRecoveryTestDevice.setLogStartDelay(-1);

        // TestDevice without fastboot
        mNoFastbootTestDevice = new TestableTestDevice();
        mNoFastbootTestDevice.setFastbootEnabled(false);
        mNoFastbootTestDevice.setRecovery(mMockRecovery);
        mNoFastbootTestDevice.setCommandTimeout(100);
        mNoFastbootTestDevice.setLogStartDelay(-1);
    }

    /**
     * Test {@link TestDevice#enableAdbRoot()} when adb is already root
     */
    public void testEnableAdbRoot_alreadyRoot() throws Exception {
        injectShellResponse("id", "uid=0(root) gid=0(root)");
        EasyMock.replay(mMockIDevice);
        assertTrue(mTestDevice.enableAdbRoot());
    }

    /**
     * Test {@link TestDevice#enableAdbRoot()} when adb is not root
     */
    public void testEnableAdbRoot_notRoot() throws Exception {
        setEnableAdbRootExpectations();
        EasyMock.replay(mMockIDevice, mMockRunUtil, mMockStateMonitor);
        assertTrue(mTestDevice.enableAdbRoot());
    }

    /**
     * Test {@link TestDevice#enableAdbRoot()} when "enable-root" is "false"
     */
    public void testEnableAdbRoot_noEnableRoot() throws Exception {
        boolean enableRoot = mTestDevice.getOptions().isEnableAdbRoot();
        OptionSetter setter = new OptionSetter(mTestDevice.getOptions());
        setter.setOptionValue("enable-root", "false");
        try {
            assertFalse(mTestDevice.enableAdbRoot());
        } finally {
            setter.setOptionValue("enable-root", Boolean.toString(enableRoot));
        }
    }

    /**
     * Test {@link TestDevice#disableAdbRoot()} when adb is already unroot
     */
    public void testDisableAdbRoot_alreadyUnroot() throws Exception {
        injectShellResponse("id", "uid=2000(shell) gid=2000(shell) groups=2000(shell)");
        EasyMock.replay(mMockIDevice);
        assertTrue(mTestDevice.disableAdbRoot());
        EasyMock.verify(mMockIDevice);
    }

    /**
     * Test {@link TestDevice#disableAdbRoot()} when adb is root
     */
    public void testDisableAdbRoot_unroot() throws Exception {
        injectShellResponse("id", "uid=0(root) gid=0(root)");
        injectShellResponse("id", "uid=2000(shell) gid=2000(shell)");
        CommandResult adbResult = new CommandResult();
        adbResult.setStatus(CommandStatus.SUCCESS);
        adbResult.setStdout("restarting adbd as non root");
        setExecuteAdbCommandExpectations(adbResult, "unroot");
        EasyMock.expect(mMockStateMonitor.waitForDeviceNotAvailable(EasyMock.anyLong())).andReturn(
                Boolean.TRUE);
        EasyMock.expect(mMockStateMonitor.waitForDeviceOnline()).andReturn(
                mMockIDevice);
        EasyMock.replay(mMockIDevice, mMockRunUtil, mMockStateMonitor);
        assertTrue(mTestDevice.disableAdbRoot());
        EasyMock.verify(mMockIDevice, mMockRunUtil, mMockStateMonitor);
    }

    /**
     * Configure EasyMock expectations for a successful adb root call
     */
    private void setEnableAdbRootExpectations() throws Exception {
        injectShellResponse("id", "uid=2000(shell) gid=2000(shell)");
        injectShellResponse("id", "uid=0(root) gid=0(root)");
        CommandResult adbResult = new CommandResult();
        adbResult.setStatus(CommandStatus.SUCCESS);
        adbResult.setStdout("restarting adbd as root");
        setExecuteAdbCommandExpectations(adbResult, "root");
        EasyMock.expect(mMockStateMonitor.waitForDeviceNotAvailable(EasyMock.anyLong())).andReturn(
                Boolean.TRUE);
        EasyMock.expect(mMockStateMonitor.waitForDeviceOnline()).andReturn(
                mMockIDevice);
    }

    /**
     * COnfigure EasMock expectations for a successful adb command call
     * @param command the adb command to execute
     * @param result the {@link CommandResult} expected from the adb command execution
     * @throws Exception
     */
    private void setExecuteAdbCommandExpectations(CommandResult result, String command)
            throws Exception {
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(),
                EasyMock.eq("adb"), EasyMock.eq("-s"), EasyMock.eq(MOCK_DEVICE_SERIAL),
                EasyMock.eq(command))).andReturn(result);
    }

    /**
     * Test that {@link TestDevice#enableAdbRoot()} reattempts adb root
     */
    public void testEnableAdbRoot_rootRetry() throws Exception {
        injectShellResponse("id", "uid=2000(shell) gid=2000(shell)");
        injectShellResponse("id", "uid=2000(shell) gid=2000(shell)");
        injectShellResponse("id", "uid=0(root) gid=0(root)");
        CommandResult adbBadResult = new CommandResult(CommandStatus.SUCCESS);
        adbBadResult.setStdout("");
        setExecuteAdbCommandExpectations(adbBadResult, "root");
        CommandResult adbResult = new CommandResult(CommandStatus.SUCCESS);
        adbResult.setStdout("restarting adbd as root");
        setExecuteAdbCommandExpectations(adbResult, "root");
        EasyMock.expect(mMockStateMonitor.waitForDeviceNotAvailable(EasyMock.anyLong())).andReturn(
                Boolean.TRUE).times(2);
        EasyMock.expect(mMockStateMonitor.waitForDeviceOnline()).andReturn(
                mMockIDevice).times(2);
        EasyMock.replay(mMockIDevice, mMockRunUtil, mMockStateMonitor);
        assertTrue(mTestDevice.enableAdbRoot());
    }

    /**
     * Test that {@link TestDevice#isAdbRoot()} for device without adb root.
     */
    public void testIsAdbRootForNonRoot() throws Exception {
        injectShellResponse("id", "uid=2000(shell) gid=2000(shell)");
        EasyMock.replay(mMockIDevice);
        assertFalse(mTestDevice.isAdbRoot());
    }

    /**
     * Test that {@link TestDevice#isAdbRoot()} for device with adb root.
     */
    public void testIsAdbRootForRoot() throws Exception {
        injectShellResponse("id", "uid=0(root) gid=0(root)");
        EasyMock.replay(mMockIDevice);
        assertTrue(mTestDevice.isAdbRoot());
    }

    /**
     * Test {@link TestDevice#getProductType()} when device is in fastboot and IDevice has not
     * cached product type property
     */
    public void testGetProductType_fastboot() throws DeviceNotAvailableException {
        EasyMock.expect(mMockIDevice.getProperty(EasyMock.<String>anyObject())).andReturn(null);
        CommandResult fastbootResult = new CommandResult();
        fastbootResult.setStatus(CommandStatus.SUCCESS);
        // output of this cmd goes to stderr
        fastbootResult.setStdout("");
        fastbootResult.setStderr("product: nexusone\n" + "finished. total time: 0.001s");
        EasyMock.expect(
                mMockRunUtil.runTimedCmd(EasyMock.anyLong(), (String)EasyMock.anyObject(),
                        (String)EasyMock.anyObject(), (String)EasyMock.anyObject(),
                        (String)EasyMock.anyObject(), (String)EasyMock.anyObject())).andReturn(
                fastbootResult);
        EasyMock.replay(mMockIDevice, mMockRunUtil);
        mRecoveryTestDevice.setDeviceState(TestDeviceState.FASTBOOT);
        assertEquals("nexusone", mRecoveryTestDevice.getProductType());
    }

    /**
     * Test {@link TestDevice#getProductType()} for a device with a non-alphanumeric fastboot
     * product type
     */
    public void testGetProductType_fastbootNonalpha() throws DeviceNotAvailableException {
        EasyMock.expect(mMockIDevice.getProperty(EasyMock.<String>anyObject())).andReturn(null);
        CommandResult fastbootResult = new CommandResult();
        fastbootResult.setStatus(CommandStatus.SUCCESS);
        // output of this cmd goes to stderr
        fastbootResult.setStdout("");
        fastbootResult.setStderr("product: foo-bar\n" + "finished. total time: 0.001s");
        EasyMock.expect(
                mMockRunUtil.runTimedCmd(EasyMock.anyLong(), (String)EasyMock.anyObject(),
                        (String)EasyMock.anyObject(), (String)EasyMock.anyObject(),
                        (String)EasyMock.anyObject(), (String)EasyMock.anyObject())).andReturn(
                fastbootResult);
        EasyMock.replay(mMockIDevice, mMockRunUtil);
        mRecoveryTestDevice.setDeviceState(TestDeviceState.FASTBOOT);
        assertEquals("foo-bar", mRecoveryTestDevice.getProductType());
    }

    /**
     * Verify that {@link TestDevice#getProductType()} throws an exception if requesting a product
     * type directly fails while the device is in fastboot.
     */
    public void testGetProductType_fastbootFail() {
        EasyMock.expect(mMockIDevice.getProperty(EasyMock.<String>anyObject())).andStubReturn(null);
        CommandResult fastbootResult = new CommandResult();
        fastbootResult.setStatus(CommandStatus.SUCCESS);
        // output of this cmd goes to stderr
        fastbootResult.setStdout("");
        fastbootResult.setStderr("product: \n" + "finished. total time: 0.001s");
        EasyMock.expect(
                mMockRunUtil.runTimedCmd(EasyMock.anyLong(), (String)EasyMock.anyObject(),
                        (String)EasyMock.anyObject(), (String)EasyMock.anyObject(),
                        (String)EasyMock.anyObject(), (String)EasyMock.anyObject())).andReturn(
                fastbootResult).anyTimes();
        EasyMock.replay(mMockIDevice);
        EasyMock.replay(mMockRunUtil);
        mTestDevice.setDeviceState(TestDeviceState.FASTBOOT);
        try {
            String type = mTestDevice.getProductType();
            fail(String.format("DeviceNotAvailableException not thrown; productType was '%s'",
                    type));
        } catch (DeviceNotAvailableException e) {
            // expected
        }
    }

    /**
     * Test {@link TestDevice#getProductType()} when device is in adb and IDevice has not cached
     * product type property
     */
    public void testGetProductType_adb() throws Exception {
        EasyMock.expect(mMockIDevice.getProperty(DeviceProperties.BOARD)).andReturn(null);
        final String expectedOutput = "nexusone";
        injectSystemProperty(DeviceProperties.BOARD, expectedOutput);
        EasyMock.replay(mMockIDevice);
        assertEquals(expectedOutput, mTestDevice.getProductType());
    }

    /**
     * Verify that {@link TestDevice#getProductType()} throws an exception if requesting a product
     * type directly still fails.
     */
    public void testGetProductType_adbFail() throws Exception {
        EasyMock.expect(mMockIDevice.getProperty(EasyMock.<String>anyObject())).andStubReturn(null);
        injectSystemProperty(DeviceProperties.BOARD, null).times(3);
        EasyMock.expect(mMockIDevice.getState()).andReturn(DeviceState.ONLINE).times(2);
        EasyMock.replay(mMockIDevice);
        try {
            mTestDevice.getProductType();
            fail("DeviceNotAvailableException not thrown");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
    }

    /**
     * Test {@link TestDevice#clearErrorDialogs()} when both a error and anr dialog are present.
     */
    public void testClearErrorDialogs() throws Exception {
        final String anrOutput = "debugging=false crashing=false null notResponding=true "
                + "com.android.server.am.AppNotRespondingDialog@4534aaa0 bad=false\n blah\n";
        final String crashOutput = "debugging=false crashing=true "
                + "com.android.server.am.AppErrorDialog@45388a60 notResponding=false null bad=false"
                + "blah \n";
        // construct a string with 2 error dialogs of each type to ensure proper detection
        final String fourErrors = anrOutput + anrOutput + crashOutput + crashOutput;
        injectShellResponse(null, fourErrors);
        mMockIDevice.executeShellCommand((String)EasyMock.anyObject(),
                (IShellOutputReceiver)EasyMock.anyObject(),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        // expect 4 key events to be sent - one for each dialog
        // and expect another dialog query - but return nothing
        EasyMock.expectLastCall().times(5);

        EasyMock.replay(mMockIDevice);
        mTestDevice.clearErrorDialogs();
    }

    /**
     * Test that the unresponsive device exception is propagated from the recovery to TestDevice.
     * @throws Exception
     */
    public void testRecoverDevice_ThrowException() throws Exception {
        TestDevice testDevice = new TestDevice(mMockIDevice, mMockStateMonitor, mMockDvcMonitor) {
            @Override
            public boolean enableAdbRoot() throws DeviceNotAvailableException {
                return true;
            }
        };
        testDevice.setRecovery(new IDeviceRecovery() {

            @Override
            public void recoverDeviceRecovery(IDeviceStateMonitor monitor)
                    throws DeviceNotAvailableException {
                throw new DeviceNotAvailableException();
            }

            @Override
            public void recoverDeviceBootloader(IDeviceStateMonitor monitor)
                    throws DeviceNotAvailableException {
                throw new DeviceNotAvailableException();
            }

            @Override
            public void recoverDevice(IDeviceStateMonitor monitor, boolean recoverUntilOnline)
                    throws DeviceNotAvailableException {
                throw new DeviceUnresponsiveException();
            }
        });
        testDevice.setRecoveryMode(RecoveryMode.AVAILABLE);
        mMockIDevice.executeShellCommand((String) EasyMock.anyObject(),
                (CollectingOutputReceiver)EasyMock.anyObject(), EasyMock.anyLong(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expectLastCall();
        EasyMock.replay(mMockIDevice);
        try {
            testDevice.recoverDevice();
        } catch (DeviceNotAvailableException dnae) {
            assertTrue(dnae instanceof DeviceUnresponsiveException);
            return;
        }
        fail();
    }

    /**
     * Simple normal case test for
     * {@link TestDevice#executeShellCommand(String, IShellOutputReceiver)}.
     * <p/>
     * Verify that the shell command is routed to the IDevice.
     */
    public void testExecuteShellCommand_receiver() throws IOException, DeviceNotAvailableException,
            TimeoutException, AdbCommandRejectedException, ShellCommandUnresponsiveException {
        final String testCommand = "simple command";
        // expect shell command to be called
        mMockIDevice.executeShellCommand(EasyMock.eq(testCommand), EasyMock.eq(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        EasyMock.replay(mMockIDevice);
        mTestDevice.executeShellCommand(testCommand, mMockReceiver);
    }

    /**
     * Simple normal case test for
     * {@link TestDevice#executeShellCommand(String)}.
     * <p/>
     * Verify that the shell command is routed to the IDevice, and shell output is collected.
     */
    public void testExecuteShellCommand() throws Exception {
        final String testCommand = "simple command";
        final String expectedOutput = "this is the output\r\n in two lines\r\n";
        injectShellResponse(testCommand, expectedOutput);
        EasyMock.replay(mMockIDevice);
        assertEquals(expectedOutput, mTestDevice.executeShellCommand(testCommand));
    }

    /**
     * Test {@link TestDevice#executeShellCommand(String, IShellOutputReceiver)} behavior when
     * {@link IDevice} throws IOException and recovery immediately fails.
     * <p/>
     * Verify that a DeviceNotAvailableException is thrown.
     */
    public void testExecuteShellCommand_recoveryFail() throws Exception {
        final String testCommand = "simple command";
        // expect shell command to be called
        mMockIDevice.executeShellCommand(EasyMock.eq(testCommand), EasyMock.eq(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        EasyMock.expectLastCall().andThrow(new IOException());
        mMockRecovery.recoverDevice(EasyMock.eq(mMockStateMonitor), EasyMock.eq(false));
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());
        EasyMock.replay(mMockIDevice);
        EasyMock.replay(mMockRecovery);
        try {
            mRecoveryTestDevice.executeShellCommand(testCommand, mMockReceiver);
            fail("DeviceNotAvailableException not thrown");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
    }

    /**
     * Test {@link TestDevice#executeShellCommand(String, IShellOutputReceiver)} behavior when
     * {@link IDevice} throws IOException and device is in recovery until online mode.
     * <p/>
     * Verify that a DeviceNotAvailableException is thrown.
     */
    public void testExecuteShellCommand_recoveryUntilOnline() throws Exception {
        final String testCommand = "simple command";
        // expect shell command to be called
        mRecoveryTestDevice.setRecoveryMode(RecoveryMode.ONLINE);
        mMockIDevice.executeShellCommand(EasyMock.eq(testCommand), EasyMock.eq(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        EasyMock.expectLastCall().andThrow(new IOException());
        mMockRecovery.recoverDevice(EasyMock.eq(mMockStateMonitor), EasyMock.eq(true));
        setEnableAdbRootExpectations();
        mMockIDevice.executeShellCommand(EasyMock.eq(testCommand), EasyMock.eq(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        EasyMock.replay(mMockIDevice, mMockRecovery, mMockRunUtil, mMockStateMonitor);
        mRecoveryTestDevice.executeShellCommand(testCommand, mMockReceiver);
    }

    /**
     * Test {@link TestDevice#executeShellCommand(String, IShellOutputReceiver)} behavior when
     * {@link IDevice} throws IOException and recovery succeeds.
     * <p/>
     * Verify that command is re-tried.
     */
    public void testExecuteShellCommand_recoveryRetry() throws Exception {
        mTestDevice =
                new TestDevice(mMockIDevice, mMockStateMonitor, mMockDvcMonitor) {
                    @Override
                    IWifiHelper createWifiHelper() {
                        return mMockWifi;
                    }

                    @Override
                    public boolean isEncryptionSupported() throws DeviceNotAvailableException {
                        return false;
                    }

                    @Override
                    public boolean isAdbRoot() throws DeviceNotAvailableException {
                        return true;
                    }

                    @Override
                    protected IRunUtil getRunUtil() {
                        return mMockRunUtil;
                    }

                    @Override
                    void doReboot()
                            throws DeviceNotAvailableException, UnsupportedOperationException {}
                };
        mTestDevice.setRecovery(mMockRecovery);
        final String testCommand = "simple command";
        // expect shell command to be called
        mMockIDevice.executeShellCommand(EasyMock.eq(testCommand), EasyMock.eq(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        EasyMock.expectLastCall().andThrow(new IOException());
        assertRecoverySuccess();
        mMockIDevice.executeShellCommand(EasyMock.eq(testCommand), EasyMock.eq(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        EasyMock.expect(mMockStateMonitor.waitForDeviceOnline()).andReturn(mMockIDevice);
        injectSystemProperty("ro.build.version.sdk", "23");
        replayMocks();
        mTestDevice.executeShellCommand(testCommand, mMockReceiver);
        verifyMocks();
    }

    /** Set expectations for a successful recovery operation
     */
    private void assertRecoverySuccess() throws DeviceNotAvailableException, IOException,
            TimeoutException, AdbCommandRejectedException, ShellCommandUnresponsiveException {
        mMockRecovery.recoverDevice(EasyMock.eq(mMockStateMonitor), EasyMock.eq(false));
        mMockIDevice.executeShellCommand(
                EasyMock.eq("dumpsys input"),
                EasyMock.anyObject(),
                EasyMock.anyLong(),
                EasyMock.eq(TimeUnit.MILLISECONDS));
        // expect post boot up steps
        mMockIDevice.executeShellCommand(
                EasyMock.eq(TestDevice.DISMISS_KEYGUARD_WM_CMD),
                (IShellOutputReceiver) EasyMock.anyObject(),
                EasyMock.anyLong(),
                (TimeUnit) EasyMock.anyObject());
    }

    /**
     * Test {@link TestDevice#executeShellCommand(String, IShellOutputReceiver)} behavior when
     * command times out and recovery succeeds.
     * <p/>
     * Verify that command is re-tried.
     */
    public void testExecuteShellCommand_recoveryTimeoutRetry() throws Exception {
        mTestDevice =
                new TestDevice(mMockIDevice, mMockStateMonitor, mMockDvcMonitor) {
                    @Override
                    IWifiHelper createWifiHelper() {
                        return mMockWifi;
                    }

                    @Override
                    public boolean isEncryptionSupported() throws DeviceNotAvailableException {
                        return false;
                    }

                    @Override
                    public boolean isAdbRoot() throws DeviceNotAvailableException {
                        return true;
                    }

                    @Override
                    protected IRunUtil getRunUtil() {
                        return mMockRunUtil;
                    }

                    @Override
                    void doReboot()
                            throws DeviceNotAvailableException, UnsupportedOperationException {}
                };
        mTestDevice.setRecovery(mMockRecovery);
        final String testCommand = "simple command";
        // expect shell command to be called - and never return from that call
        mMockIDevice.executeShellCommand(EasyMock.eq(testCommand), EasyMock.eq(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        EasyMock.expectLastCall().andThrow(new TimeoutException());
        assertRecoverySuccess();
        // now expect shellCommand to be executed again, and succeed
        mMockIDevice.executeShellCommand(EasyMock.eq(testCommand), EasyMock.eq(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        EasyMock.expect(mMockStateMonitor.waitForDeviceOnline()).andReturn(mMockIDevice);
        injectSystemProperty("ro.build.version.sdk", "23");
        replayMocks();
        mTestDevice.executeShellCommand(testCommand, mMockReceiver);
        verifyMocks();
    }

    /**
     * Test {@link TestDevice#executeShellCommand(String, IShellOutputReceiver)} behavior when
     * {@link IDevice} repeatedly throws IOException and recovery succeeds.
     * <p/>
     * Verify that DeviceNotAvailableException is thrown.
     */
    public void testExecuteShellCommand_recoveryAttempts() throws Exception {
        mTestDevice =
                new TestDevice(mMockIDevice, mMockStateMonitor, mMockDvcMonitor) {
                    @Override
                    IWifiHelper createWifiHelper() {
                        return mMockWifi;
                    }

                    @Override
                    public boolean isEncryptionSupported() throws DeviceNotAvailableException {
                        return false;
                    }

                    @Override
                    public boolean isAdbRoot() throws DeviceNotAvailableException {
                        return true;
                    }

                    @Override
                    protected IRunUtil getRunUtil() {
                        return mMockRunUtil;
                    }

                    @Override
                    void doReboot()
                            throws DeviceNotAvailableException, UnsupportedOperationException {}
                };
        mTestDevice.setRecovery(mMockRecovery);
        final String testCommand = "simple command";
        // expect shell command to be called
        mMockIDevice.executeShellCommand(EasyMock.eq(testCommand), EasyMock.eq(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        EasyMock.expectLastCall().andThrow(new IOException()).times(
                TestDevice.MAX_RETRY_ATTEMPTS+1);
        for (int i=0; i <= TestDevice.MAX_RETRY_ATTEMPTS; i++) {
            assertRecoverySuccess();
        }
        EasyMock.expect(mMockIDevice.getState()).andReturn(DeviceState.ONLINE).times(2);
        EasyMock.expect(mMockStateMonitor.waitForDeviceOnline()).andReturn(mMockIDevice).times(3);
        injectSystemProperty("ro.build.version.sdk", "23").times(3);
        replayMocks();
        try {
            mTestDevice.executeShellCommand(testCommand, mMockReceiver);
            fail("DeviceUnresponsiveException not thrown");
        } catch (DeviceUnresponsiveException e) {
            // expected
        }
        verifyMocks();
    }

    /**
     * Puts all the mock objects into replay mode
     */
    private void replayMocks() {
        EasyMock.replay(mMockIDevice, mMockRecovery, mMockStateMonitor, mMockRunUtil, mMockWifi);
    }

    /**
     * Verify all the mock objects
     */
    private void verifyMocks() {
        EasyMock.verify(mMockIDevice, mMockRecovery, mMockStateMonitor, mMockRunUtil, mMockWifi);
    }


    /**
     * Unit test for {@link TestDevice#getExternalStoreFreeSpace()}.
     * <p/>
     * Verify that output of 'adb shell df' command is parsed correctly.
     */
    public void testGetExternalStoreFreeSpace() throws Exception {
        final String dfOutput =
            "/mnt/sdcard: 3864064K total, 1282880K used, 2581184K available (block size 32768)";
        assertGetExternalStoreFreeSpace(dfOutput, 2581184);
    }

    /**
     * Unit test for {@link TestDevice#getExternalStoreFreeSpace()}.
     * <p/>
     * Verify that the table-based output of 'adb shell df' command is parsed correctly.
     */
    public void testGetExternalStoreFreeSpace_table() throws Exception {
        final String dfOutput =
            "Filesystem             Size   Used   Free   Blksize\n" +
            "/mnt/sdcard              3G   787M     2G   4096";
        assertGetExternalStoreFreeSpace(dfOutput, 2 * 1024 * 1024);
    }

    /**
     * Unit test for {@link TestDevice#getExternalStoreFreeSpace()}.
     * <p/>
     * Verify that the coreutils-like output of 'adb shell df' command is parsed correctly.
     */
    public void testGetExternalStoreFreeSpace_toybox() throws Exception {
        final String dfOutput =
            "Filesystem      1K-blocks	Used  Available Use% Mounted on\n" +
            "/dev/fuse        11585536    1316348   10269188  12% /mnt/sdcard";
        assertGetExternalStoreFreeSpace(dfOutput, 10269188);
    }

    /**
     * Unit test for {@link TestDevice#getExternalStoreFreeSpace()}.
     * <p/>
     * Verify that the coreutils-like output of 'adb shell df' command is parsed correctly. This
     * variant tests the fact that the returned mount point in last column of command output may
     * not match the original path provided as parameter to df.
     */
    public void testGetExternalStoreFreeSpace_toybox2() throws Exception {
        final String dfOutput =
            "Filesystem     1K-blocks   Used Available Use% Mounted on\n" +
            "/dev/fuse       27240188 988872  26251316   4% /storage/emulated";
        assertGetExternalStoreFreeSpace(dfOutput, 26251316);
    }

    /**
     * Unit test for {@link TestDevice#getExternalStoreFreeSpace()}.
     * <p/>
     * Verify behavior when 'df' command returns unexpected content
     */
    public void testGetExternalStoreFreeSpace_badOutput() throws Exception {
        final String dfOutput =
            "/mnt/sdcard: blaH";
        assertGetExternalStoreFreeSpace(dfOutput, 0);
    }

    /**
     * Unit test for {@link TestDevice#getExternalStoreFreeSpace()}.
     * <p/>
     * Verify behavior when first 'df' attempt returns empty output
     */
    public void testGetExternalStoreFreeSpace_emptyOutput() throws Exception {
        final String mntPoint = "/mnt/sdcard";
        final String expectedCmd = "df " + mntPoint;
        EasyMock.expect(mMockStateMonitor.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE)).andReturn(
                mntPoint);
        // expect shell command to be called, and return the empty df output
        injectShellResponse(expectedCmd, "");
        final String dfOutput =
                "/mnt/sdcard: 3864064K total, 1282880K used, 2581184K available (block size 32768)";
        injectShellResponse(expectedCmd, dfOutput);
        EasyMock.replay(mMockIDevice, mMockStateMonitor);
        assertEquals(2581184, mTestDevice.getExternalStoreFreeSpace());
    }

    /**
     * Helper method to verify the {@link TestDevice#getExternalStoreFreeSpace()} method under
     * different conditions.
     *
     * @param dfOutput the test output to inject
     * @param expectedFreeSpaceKB the expected free space
     */
    private void assertGetExternalStoreFreeSpace(final String dfOutput, long expectedFreeSpaceKB)
            throws Exception {
        final String mntPoint = "/mnt/sdcard";
        final String expectedCmd = "df " + mntPoint;
        EasyMock.expect(mMockStateMonitor.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE)).andReturn(
                mntPoint);
        // expect shell command to be called, and return the test df output
        injectShellResponse(expectedCmd, dfOutput);
        EasyMock.replay(mMockIDevice, mMockStateMonitor);
        assertEquals(expectedFreeSpaceKB, mTestDevice.getExternalStoreFreeSpace());
    }

    /**
     * Unit test for {@link TestDevice#syncFiles(File, String)}.
     * <p/>
     * Verify behavior when given local file does not exist
     */
    public void testSyncFiles_missingLocal() throws Exception {
        EasyMock.replay(mMockIDevice);
        assertFalse(mTestDevice.syncFiles(new File("idontexist"), "/sdcard"));
    }

    /**
     * Test {@link TestDevice#runInstrumentationTests(IRemoteAndroidTestRunner, Collection)}
     * success case.
     */
    public void testRunInstrumentationTests() throws Exception {
        IRemoteAndroidTestRunner mockRunner = EasyMock.createMock(IRemoteAndroidTestRunner.class);
        EasyMock.expect(mockRunner.getPackageName()).andStubReturn("com.example");
        Collection<ITestLifeCycleReceiver> listeners = new ArrayList<>(0);
        mockRunner.setMaxTimeToOutputResponse(EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        // expect runner.run command to be called
        Collection<ITestRunListener> expectedMock = EasyMock.anyObject();
        mockRunner.run(expectedMock);
        EasyMock.replay(mockRunner);
        mTestDevice.runInstrumentationTests(mockRunner, listeners);
    }

    /**
     * Test {@link TestDevice#runInstrumentationTests(IRemoteAndroidTestRunner, Collection)}
     * when recovery fails.
     */
    public void testRunInstrumentationTests_recoveryFails() throws Exception {
        IRemoteAndroidTestRunner mockRunner = EasyMock.createMock(IRemoteAndroidTestRunner.class);
        Collection<ITestLifeCycleReceiver> listeners = new ArrayList<>(1);
        ITestLifeCycleReceiver listener = EasyMock.createMock(ITestLifeCycleReceiver.class);
        listeners.add(listener);
        mockRunner.setMaxTimeToOutputResponse(EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        Collection<ITestRunListener> expectedMock = EasyMock.anyObject();
        mockRunner.run(expectedMock);
        EasyMock.expectLastCall().andThrow(new IOException());
        EasyMock.expect(mockRunner.getPackageName()).andReturn("foo");
        listener.testRunFailed((String)EasyMock.anyObject());
        mMockRecovery.recoverDevice(EasyMock.eq(mMockStateMonitor), EasyMock.eq(false));
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());
        EasyMock.replay(listener, mockRunner, mMockIDevice, mMockRecovery);
        try {
            mRecoveryTestDevice.runInstrumentationTests(mockRunner, listeners);
            fail("DeviceNotAvailableException not thrown");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
    }

    /**
     * Test {@link TestDevice#runInstrumentationTests(IRemoteAndroidTestRunner, Collection)}
     * when recovery succeeds.
     */
    public void testRunInstrumentationTests_recoverySucceeds() throws Exception {
        IRemoteAndroidTestRunner mockRunner = EasyMock.createMock(IRemoteAndroidTestRunner.class);
        Collection<ITestLifeCycleReceiver> listeners = new ArrayList<>(1);
        ITestLifeCycleReceiver listener = EasyMock.createMock(ITestLifeCycleReceiver.class);
        listeners.add(listener);
        mockRunner.setMaxTimeToOutputResponse(EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject());
        Collection<ITestRunListener> expectedMock = EasyMock.anyObject();
        mockRunner.run(expectedMock);
        EasyMock.expectLastCall().andThrow(new IOException());
        EasyMock.expect(mockRunner.getPackageName()).andReturn("foo");
        listener.testRunFailed((String)EasyMock.anyObject());
        assertRecoverySuccess();
        EasyMock.replay(listener, mockRunner, mMockIDevice, mMockRecovery);
        mTestDevice.runInstrumentationTests(mockRunner, listeners);
    }

    /**
     * Test {@link TestDevice#executeFastbootCommand(String...)} throws an exception when fastboot
     * is not available.
     */
    public void testExecuteFastbootCommand_nofastboot() throws Exception {
        try {
            mNoFastbootTestDevice.executeFastbootCommand("");
            fail("UnsupportedOperationException not thrown");
        } catch (UnsupportedOperationException e) {
            // expected
        }
    }

    /**
     * Test {@link TestDevice#executeLongFastbootCommand(String...)} throws an exception when
     * fastboot is not available.
     */
    public void testExecuteLongFastbootCommand_nofastboot() throws Exception {
        try {
            mNoFastbootTestDevice.executeFastbootCommand("");
            fail("UnsupportedOperationException not thrown");
        } catch (UnsupportedOperationException e) {
            // expected
        }
    }

    /**
     * Test that state changes are ignore while {@link TestDevice#executeFastbootCommand(String...)}
     * is active.
     */
    public void testExecuteFastbootCommand_state() throws Exception {
        final long waitTimeMs = 150;
        // build a fastboot response that will block
        IAnswer<CommandResult> blockResult = new IAnswer<CommandResult>() {
            @Override
            public CommandResult answer() throws Throwable {
                synchronized(this) {
                    // first inform this test that fastboot cmd is executing
                    notifyAll();
                    // now wait for test to unblock us when its done testing logic
                    wait(waitTimeMs);
                }
                return new CommandResult(CommandStatus.SUCCESS);
            }
        };
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), EasyMock.eq("fastboot"),
                EasyMock.eq("-s"),EasyMock.eq(MOCK_DEVICE_SERIAL), EasyMock.eq("foo"))).andAnswer(
                        blockResult).times(2);

        // expect
        mMockStateMonitor.setState(TestDeviceState.FASTBOOT);
        mMockStateMonitor.setState(TestDeviceState.NOT_AVAILABLE);
        mMockRecovery.recoverDeviceBootloader((IDeviceStateMonitor)EasyMock.anyObject());
        EasyMock.expectLastCall().times(2);
        replayMocks();

        mTestDevice.setDeviceState(TestDeviceState.FASTBOOT);
        assertEquals(TestDeviceState.FASTBOOT, mTestDevice.getDeviceState());

        // start fastboot command in background thread
        Thread fastbootThread = new Thread() {
            @Override
            public void run() {
                try {
                    mTestDevice.executeFastbootCommand("foo");
                } catch (DeviceNotAvailableException e) {
                    CLog.e(e);
                }
            }
        };
        fastbootThread.setName(getClass().getCanonicalName() + "#testExecuteFastbootCommand_state");
        fastbootThread.start();
        try {
            synchronized (blockResult) {
                blockResult.wait(waitTimeMs);
            }
            // expect to ignore this
            mTestDevice.setDeviceState(TestDeviceState.NOT_AVAILABLE);
            assertEquals(TestDeviceState.FASTBOOT, mTestDevice.getDeviceState());
        } finally {
            synchronized (blockResult) {
                blockResult.notifyAll();
            }
        }
        fastbootThread.join();
        mTestDevice.setDeviceState(TestDeviceState.NOT_AVAILABLE);
        assertEquals(TestDeviceState.NOT_AVAILABLE, mTestDevice.getDeviceState());
        verifyMocks();
    }

    /**
     * Test recovery mode is entered when fastboot command fails
     */
    public void testExecuteFastbootCommand_recovery() throws UnsupportedOperationException,
           DeviceNotAvailableException {
        CommandResult result = new CommandResult(CommandStatus.EXCEPTION);
        EasyMock.expect(mMockRunUtil.runTimedCmd(
                EasyMock.anyLong(), EasyMock.eq("fastboot"), EasyMock.eq("-s"),
                EasyMock.eq(MOCK_DEVICE_SERIAL), EasyMock.eq("foo"))).andReturn(result);
        mMockRecovery.recoverDeviceBootloader((IDeviceStateMonitor)EasyMock.anyObject());
        CommandResult successResult = new CommandResult(CommandStatus.SUCCESS);
        successResult.setStderr("");
        successResult.setStdout("");
        // now expect a successful retry
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), EasyMock.eq("fastboot"),
                EasyMock.eq("-s"),EasyMock.eq(MOCK_DEVICE_SERIAL), EasyMock.eq("foo"))).andReturn(
                        successResult);
        replayMocks();
        mTestDevice.executeFastbootCommand("foo");
        verifyMocks();

    }

    /**
     * Basic test for encryption if encryption is not supported.
     * <p>
     * Calls {@link TestDevice#encryptDevice(boolean)}, {@link TestDevice#unlockDevice()}, and
     * {@link TestDevice#unencryptDevice()} and makes sure that a
     * {@link UnsupportedOperationException} is thrown for each method.
     * </p>
     */
    public void testEncryptionUnsupported() throws Exception {
        setEncryptedUnsupportedExpectations();
        setEncryptedUnsupportedExpectations();
        setEncryptedUnsupportedExpectations();
        EasyMock.replay(mMockIDevice, mMockRunUtil, mMockStateMonitor);

        try {
            mTestDevice.encryptDevice(false);
            fail("encryptUserData() did not throw UnsupportedOperationException");
        } catch (UnsupportedOperationException e) {
            // Expected
        }
        try {
            mTestDevice.unlockDevice();
            fail("decryptUserData() did not throw UnsupportedOperationException");
        } catch (UnsupportedOperationException e) {
            // Expected
        }
        try {
            mTestDevice.unencryptDevice();
            fail("unencryptUserData() did not throw UnsupportedOperationException");
        } catch (UnsupportedOperationException e) {
            // Expected
        }
        return;
    }

    /**
     * Unit test for {@link TestDevice#encryptDevice(boolean)}.
     */
    public void testEncryptDevice_alreadyEncrypted() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public boolean isDeviceEncrypted() throws DeviceNotAvailableException {
                return true;
            }
        };
        setEncryptedSupported();
        EasyMock.replay(mMockIDevice, mMockRunUtil, mMockStateMonitor);
        assertTrue(mTestDevice.encryptDevice(false));
        EasyMock.verify(mMockIDevice, mMockRunUtil, mMockStateMonitor);
    }

    /**
     * Unit test for {@link TestDevice#encryptDevice(boolean)}.
     */
    public void testEncryptDevice_encryptionFails() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public boolean isDeviceEncrypted() throws DeviceNotAvailableException {
                return false;
            }
        };
        setEncryptedSupported();
        setEnableAdbRootExpectations();
        injectShellResponse("vdc cryptfs enablecrypto wipe \"android\"",
                "500 2280 Usage: cryptfs enablecrypto\r\n");
        injectShellResponse("vdc cryptfs enablecrypto wipe default", "200 0 -1\r\n");
        EasyMock.expect(mMockStateMonitor.waitForDeviceNotAvailable(EasyMock.anyLong())).andReturn(
                Boolean.TRUE);
        EasyMock.expect(mMockStateMonitor.waitForDeviceOnline()).andReturn(
                mMockIDevice);
        EasyMock.replay(mMockIDevice, mMockRunUtil, mMockStateMonitor);
        assertFalse(mTestDevice.encryptDevice(false));
        EasyMock.verify(mMockIDevice, mMockRunUtil, mMockStateMonitor);
    }

    /**
     * Unit test for {@link TestDevice#unencryptDevice()} with fastboot erase.
     */
    public void testUnencryptDevice_erase() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public boolean isDeviceEncrypted() throws DeviceNotAvailableException {
                        return true;
                    }

                    @Override
                    public void rebootIntoBootloader()
                            throws DeviceNotAvailableException, UnsupportedOperationException {
                        // do nothing.
                    }

                    @Override
                    public void rebootUntilOnline() throws DeviceNotAvailableException {
                        // do nothing.
                    }

                    @Override
                    public CommandResult fastbootWipePartition(String partition)
                            throws DeviceNotAvailableException {
                        return null;
                    }

                    @Override
                    public void postBootSetup() {
                        super.postBootSetup();
                    }
                };
        setEncryptedSupported();
        EasyMock.expect(mMockStateMonitor.waitForDeviceAvailable(EasyMock.anyLong()))
                .andReturn(mMockIDevice);
        EasyMock.replay(mMockIDevice, mMockRunUtil, mMockStateMonitor);
        assertTrue(mTestDevice.unencryptDevice());
        EasyMock.verify(mMockIDevice, mMockRunUtil, mMockStateMonitor);
    }

    /**
     * Unit test for {@link TestDevice#unencryptDevice()} with fastboot wipe.
     */
    public void testUnencryptDevice_wipe() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public boolean isDeviceEncrypted() throws DeviceNotAvailableException {
                return true;
            }
            @Override
            public void rebootIntoBootloader()
                    throws DeviceNotAvailableException, UnsupportedOperationException {
                // do nothing.
            }
            @Override
            public void rebootUntilOnline() throws DeviceNotAvailableException {
                // do nothing.
            }
            @Override
            public CommandResult fastbootWipePartition(String partition)
                    throws DeviceNotAvailableException {
                return null;
            }
            @Override
            public void reboot() throws DeviceNotAvailableException {
                // do nothing.
            }
        };
        mTestDevice.getOptions().setUseFastbootErase(true);
        setEncryptedSupported();
        injectShellResponse("vdc volume list", "110 sdcard /mnt/sdcard1");
        injectShellResponse("vdc volume format sdcard", "200 0 -1:success");
        EasyMock.replay(mMockIDevice, mMockRunUtil, mMockStateMonitor);
        assertTrue(mTestDevice.unencryptDevice());
        EasyMock.verify(mMockIDevice, mMockRunUtil, mMockStateMonitor);
    }

    /**
     * Configure EasyMock for a encryption check call, that returns that encryption is unsupported
     */
    private void setEncryptedUnsupportedExpectations() throws Exception {
        setEnableAdbRootExpectations();
        injectShellResponse("vdc cryptfs enablecrypto", "\r\n");
    }

    /**
     * Configure EasyMock for a encryption check call, that returns that encryption is unsupported
     */
    private void setEncryptedSupported() throws Exception {
        setEnableAdbRootExpectations();
        injectShellResponse("vdc cryptfs enablecrypto",
                "500 29805 Usage: cryptfs enablecrypto <wipe|inplace> "
                + "default|password|pin|pattern [passwd] [noui]\r\n");
    }

    /**
     * Simple test for {@link TestDevice#switchToAdbUsb()}
     */
    public void testSwitchToAdbUsb() throws Exception  {
        setExecuteAdbCommandExpectations(new CommandResult(CommandStatus.SUCCESS), "usb");
        replayMocks();
        mTestDevice.switchToAdbUsb();
        verifyMocks();
    }

    /**
     * Test for {@link TestDevice#switchToAdbTcp()} when device has no ip address
     */
    public void testSwitchToAdbTcp_noIp() throws Exception {
        EasyMock.expect(mMockWifi.getIpAddress()).andReturn(null);
        replayMocks();
        assertNull(mTestDevice.switchToAdbTcp());
        verifyMocks();
    }

    /**
     * Test normal success case for {@link TestDevice#switchToAdbTcp()}.
     */
    public void testSwitchToAdbTcp() throws Exception {
        EasyMock.expect(mMockWifi.getIpAddress()).andReturn("ip");
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), EasyMock.eq("adb"),
                EasyMock.eq("-s"), EasyMock.eq("serial"), EasyMock.eq("tcpip"),
                EasyMock.eq("5555"))).andReturn(
                        new CommandResult(CommandStatus.SUCCESS));
        replayMocks();
        assertEquals("ip:5555", mTestDevice.switchToAdbTcp());
        verifyMocks();
    }

    /**
     * Test simple success case for
     * {@link TestDevice#installPackage(File, File, boolean, String...)}.
     */
    public void testInstallPackages() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {"Success"});
                        return receiver;
                    }
                };
        final String certFile = "foo.dc";
        final String apkFile = "foo.apk";
        EasyMock.expect(mMockIDevice.syncPackageToDevice(EasyMock.contains(certFile))).andReturn(
                certFile);
        EasyMock.expect(mMockIDevice.syncPackageToDevice(EasyMock.contains(apkFile))).andReturn(
                apkFile);
        // expect apk path to be passed as extra arg
        mMockIDevice.installRemotePackage(
                EasyMock.eq(certFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES),
                EasyMock.eq("-l"),
                EasyMock.contains(apkFile));
        EasyMock.expectLastCall();
        mMockIDevice.removeRemotePackage(certFile);
        mMockIDevice.removeRemotePackage(apkFile);

        replayMocks();

        assertNull(mTestDevice.installPackage(new File(apkFile), new File(certFile), true, "-l"));
    }

    /**
     * Test that isRuntimePermissionSupported returns correct result for device reporting LRX22F
     * build attributes
     * @throws Exception
     */
    public void testRuntimePermissionSupportedLmpRelease() throws Exception {
        injectSystemProperty("ro.build.version.sdk", "21");
        injectSystemProperty(TestDevice.BUILD_CODENAME_PROP, "REL");
        injectSystemProperty(TestDevice.BUILD_ID_PROP, "1642709");
        replayMocks();
        assertFalse(mTestDevice.isRuntimePermissionSupported());
    }

    /**
     * Test that isRuntimePermissionSupported returns correct result for device reporting LMP MR1
     * dev build attributes
     * @throws Exception
     */
    public void testRuntimePermissionSupportedLmpMr1Dev() throws Exception {
        injectSystemProperty("ro.build.version.sdk", "22");
        injectSystemProperty(TestDevice.BUILD_CODENAME_PROP, "REL");
        injectSystemProperty(TestDevice.BUILD_ID_PROP, "1844090");
        replayMocks();
        assertFalse(mTestDevice.isRuntimePermissionSupported());
    }

    /**
     * Test that isRuntimePermissionSupported returns correct result for device reporting random
     * dev build attributes
     * @throws Exception
     */
    public void testRuntimePermissionSupportedNonMncLocal() throws Exception {
        injectSystemProperty("ro.build.version.sdk", "21");
        injectSystemProperty(TestDevice.BUILD_CODENAME_PROP, "LMP");
        injectSystemProperty(TestDevice.BUILD_ID_PROP, "eng.foo.20150414.190304");
        replayMocks();
        assertFalse(mTestDevice.isRuntimePermissionSupported());
    }

    /**
     * Test that isRuntimePermissionSupported returns correct result for device reporting early MNC
     * dev build attributes
     * @throws Exception
     */
    public void testRuntimePermissionSupportedEarlyMnc() throws Exception {
        setMockIDeviceRuntimePermissionNotSupported();
        replayMocks();
        assertFalse(mTestDevice.isRuntimePermissionSupported());
    }

    /**
     * Test that isRuntimePermissionSupported returns correct result for device reporting early MNC
     * dev build attributes
     * @throws Exception
     */
    public void testRuntimePermissionSupportedMncPostSwitch() throws Exception {
        setMockIDeviceRuntimePermissionSupported();
        replayMocks();
        assertTrue(mTestDevice.isRuntimePermissionSupported());
    }

    /**
     * Convenience method for setting up mMockIDevice to not support runtime permission
     */
    private void setMockIDeviceRuntimePermissionNotSupported() {
        injectSystemProperty("ro.build.version.sdk", "22");
    }

    /**
     * Convenience method for setting up mMockIDevice to support runtime permission
     */
    private void setMockIDeviceRuntimePermissionSupported() {
        injectSystemProperty("ro.build.version.sdk", "23");
    }

    /**
     * Test default installPackage on device not supporting runtime permission has expected
     * list of args
     * @throws Exception
     */
    public void testInstallPackage_default_runtimePermissionNotSupported() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {"Success"});
                        return receiver;
                    }
                };
        final String apkFile = "foo.apk";
        setMockIDeviceRuntimePermissionNotSupported();
        mMockIDevice.installPackage(
                EasyMock.contains(apkFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES));
        EasyMock.expectLastCall();
        replayMocks();
        assertNull(mTestDevice.installPackage(new File(apkFile), true));
    }

    /**
     * Test default installPackage on device supporting runtime permission has expected list of args
     * @throws Exception
     */
    public void testInstallPackage_default_runtimePermissionSupported() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {"Success"});
                        return receiver;
                    }
                };
        final String apkFile = "foo.apk";
        setMockIDeviceRuntimePermissionSupported();
        mMockIDevice.installPackage(
                EasyMock.contains(apkFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES),
                EasyMock.eq("-g"));
        EasyMock.expectLastCall();
        replayMocks();
        assertNull(mTestDevice.installPackage(new File(apkFile), true));
    }

    /**
     * Test default installPackageForUser on device not supporting runtime permission has expected
     * list of args
     * @throws Exception
     */
    public void testinstallPackageForUser_default_runtimePermissionNotSupported() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {"Success"});
                        return receiver;
                    }
                };
        final String apkFile = "foo.apk";
        int uid = 123;
        setMockIDeviceRuntimePermissionNotSupported();
        mMockIDevice.installPackage(
                EasyMock.contains(apkFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES),
                EasyMock.eq("--user"),
                EasyMock.eq(Integer.toString(uid)));
        EasyMock.expectLastCall();
        replayMocks();
        assertNull(mTestDevice.installPackageForUser(new File(apkFile), true, uid));
    }

    /**
     * Test default installPackageForUser on device supporting runtime permission has expected
     * list of args
     * @throws Exception
     */
    public void testinstallPackageForUser_default_runtimePermissionSupported() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {"Success"});
                        return receiver;
                    }
                };
        final String apkFile = "foo.apk";
        int uid = 123;
        setMockIDeviceRuntimePermissionSupported();
        mMockIDevice.installPackage(
                EasyMock.contains(apkFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES),
                EasyMock.eq("-g"),
                EasyMock.eq("--user"),
                EasyMock.eq(Integer.toString(uid)));
        EasyMock.expectLastCall();
        replayMocks();
        assertNull(mTestDevice.installPackageForUser(new File(apkFile), true, uid));
    }

    /**
     * Test runtime permission variant of installPackage throws exception on unsupported device
     * platform
     * @throws Exception
     */
    public void testInstallPackage_throw() throws Exception {
        final String apkFile = "foo.apk";
        setMockIDeviceRuntimePermissionNotSupported();
        replayMocks();
        try {
            mTestDevice.installPackage(new File(apkFile), true, true);
        } catch (UnsupportedOperationException uoe) {
            // ignore, exception thrown here is expected
            return;
        }
        fail("installPackage did not throw IllegalArgumentException");
    }

    /**
     * Test runtime permission variant of installPackage has expected list of args on a supported
     * device when granting
     * @throws Exception
     */
    public void testInstallPackage_grant_runtimePermissionSupported() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {"Success"});
                        return receiver;
                    }
                };
        final String apkFile = "foo.apk";
        setMockIDeviceRuntimePermissionSupported();
        mMockIDevice.installPackage(
                EasyMock.contains(apkFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES),
                EasyMock.eq("-g"));
        EasyMock.expectLastCall();
        replayMocks();
        assertNull(mTestDevice.installPackage(new File(apkFile), true, true));
    }

    /** Test installing an apk that times out without any messages. */
    public void testInstallPackage_timeout() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {});
                        return receiver;
                    }
                };
        final String apkFile = "foo.apk";
        setMockIDeviceRuntimePermissionSupported();
        mMockIDevice.installPackage(
                EasyMock.contains(apkFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES),
                EasyMock.eq("-g"));
        EasyMock.expectLastCall();
        replayMocks();
        assertEquals(
                String.format("Installation of %s timed out", new File(apkFile).getAbsolutePath()),
                mTestDevice.installPackage(new File(apkFile), true, true));
    }

    /**
     * Test runtime permission variant of installPackage has expected list of args on a supported
     * device when not granting
     * @throws Exception
     */
    public void testInstallPackage_noGrant_runtimePermissionSupported() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {"Success"});
                        return receiver;
                    }
                };
        final String apkFile = "foo.apk";
        setMockIDeviceRuntimePermissionSupported();
        mMockIDevice.installPackage(
                EasyMock.contains(apkFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES));
        EasyMock.expectLastCall();
        replayMocks();
        assertNull(mTestDevice.installPackage(new File(apkFile), true, false));
    }

    /**
     * Test grant permission variant of installPackageForUser throws exception on unsupported
     * device platform
     * @throws Exception
     */
    public void testInstallPackageForUser_throw() throws Exception {
        final String apkFile = "foo.apk";
        setMockIDeviceRuntimePermissionNotSupported();
        replayMocks();
        try {
            mTestDevice.installPackageForUser(new File(apkFile), true, true, 123);
        } catch (UnsupportedOperationException uoe) {
            // ignore, exception thrown here is expected
            return;
        }
        fail("installPackage did not throw IllegalArgumentException");
    }

    /**
     * Test grant permission variant of installPackageForUser has expected list of args on a
     * supported device when granting
     * @throws Exception
     */
    public void testInstallPackageForUser_grant_runtimePermissionSupported() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {"Success"});
                        return receiver;
                    }
                };
        final String apkFile = "foo.apk";
        int uid = 123;
        setMockIDeviceRuntimePermissionSupported();
        mMockIDevice.installPackage(
                EasyMock.contains(apkFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES),
                EasyMock.eq("-g"),
                EasyMock.eq("--user"),
                EasyMock.eq(Integer.toString(uid)));
        EasyMock.expectLastCall();
        replayMocks();
        assertNull(mTestDevice.installPackageForUser(new File(apkFile), true, true, uid));
    }

    /**
     * Test grant permission variant of installPackageForUser has expected list of args on a
     * supported device when not granting
     * @throws Exception
     */
    public void testInstallPackageForUser_noGrant_runtimePermissionSupported() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    InstallReceiver createInstallReceiver() {
                        InstallReceiver receiver = new InstallReceiver();
                        receiver.processNewLines(new String[] {"Success"});
                        return receiver;
                    }
                };
        final String apkFile = "foo.apk";
        int uid = 123;
        setMockIDeviceRuntimePermissionSupported();
        mMockIDevice.installPackage(
                EasyMock.contains(apkFile),
                EasyMock.eq(true),
                EasyMock.anyObject(),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_MINUTES),
                EasyMock.eq(TestDevice.INSTALL_TIMEOUT_TO_OUTPUT_MINUTES),
                EasyMock.eq(TimeUnit.MINUTES),
                EasyMock.eq("--user"),
                EasyMock.eq(Integer.toString(uid)));
        EasyMock.expectLastCall();
        replayMocks();
        assertNull(mTestDevice.installPackageForUser(new File(apkFile), true, false, uid));
    }

    /**
     * Helper method to build a response to a executeShellCommand call
     *
     * @param expectedCommand the shell command to expect or null to skip verification of command
     * @param response the response to simulate
     */
    private void injectShellResponse(final String expectedCommand, final String response)
            throws Exception {
        injectShellResponse(expectedCommand, response, false);
    }

    /**
     * Helper method to build a response to a executeShellCommand call
     *
     * @param expectedCommand the shell command to expect or null to skip verification of command
     * @param response the response to simulate
     * @param asStub whether to set a single expectation or a stub expectation
     */
    private void injectShellResponse(final String expectedCommand, final String response,
            boolean asStub) throws Exception {
        IAnswer<Object> shellAnswer = new IAnswer<Object>() {
            @Override
            public Object answer() throws Throwable {
                IShellOutputReceiver receiver =
                    (IShellOutputReceiver)EasyMock.getCurrentArguments()[1];
                byte[] inputData = response.getBytes();
                receiver.addOutput(inputData, 0, inputData.length);
                return null;
            }
        };
        if (expectedCommand != null) {
            mMockIDevice.executeShellCommand(EasyMock.eq(expectedCommand),
                    EasyMock.<IShellOutputReceiver>anyObject(),
                    EasyMock.anyLong(), EasyMock.<TimeUnit>anyObject());
        } else {
            mMockIDevice.executeShellCommand(EasyMock.<String>anyObject(),
                    EasyMock.<IShellOutputReceiver>anyObject(),
                    EasyMock.anyLong(), EasyMock.<TimeUnit>anyObject());

        }
        if (asStub) {
            EasyMock.expectLastCall().andStubAnswer(shellAnswer);
        } else {
            EasyMock.expectLastCall().andAnswer(shellAnswer);
        }
    }

    /**
     * Helper method to inject a response to {@link TestDevice#getProperty(String)} calls
     * @param property property name
     * @param value property value
     * @return preset {@link IExpectationSetters} returned by {@link EasyMock} where further
     * expectations can be added
     */
    private IExpectationSetters<Future<String>> injectSystemProperty(
            final String property, final String value) {
        SettableFuture<String> valueResponse = SettableFuture.create();
        valueResponse.set(value);
        EasyMock.expect(mMockIDevice.getState()).andReturn(DeviceState.ONLINE);
        return EasyMock.expect(mMockIDevice.getSystemProperty(property)).andReturn(valueResponse);
    }

    /**
     * Helper method to build response to a reboot call
     * @throws Exception
     */
    private void setRebootExpectations() throws Exception {
        EasyMock.expect(mMockStateMonitor.waitForDeviceOnline()).andReturn(
                mMockIDevice);
        setEnableAdbRootExpectations();
        setEncryptedUnsupportedExpectations();
        EasyMock.expect(mMockStateMonitor.waitForDeviceAvailable(EasyMock.anyLong())).andReturn(
                mMockIDevice);
    }

    /**
     * Test normal success case for {@link TestDevice#reboot()}
     */
    public void testReboot() throws Exception {
        setRebootExpectations();
        replayMocks();
        mTestDevice.reboot();
        verifyMocks();
    }

    /**
     * Test {@link TestDevice#reboot()} attempts a recovery upon failure
     */
    public void testRebootRecovers() throws Exception {
        EasyMock.expect(mMockStateMonitor.waitForDeviceOnline()).andReturn(
                mMockIDevice);
        setEnableAdbRootExpectations();
        setEncryptedUnsupportedExpectations();
        EasyMock.expect(mMockStateMonitor.waitForDeviceAvailable(EasyMock.anyLong())).andReturn(null);
        mMockRecovery.recoverDevice(mMockStateMonitor, false);
        replayMocks();
        mRecoveryTestDevice.reboot();
        verifyMocks();
    }

    /**
     * Unit test for {@link TestDevice#getInstalledPackageNames()}.
     */
    public void testGetInstalledPackageNames() throws Exception {
        final String output = "package:/system/app/LiveWallpapers.apk=com.android.wallpaper\n" +
                "package:/system/app/LiveWallpapersPicker.apk=com.android.wallpaper.livepicker";
        injectShellResponse(TestDevice.LIST_PACKAGES_CMD, output);
        EasyMock.replay(mMockIDevice, mMockStateMonitor);
        Set<String> actualPkgs = mTestDevice.getInstalledPackageNames();
        assertEquals(2, actualPkgs.size());
        assertTrue(actualPkgs.contains("com.android.wallpaper"));
        assertTrue(actualPkgs.contains("com.android.wallpaper.livepicker"));
    }

    /**
     * Unit test for {@link TestDevice#getInstalledPackageNames()}.
     * <p/>
     * Test bad output.
     */
    public void testGetInstalledPackageNamesForBadOutput() throws Exception {
        final String output = "junk output";
        injectShellResponse(TestDevice.LIST_PACKAGES_CMD, output);
        EasyMock.replay(mMockIDevice, mMockStateMonitor);
        Set<String> actualPkgs = mTestDevice.getInstalledPackageNames();
        assertEquals(0, actualPkgs.size());
    }

    /**
     * Unit test to make sure that the simple convenience constructor for
     * {@link MountPointInfo#MountPointInfo(String, String, String, List)} works as expected.
     */
    public void testMountInfo_simple() throws Exception {
        List<String> empty = Collections.emptyList();
        MountPointInfo info = new MountPointInfo("filesystem", "mountpoint", "type", empty);
        assertEquals("filesystem", info.filesystem);
        assertEquals("mountpoint", info.mountpoint);
        assertEquals("type", info.type);
        assertEquals(empty, info.options);
    }

    /**
     * Unit test to make sure that the mount-option-parsing convenience constructor for
     * {@link MountPointInfo#MountPointInfo(String, String, String, List)} works as expected.
     */
    public void testMountInfo_parseOptions() throws Exception {
        MountPointInfo info = new MountPointInfo("filesystem", "mountpoint", "type", "rw,relatime");
        assertEquals("filesystem", info.filesystem);
        assertEquals("mountpoint", info.mountpoint);
        assertEquals("type", info.type);

        // options should be parsed
        assertNotNull(info.options);
        assertEquals(2, info.options.size());
        assertEquals("rw", info.options.get(0));
        assertEquals("relatime", info.options.get(1));
    }

    /**
     * A unit test to ensure {@link TestDevice#getMountPointInfo()} works as expected.
     */
    public void testGetMountPointInfo() throws Exception {
        injectShellResponse("cat /proc/mounts", ArrayUtil.join("\r\n",
                "rootfs / rootfs ro,relatime 0 0",
                "tmpfs /dev tmpfs rw,nosuid,relatime,mode=755 0 0",
                "devpts /dev/pts devpts rw,relatime,mode=600 0 0",
                "proc /proc proc rw,relatime 0 0",
                "sysfs /sys sysfs rw,relatime 0 0",
                "none /acct cgroup rw,relatime,cpuacct 0 0",
                "tmpfs /mnt/asec tmpfs rw,relatime,mode=755,gid=1000 0 0",
                "tmpfs /mnt/obb tmpfs rw,relatime,mode=755,gid=1000 0 0",
                "none /dev/cpuctl cgroup rw,relatime,cpu 0 0",
                "/dev/block/vold/179:3 /mnt/secure/asec vfat rw,dirsync,nosuid,nodev," +
                    "noexec,relatime,uid=1000,gid=1015,fmask=0702,dmask=0702," +
                    "allow_utime=0020,codepage=cp437,iocharset=iso8859-1,shortname=mixed," +
                    "utf8,errors=remount-ro 0 0",
                "tmpfs /storage/sdcard0/.android_secure tmpfs " +
                    "ro,relatime,size=0k,mode=000 0 0"));
        replayMocks();
        List<MountPointInfo> info = mTestDevice.getMountPointInfo();
        verifyMocks();
        assertEquals(11, info.size());

        // spot-check
        MountPointInfo mpi = info.get(0);
        assertEquals("rootfs", mpi.filesystem);
        assertEquals("/", mpi.mountpoint);
        assertEquals("rootfs", mpi.type);
        assertEquals(2, mpi.options.size());
        assertEquals("ro", mpi.options.get(0));
        assertEquals("relatime", mpi.options.get(1));

        mpi = info.get(9);
        assertEquals("/dev/block/vold/179:3", mpi.filesystem);
        assertEquals("/mnt/secure/asec", mpi.mountpoint);
        assertEquals("vfat", mpi.type);
        assertEquals(16, mpi.options.size());
        assertEquals("dirsync", mpi.options.get(1));
        assertEquals("errors=remount-ro", mpi.options.get(15));
    }

    /**
     * A unit test to ensure {@link TestDevice#getMountPointInfo(String)} works as expected.
     */
    public void testGetMountPointInfo_filter() throws Exception {
        injectShellResponse("cat /proc/mounts", ArrayUtil.join("\r\n",
                "rootfs / rootfs ro,relatime 0 0",
                "tmpfs /dev tmpfs rw,nosuid,relatime,mode=755 0 0",
                "devpts /dev/pts devpts rw,relatime,mode=600 0 0",
                "proc /proc proc rw,relatime 0 0",
                "sysfs /sys sysfs rw,relatime 0 0",
                "none /acct cgroup rw,relatime,cpuacct 0 0",
                "tmpfs /mnt/asec tmpfs rw,relatime,mode=755,gid=1000 0 0",
                "tmpfs /mnt/obb tmpfs rw,relatime,mode=755,gid=1000 0 0",
                "none /dev/cpuctl cgroup rw,relatime,cpu 0 0",
                "/dev/block/vold/179:3 /mnt/secure/asec vfat rw,dirsync,nosuid,nodev," +
                    "noexec,relatime,uid=1000,gid=1015,fmask=0702,dmask=0702," +
                    "allow_utime=0020,codepage=cp437,iocharset=iso8859-1,shortname=mixed," +
                    "utf8,errors=remount-ro 0 0",
                "tmpfs /storage/sdcard0/.android_secure tmpfs " +
                    "ro,relatime,size=0k,mode=000 0 0"),
                true /* asStub */);
        replayMocks();
        MountPointInfo mpi = mTestDevice.getMountPointInfo("/mnt/secure/asec");
        assertEquals("/dev/block/vold/179:3", mpi.filesystem);
        assertEquals("/mnt/secure/asec", mpi.mountpoint);
        assertEquals("vfat", mpi.type);
        assertEquals(16, mpi.options.size());
        assertEquals("dirsync", mpi.options.get(1));
        assertEquals("errors=remount-ro", mpi.options.get(15));

        assertNull(mTestDevice.getMountPointInfo("/a/mountpoint/too/far"));
    }

    public void testParseFreeSpaceFromFree() throws Exception {
        assertNotNull("Failed to parse free space size with decimal point",
                mTestDevice.parseFreeSpaceFromFree("/storage/emulated/legacy",
                "/storage/emulated/legacy    13.2G   296.4M    12.9G   4096"));
        assertNotNull("Failed to parse integer free space size",
                mTestDevice.parseFreeSpaceFromFree("/storage/emulated/legacy",
                "/storage/emulated/legacy     13G   395M    12G   4096"));
    }

    public void testIsDeviceInputReady_Ready() throws Exception {
        injectShellResponse("dumpsys input", ArrayUtil.join("\r\n", getDumpsysInputHeader(),
                "  DispatchEnabled: 1",
                "  DispatchFrozen: 0",
                "  FocusedApplication: <null>",
                "  FocusedWindow: name='Window{2920620f u0 com.android.launcher/"
                + "com.android.launcher2.Launcher}'",
                "  TouchStates: <no displays touched>"
                ));
        replayMocks();
        assertTrue(mTestDevice.isDeviceInputReady());
    }

    public void testIsDeviceInputReady_NotReady() throws Exception {
        injectShellResponse("dumpsys input", ArrayUtil.join("\r\n", getDumpsysInputHeader(),
                "  DispatchEnabled: 0",
                "  DispatchFrozen: 0",
                "  FocusedApplication: <null>",
                "  FocusedWindow: name='Window{2920620f u0 com.android.launcher/"
                + "com.android.launcher2.Launcher}'",
                "  TouchStates: <no displays touched>"
                ));
        replayMocks();
        assertFalse(mTestDevice.isDeviceInputReady());
    }

    public void testIsDeviceInputReady_NotSupported() throws Exception {
        injectShellResponse("dumpsys input", ArrayUtil.join("\r\n",
                "foo",
                "bar",
                "foobar",
                "barfoo"
                ));
        replayMocks();
        assertNull(mTestDevice.isDeviceInputReady());
    }

    private static String getDumpsysInputHeader() {
        return ArrayUtil.join("\r\n",
                "INPUT MANAGER (dumpsys input)",
                "",
                "Event Hub State:",
                "  BuiltInKeyboardId: -2",
                "  Devices:",
                "    -1: Virtual",
                "      Classes: 0x40000023",
                "Input Dispatcher State:"
                );
    }

    /**
     * Simple test for {@link TestDevice#handleAllocationEvent(DeviceEvent)}
     */
    public void testHandleAllocationEvent() {
        EasyMock.expect(mMockIDevice.getSerialNumber()).andStubReturn(MOCK_DEVICE_SERIAL);
        EasyMock.replay(mMockIDevice);

        assertEquals(DeviceAllocationState.Unknown, mTestDevice.getAllocationState());

        assertNotNull(mTestDevice.handleAllocationEvent(DeviceEvent.CONNECTED_ONLINE));
        assertEquals(DeviceAllocationState.Checking_Availability, mTestDevice.getAllocationState());

        assertNotNull(mTestDevice.handleAllocationEvent(DeviceEvent.AVAILABLE_CHECK_PASSED));
        assertEquals(DeviceAllocationState.Available, mTestDevice.getAllocationState());

        assertNotNull(mTestDevice.handleAllocationEvent(DeviceEvent.ALLOCATE_REQUEST));
        assertEquals(DeviceAllocationState.Allocated, mTestDevice.getAllocationState());

        assertNotNull(mTestDevice.handleAllocationEvent(DeviceEvent.FREE_AVAILABLE));
        assertEquals(DeviceAllocationState.Available, mTestDevice.getAllocationState());

        assertNotNull(mTestDevice.handleAllocationEvent(DeviceEvent.DISCONNECTED));
        assertEquals(DeviceAllocationState.Unknown, mTestDevice.getAllocationState());

        assertNotNull(mTestDevice.handleAllocationEvent(DeviceEvent.FORCE_ALLOCATE_REQUEST));
        assertEquals(DeviceAllocationState.Allocated, mTestDevice.getAllocationState());

        assertNotNull(mTestDevice.handleAllocationEvent(DeviceEvent.FREE_UNKNOWN));
        assertEquals(DeviceAllocationState.Unknown, mTestDevice.getAllocationState());
    }

    /**
     * Test that a single user is handled by {@link TestDevice#listUsers()}.
     */
    public void testListUsers_oneUser() throws Exception {
        final String listUsersCommand = "pm list users";
        injectShellResponse(listUsersCommand, ArrayUtil.join("\r\n",
                "Users:",
                "UserInfo{0:Foo:13} running"));
        replayMocks();
        ArrayList<Integer> actual = mTestDevice.listUsers();
        assertNotNull(actual);
        assertEquals(1, actual.size());
        assertEquals(0, actual.get(0).intValue());
    }

    /**
     * Test that invalid output is handled by {@link TestDevice#listUsers()}.
     */
    public void testListUsers_invalidOutput() throws Exception {
        final String listUsersCommand = "pm list users";
        final String output = "not really what we are looking for";
        injectShellResponse(listUsersCommand, output);
        replayMocks();
        try {
            mTestDevice.listUsers();
            fail("Failed to throw DeviceRuntimeException.");
        } catch (DeviceRuntimeException expected) {
            // expected
            assertEquals(
                    String.format("'%s' in not a valid output for 'pm list users'", output),
                    expected.getMessage());
        }
    }

    /** Test that invalid format of users is handled by {@link TestDevice#listUsers()}. */
    public void testListUsers_unparsableOutput() throws Exception {
        final String listUsersCommand = "pm list users";
        final String output = "Users:\n" + "\tUserInfo{0:Ownertooshort}";
        injectShellResponse(listUsersCommand, output);
        replayMocks();
        try {
            mTestDevice.listUsers();
            fail("Failed to throw DeviceRuntimeException.");
        } catch (DeviceRuntimeException expected) {
            // expected
            assertEquals(
                    String.format(
                            "device output: '%s' \nline: '\tUserInfo{0:Ownertooshort}' was not in the"
                                    + " expected format for user info.",
                            output),
                    expected.getMessage());
        }
    }

    /**
     * Test that multiple user is handled by {@link TestDevice#listUsers()}.
     */
    public void testListUsers_multiUsers() throws Exception {
        final String listUsersCommand = "pm list users";
        injectShellResponse(listUsersCommand, ArrayUtil.join("\r\n",
                "Users:",
                "UserInfo{0:Foo:13} running",
                "UserInfo{3:FooBar:14}"));
        replayMocks();
        ArrayList<Integer> actual = mTestDevice.listUsers();
        assertNotNull(actual);
        assertEquals(2, actual.size());
        assertEquals(0, actual.get(0).intValue());
        assertEquals(3, actual.get(1).intValue());
    }

    /**
     * Test that multi user output is handled by {@link TestDevice#getMaxNumberOfUsersSupported()}.
     */
    public void testMaxNumberOfUsersSupported() throws Exception {
        final String getMaxUsersCommand = "pm get-max-users";
        injectShellResponse(getMaxUsersCommand, "Maximum supported users: 4");
        replayMocks();
        assertEquals(4, mTestDevice.getMaxNumberOfUsersSupported());
    }

    /**
     * Test that invalid output is handled by {@link TestDevice#getMaxNumberOfUsersSupported()}.
     */
    public void testMaxNumberOfUsersSupported_invalid() throws Exception {
        final String getMaxUsersCommand = "pm get-max-users";
        injectShellResponse(getMaxUsersCommand, "not the output we expect");
        replayMocks();
        assertEquals(0, mTestDevice.getMaxNumberOfUsersSupported());
    }

    /**
     * Test that multi user output is handled by {@link TestDevice#getMaxNumberOfUsersSupported()}.
     */
    public void testMaxNumberOfRunningUsersSupported() throws Exception {
        injectSystemProperty("ro.build.version.sdk", "28");
        injectSystemProperty(TestDevice.BUILD_CODENAME_PROP, "REL");
        final String getMaxRunningUsersCommand = "pm get-max-running-users";
        injectShellResponse(getMaxRunningUsersCommand, "Maximum supported running users: 4");
        replayMocks();
        assertEquals(4, mTestDevice.getMaxNumberOfRunningUsersSupported());
    }

    /** Test that invalid output is handled by {@link TestDevice#getMaxNumberOfUsersSupported()}. */
    public void testMaxNumberOfRunningUsersSupported_invalid() throws Exception {
        injectSystemProperty("ro.build.version.sdk", "28");
        injectSystemProperty(TestDevice.BUILD_CODENAME_PROP, "REL");
        final String getMaxRunningUsersCommand = "pm get-max-running-users";
        injectShellResponse(getMaxRunningUsersCommand, "not the output we expect");
        replayMocks();
        assertEquals(0, mTestDevice.getMaxNumberOfRunningUsersSupported());
    }

    /**
     * Test that single user output is handled by {@link TestDevice#getMaxNumberOfUsersSupported()}.
     */
    public void testIsMultiUserSupported_singleUser() throws Exception {
        final String getMaxUsersCommand = "pm get-max-users";
        injectShellResponse(getMaxUsersCommand, "Maximum supported users: 1");
        replayMocks();
        assertFalse(mTestDevice.isMultiUserSupported());
    }

    /**
     * Test that {@link TestDevice#isMultiUserSupported()} works.
     */
    public void testIsMultiUserSupported() throws Exception {
        final String getMaxUsersCommand = "pm get-max-users";
        injectShellResponse(getMaxUsersCommand, "Maximum supported users: 4");
        replayMocks();
        assertTrue(mTestDevice.isMultiUserSupported());
    }

    /**
     * Test that invalid output is handled by {@link TestDevice#isMultiUserSupported()}.
     */
    public void testIsMultiUserSupported_invalidOutput() throws Exception {
        final String getMaxUsersCommand = "pm get-max-users";
        injectShellResponse(getMaxUsersCommand, "not the output we expect");
        replayMocks();
        assertFalse(mTestDevice.isMultiUserSupported());
    }

    /**
     * Test that successful user creation is handled by {@link TestDevice#createUser(String)}.
     */
    public void testCreateUser() throws Exception {
        final String createUserCommand = "pm create-user foo";
        injectShellResponse(createUserCommand, "Success: created user id 10");
        replayMocks();
        assertEquals(10, mTestDevice.createUser("foo"));
    }

    /**
     * Test that successful user creation is handled by
     * {@link TestDevice#createUser(String, boolean, boolean)}.
     */
    public void testCreateUserFlags() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Success: created user id 12";
            }
        };
        assertEquals(12, mTestDevice.createUser("TEST", true, true));
    }

    /**
     * Test that {@link TestDevice#createUser(String, boolean, boolean)} fails when bad output
     */
    public void testCreateUser_wrongOutput() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Success: created user id WRONG";
            }
        };
        try {
            mTestDevice.createUser("TEST", true, true);
        } catch (IllegalStateException e) {
            // expected
            return;
        }
        fail("CreateUser should have thrown an exception");
    }

    /**
     * Test that a failure to create a user is handled by {@link TestDevice#createUser(String)}.
     */
    public void testCreateUser_failed() throws Exception {
        final String createUserCommand = "pm create-user foo";
        injectShellResponse(createUserCommand, "Error");
        replayMocks();
        try {
            mTestDevice.createUser("foo");
            fail("IllegalStateException not thrown");
        } catch (IllegalStateException e) {
            // Expected
        }
    }

    /**
     * Test that successful user removal is handled by {@link TestDevice#removeUser(int)}.
     */
    public void testRemoveUser() throws Exception {
        final String removeUserCommand = "pm remove-user 10";
        injectShellResponse(removeUserCommand, "Success: removed user\n");
        replayMocks();
        assertTrue(mTestDevice.removeUser(10));
    }

    /**
     * Test that a failure to remove a user is handled by {@link TestDevice#removeUser(int)}.
     */
    public void testRemoveUser_failed() throws Exception {
        final String removeUserCommand = "pm remove-user 10";
        injectShellResponse(removeUserCommand, "Error: couldn't remove user id 10");
        replayMocks();
        assertFalse(mTestDevice.removeUser(10));
    }

    /**
     * Test that trying to run a test with a user with
     * {@link TestDevice#runInstrumentationTestsAsUser(IRemoteAndroidTestRunner, int, Collection)}
     * fails if the {@link IRemoteAndroidTestRunner} is not an instance of
     * {@link RemoteAndroidTestRunner}.
     */
    public void testrunInstrumentationTestsAsUser_failed() throws Exception {
        IRemoteAndroidTestRunner mockRunner = EasyMock.createMock(IRemoteAndroidTestRunner.class);
        EasyMock.expect(mockRunner.getPackageName()).andStubReturn("com.example");
        Collection<ITestLifeCycleReceiver> listeners = new ArrayList<>();
        EasyMock.replay(mockRunner);
        try {
            mTestDevice.runInstrumentationTestsAsUser(mockRunner, 12, listeners);
            fail("IllegalStateException not thrown.");
        } catch (IllegalStateException e) {
            //expected
        }
    }

     /**
     * Test that successful user start is handled by {@link TestDevice#startUser(int)}.
     */
    public void testStartUser() throws Exception {
        final String startUserCommand = "am start-user 10";
        injectShellResponse(startUserCommand, "Success: user started\n");
        replayMocks();
        assertTrue(mTestDevice.startUser(10));
    }

    /**
     * Test that a failure to start user is handled by {@link TestDevice#startUser(int)}.
     */
    public void testStartUser_failed() throws Exception {
        final String startUserCommand = "am start-user 10";
        injectShellResponse(startUserCommand, "Error: could not start user\n");
        replayMocks();
        assertFalse(mTestDevice.startUser(10));
    }

    /**
     * Test that remount works as expected on a device not supporting dm verity
     * @throws Exception
     */
    public void testRemount_verityUnsupported() throws Exception {
        injectSystemProperty("partition.system.verified", "");
        setExecuteAdbCommandExpectations(new CommandResult(CommandStatus.SUCCESS), "remount");
        EasyMock.expect(mMockStateMonitor.waitForDeviceAvailable()).andReturn(mMockIDevice);
        replayMocks();
        mTestDevice.remountSystemWritable();
        verifyMocks();
    }

    /**
     * Test that remount works as expected on a device supporting dm verity v1
     * @throws Exception
     */
    public void testRemount_veritySupportedV1() throws Exception {
        injectSystemProperty("partition.system.verified", "1");
        setExecuteAdbCommandExpectations(
                new CommandResult(CommandStatus.SUCCESS), "disable-verity");
        setRebootExpectations();
        setExecuteAdbCommandExpectations(new CommandResult(CommandStatus.SUCCESS), "remount");
        EasyMock.expect(mMockStateMonitor.waitForDeviceAvailable()).andReturn(mMockIDevice);
        replayMocks();
        mTestDevice.remountSystemWritable();
        verifyMocks();
    }

    /**
     * Test that remount works as expected on a device supporting dm verity v2
     * @throws Exception
     */
    public void testRemount_veritySupportedV2() throws Exception {
        injectSystemProperty("partition.system.verified", "2");
        setExecuteAdbCommandExpectations(
                new CommandResult(CommandStatus.SUCCESS), "disable-verity");
        setRebootExpectations();
        setExecuteAdbCommandExpectations(new CommandResult(CommandStatus.SUCCESS), "remount");
        EasyMock.expect(mMockStateMonitor.waitForDeviceAvailable()).andReturn(mMockIDevice);
        replayMocks();
        mTestDevice.remountSystemWritable();
        verifyMocks();
    }

    /**
     * Test that remount works as expected on a device supporting dm verity but with unknown version
     * @throws Exception
     */
    public void testRemount_veritySupportedNonNumerical() throws Exception {
        injectSystemProperty("partition.system.verified", "foo");
        setExecuteAdbCommandExpectations(
                new CommandResult(CommandStatus.SUCCESS), "disable-verity");
        setRebootExpectations();
        setExecuteAdbCommandExpectations(new CommandResult(CommandStatus.SUCCESS), "remount");
        EasyMock.expect(mMockStateMonitor.waitForDeviceAvailable()).andReturn(mMockIDevice);
        replayMocks();
        mTestDevice.remountSystemWritable();
        verifyMocks();
    }

    /**
     * Test that {@link TestDevice#getBuildSigningKeys()} works for the typical "test-keys" case
     * @throws Exception
     */
    public void testGetBuildSigningKeys_test_keys() throws Exception {
        injectSystemProperty(TestDevice.BUILD_TAGS, "test-keys");
        replayMocks();
        assertEquals("test-keys", mTestDevice.getBuildSigningKeys());
    }

    /**
     * Test that {@link TestDevice#getBuildSigningKeys()} works for the case where build tags is a
     * comma separated list
     * @throws Exception
     */
    public void testGetBuildSigningKeys_test_keys_commas() throws Exception {
        injectSystemProperty(TestDevice.BUILD_TAGS, "test-keys,foo,bar,yadda");
        replayMocks();
        assertEquals("test-keys", mTestDevice.getBuildSigningKeys());
    }

    /**
     * Test that {@link TestDevice#getBuildSigningKeys()} returns null for non-matching case
     * @throws Exception
     */
    public void testGetBuildSigningKeys_not_matched() throws Exception {
        injectSystemProperty(TestDevice.BUILD_TAGS, "huh,foo,bar,yadda");
        replayMocks();
        assertNull(mTestDevice.getBuildSigningKeys());
    }

    /**
     * Test that {@link TestDevice#getCurrentUser()} returns the current user id.
     * @throws Exception
     */
    public void testGetCurrentUser() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "3\n";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return MIN_API_LEVEL_GET_CURRENT_USER;
            }
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return "N\n";
            }
        };
        int res = mTestDevice.getCurrentUser();
        assertEquals(3, res);
    }

    /**
     * Test that {@link TestDevice#getCurrentUser()} returns null when output is not expected
     * @throws Exception
     */
    public void testGetCurrentUser_null() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "not found.";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return MIN_API_LEVEL_GET_CURRENT_USER;
            }
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return "N\n";
            }
        };
        int res = mTestDevice.getCurrentUser();
        assertEquals(NativeDevice.INVALID_USER_ID, res);
    }

    /**
     * Test that {@link TestDevice#getCurrentUser()} returns null when api level is too low
     * @throws Exception
     */
    public void testGetCurrentUser_lowApi() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 15;
            }
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return "REL\n";
            }
        };
        try {
            mTestDevice.getCurrentUser();
        } catch (IllegalArgumentException e) {
            // expected
            return;
        }
        fail("getCurrentUser should have thrown an exception.");
    }

    /**
     * Unit test for {@link TestDevice#getUserFlags(int)}.
     */
    public void testGetUserFlag() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Users:\n\tUserInfo{0:Owner:13} running";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        int flags = mTestDevice.getUserFlags(0);
        // Expected 19 because using radix 16 (so 13 becomes (1 * 16^1 + 3 * 16^0) = 19)
        assertEquals(19, flags);
    }

    /**
     * Unit test for {@link TestDevice#getUserFlags(int)} when command return empty list
     * of users.
     */
    public void testGetUserFlag_emptyReturn() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        int flags = mTestDevice.getUserFlags(2);
        assertEquals(NativeDevice.INVALID_USER_ID, flags);
    }

    /**
     * Unit test for {@link TestDevice#getUserFlags(int)} when there is multiple users in
     * the list.
     */
    public void testGetUserFlag_multiUser() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Users:\n\tUserInfo{0:Owner:13}\n\tUserInfo{WRONG:Owner:14}\n\t"
                        + "UserInfo{}\n\tUserInfo{3:Owner:15} Running";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        int flags = mTestDevice.getUserFlags(3);
        assertEquals(21, flags);
    }

    /**
     * Unit test for {@link TestDevice#getUserSerialNumber(int)}
     */
    public void testGetUserSerialNumber() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Users:\nUserInfo{0:Owner:13} serialNo=666";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        int serial = mTestDevice.getUserSerialNumber(0);
        assertEquals(666, serial);
    }

    /**
     * Unit test for {@link TestDevice#getUserSerialNumber(int)} when the dumpsys return some
     * bad data.
     */
    public void testGetUserSerialNumber_badData() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Users:\nUserInfo{0:Owner:13} serialNo=WRONG";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        int serial = mTestDevice.getUserSerialNumber(0);
        assertEquals(NativeDevice.INVALID_USER_ID, serial);
    }

    /**
     * Unit test for {@link TestDevice#getUserSerialNumber(int)} when the dumpsys return an empty
     * serial
     */
    public void testGetUserSerialNumber_emptySerial() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Users:\nUserInfo{0:Owner:13} serialNo=";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        int serial = mTestDevice.getUserSerialNumber(0);
        assertEquals(NativeDevice.INVALID_USER_ID, serial);
    }

    /**
     * Unit test for {@link TestDevice#getUserSerialNumber(int)} when there is multiple users in
     * the dumpsys
     */
    public void testGetUserSerialNumber_multiUsers() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Users:\nUserInfo{0:Owner:13} serialNo=1\nUserInfo{1:Owner:13} serialNo=2"
                        + "\nUserInfo{2:Owner:13} serialNo=3";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        int serial = mTestDevice.getUserSerialNumber(2);
        assertEquals(3, serial);
    }

    /**
     * Unit test for {@link TestDevice#switchUser(int)} when user requested is already is current
     * user.
     */
    public void testSwitchUser_alreadySameUser() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public int getCurrentUser() throws DeviceNotAvailableException {
                return 0;
            }
            @Override
            public void prePostBootSetup() {
                // skip for this test
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return MIN_API_LEVEL_GET_CURRENT_USER;
            }
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return "N\n";
            }
        };
        assertTrue(mTestDevice.switchUser(0));
    }

    /**
     * Unit test for {@link TestDevice#switchUser(int)} when user switch instantly.
     */
    public void testSwitchUser() throws Exception {
        mTestDevice = new TestableTestDevice() {
            int ret = 0;
            @Override
            public int getCurrentUser() throws DeviceNotAvailableException {
                return ret;
            }
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                ret = 10;
                return "";
            }
            @Override
            public void prePostBootSetup() {
                // skip for this test
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return MIN_API_LEVEL_GET_CURRENT_USER;
            }
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return "N\n";
            }
        };
        assertTrue(mTestDevice.switchUser(10));
    }

    /**
     * Unit test for {@link TestDevice#switchUser(int)} when user switch with a short delay.
     */
    public void testSwitchUser_delay() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    int ret = 0;

                    @Override
                    public int getCurrentUser() throws DeviceNotAvailableException {
                        return ret;
                    }

                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        test.setName(getClass().getCanonicalName() + "#testSwitchUser_delay");
                        test.start();
                        return "";
                    }

                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return MIN_API_LEVEL_GET_CURRENT_USER;
                    }

                    @Override
                    public String getProperty(String name) throws DeviceNotAvailableException {
                        return "N\n";
                    }

                    @Override
                    public void prePostBootSetup() {
                        // skip for this test
                    }

                    @Override
                    protected long getCheckNewUserSleep() {
                        return 100;
                    }

                    Thread test =
                            new Thread(
                                    new Runnable() {
                                        @Override
                                        public void run() {
                                            RunUtil.getDefault().sleep(100);
                                            ret = 10;
                                        }
                                    });
                };
        assertTrue(mTestDevice.switchUser(10));
    }

    /**
     * Unit test for {@link TestDevice#switchUser(int)} when user never change.
     */
    public void testSwitchUser_noChange() throws Exception {
        mTestDevice = new TestableTestDevice() {
            int ret = 0;
            @Override
            public int getCurrentUser() throws DeviceNotAvailableException {
                return ret;
            }
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                ret = 0;
                return "";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return MIN_API_LEVEL_GET_CURRENT_USER;
            }
            @Override
            public String getProperty(String name) throws DeviceNotAvailableException {
                return "N\n";
            }
            @Override
            protected long getCheckNewUserSleep() {
                return 50;
            }
            @Override
            public void prePostBootSetup() {
                // skip for this test
            }
        };
        assertFalse(mTestDevice.switchUser(10, 100));
    }

    /**
     * Unit test for {@link TestDevice#stopUser(int)}, cannot stop current user.
     */
    public void testStopUser_notCurrent() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "Can't stop current user";
                    }

                    @Override
                    public int getCurrentUser() throws DeviceNotAvailableException {
                        return 0;
                    }

                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return MIN_API_LEVEL_STOP_USER;
                    }

                    @Override
                    public String getProperty(String name) throws DeviceNotAvailableException {
                        return "N\n";
                    }
                };
        assertFalse(mTestDevice.stopUser(0));
    }

    /**
     * Unit test for {@link TestDevice#stopUser(int)}, cannot stop system
     */
    public void testStopUser_notSystem() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "Error: Can't stop system user 0";
                    }

                    @Override
                    public int getCurrentUser() throws DeviceNotAvailableException {
                        return 10;
                    }

                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return MIN_API_LEVEL_STOP_USER;
                    }

                    @Override
                    public String getProperty(String name) throws DeviceNotAvailableException {
                        return "N\n";
                    }
                };
        assertFalse(mTestDevice.stopUser(0));
    }

    /**
     * Unit test for {@link TestDevice#stopUser(int, boolean, boolean)}, for a success stop in API
     * 22.
     */
    public void testStopUser_success_api22() throws Exception {
        verifyStopUserSuccess(22, false, false, "am stop-user 10");
    }

    /**
     * Unit test for {@link TestDevice#stopUser(int, boolean, boolean)}, for a success stop in API
     * 23.
     */
    public void testStopUser_success_api23() throws Exception {
        verifyStopUserSuccess(23, true, false, "am stop-user -w 10");
    }

    /**
     * Unit test for {@link TestDevice#stopUser(int, boolean, boolean)}, for a success stop in API
     * 24.
     */
    public void testStopUser_success_api24() throws Exception {
        verifyStopUserSuccess(24, true, true, "am stop-user -w -f 10");
    }

    private void verifyStopUserSuccess(
            int apiLevel, boolean wait, boolean force, String expectedCommand) throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        if (command.contains("am")) {
                            assertEquals(expectedCommand, command);
                        } else if (command.contains("pm")) {
                            assertEquals("pm list users", command);
                        } else {
                            fail("Unexpected command");
                        }
                        return "Users:\n\tUserInfo{0:Test:13}";
                    }

                    @Override
                    public int getCurrentUser() throws DeviceNotAvailableException {
                        return 0;
                    }

                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return apiLevel;
                    }

                    @Override
                    public String getProperty(String name) throws DeviceNotAvailableException {
                        return "N\n";
                    }
                };
        assertTrue(mTestDevice.stopUser(10, wait, force));
    }

    public void testStopUser_wait_api22() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return 22;
                    }
                };
        try {
            mTestDevice.stopUser(10, true, false);
            fail("IllegalArgumentException should be thrown");
        } catch (IllegalArgumentException ex) {
            // expected
        }
    }

    public void testStopUser_force_api22() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return 22;
                    }
                };
        try {
            mTestDevice.stopUser(10, false, true);
            fail("IllegalArgumentException should be thrown");
        } catch (IllegalArgumentException ex) {
            // expected
        }
    }

    /**
     * Unit test for {@link TestDevice#stopUser(int)}, for a failed stop
     */
    public void testStopUser_failed() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        if (command.contains("am")) {
                            assertEquals("am stop-user 0", command);
                        } else if (command.contains("pm")) {
                            assertEquals("pm list users", command);
                        } else {
                            fail("Unexpected command");
                        }
                        return "Users:\n\tUserInfo{0:Test:13} running";
                    }

                    @Override
                    public int getCurrentUser() throws DeviceNotAvailableException {
                        return 10;
                    }

                    @Override
                    public int getApiLevel() throws DeviceNotAvailableException {
                        return MIN_API_LEVEL_STOP_USER;
                    }

                    @Override
                    public String getProperty(String name) throws DeviceNotAvailableException {
                        return "N\n";
                    }
                };
        assertFalse(mTestDevice.stopUser(0));
    }

    /**
     * Unit test for {@link TestDevice#isUserRunning(int)}.
     */
    public void testIsUserIdRunning_true() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Users:\n\tUserInfo{0:Test:13} running";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return MIN_API_LEVEL_GET_CURRENT_USER;
            }
        };
        assertTrue(mTestDevice.isUserRunning(0));
    }

    /**
     * Unit test for {@link TestDevice#isUserRunning(int)}.
     */
    public void testIsUserIdRunning_false() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Users:\n\tUserInfo{0:Test:13} running\n\tUserInfo{10:New user:10}";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        assertFalse(mTestDevice.isUserRunning(10));
    }

    /**
     * Unit test for {@link TestDevice#isUserRunning(int)}.
     */
    public void testIsUserIdRunning_badFormat() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Users:\n\tUserInfo{WRONG:Test:13} running";
            }
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        assertFalse(mTestDevice.isUserRunning(0));
    }

    /**
     * Unit test for {@link TestDevice#hasFeature(String)} on success.
     */
    public void testHasFeature_true() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "feature:com.google.android.feature.EXCHANGE_6_2\n" +
                        "feature:com.google.android.feature.GOOGLE_BUILD\n" +
                        "feature:com.google.android.feature.GOOGLE_EXPERIENCE";
            }
        };
        assertTrue(mTestDevice.hasFeature("com.google.android.feature.EXCHANGE_6_2"));
    }

    /**
     * Unit test for {@link TestDevice#hasFeature(String)} on failure.
     */
    public void testHasFeature_fail() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "feature:com.google.android.feature.EXCHANGE_6_2\n" +
                        "feature:com.google.android.feature.GOOGLE_BUILD\n" +
                        "feature:com.google.android.feature.GOOGLE_EXPERIENCE";
            }
        };
        assertFalse(mTestDevice.hasFeature("feature:test"));
    }

    /**
     * Unit test for {@link TestDevice#getSetting(int, String, String)}.
     */
    public void testGetSetting() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "78";
            }
        };
        assertEquals("78", mTestDevice.getSetting(0, "system", "screen_brightness"));
    }

    /**
     * Unit test for {@link TestDevice#getSetting(String, String)}.
     */
    public void testGetSetting_SystemUser() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "78";
            }
        };
        assertEquals("78", mTestDevice.getSetting("system", "screen_brightness"));
    }

    /**
     * Unit test for {@link TestDevice#getSetting(int, String, String)}.
     */
    public void testGetSetting_nulloutput() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "null";
            }
        };
        assertNull(mTestDevice.getSetting(0, "system", "screen_brightness"));
    }

    /**
     * Unit test for {@link TestDevice#getSetting(int, String, String)} with a namespace
     * that is not in {global, system, secure}.
     */
    public void testGetSetting_unexpectedNamespace() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        assertNull(mTestDevice.getSetting(0, "TEST", "screen_brightness"));
    }

    /**
     * Unit test for {@link TestDevice#setSetting(int, String, String, String)}
     * with a namespace that is not in {global, system, secure}.
     */
    public void testSetSetting_unexpectedNamespace() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        try {
            mTestDevice.setSetting(0, "TEST", "screen_brightness", "75");
        } catch (IllegalArgumentException e) {
            // expected
            return;
        }
        fail("putSettings should have thrown an exception.");
    }

    /**
     * Unit test for {@link TestDevice#setSetting(int, String, String, String)}
     * with a normal case.
     */
    public void testSetSettings() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        try {
            mTestDevice.setSetting(0, "system", "screen_brightness", "75");
        } catch (IllegalArgumentException e) {
            fail("putSettings should not have thrown an exception.");
        }
    }

    /**
     * Unit test for {@link TestDevice#setSetting(String, String, String)}
     * with a normal case.
     */
    public void testSetSettings_SystemUser() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 22;
            }
        };
        try {
            mTestDevice.setSetting("system", "screen_brightness", "75");
        } catch (IllegalArgumentException e) {
            fail("putSettings should not have thrown an exception.");
        }
    }

    /**
     * Unit test for {@link TestDevice#setSetting(int, String, String, String)}
     * when API level is too low
     */
    public void testSetSettings_lowApi() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public int getApiLevel() throws DeviceNotAvailableException {
                return 21;
            }
        };
        try {
            mTestDevice.setSetting(0, "system", "screen_brightness", "75");
        } catch (IllegalArgumentException e) {
            // expected
            return;
        }
        fail("putSettings should have thrown an exception.");
    }

    /**
     * Unit test for {@link TestDevice#getAndroidId(int)}.
     */
    public void testGetAndroidId() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "4433829313704884235";
            }
            @Override
            public boolean isAdbRoot() throws DeviceNotAvailableException {
                return true;
            }
        };
        assertEquals("4433829313704884235", mTestDevice.getAndroidId(0));
    }

    /**
     * Unit test for {@link TestDevice#getAndroidId(int)} when db containing the id is not found
     */
    public void testGetAndroidId_notFound() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public String executeShellCommand(String command) throws DeviceNotAvailableException {
                return "Error: unable to open database"
                        + "\"/data/0/com.google.android.gsf/databases/gservices.db\": "
                        + "unable to open database file";
            }
            @Override
            public boolean isAdbRoot() throws DeviceNotAvailableException {
                return true;
            }
        };
        assertNull(mTestDevice.getAndroidId(0));
    }

    /**
     * Unit test for {@link TestDevice#getAndroidId(int)} when adb root not enabled.
     */
    public void testGetAndroidId_notRoot() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public boolean isAdbRoot() throws DeviceNotAvailableException {
                return false;
            }
        };
        assertNull(mTestDevice.getAndroidId(0));
    }

    /**
     * Unit test for {@link TestDevice#getAndroidIds()}
     */
    public void testGetAndroidIds() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public ArrayList<Integer> listUsers() throws DeviceNotAvailableException {
                ArrayList<Integer> test = new ArrayList<Integer>();
                test.add(0);
                test.add(1);
                return test;
            }
            @Override
            public String getAndroidId(int userId) throws DeviceNotAvailableException {
                return "44444";
            }
        };
        Map<Integer, String> expected = new HashMap<Integer, String>();
        expected.put(0, "44444");
        expected.put(1, "44444");
        assertEquals(expected, mTestDevice.getAndroidIds());
    }

    /**
     * Unit test for {@link TestDevice#getAndroidIds()} when no user are found.
     */
    public void testGetAndroidIds_noUser() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public ArrayList<Integer> listUsers() throws DeviceNotAvailableException {
                return null;
            }
        };
        assertNull(mTestDevice.getAndroidIds());
    }

    /**
     * Unit test for {@link TestDevice#getAndroidIds()} when no match is found for user ids.
     */
    public void testGetAndroidIds_noMatch() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            public ArrayList<Integer> listUsers() throws DeviceNotAvailableException {
                ArrayList<Integer> test = new ArrayList<Integer>();
                test.add(0);
                test.add(1);
                return test;
            }
            @Override
            public String getAndroidId(int userId) throws DeviceNotAvailableException {
                return null;
            }
        };
        Map<Integer, String> expected = new HashMap<Integer, String>();
        expected.put(0, null);
        expected.put(1, null);
        assertEquals(expected, mTestDevice.getAndroidIds());
    }

    /**
     * Test for {@link TestDevice#getScreenshot()} when action failed.
     */
    public void testGetScreenshot_failure() throws Exception {
        mTestDevice = new TestableTestDevice() {
            @Override
            protected boolean performDeviceAction(
                    String actionDescription, DeviceAction action, int retryAttempts)
                    throws DeviceNotAvailableException {
                return false;
            }
        };
        InputStreamSource res = mTestDevice.getScreenshot();
        assertNotNull(res);
        assertEquals(
                "Error: device reported null for screenshot.",
                StreamUtil.getStringFromStream(res.createInputStream()));
    }

    /**
     * Test for {@link TestDevice#getScreenshot()} when action succeed.
     */
    public void testGetScreenshot() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    protected boolean performDeviceAction(
                            String actionDescription, DeviceAction action, int retryAttempts)
                            throws DeviceNotAvailableException {
                        return true;
                    }

                    @Override
                    byte[] compressRawImage(RawImage rawImage, String format, boolean rescale) {
                        return "image".getBytes();
                    }
                };
        InputStreamSource data = mTestDevice.getScreenshot();
        assertNotNull(data);
        assertTrue(data instanceof ByteArrayInputStreamSource);
    }

    /**
     * Helper to retrieve the test file
     */
    private File getTestImageResource() throws Exception {
        InputStream imageZip = getClass().getResourceAsStream(RAWIMAGE_RESOURCE);
        File imageZipFile = FileUtil.createTempFile("rawImage", ".zip");
        try {
            FileUtil.writeToFile(imageZip, imageZipFile);
            File dir = ZipUtil2.extractZipToTemp(imageZipFile, "test-raw-image");
            return new File(dir, "rawImageScreenshot.raw");
        } finally {
            FileUtil.deleteFile(imageZipFile);
        }
    }

    /**
     * Helper to create the rawImage to test.
     */
    private RawImage prepareRawImage(File rawImageFile) throws Exception {
        RawImage sRawImage = null;
        String data = FileUtil.readStringFromFile(rawImageFile);
        sRawImage = new RawImage();
        sRawImage.alpha_length = 8;
        sRawImage.alpha_offset = 24;
        sRawImage.blue_length = 8;
        sRawImage.blue_offset = 16;
        sRawImage.bpp = 32;
        sRawImage.green_length = 8;
        sRawImage.green_offset = 8;
        sRawImage.height = 1920;
        sRawImage.red_length = 8;
        sRawImage.red_offset = 0;
        sRawImage.size = 8294400;
        sRawImage.version = 1;
        sRawImage.width = 1080;
        sRawImage.data = data.getBytes();
        return sRawImage;
    }

    /**
     * Test for {@link TestDevice#compressRawImage(RawImage, String, boolean)} properly reduce the
     * image size with different encoding.
     */
    public void testCompressScreenshot() throws Exception {
        InputStream imageData = getClass().getResourceAsStream("/testdata/SmallRawImage.raw");
        File testImageFile = FileUtil.createTempFile("raw-to-buffered", ".raw");
        FileUtil.writeToFile(imageData, testImageFile);
        RawImage testImage = null;
        try {
            testImage = prepareRawImage(testImageFile);
            // We used the small image so we adapt the size.
            testImage.height = 25;
            testImage.size = 2000;
            testImage.width = 25;
            // Size of the raw test data
            Assert.assertEquals(3000, testImage.data.length);
            byte[] result = mTestDevice.compressRawImage(testImage, "PNG", true);
            // Size after compressing can vary a bit depending of the JDK
            if (result.length != 107 && result.length != 117) {
                fail(
                        String.format(
                                "Should have compress the length as expected, got %s, "
                                        + "expected 107 or 117",
                                result.length));
            }

            // Do it again with JPEG encoding
            Assert.assertEquals(3000, testImage.data.length);
            result = mTestDevice.compressRawImage(testImage, "JPEG", true);
            // Size after compressing as JPEG
            Assert.assertEquals(1041, result.length);
        } finally {
            if (testImage != null) {
                testImage.data = null;
            }
            FileUtil.deleteFile(testImageFile);
        }
    }

    /**
     * Test for {@link TestDevice#rawImageToBufferedImage(RawImage, String)}.
     *
     * @throws Exception
     */
    public void testRawImageToBufferedImage() throws Exception {
        InputStream imageData = getClass().getResourceAsStream("/testdata/SmallRawImage.raw");
        File testImageFile = FileUtil.createTempFile("raw-to-buffered", ".raw");
        FileUtil.writeToFile(imageData, testImageFile);
        RawImage testImage = null;
        try {
            testImage = prepareRawImage(testImageFile);
            // We used the small image so we adapt the size.
            testImage.height = 25;
            testImage.size = 2000;
            testImage.width = 25;

            // Test PNG format
            BufferedImage bufferedImage = mTestDevice.rawImageToBufferedImage(testImage, "PNG");
            assertEquals(testImage.width, bufferedImage.getWidth());
            assertEquals(testImage.height, bufferedImage.getHeight());
            assertEquals(BufferedImage.TYPE_INT_ARGB, bufferedImage.getType());

            // Test JPEG format
            bufferedImage = mTestDevice.rawImageToBufferedImage(testImage, "JPEG");
            assertEquals(testImage.width, bufferedImage.getWidth());
            assertEquals(testImage.height, bufferedImage.getHeight());
            assertEquals(BufferedImage.TYPE_3BYTE_BGR, bufferedImage.getType());
        } finally {
            if (testImage != null) {
                testImage.data = null;
            }
            FileUtil.deleteFile(testImageFile);
        }
    }

    /**
     * Test for {@link TestDevice#rescaleImage(BufferedImage)}.
     *
     * @throws Exception
     */
    public void testRescaleImage() throws Exception {
        File testImageFile = getTestImageResource();
        RawImage testImage = null;
        try {
            testImage = prepareRawImage(testImageFile);
            BufferedImage bufferedImage = mTestDevice.rawImageToBufferedImage(testImage, "PNG");

            BufferedImage scaledImage = mTestDevice.rescaleImage(bufferedImage);
            assertEquals(bufferedImage.getWidth() / 2, scaledImage.getWidth());
            assertEquals(bufferedImage.getHeight() / 2, scaledImage.getHeight());
        } finally {
            if (testImage != null) {
                testImage.data = null;
            }
            FileUtil.recursiveDelete(testImageFile.getParentFile());
        }
    }

    /**
     * Test for {@link TestDevice#compressRawImage(RawImage, String, boolean)} does not rescale
     * image if specified.
     */
    public void testCompressScreenshotNoRescale() throws Exception {
        File testImageFile = getTestImageResource();
        final RawImage[] testImage = new RawImage[1];
        testImage[0] = prepareRawImage(testImageFile);

        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    byte[] getImageData(BufferedImage image, String format) {
                        assertEquals(testImage[0].width, image.getWidth());
                        assertEquals(testImage[0].height, image.getHeight());
                        return super.getImageData(image, format);
                    }
                };
        try {
            byte[] result = mTestDevice.compressRawImage(testImage[0], "PNG", false);
            assertNotNull(result);
        } finally {
            FileUtil.recursiveDelete(testImageFile.getParentFile());
            testImage[0].data = null;
            testImage[0] = null;
        }
    }

    /** Test for {@link TestDevice#getKeyguardState()} produces the proper output. */
    public void testGetKeyguardState() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "KeyguardController:\n"
                                + "  mKeyguardShowing=true\n"
                                + "  mKeyguardGoingAway=false\n"
                                + "  mOccluded=false\n";
                    }
                };
        KeyguardControllerState state = mTestDevice.getKeyguardState();
        Assert.assertTrue(state.isKeyguardShowing());
        Assert.assertFalse(state.isKeyguardOccluded());
    }

    /** New output of dumpsys is not as clean and has stuff in front. */
    public void testGetKeyguardState_new() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "isHomeRecentsComponent=true  KeyguardController:\n"
                                + "  mKeyguardShowing=true\n"
                                + "  mKeyguardGoingAway=false\n"
                                + "  mOccluded=false\n";
                    }
                };
        KeyguardControllerState state = mTestDevice.getKeyguardState();
        Assert.assertTrue(state.isKeyguardShowing());
        Assert.assertFalse(state.isKeyguardOccluded());
    }

    /** Test for {@link TestDevice#getKeyguardState()} when the device does not support it. */
    public void testGetKeyguardState_unsupported() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "\n";
                    }
                };
        assertNull(mTestDevice.getKeyguardState());
    }

    public void testSetDeviceOwner_success() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "Success: Device owner set to package ComponentInfo{xxx/yyy}";
                    }
                };
        assertTrue(mTestDevice.setDeviceOwner("xxx/yyy", 0));
    }

    public void testSetDeviceOwner_fail() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "java.lang.IllegalStateException: Trying to set the device owner";
                    }
                };
        assertFalse(mTestDevice.setDeviceOwner("xxx/yyy", 0));
    }

    public void testRemoveAdmin_success() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "Success: Admin removed";
                    }
                };
        assertTrue(mTestDevice.removeAdmin("xxx/yyy", 0));
    }

    public void testRemoveAdmin_fail() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public String executeShellCommand(String command)
                            throws DeviceNotAvailableException {
                        return "java.lang.SecurityException: Attempt to remove non-test admin";
                    }
                };
        assertFalse(mTestDevice.removeAdmin("xxx/yyy", 0));
    }

    public void testRemoveOwners() throws Exception {
        mTestDevice =
                Mockito.spy(
                        new TestableTestDevice() {
                            @Override
                            public String executeShellCommand(String command)
                                    throws DeviceNotAvailableException {
                                return "Current Device Policy Manager state:\n"
                                        + "  Device Owner: \n"
                                        + "    admin=ComponentInfo{aaa/aaa}\n"
                                        + "    name=\n"
                                        + "    package=aaa\n"
                                        + "    User ID: 0\n"
                                        + "\n"
                                        + "  Profile Owner (User 10): \n"
                                        + "    admin=ComponentInfo{bbb/bbb}\n"
                                        + "    name=bbb\n"
                                        + "    package=bbb\n";
                            }
                        });
        mTestDevice.removeOwners();

        // Verified removeAdmin is called to remove owners.
        ArgumentCaptor<String> stringCaptor = ArgumentCaptor.forClass(String.class);
        ArgumentCaptor<Integer> intCaptor = ArgumentCaptor.forClass(Integer.class);
        Mockito.verify(mTestDevice, Mockito.times(2))
                .removeAdmin(stringCaptor.capture(), intCaptor.capture());
        List<String> stringArgs = stringCaptor.getAllValues();
        List<Integer> intArgs = intCaptor.getAllValues();

        assertEquals("aaa/aaa", stringArgs.get(0));
        assertEquals(Integer.valueOf(0), intArgs.get(0));

        assertEquals("bbb/bbb", stringArgs.get(1));
        assertEquals(Integer.valueOf(10), intArgs.get(1));
    }

    /** Test that the output of cryptfs allows for encryption for newest format. */
    public void testIsEncryptionSupported_newformat() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public boolean isAdbRoot() throws DeviceNotAvailableException {
                        return true;
                    }

                    @Override
                    public boolean enableAdbRoot() throws DeviceNotAvailableException {
                        return true;
                    }
                };
        injectShellResponse(
                "vdc cryptfs enablecrypto",
                "500 8674 Usage with ext4crypt: cryptfs enablecrypto inplace default noui\r\n");
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        assertTrue(mTestDevice.isEncryptionSupported());
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /** Test that the output of cryptfs does not allow for encryption. */
    public void testIsEncryptionSupported_failure() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public boolean isAdbRoot() throws DeviceNotAvailableException {
                        return true;
                    }

                    @Override
                    public boolean enableAdbRoot() throws DeviceNotAvailableException {
                        return true;
                    }
                };
        injectShellResponse("vdc cryptfs enablecrypto", "500 8674 Command not recognized\r\n");
        EasyMock.replay(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
        assertFalse(mTestDevice.isEncryptionSupported());
        EasyMock.verify(mMockIDevice, mMockStateMonitor, mMockDvcMonitor);
    }

    /** Test when getting the heapdump is successful. */
    public void testGetHeapDump() throws Exception {
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public File pullFile(String remoteFilePath) throws DeviceNotAvailableException {
                        return new File("test");
                    }
                };
        injectShellResponse("pidof system_server", "929");
        injectShellResponse("am dumpheap 929 /data/dump.hprof", "");
        injectShellResponse("ls \"/data/dump.hprof\"", "/data/dump.hprof");
        injectShellResponse("rm /data/dump.hprof", "");
        EasyMock.replay(mMockIDevice, mMockRunUtil);
        File res = mTestDevice.dumpHeap("system_server", "/data/dump.hprof");
        assertNotNull(res);
        EasyMock.verify(mMockIDevice, mMockRunUtil);
    }

    /** Test when we fail to get the process pid. */
    public void testGetHeapDump_nopid() throws Exception {
        injectShellResponse("pidof system_server", "\n");
        EasyMock.replay(mMockIDevice, mMockRunUtil);
        File res = mTestDevice.dumpHeap("system_server", "/data/dump.hprof");
        assertNull(res);
        EasyMock.verify(mMockIDevice, mMockRunUtil);
    }

    public void testGetHeapDump_nullPath() throws DeviceNotAvailableException {
        try {
            mTestDevice.dumpHeap("system_server", null);
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            // expected
        }
    }

    public void testGetHeapDump_emptyPath() throws DeviceNotAvailableException {
        try {
            mTestDevice.dumpHeap("system_server", "");
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            // expected
        }
    }

    public void testGetHeapDump_nullService() throws DeviceNotAvailableException {
        try {
            mTestDevice.dumpHeap(null, "/data/hprof");
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            // expected
        }
    }

    public void testGetHeapDump_emptyService() throws DeviceNotAvailableException {
        try {
            mTestDevice.dumpHeap("", "/data/hprof");
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            // expected
        }
    }

    /**
     * Test that the wifi helper is uninstalled at postInvocationTearDown if it was installed
     * before.
     */
    public void testPostInvocationWifiTearDown() throws Exception {
        // A TestDevice with a no-op recoverDevice() implementation
        mTestDevice =
                new TestableTestDevice() {
                    @Override
                    public void recoverDevice() throws DeviceNotAvailableException {
                        // ignore
                    }

                    @Override
                    public String installPackage(
                            File packageFile, boolean reinstall, String... extraArgs)
                            throws DeviceNotAvailableException {
                        // Fake install is successfull
                        return null;
                    }

                    @Override
                    IWifiHelper createWifiHelper() throws DeviceNotAvailableException {
                        super.createWifiHelper();
                        return mMockWifi;
                    }

                    @Override
                    IWifiHelper createWifiHelper(boolean doSetup)
                            throws DeviceNotAvailableException {
                        return mMockWifi;
                    }
                };
        mMockIDevice.executeShellCommand(
                EasyMock.eq("dumpsys package com.android.tradefed.utils.wifi"),
                EasyMock.anyObject(),
                EasyMock.anyLong(),
                EasyMock.anyObject());
        EasyMock.expect(mMockWifi.getIpAddress()).andReturn("ip");
        // Wifi is cleaned up by the post invocation tear down.
        mMockWifi.cleanUp();
        replayMocks();
        mTestDevice.getIpAddress();
        mTestDevice.postInvocationTearDown();
        verifyMocks();
    }
}
