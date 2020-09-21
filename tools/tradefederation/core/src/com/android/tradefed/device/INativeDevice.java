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
import com.android.ddmlib.Log.LogLevel;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.device.ITestDevice.MountPointInfo;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.result.ITestLifeCycleReceiver;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.Bugreport;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.ProcessInfo;
import com.android.tradefed.util.TimeUtil;

import com.google.errorprone.annotations.MustBeClosed;

import java.io.File;
import java.io.InputStream;
import java.util.Collection;
import java.util.Date;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * Provides an reliable and slightly higher level API to a ddmlib {@link IDevice}.
 * <p/>
 * Retries device commands for a configurable amount, and provides a device recovery
 * interface for devices which are unresponsive.
 */
public interface INativeDevice {

    /**
     * Default value when API Level cannot be detected
     */
    public final static int UNKNOWN_API_LEVEL = -1;

    /**
     * Set the {@link TestDeviceOptions} for the device
     */
    public void setOptions(TestDeviceOptions options);

    /**
     * Returns a reference to the associated ddmlib {@link IDevice}.
     * <p/>
     * A new {@link IDevice} may be allocated by DDMS each time the device disconnects and
     * reconnects from adb. Thus callers should not keep a reference to the {@link IDevice},
     * because that reference may become stale.
     *
     * @return the {@link IDevice}
     */
    public IDevice getIDevice();

    /**
     * Convenience method to get serial number of this device.
     *
     * @return the {@link String} serial number
     */
    public String getSerialNumber();

    /**
     * Retrieve the given property value from the device.
     *
     * @param name the property name
     * @return the property value or <code>null</code> if it does not exist
     * @throws DeviceNotAvailableException
     */
    public String getProperty(String name) throws DeviceNotAvailableException;

    /**
     * Convenience method to get the bootloader version of this device.
     * <p/>
     * Will attempt to retrieve bootloader version from the device's current state. (ie if device
     * is in fastboot mode, it will attempt to retrieve version from fastboot)
     *
     * @return the {@link String} bootloader version or <code>null</code> if it cannot be found
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     */
    public String getBootloaderVersion() throws DeviceNotAvailableException;

    /**
     * Convenience method to get baseband (radio) version of this device. Getting the radio version
     * is device specific, so it might not return the correct information for all devices. This
     * method relies on the gsm.version.baseband propery to return the correct version information.
     * This is not accurate for some CDMA devices and the version returned here might not match
     * the version reported from fastboot and might not return the version for the CDMA radio.
     * TL;DR this method only reports accurate version if the gsm.version.baseband property is the
     * same as the version returned by <code>fastboot getvar version-baseband</code>.
     *
     * @return the {@link String} baseband version or <code>null</code> if it cannot be determined
     *          (device has no radio or version string cannot be read)
     * @throws DeviceNotAvailableException if the connection with the device is lost and cannot
     *          be recovered.
     */
    public String getBasebandVersion() throws DeviceNotAvailableException;

    /**
     * Convenience method to get the product type of this device.
     * <p/>
     * This method will work if device is in either adb or fastboot mode.
     *
     * @return the {@link String} product type name. Will not be null
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered, or if product type can not be determined
     */
    public String getProductType() throws DeviceNotAvailableException;

    /**
     * Convenience method to get the product variant of this device.
     * <p/>
     * This method will work if device is in either adb or fastboot mode.
     *
     * @return the {@link String} product variant name or <code>null</code> if it cannot be
     *         determined
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     */
    public String getProductVariant() throws DeviceNotAvailableException;

    /**
     * Convenience method to get the product type of this device when its in fastboot mode.
     * <p/>
     * This method should only be used if device should be in fastboot. Its a bit safer variant
     * than the generic {@link #getProductType()} method in this case, because ITestDevice
     * will know to recover device into fastboot if device is in incorrect state or is
     * unresponsive.
     *
     * @return the {@link String} product type name or <code>null</code> if it cannot be determined
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     */
    public String getFastbootProductType() throws DeviceNotAvailableException;

    /**
     * Convenience method to get the product type of this device when its in fastboot mode.
     * <p/>
     * This method should only be used if device should be in fastboot. Its a bit safer variant
     * than the generic {@link #getProductType()} method in this case, because ITestDevice
     * will know to recover device into fastboot if device is in incorrect state or is
     * unresponsive.
     *
     * @return the {@link String} product type name or <code>null</code> if it cannot be determined
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     */
    public String getFastbootProductVariant() throws DeviceNotAvailableException;

    /**
     * Retrieve the alias of the build that the device is currently running.
     *
     * <p>Build alias is usually a more readable string than build id (typically a number for
     * Nexus builds). For example, final Android 4.2 release has build alias JDQ39, and build id
     * 573038
     * @return the build alias or fall back to build id if it could not be retrieved
     * @throws DeviceNotAvailableException
     */
    public String getBuildAlias() throws DeviceNotAvailableException;

