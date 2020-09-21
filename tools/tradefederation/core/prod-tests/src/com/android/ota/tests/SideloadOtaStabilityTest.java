/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.ota.tests;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.not;
import static org.junit.Assert.assertThat;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.build.OtaDeviceBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IManagedTestDevice;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.log.LogReceiver;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IResumableTest;
import com.android.tradefed.util.DeviceRecoveryModeUtil;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketAddress;
import java.util.HashMap;
import java.util.Map;

/**
 * A test that will perform repeated flash + install OTA actions on a device.
 * <p/>
 * adb must have root.
 * <p/>
 * Expects a {@link OtaDeviceBuildInfo}.
 * <p/>
 * Note: this test assumes that the {@link ITargetPreparer}s included in this test's
 * {@link IConfiguration} will flash the device back to a baseline build, and prepare the device to
 * receive the OTA to a new build.
 */
@OptionClass(alias = "ota-stability")
public class SideloadOtaStabilityTest implements IDeviceTest, IBuildReceiver,
        IConfigurationReceiver, IResumableTest {

    private static final String UNCRYPT_FILE_PATH = "/cache/recovery/uncrypt_file";
    private static final String UNCRYPT_STATUS_FILE = "/cache/recovery/uncrypt_status";
    private static final String BLOCK_MAP_PATH = "@/cache/recovery/block.map";
    private static final String RECOVERY_COMMAND_PATH = "/cache/recovery/command";
    private static final String LOG_RECOV = "/cache/recovery/last_log";
    private static final String LOG_KMSG = "/cache/recovery/last_kmsg";
    private static final String KMSG_CMD = "cat /proc/kmsg";
    private static final long SOCKET_RETRY_CT = 30;
    private static final long UNCRYPT_TIMEOUT = 15 * 60 * 1000;

    private static final long SHORT_WAIT_UNCRYPT = 3 * 1000;

    private OtaDeviceBuildInfo mOtaDeviceBuild;
    private IConfiguration mConfiguration;
    private ITestDevice mDevice;

    @Option(name = "run-name", description =
            "The name of the ota stability test run. Used to report metrics.")
    private String mRunName = "ota-stability";

    @Option(name = "iterations", description =
            "Number of ota stability 'flash + wait for ota' iterations to run.")
    private int mIterations = 20;

    @Option(name = "resume", description = "Resume the ota test run if an device setup error "
            + "stopped the previous test run.")
    private boolean mResumeMode = false;

    @Option(name = "max-install-time", description =
            "The maximum time to wait for an ota to install in seconds.")
    private int mMaxInstallOnlineTimeSec = 5 * 60;

    @Option(name = "package-data-path", description =
            "path on /data for the package to be saved to")
    /* This is currently the only path readable by uncrypt on the userdata partition */
    private String mPackageDataPath = "/data/data/com.google.android.gsf/app_download/update.zip";

    @Option(name = "max-reboot-time", description =
            "The maximum time to wait for a device to reboot out of recovery if it fails")
    private long mMaxRebootTimeSec = 5 * 60;

    @Option(name = "uncrypt-only", description = "if true, only uncrypt then exit")
    private boolean mUncryptOnly = false;

    /** controls if this test should be resumed. Only used if mResumeMode is enabled */
    private boolean mResumable = true;

    private long mUncryptDuration;
    private LogReceiver mKmsgReceiver;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setConfiguration(IConfiguration configuration) {
        mConfiguration = configuration;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mOtaDeviceBuild = (OtaDeviceBuildInfo) buildInfo;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * Set the run name
     */
    void setRunName(String runName) {
        mRunName = runName;
    }

    /**
     * Return the number of iterations.
     * <p/>
     * Exposed for unit testing
     */
    public int getIterations() {
        return mIterations;
    }

    /**
     * Set the iterations
     */
    void setIterations(int iterations) {
        mIterations = iterations;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // started run, turn to off
        mResumable = false;
        mKmsgReceiver = new LogReceiver(getDevice(), KMSG_CMD, "kmsg");
        checkFields();

        CLog.i("Starting OTA sideload test from %s to %s, for %d iterations",
                mOtaDeviceBuild.getDeviceImageVersion(),
                mOtaDeviceBuild.getOtaBuild().getOtaPackageVersion(), mIterations);

        long startTime = System.currentTimeMillis();
        listener.testRunStarted(mRunName, 0);
        int actualIterations = 0;
        BootTimeInfo lastBootTime = null;
        try {
            while (actualIterations < mIterations) {
                if (actualIterations != 0) {
                    // don't need to flash device on first iteration
                    flashDevice();
                }
                lastBootTime = installOta(listener, mOtaDeviceBuild.getOtaBuild());
                actualIterations++;
                CLog.i("Device %s successfully OTA-ed to build %s. Iteration: %d of %d",
                        mDevice.getSerialNumber(),
                        mOtaDeviceBuild.getOtaBuild().getOtaPackageVersion(),
                        actualIterations, mIterations);
            }
        } catch (AssertionError | BuildError e) {
            CLog.e(e);
        } catch (TargetSetupError e) {
            CLog.i("Encountered TargetSetupError, marking this test as resumable");
            mResumable = true;
            CLog.e(e);
            // throw up an exception so this test can be resumed
            Assert.fail(e.toString());
        } finally {
            // if the device is down, we need to recover it so we can safely pull logs
            IManagedTestDevice managedDevice = (IManagedTestDevice) mDevice;
            if (!managedDevice.getDeviceState().equals(TestDeviceState.ONLINE)) {
                // not all IDeviceRecovery implementations can handle getting out of recovery mode,
                // so we should just reboot in that case since we no longer need to be in
                // recovery
                CLog.i("Device is not online, attempting to recover before capturing logs");
                DeviceRecoveryModeUtil.bootOutOfRecovery((IManagedTestDevice) mDevice,
                        mMaxInstallOnlineTimeSec * 1000);
            }
            double updateTime = sendRecoveryLog(listener);
            Map<String, String> metrics = new HashMap<>(1);
            metrics.put("iterations", Integer.toString(actualIterations));
            metrics.put("failed_iterations", Integer.toString(mIterations - actualIterations));
            metrics.put("update_time", Double.toString(updateTime));
            metrics.put("uncrypt_time", Long.toString(mUncryptDuration));
            if (lastBootTime != null) {
                metrics.put("boot_time_online", Double.toString(lastBootTime.mOnlineTime));
                metrics.put("boot_time_available", Double.toString(lastBootTime.mAvailTime));
            }
            long endTime = System.currentTimeMillis() - startTime;
            listener.testRunEnded(endTime, TfMetricProtoUtil.upgradeConvert(metrics));
        }
    }

    /**
     * Flash the device back to baseline build.
     * <p/>
     * Currently does this by re-running {@link ITargetPreparer#setUp(ITestDevice, IBuildInfo)}
     */
    private void flashDevice() throws TargetSetupError, BuildError, DeviceNotAvailableException {
        // assume the target preparers will flash the device back to device build
        for (ITargetPreparer preparer : mConfiguration.getTargetPreparers()) {
            preparer.setUp(mDevice, mOtaDeviceBuild);
        }
    }

    private void checkFields() {
        if (mDevice == null) {
            throw new IllegalArgumentException("missing device");
        }
        if (mConfiguration == null) {
            throw new IllegalArgumentException("missing configuration");
        }
        if (mOtaDeviceBuild == null) {
            throw new IllegalArgumentException("missing build info");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isResumable() {
        return mResumeMode && mResumable;
    }

    /**
     * Actually install the OTA.
     * @param listener
     * @param otaBuild
     * @return the amount of time in ms that the device took to boot after installing.
     * @throws DeviceNotAvailableException
     */
    private BootTimeInfo installOta(ITestInvocationListener listener, IDeviceBuildInfo otaBuild)
            throws DeviceNotAvailableException {
        TestDescription test =
                new TestDescription(getClass().getName(), String.format("apply_ota[%s]", mRunName));
        Map<String, String> metrics = new HashMap<String, String>();
        listener.testStarted(test);
        try {
            mKmsgReceiver.start();
            try {
                CLog.i("Pushing OTA package %s", otaBuild.getOtaPackageFile().getAbsolutePath());
                Assert.assertTrue(mDevice.pushFile(otaBuild.getOtaPackageFile(), mPackageDataPath));
                // this file needs to be uncrypted, since /data isn't mounted in recovery
                // block.map should be empty since cache should be cleared
                mDevice.pushString(mPackageDataPath + "\n", UNCRYPT_FILE_PATH);
                // Flushing the file to flash.
                mDevice.executeShellCommand("sync");

                mUncryptDuration = doUncrypt(SocketFactory.getInstance(), listener);
                metrics.put("uncrypt_duration", Long.toString(mUncryptDuration));
                String installOtaCmd = String.format("--update_package=%s\n", BLOCK_MAP_PATH);
                mDevice.pushString(installOtaCmd, RECOVERY_COMMAND_PATH);
                CLog.i("Rebooting to install OTA");
            } finally {
                // Kmsg contents during the OTA will be capture in last_kmsg, so we can turn off the
                // kmsg receiver now
                mKmsgReceiver.postLog(listener);
                mKmsgReceiver.stop();
            }
            // uncrypt is complete
            if (mUncryptOnly) {
                return new BootTimeInfo(-1, -1);
            }
            try {
                mDevice.rebootIntoRecovery();
            } catch (DeviceNotAvailableException e) {
                // The device will only enter the RECOVERY state if it hits the recovery menu.
                // Since we added a command to /cache/recovery/command, recovery mode executes the
                // command rather than booting into the menu. While applying the update as a result
                // of the installed command, the device reports its state as NOT_AVAILABLE. If the
                // device *actually* becomes unavailable, we will catch the resulting DNAE in the
                // next call to waitForDeviceOnline.
                CLog.i("Didn't go to recovery, went straight to update");
            }

            mDevice.waitForDeviceNotAvailable(mMaxInstallOnlineTimeSec * 1000);
            long start = System.currentTimeMillis();

            try {
                mDevice.waitForDeviceOnline(mMaxInstallOnlineTimeSec * 1000);
            } catch (DeviceNotAvailableException e) {
                CLog.e("Device %s did not come back online after recovery", mDevice.getSerialNumber());
                listener.testFailed(test, e.getLocalizedMessage());
                listener.testRunFailed("Device did not come back online after recovery");
                sendUpdatePackage(listener, otaBuild);
                throw new AssertionError("Device did not come back online after recovery");
            }
            double onlineTime = (System.currentTimeMillis() - start) / 1000.0;
            try {
                mDevice.waitForDeviceAvailable();
            } catch (DeviceNotAvailableException e) {
                CLog.e("Device %s did not boot up successfully after installing OTA",
                        mDevice.getSerialNumber());
                listener.testFailed(test, e.getLocalizedMessage());
                listener.testRunFailed("Device failed to boot after OTA");
                sendUpdatePackage(listener, otaBuild);
                throw new AssertionError("Device failed to boot after OTA");
            }
            double availTime = (System.currentTimeMillis() - start) / 1000.0;
            return new BootTimeInfo(availTime, onlineTime);
        } finally {
            listener.testEnded(test, TfMetricProtoUtil.upgradeConvert(metrics));
        }
    }

    private InputStreamSource pullLogFile(String location)
            throws DeviceNotAvailableException {
        try {
            // get recovery log
            File destFile = FileUtil.createTempFile("recovery", "log");
            boolean gotFile = mDevice.pullFile(location, destFile);
            if (gotFile) {
                return new FileInputStreamSource(destFile, true /* delete */);
            }
        } catch (IOException e) {
            CLog.e("Failed to get recovery log from device %s", mDevice.getSerialNumber());
            CLog.e(e);
        }
        return null;
    }

    protected void sendUpdatePackage(ITestInvocationListener listener, IDeviceBuildInfo otaBuild) {
        InputStreamSource pkgSource = null;
        try {
            pkgSource = new FileInputStreamSource(otaBuild.getOtaPackageFile());
            listener.testLog(mRunName + "_package", LogDataType.ZIP, pkgSource);
        } catch (NullPointerException e) {
            CLog.w("Couldn't save update package due to exception");
            CLog.e(e);
            return;
        } finally {
            StreamUtil.cancel(pkgSource);
        }
    }

    protected double sendRecoveryLog(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        InputStreamSource lastLog = pullLogFile(LOG_RECOV);
        InputStreamSource lastKmsg = pullLogFile(LOG_KMSG);
        InputStreamSource blockMap = pullLogFile(BLOCK_MAP_PATH.substring(1));
        double elapsedTime = 0;
        // last_log contains a timing metric in its last line, capture it here and return it
        // for the metrics map to report
        try {
            if (lastLog == null || lastKmsg == null) {
                CLog.w(
                        "Could not find last_log at directory %s, or last_kmsg at directory %s",
                        LOG_RECOV, LOG_KMSG);
                return elapsedTime;
            }

            try {
                String[] lastLogLines = StreamUtil.getStringFromSource(lastLog).split("\n");
                String endLine = lastLogLines[lastLogLines.length - 1];
                elapsedTime = Double.parseDouble(
                        endLine.substring(endLine.indexOf('[') + 1, endLine.indexOf(']')).trim());
            } catch (IOException | NumberFormatException | NullPointerException e) {
                CLog.w("Couldn't get elapsed time from last_log due to exception");
                CLog.e(e);
            }
            listener.testLog(this.mRunName + "_recovery_log", LogDataType.TEXT,
                    lastLog);
            listener.testLog(this.mRunName + "_recovery_kmsg", LogDataType.TEXT,
                    lastKmsg);
            if (blockMap == null) {
                CLog.w("Could not find block.map");
            } else {
                listener.testLog(this.mRunName + "_block_map", LogDataType.TEXT,
                    blockMap);
            }
            return elapsedTime;
        } finally {
            StreamUtil.cancel(lastLog);
            StreamUtil.cancel(lastKmsg);
            StreamUtil.cancel(blockMap);
        }
    }

    /**
     * Uncrypt needs to attach to a socket before it will actually begin work, so we need to attach
     * a socket to it.
     *
     * @return Elapse time of uncrypt
     */
    public long doUncrypt(
            ISocketFactory sockets, @SuppressWarnings("unused") ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        // init has to start uncrypt or the socket will not be allocated
        CLog.i("Starting uncrypt service");
        mDevice.executeShellCommand("setprop ctl.start uncrypt");
        try {
            // This is a workaround for known issue with f2fs system.
            CLog.i("Sleeping %d for uncrypt to be ready for socket connection.",
                    SHORT_WAIT_UNCRYPT);
            Thread.sleep(SHORT_WAIT_UNCRYPT);
        } catch (InterruptedException e) {
            CLog.i("Got interrupted when waiting to uncrypt file.");
            Thread.currentThread().interrupt();
        }
        // MNC version of uncrypt does not require socket connection
        if (mDevice.getApiLevel() < 24) {
            CLog.i("Waiting for MNC uncrypt service finish");
            return waitForUncrypt();
        }
        int port;
        try {
            port = getFreePort();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        // The socket uncrypt wants to run on is a local unix socket, so we can forward a tcp
        // port to connect to it.
        CLog.i("Connecting to uncrypt on port %d", port);
        mDevice.executeAdbCommand("forward", "tcp:" + port, "localreserved:uncrypt");
        // connect to uncrypt!
        String hostname = "localhost";
        long start = System.currentTimeMillis();
        try (Socket uncrypt = sockets.createClientSocket(hostname, port)) {
            connectSocket(uncrypt, hostname, port);
            try (DataInputStream dis = new DataInputStream(uncrypt.getInputStream());
                 DataOutputStream dos = new DataOutputStream(uncrypt.getOutputStream())) {
                int status = Integer.MIN_VALUE;
                while (true) {
                    status = dis.readInt();
                    if (isUncryptSuccess(status)) {
                        dos.writeInt(0);
                        break;
                    }
                }
                CLog.i("Final uncrypt status: %d", status);
                return System.currentTimeMillis() - start;
            }
        } catch (IOException e) {
            CLog.e("Lost connection with uncrypt due to IOException:");
            CLog.e(e);
            return waitForUncrypt();
        }
    }

    private long waitForUncrypt() throws DeviceNotAvailableException {
        CLog.i("Continuing to watch uncrypt progress for %d ms", UNCRYPT_TIMEOUT);
        long time = 0;
        int lastStatus = -1;
        long start = System.currentTimeMillis();
        while ((time = System.currentTimeMillis() - start) < UNCRYPT_TIMEOUT) {
            int status = readUncryptStatusFromFile();
            if (isUncryptSuccess(status)) {
                return time;
            }
            if (status != lastStatus) {
                lastStatus = status;
                CLog.d("uncrypt status: %d", status);
            }
            try {
                Thread.sleep(1000);
            } catch (InterruptedException unused) {
                Thread.currentThread().interrupt();
            }
        }
        CLog.e("Uncrypt didn't finish, last status was %d", lastStatus);
        throw new RuntimeException("Uncrypt didn't succeed after timeout");
    }

    private boolean isUncryptSuccess(int status) {
        if (status == 100) {
            CLog.i("Uncrypt finished successfully");
            return true;
        } else if (status > 100 || status < 0) {
            CLog.e("Uncrypt returned error status %d", status);
            assertThat(status, not(equalTo(100)));
        }
        return false;
    }

    protected int getFreePort() throws IOException {
        try (ServerSocket sock = new ServerSocket(0)) {
            return sock.getLocalPort();
        }
    }

    /**
     * Read the error status from uncrypt_status on device.
     *
     * @return 0 if uncrypt succeed, -1 if file not exists, specific error code otherwise.
     */
    protected int readUncryptStatusFromFile()
            throws DeviceNotAvailableException {
        try {
            InputStreamSource status = pullLogFile(UNCRYPT_STATUS_FILE);
            if (status == null) {
                return -1;
            }
            String[] lastLogLines = StreamUtil.getStringFromSource(status).split("\n");
            String endLine = lastLogLines[lastLogLines.length - 1];
            if (endLine.toLowerCase().startsWith("uncrypt_error:")) {
                String[] elements = endLine.split(":");
                return Integer.parseInt(elements[1].trim());
            } else {
                // MNC device case
                return Integer.parseInt(endLine.trim());
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    protected void connectSocket(Socket s, String host, int port) {
        SocketAddress address = new InetSocketAddress(host, port);
        boolean connected = false;
        for (int i = 0; i < SOCKET_RETRY_CT; i++) {
            try {
                if (!s.isConnected()) {
                    s.connect(address);
                }
                connected = true;
                break;
            } catch (IOException unused) {
                try {
                    Thread.sleep(1000);
                    CLog.d("Uncrypt socket was not ready on iteration %d of %d, retrying", i,
                            SOCKET_RETRY_CT);
                } catch (InterruptedException e) {
                    CLog.w("Interrupted while connecting uncrypt socket on iteration %d", i);
                    Thread.currentThread().interrupt();
                }
            }
        }
        if (!connected) {
            throw new RuntimeException("failed to connect uncrypt socket");
        }
    }

    /**
     * Provides a client socket. Allows for providing mock sockets to doUncrypt in unit testing.
     */
    public interface ISocketFactory {
        public Socket createClientSocket(String host, int port) throws IOException;
    }

    /**
     * Default implementation of {@link ISocketFactory}, which provides a {@link Socket}.
     */
    protected static class SocketFactory implements ISocketFactory {
        private static SocketFactory sInstance;

        private SocketFactory() {
        }

        public static SocketFactory getInstance() {
            if (sInstance == null) {
                sInstance = new SocketFactory();
            }
            return sInstance;
        }

        @Override
        public Socket createClientSocket(String host, int port) throws IOException {
            return new Socket(host, port);
        }
    }

    private static class BootTimeInfo {
        /**
         * Time (s) until device is completely available
         */
        public double mAvailTime;
        /**
         * Time (s) until device is in "ONLINE" state
         */
        public double mOnlineTime;

        public BootTimeInfo(double avail, double online) {
            mAvailTime = avail;
            mOnlineTime = online;
        }
    }
}