    /**
     * Retrieve the build the device is currently running.
     *
     * @return the build id or {@link IBuildInfo#UNKNOWN_BUILD_ID} if it could not be retrieved
     * @throws DeviceNotAvailableException
     */
    public String getBuildId() throws DeviceNotAvailableException;

    /**
     * Retrieve the build flavor for the device.
     *
     * @return the build flavor or null if it could not be retrieved
     * @throws DeviceNotAvailableException
     */
    public String getBuildFlavor() throws DeviceNotAvailableException;

    /**
     * Executes the given adb shell command, retrying multiple times if command fails.
     * <p/>
     * A simpler form of
     * {@link #executeShellCommand(String, IShellOutputReceiver, long, TimeUnit, int)} with
     * default values.
     *
     * @param command the adb shell command to run
     * @param receiver the {@link IShellOutputReceiver} to direct shell output to.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     */
    public void executeShellCommand(String command, IShellOutputReceiver receiver)
        throws DeviceNotAvailableException;

    /**
     * Executes a adb shell command, with more parameters to control command behavior.
     *
     * @see #executeShellCommand(String, IShellOutputReceiver)
     * @param command the adb shell command to run
     * @param receiver the {@link IShellOutputReceiver} to direct shell output to.
     * @param maxTimeToOutputShellResponse the maximum amount of time during which the command is
     *            allowed to not output any response; unit as specified in <code>timeUnit</code>
     * @param timeUnit unit for <code>maxTimeToOutputShellResponse</code>
     * @param retryAttempts the maximum number of times to retry command if it fails due to a
     *            exception. DeviceNotResponsiveException will be thrown if <var>retryAttempts</var>
     *            are performed without success.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     * @see TimeUtil
     */
    public void executeShellCommand(String command, IShellOutputReceiver receiver,
            long maxTimeToOutputShellResponse, TimeUnit timeUnit, int retryAttempts)
                    throws DeviceNotAvailableException;

    /**
     * Executes a adb shell command, with more parameters to control command behavior.
     *
     * @see #executeShellCommand(String, IShellOutputReceiver)
     * @param command the adb shell command to run
     * @param receiver the {@link IShellOutputReceiver} to direct shell output to.
     * @param maxTimeoutForCommand the maximum timeout for the command to complete; unit as
     *     specified in <code>timeUnit</code>
     * @param maxTimeToOutputShellResponse the maximum amount of time during which the command is
     *     allowed to not output any response; unit as specified in <code>timeUnit</code>
     * @param timeUnit unit for <code>maxTimeToOutputShellResponse</code>
     * @param retryAttempts the maximum number of times to retry command if it fails due to a
     *     exception. DeviceNotResponsiveException will be thrown if <var>retryAttempts</var> are
     *     performed without success.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *     recovered.
     * @see TimeUtil
     */
    public void executeShellCommand(
            String command,
            IShellOutputReceiver receiver,
            long maxTimeoutForCommand,
            long maxTimeToOutputShellResponse,
            TimeUnit timeUnit,
            int retryAttempts)
            throws DeviceNotAvailableException;

    /**
     * Helper method which executes a adb shell command and returns output as a {@link String}.
     *
     * @param command the adb shell command to run
     * @return the shell output
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public String executeShellCommand(String command) throws DeviceNotAvailableException;

    /**
     * Helper method which executes a adb shell command and returns the results as a {@link
     * CommandResult} properly populated with the command status output, stdout and stderr.
     *
     * @param command The command that should be run.
     * @return The result in {@link CommandResult}.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *     recovered.
     */
    public CommandResult executeShellV2Command(String command) throws DeviceNotAvailableException;

    /**
     * Executes a adb shell command, with more parameters to control command behavior.
     *
     * @see #executeShellV2Command(String)
     * @param command the adb shell command to run
     * @param maxTimeoutForCommand the maximum timeout for the command to complete; unit as
     *     specified in <code>timeUnit</code>
     * @param timeUnit unit for <code>maxTimeToOutputShellResponse</code>
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *     recovered.
     * @see TimeUtil
     */
    public CommandResult executeShellV2Command(
            String command, final long maxTimeoutForCommand, final TimeUnit timeUnit)
            throws DeviceNotAvailableException;

    /**
     * Executes a adb shell command, with more parameters to control command behavior.
     *
     * @see #executeShellV2Command(String)
     * @param command the adb shell command to run
     * @param maxTimeoutForCommand the maximum timeout for the command to complete; unit as
     *     specified in <code>timeUnit</code>
     * @param timeUnit unit for <code>maxTimeToOutputShellResponse</code>
     * @param retryAttempts the maximum number of times to retry command if it fails due to a
     *     exception. DeviceNotResponsiveException will be thrown if <var>retryAttempts</var> are
     *     performed without success.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *     recovered.
     * @see TimeUtil
     */
    public CommandResult executeShellV2Command(
            String command,
            final long maxTimeoutForCommand,
            final TimeUnit timeUnit,
            int retryAttempts)
            throws DeviceNotAvailableException;

    /**
     * Helper method which executes a adb command as a system command.
     * <p/>
     * {@link #executeShellCommand(String)} should be used instead wherever possible, as that
     * method provides better failure detection and performance.
     *
     * @param commandArgs the adb command and arguments to run
     * @return the stdout from command. <code>null</code> if command failed to execute.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public String executeAdbCommand(String... commandArgs) throws DeviceNotAvailableException;

    /**
     * Helper method which executes a fastboot command as a system command with a default timeout
     * of 2 minutes.
     * <p/>
     * Expected to be used when device is already in fastboot mode.
     *
     * @param commandArgs the fastboot command and arguments to run
     * @return the CommandResult containing output of command
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public CommandResult executeFastbootCommand(String... commandArgs)
            throws DeviceNotAvailableException;

    /**
     * Helper method which executes a fastboot command as a system command.
     * <p/>
     * Expected to be used when device is already in fastboot mode.
     *
     * @param timeout the time in milliseconds before the command expire
     * @param commandArgs the fastboot command and arguments to run
     * @return the CommandResult containing output of command
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public CommandResult executeFastbootCommand(long timeout, String... commandArgs)
            throws DeviceNotAvailableException;

    /**
     * Helper method which executes a long running fastboot command as a system command.
     * <p/>
     * Identical to {@link #executeFastbootCommand(String...)} except uses a longer timeout.
     *
     * @param commandArgs the fastboot command and arguments to run
     * @return the CommandResult containing output of command
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public CommandResult executeLongFastbootCommand(String... commandArgs)
            throws DeviceNotAvailableException;

    /**
     * Get whether to use fastboot erase or fastboot format to wipe a partition on the device.
     *
     * @return {@code true} if fastboot erase will be used or {@code false} if fastboot format will
     * be used.
     * @see #fastbootWipePartition(String)
     */
    public boolean getUseFastbootErase();

    /**
     * Set whether to use fastboot erase or fastboot format to wipe a partition on the device.
     *
     * @param useFastbootErase {@code true} if fastboot erase should be used or {@code false} if
     * fastboot format should be used.
     * @see #fastbootWipePartition(String)
     */
    public void setUseFastbootErase(boolean useFastbootErase);

    /**
     * Helper method which wipes a partition for the device.
     * <p/>
     * If {@link #getUseFastbootErase()} is {@code true}, then fastboot erase will be used to wipe
     * the partition. The device must then create a filesystem the next time the device boots.
     * Otherwise, fastboot format is used which will create a new filesystem on the device.
     * <p/>
     * Expected to be used when device is already in fastboot mode.
     *
     * @param partition the partition to wipe
     * @return the CommandResult containing output of command
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public CommandResult fastbootWipePartition(String partition) throws DeviceNotAvailableException;

    /**
     * Runs instrumentation tests, and provides device recovery.
     *
     * <p>If connection with device is lost before test run completes, and recovery succeeds, all
     * listeners will be informed of testRunFailed and "false" will be returned. The test command
     * will not be rerun. It is left to callers to retry if necessary.
     *
     * <p>If connection with device is lost before test run completes, and recovery fails, all
     * listeners will be informed of testRunFailed and DeviceNotAvailableException will be thrown.
     *
     * @param runner the {@link IRemoteAndroidTestRunner} which runs the tests
     * @param listeners the test result listeners
     * @return <code>true</code> if test command completed. <code>false</code> if it failed to
     *     complete due to device communication exception, but recovery succeeded
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *     recovered. ie test command failed to complete and recovery failed.
     */
    public boolean runInstrumentationTests(
            IRemoteAndroidTestRunner runner, Collection<ITestLifeCycleReceiver> listeners)
            throws DeviceNotAvailableException;

    /**
     * Convenience method for performing {@link #runInstrumentationTests(IRemoteAndroidTestRunner,
     * Collection)} with one or more listeners passed as parameters.
     *
     * @param runner the {@link IRemoteAndroidTestRunner} which runs the tests
     * @param listeners the test result listener(s)
     * @return <code>true</code> if test command completed. <code>false</code> if it failed to
     *     complete, but recovery succeeded
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *     recovered. ie test command failed to complete and recovery failed.
     */
    public boolean runInstrumentationTests(
            IRemoteAndroidTestRunner runner, ITestLifeCycleReceiver... listeners)
            throws DeviceNotAvailableException;

    /**
     * Same as {@link ITestDevice#runInstrumentationTests(IRemoteAndroidTestRunner, Collection)} but
     * runs the test for the given user.
     */
    public boolean runInstrumentationTestsAsUser(
            IRemoteAndroidTestRunner runner,
            int userId,
            Collection<ITestLifeCycleReceiver> listeners)
            throws DeviceNotAvailableException;

    /**
     * Same as {@link ITestDevice#runInstrumentationTests(IRemoteAndroidTestRunner,
     * ITestLifeCycleReceiver...)} but runs the test for a given user.
     */
    public boolean runInstrumentationTestsAsUser(
            IRemoteAndroidTestRunner runner, int userId, ITestLifeCycleReceiver... listeners)
            throws DeviceNotAvailableException;

    /**
     * Check whether platform on device supports runtime permission granting
     * @return True if runtime permission are supported, false otherwise.
     * @throws DeviceNotAvailableException
     */
    public boolean isRuntimePermissionSupported() throws DeviceNotAvailableException;

    /**
     * Retrieves a file off device.
     *
     * @param remoteFilePath the absolute path to file on device.
     * @param localFile the local file to store contents in. If non-empty, contents will be
     *            replaced.
     * @return <code>true</code> if file was retrieved successfully. <code>false</code> otherwise.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     */
    public boolean pullFile(String remoteFilePath, File localFile)
            throws DeviceNotAvailableException;

    /**
     * Retrieves a file off device, stores it in a local temporary {@link File}, and returns that
     * {@code File}.
     *
     * @param remoteFilePath the absolute path to file on device.
     * @return A {@link File} containing the contents of the device file, or {@code null} if the
     *         copy failed for any reason (including problems with the host filesystem)
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     */
    public File pullFile(String remoteFilePath) throws DeviceNotAvailableException;

    /**
     * Retrieves a file off device, and returns the contents.
     *
     * @param remoteFilePath the absolute path to file on device.
     * @return A {@link String} containing the contents of the device file, or {@code null} if the
     *         copy failed for any reason (including problems with the host filesystem)
     */
    public String pullFileContents(String remoteFilePath) throws DeviceNotAvailableException;

    /**
     * A convenience method to retrieve a file from the device's external storage, stores it in a
     * local temporary {@link File}, and return a reference to that {@code File}.
     *
     * @param remoteFilePath the path to file on device, relative to the device's external storage
     *        mountpoint
     * @return A {@link File} containing the contents of the device file, or {@code null} if the
     *         copy failed for any reason (including problems with the host filesystem)
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     */
    public File pullFileFromExternal(String remoteFilePath) throws DeviceNotAvailableException;

    /**
     * Recursively pull directory contents from device.
     *
     * @param deviceFilePath the absolute file path of the remote source
     * @param localDir the local directory to pull files into
     * @return <code>true</code> if file was pulled successfully. <code>false</code> otherwise.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public boolean pullDir(String deviceFilePath, File localDir)
            throws DeviceNotAvailableException;

    /**
     * Push a file to device
     *
     * @param localFile the local file to push
     * @param deviceFilePath the remote destination absolute file path
     * @return <code>true</code> if file was pushed successfully. <code>false</code> otherwise.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public boolean pushFile(File localFile, String deviceFilePath)
            throws DeviceNotAvailableException;

    /**
     * Push file created from a string to device
     *
     * @param contents the contents of the file to push
     * @param deviceFilePath the remote destination absolute file path
     * @return <code>true</code> if string was pushed successfully. <code>false</code> otherwise.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public boolean pushString(String contents, String deviceFilePath)
            throws DeviceNotAvailableException;

    /**
     * Recursively push directory contents to device.
     *
     * @param localDir the local directory to push
     * @param deviceFilePath the absolute file path of the remote destination
     * @return <code>true</code> if file was pushed successfully. <code>false</code> otherwise.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public boolean pushDir(File localDir, String deviceFilePath)
            throws DeviceNotAvailableException;

    /**
     * Incrementally syncs the contents of a local file directory to device.
     * <p/>
     * Decides which files to push by comparing timestamps of local files with their remote
     * equivalents. Only 'newer' or non-existent files will be pushed to device. Thus overhead
     * should be relatively small if file set on device is already up to date.
     * <p/>
     * Hidden files (with names starting with ".") will be ignored.
     * <p/>
     * Example usage: syncFiles("/tmp/files", "/sdcard") will created a /sdcard/files directory if
     * it doesn't already exist, and recursively push the /tmp/files contents to /sdcard/files.
     *
     * @param localFileDir the local file directory containing files to recursively push.
     * @param deviceFilePath the remote destination absolute file path root. All directories in thos
     *            file path must be readable. ie pushing to /data/local/tmp when adb is not root
     *            will fail
     * @return <code>true</code> if files were synced successfully. <code>false</code> otherwise.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *             recovered.
     */
    public boolean syncFiles(File localFileDir, String deviceFilePath)
            throws DeviceNotAvailableException;

    /**
     * Helper method to determine if file on device exists.
     *
     * @param deviceFilePath the absolute path of file on device to check
     * @return <code>true</code> if file exists, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public boolean doesFileExist(String deviceFilePath) throws DeviceNotAvailableException;

    /**
     * Retrieve a reference to a remote file on device.
     *
     * @param path the file path to retrieve. Can be an absolute path or path relative to '/'. (ie
     *            both "/system" and "system" syntax is supported)
     * @return the {@link IFileEntry} or <code>null</code> if file at given <var>path</var> cannot
     *         be found
     * @throws DeviceNotAvailableException
     */
    public IFileEntry getFileEntry(String path) throws DeviceNotAvailableException;

    /**
     * Return True if the path on the device is a directory, false otherwise.
     *
     * @throws DeviceNotAvailableException
     */
    public boolean isDirectory(String deviceFilePath) throws DeviceNotAvailableException;

    /**
     * Alternative to using {@link IFileEntry} that sometimes won't work because of permissions.
     *
     * @param deviceFilePath is the path on the device where to do the search
     * @return Array of string containing all the file in a path on the device.
     * @throws DeviceNotAvailableException
     */
    public String[] getChildren(String deviceFilePath) throws DeviceNotAvailableException;

    /**
     * Helper method to determine amount of free space on device external storage.
     *
     * @return the amount of free space in KB
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public long getExternalStoreFreeSpace() throws DeviceNotAvailableException;

    /**
     * Helper method to determine amount of free space on device partition.
     *
     * @return the amount of free space in KB
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *     recovered.
     */
    public long getPartitionFreeSpace(String partition) throws DeviceNotAvailableException;

    /**
     * Returns a mount point.
     * <p/>
     * Queries the device directly if the cached info in {@link IDevice} is not available.
     * <p/>
     * TODO: move this behavior to {@link IDevice#getMountPoint(String)}
     *
     * @param mountName the name of the mount point
     * @return the mount point or <code>null</code>
     * @see IDevice#getMountPoint(String)
     */
    public String getMountPoint(String mountName);

    /**
     * Returns a parsed version of the information in /proc/mounts on the device
     *
     * @return A {@link List} of {@link MountPointInfo} containing the information in "/proc/mounts"
     */
    public List<MountPointInfo> getMountPointInfo() throws DeviceNotAvailableException;

    /**
     * Returns a {@link MountPointInfo} corresponding to the specified mountpoint path, or
     * <code>null</code> if that path has nothing mounted or otherwise does not appear in
     * /proc/mounts as a mountpoint.
     *
     * @return A {@link List} of {@link MountPointInfo} containing the information in "/proc/mounts"
     * @see #getMountPointInfo()
     */
    public MountPointInfo getMountPointInfo(String mountpoint) throws DeviceNotAvailableException;

    /**
     * Start capturing logcat output from device in the background.
     * <p/>
     * Will have no effect if logcat output is already being captured.
     * Data can be later retrieved via getLogcat.
     * <p/>
     * When the device is no longer in use, {@link #stopLogcat()} must be called.
     * <p/>
     * {@link #startLogcat()} and {@link #stopLogcat()} do not normally need to be called when
     * within a TF invocation context, as the TF framework will start and stop logcat.
     */
    public void startLogcat();

    /**
     * Stop capturing logcat output from device, and discard currently saved logcat data.
     * <p/>
     * Will have no effect if logcat output is not being captured.
     */
    public void stopLogcat();

    /**
     * Deletes any accumulated logcat data.
     * <p/>
     * This is useful for cases when you want to ensure {@link ITestDevice#getLogcat()} only returns
     * log data produced after a certain point (such as after flashing a new device build, etc).
     */
    public void clearLogcat();

    /**
     * Grabs a snapshot stream of the logcat data.
     *
     * <p>Works in two modes:
     * <li>If the logcat is currently being captured in the background, will return up to {@link
     *     TestDeviceOptions#getMaxLogcatDataSize()} bytes of the current contents of the background
     *     logcat capture
     * <li>Otherwise, will return a static dump of the logcat data if device is currently responding
     */
    @MustBeClosed
    public InputStreamSource getLogcat();

    /**
     * Grabs a snapshot stream of captured logcat data starting the date provided. The time on the
     * device should be used {@link #getDeviceDate}.
     *
     * <p>
     *
     * @param date in millisecond since epoch format of when to start the snapshot until present.
     *     (can be be obtained using 'date +%s')
     */
    @MustBeClosed
    public InputStreamSource getLogcatSince(long date);

    /**
     * Grabs a snapshot stream of the last <code>maxBytes</code> of captured logcat data.
     *
     * <p>Useful for cases when you want to capture frequent snapshots of the captured logcat data
     * without incurring the potentially big disk space penalty of getting the entire {@link
     * #getLogcat()} snapshot.
     *
     * @param maxBytes the maximum amount of data to return. Should be an amount that can
     *     comfortably fit in memory
     */
    @MustBeClosed
    public InputStreamSource getLogcat(int maxBytes);

    /**
     * Get a dump of the current logcat for device. Unlike {@link #getLogcat()}, this method will
     * always return a static dump of the logcat.
     *
     * <p>Has the disadvantage that nothing will be returned if device is not reachable.
     *
     * @return a {@link InputStreamSource} of the logcat data. An empty stream is returned if fail
     *     to capture logcat data.
     */
    @MustBeClosed
    public InputStreamSource getLogcatDump();

    /**
     * Perform instructions to configure device for testing that after every boot.
     * <p/>
     * Should be called after device is fully booted/available
     * <p/>
     * In normal circumstances this method doesn't need to be called explicitly, as
     * implementations should perform these steps automatically when performing a reboot.
     * <p/>
     * Where it may need to be called is when device reboots due to other events (eg when a
     * fastboot update command has completed)
     *
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public void postBootSetup() throws DeviceNotAvailableException;

    /**
     * Reboots the device into bootloader mode.
     * <p/>
     * Blocks until device is in bootloader mode.
     *
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public void rebootIntoBootloader() throws DeviceNotAvailableException;

    /**
     * Reboots the device into adb mode.
     * <p/>
     * Blocks until device becomes available.
     *
     * @throws DeviceNotAvailableException if device is not available after reboot
     */
    public void reboot() throws DeviceNotAvailableException;

    /**
     * Reboots the device into adb recovery mode.
     * <p/>
     * Blocks until device enters recovery
     *
     * @throws DeviceNotAvailableException if device is not available after reboot
     */
    public void rebootIntoRecovery() throws DeviceNotAvailableException;

    /**
     * An alternate to {@link #reboot()} that only blocks until device is online ie visible to adb.
     *
     * @throws DeviceNotAvailableException if device is not available after reboot
     */
    public void rebootUntilOnline() throws DeviceNotAvailableException;

    /**
     * Issues a command to reboot device and returns on command complete and when device is no
     * longer visible to adb.
     *
     * @throws DeviceNotAvailableException
     */
    public void nonBlockingReboot() throws DeviceNotAvailableException;

    /**
     * Turns on adb root. If the "enable-root" setting is "false", will log a message and
     * return without enabling root.
     * <p/>
     * Enabling adb root may cause device to disconnect from adb. This method will block until
     * device is available.
     *
     * @return <code>true</code> if successful.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public boolean enableAdbRoot() throws DeviceNotAvailableException;

    /**
     * Turns off adb root.
     * <p/>
     * Disabling adb root may cause device to disconnect from adb. This method will block until
     * device is available.
     *
     * @return <code>true</code> if successful.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public boolean disableAdbRoot() throws DeviceNotAvailableException;

    /**
     * @return <code>true</code> if device currently has adb root, <code>false</code> otherwise.
     *
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public boolean isAdbRoot() throws DeviceNotAvailableException;

    /**
     * Encrypts the device.
     * <p/>
     * Encrypting the device may be done inplace or with a wipe.  Inplace encryption will not wipe
     * any data on the device but normally takes a couple orders of magnitude longer than the wipe.
     * <p/>
     * This method will reboot the device if it is not already encrypted and will block until device
     * is online.  Also, it will not decrypt the device after the reboot.  Therefore, the device
     * might not be fully booted and/or ready to be tested when this method returns.
     *
     * @param inplace if the encryption process should take inplace and the device should not be
     * wiped.
     * @return <code>true</code> if successful.
     * @throws DeviceNotAvailableException if device is not available after reboot.
     * @throws UnsupportedOperationException if encryption is not supported on the device.
     */
    public boolean encryptDevice(boolean inplace) throws DeviceNotAvailableException,
            UnsupportedOperationException;

    /**
     * Unencrypts the device.
     * <p/>
     * Unencrypting the device may cause device to be wiped and may reboot device. This method will
     * block until device is available and ready for testing.  Requires fastboot inorder to wipe the
     * userdata partition.
     *
     * @return <code>true</code> if successful.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     * @throws UnsupportedOperationException if encryption is not supported on the device.
     */
    public boolean unencryptDevice() throws DeviceNotAvailableException,
            UnsupportedOperationException;

    /**
     * Unlocks the device if the device is in an encrypted state.
     * </p>
     * This method may restart the framework but will not call {@link #postBootSetup()}. Therefore,
     * the device might not be fully ready to be tested when this method returns.
     *
     * @return <code>true</code> if successful or if the device is unencrypted.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     * @throws UnsupportedOperationException if encryption is not supported on the device.
     */
    public boolean unlockDevice() throws DeviceNotAvailableException,
            UnsupportedOperationException;

    /**
     * Returns if the device is encrypted.
     *
     * @return <code>true</code> if the device is encrypted.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public boolean isDeviceEncrypted() throws DeviceNotAvailableException;

    /**
     * Returns if encryption is supported on the device.
     *
     * @return <code>true</code> if the device supports encryption.
     * @throws DeviceNotAvailableException
     */
    public boolean isEncryptionSupported() throws DeviceNotAvailableException;

    /**
     * Waits for the device to be responsive and available for testing.
     *
     * @param waitTime the time in ms to wait
     * @throws DeviceNotAvailableException if device is still unresponsive after waitTime expires.
     */
    public void waitForDeviceAvailable(final long waitTime) throws DeviceNotAvailableException;

    /**
     * Waits for the device to be responsive and available for testing.  Uses default timeout.
     *
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public void waitForDeviceAvailable() throws DeviceNotAvailableException;

    /**
     * Blocks until device is visible via adb.
     * <p/>
     * Note the device may not necessarily be responsive to commands on completion. Use
     * {@link #waitForDeviceAvailable()} instead.
     *
     * @param waitTime the time in ms to wait
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public void waitForDeviceOnline(final long waitTime) throws DeviceNotAvailableException;

    /**
     * Blocks until device is visible via adb.  Uses default timeout
     * <p/>
     * Note the device may not necessarily be responsive to commands on completion. Use
     * {@link #waitForDeviceAvailable()} instead.
     *
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered.
     */
    public void waitForDeviceOnline() throws DeviceNotAvailableException;

    /**
     * Blocks for the device to be not available ie missing from adb
     *
     * @param waitTime the time in ms to wait
     * @return <code>true</code> if device becomes not available before time expires.
     *         <code>false</code> otherwise
     */
    public boolean waitForDeviceNotAvailable(final long waitTime);

    /**
     * Blocks for the device to be in the 'adb recovery' state (note this is distinct from
     * {@link IDeviceRecovery}).
     *
     * @param waitTime the time in ms to wait
     * @return <code>true</code> if device boots into recovery before time expires.
     *         <code>false</code> otherwise
     */
    public boolean waitForDeviceInRecovery(final long waitTime);

    /**
     * Waits for device to be responsive to a basic adb shell command.
     *
     * @param waitTime the time in ms to wait
     * @return <code>true</code> if device becomes responsive before <var>waitTime</var> elapses.
     */
    public boolean waitForDeviceShell(final long waitTime);

    /**
     * Blocks until the device's boot complete flag is set.
     *
     * @param timeOut time in msecs to wait for the flag to be set
     * @return true if device's boot complete flag is set within the timeout
     * @throws DeviceNotAvailableException
     */
    public boolean waitForBootComplete(long timeOut) throws DeviceNotAvailableException;

    /**
     * Set the {@link IDeviceRecovery} to use for this device. Should be set when device is first
     * allocated.
     *
     * @param recovery the {@link IDeviceRecovery}
     */
    public void setRecovery(IDeviceRecovery recovery);

    /**
     * Set the current recovery mode to use for the device.
     * <p/>
     * Used to control what recovery method to use when a device communication problem is
     * encountered. Its recommended to only use this method sparingly when needed (for example,
     * when framework is down, etc
     *
     * @param mode whether 'recover till online only' mode should be on or not.
     */
    public void setRecoveryMode(RecoveryMode mode);

    /**
     * Get the current recovery mode used for the device.
     *
     * @return the current recovery mode used for the device.
     */
    public RecoveryMode getRecoveryMode();

    /**
     * Get the device's state.
     */
    public TestDeviceState getDeviceState();

    /**
     * @return <code>true</code> if device is connected to adb-over-tcp, <code>false</code>
     * otherwise.
     */
    public boolean isAdbTcp();

    /**
     * Switch device to adb-over-tcp mode.
     *
     * @return the tcp serial number or <code>null</code> if device could not be switched
     * @throws DeviceNotAvailableException
     */
    public String switchToAdbTcp() throws DeviceNotAvailableException;

    /**
     * Switch device to adb over usb mode.
     *
     * @return <code>true</code> if switch was successful, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException
     */
    public boolean switchToAdbUsb() throws DeviceNotAvailableException;

    /**
     * Get the stream of emulator stdout and stderr
     * @return emulator output
     */
    public InputStreamSource getEmulatorOutput();

    /**
     * Close and delete the emulator output.
     */
    public void stopEmulatorOutput();

    /**
     * Get the device API Level. Defaults to {@link #UNKNOWN_API_LEVEL}.
     *
     * @return an integer indicating the API Level of device
     * @throws DeviceNotAvailableException
     */
    public int getApiLevel() throws DeviceNotAvailableException;

    /**
     * Helper to get the time difference between the device and a given {@link Date}. Use Epoch time
     * internally.
     *
     * @return the difference in milliseconds
     */
    public long getDeviceTimeOffset(Date date) throws DeviceNotAvailableException;

    /**
     * Sets the date on device
     * <p>
     * Note: setting date on device requires root
     * @param date specify a particular date; will use host date if <code>null</code>
     * @throws DeviceNotAvailableException
     */
    public void setDate(Date date) throws DeviceNotAvailableException;

    /**
     * Return the date of the device in millisecond since epoch.
     *
     * <p>
     *
     * @return the date of the device in epoch format.
     * @throws DeviceNotAvailableException
     */
    public long getDeviceDate() throws DeviceNotAvailableException;

    /**
     * Make the system partition on the device writable. May reboot the device.
     * @throws DeviceNotAvailableException
     */
    public void remountSystemWritable() throws DeviceNotAvailableException;

    /**
     * Returns the key type used to sign the device image
     * <p>
     * Typically Android devices may be signed with test-keys (like in AOSP) or release-keys
     * (controlled by individual device manufacturers)
     * @return The signing key if found, null otherwise.
     * @throws DeviceNotAvailableException
     */
    public String getBuildSigningKeys() throws DeviceNotAvailableException;

    /**
     * Retrieves a bugreport from the device.
     * <p/>
     * The implementation of this is guaranteed to continue to work on a device without an sdcard
     * (or where the sdcard is not yet mounted).
     *
     * @return An {@link InputStreamSource} which will produce the bugreport contents on demand.  In
     *         case of failure, the {@code InputStreamSource} will produce an empty
     *         {@link InputStream}.
     */
    public InputStreamSource getBugreport();

    /**
     * Retrieves a bugreportz from the device. Zip format bugreport contains the main bugreport
     * and other log files that are useful for debugging.
     * <p/>
     * Only supported for 'adb version' > 1.0.36
     *
     * @return a {@link InputStreamSource} of the zip file containing the bugreportz, return null
     *         in case of failure.
     */
    public InputStreamSource getBugreportz();

    /**
     * Helper method to take a bugreport and log it to the reporters.
     *
     * @param dataName name under which the bugreport will be reported.
     * @param listener an {@link ITestLogger} to log the bugreport.
     * @return True if the logging was successful, false otherwise.
     */
    public boolean logBugreport(String dataName, ITestLogger listener);

    /**
     * Take a bugreport and returns it inside a {@link Bugreport} object to handle it. Return null
     * in case of issue.
     * </p>
     * File referenced in the Bugreport object need to be cleaned via {@link Bugreport#close()}.
     */
    public Bugreport takeBugreport();

    /**
     * Get the device class.
     *
     * @return the {@link String} device class.
     */
    public String getDeviceClass();

    /**
     * Extra steps for device specific required setup that will be executed on the device prior
     * to the invocation flow.
     */
    public void preInvocationSetup(IBuildInfo info)
            throws TargetSetupError, DeviceNotAvailableException;

    /**
     * Extra steps for device specific required clean up that will be executed after the invocation
     * is done.
     */
    public void postInvocationTearDown();

    /**
     * Return true if the device is headless (no screen), false otherwise.
     */
    public boolean isHeadless() throws DeviceNotAvailableException;

    /**
     * Return a {@link DeviceDescriptor} from the device information to get info on it without
     * passing the actual device object.
     */
    public DeviceDescriptor getDeviceDescriptor();

    /**
     * Helper method runs the "ps" command and returns list of USER, PID and NAME of all the
     * processes.
     *
     * @return List of ProcessInfo objects
     */
    public List<ProcessInfo> getProcesses() throws DeviceNotAvailableException;

    /**
     * Helper method runs the "ps" command and returns USER, PID and NAME of the given process name.
     *
     * @return ProcessInfo of given processName
     */
    public ProcessInfo getProcessByName(String processName) throws DeviceNotAvailableException;

    /** Returns the pid of the service or null if something went wrong. */
    public String getProcessPid(String process) throws DeviceNotAvailableException;

    /**
     * Log a message in the logcat of the device. This is a safe call that will not throw even if
     * the logging fails.
     *
     * @param tag The tag under which we log our message in the logcat.
     * @param level The debug level of the message in the logcat.
     * @param format The message format.
     * @param args the args to be replaced via String.format().
     */
    public void logOnDevice(String tag, LogLevel level, String format, Object... args);
}
