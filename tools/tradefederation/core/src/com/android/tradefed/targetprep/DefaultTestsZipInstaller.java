/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.ddmlib.FileListingService;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IFileEntry;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;


/**
 * A default implementation of tests zip installer.
 */
public class DefaultTestsZipInstaller implements ITestsZipInstaller {
    private static final int RM_ATTEMPTS = 3;
    private static final String DEVICE_DATA_PATH = buildAbsPath(FileListingService.DIRECTORY_DATA);
    private static final File DEVICE_DATA_FILE = new File(DEVICE_DATA_PATH);

    /**
     * A list of /data subdirectories to NOT wipe when doing UserDataFlashOption.TESTS_ZIP
     */
    private Set<String> mDataWipeSkipList;

    /**
     * Default constructor.
     */
    public DefaultTestsZipInstaller() {
    }

    /**
     * This convenience constructor allows the caller to set the skip list directly, rather than
     * needing to call {@link #setDataWipeSkipList} separately.
     *
     * @param skipList The collection of paths under {@code /data} to keep when clearing the
     * filesystem @see #setDataWipeSkipList
     */
    public DefaultTestsZipInstaller(Collection<String> skipList) {
        setDataWipeSkipList(skipList);
    }

    /**
     * This convenience constructor allows the caller to set the skip list directly, rather than
     * needing to call {@link #setDataWipeSkipList} separately.
     *
     * @param skipList The collection of paths under {@code /data} to keep when clearing the
     * filesystem @see #setDataWipeSkipList
     */
    public DefaultTestsZipInstaller(String... skipList) {
        setDataWipeSkipList(skipList);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDataWipeSkipList(Collection<String> skipList) {
        mDataWipeSkipList = new HashSet<String>(skipList.size());
        mDataWipeSkipList.addAll(skipList);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDataWipeSkipList(String... skipList) {
        mDataWipeSkipList = new HashSet<String>(skipList.length);
        mDataWipeSkipList.addAll(Arrays.asList(skipList));
    }

    /**
     * Get the directory of directories to wipe, used for testing only.
     * @return the set of directories to skip when wiping a directory
     */
    public Set<String> getDataWipeSkipList() {
        return mDataWipeSkipList;
    }

    /**
     * {@inheritDoc}
     * <p>
     * This implementation will reboot the device into userland before
     * proceeding. It will also stop the Android runtime and leave it down upon return
     */
    @Override
    public void pushTestsZipOntoData(ITestDevice device, IDeviceBuildInfo deviceBuild)
            throws DeviceNotAvailableException, TargetSetupError {
        CLog.i(String.format("Pushing test zips content onto userdata on %s",
                device.getSerialNumber()));

        RecoveryMode cachedRecoveryMode = device.getRecoveryMode();
        device.setRecoveryMode(RecoveryMode.ONLINE);

        doDeleteData(device);

        CLog.d("Syncing test files/apks");
        File hostDir = new File(deviceBuild.getTestsDir(), "DATA");

        File[] hostDataFiles = getTestsZipDataFiles(hostDir, device);
        for (File hostSubDir : hostDataFiles) {
            device.syncFiles(hostSubDir, DEVICE_DATA_PATH);
        }

        // FIXME: this may end up mixing host slashes and device slashes
        for (File dir : findDirs(hostDir, DEVICE_DATA_FILE)) {
            device.executeShellCommand("chown system.system " + dir.getPath());
        }

        device.setRecoveryMode(cachedRecoveryMode);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void deleteData(ITestDevice device) throws DeviceNotAvailableException,
            TargetSetupError {
        RecoveryMode cachedRecoveryMode = device.getRecoveryMode();
        device.setRecoveryMode(RecoveryMode.ONLINE);

        doDeleteData(device);

        device.setRecoveryMode(cachedRecoveryMode);
    }

    /**
     * Deletes userdata from device without toggling {@link RecoveryMode}.
     * <p/>
     * Expects callers to have set device to {@link RecoveryMode#ONLINE}.
     */
    private void doDeleteData(ITestDevice device) throws DeviceNotAvailableException,
            TargetSetupError {
        // Stop the runtime, so it doesn't notice us mucking with the filesystem
        device.executeShellCommand("stop");
        // Stop installd to prevent it from writing to /data/data
        device.executeShellCommand("stop installd");

        CLog.d("clearing " + FileListingService.DIRECTORY_DATA + " directory on device "
                + device.getSerialNumber());

        // Touch a file so that we can make sure the filesystem is mounted and r/w and usable.  If
        // this method is a no-op, then the filesystem might be corrupt and mounted r/o, or might
        // not be mounted at all.
        String turtlePath = buildRelPath(DEVICE_DATA_PATH,
                String.format("turtles-%d.txt", System.currentTimeMillis()));
        boolean yayTurtle = device.pushString("I like turtles", turtlePath);
        if (!yayTurtle) {
            throw new TargetSetupError(String.format("Failed userdata write check on device %s",
                    device.getSerialNumber()), device.getDeviceDescriptor());
        }

        IFileEntry dataEntry = device.getFileEntry(FileListingService.DIRECTORY_DATA);
        if (dataEntry == null) {
            throw new TargetSetupError(String.format("Could not find %s folder on %s",
                    FileListingService.DIRECTORY_DATA, device.getSerialNumber()),
                    device.getDeviceDescriptor());
        }
        for (IFileEntry dataSubDir : dataEntry.getChildren(false)) {
            if (!mDataWipeSkipList.contains(dataSubDir.getName())) {
                deleteDir(device, dataSubDir.getFullEscapedPath());
            }
        }
    }

    /**
     * @param fullEscapedPath
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError
     */
    private void deleteDir(ITestDevice device, String fullEscapedPath)
            throws DeviceNotAvailableException, TargetSetupError {
        String result = "unknown";
        for (int i = 1; i <= RM_ATTEMPTS; i++) {
            result = device.executeShellCommand(String.format("rm -r %s", fullEscapedPath));
            if (!device.doesFileExist(fullEscapedPath)) {
                return;
            }
            CLog.d("Failed to delete dir %s on device %s on attempt %d of %d: stdout: %s",
                    fullEscapedPath, device.getSerialNumber(), i, RM_ATTEMPTS, result);
            // do exponential backoff
            getRunUtil().sleep(1000 * i * i);
        }
        throw new TargetSetupError(String.format("Failed to delete dir %s. rm output: %s",
                fullEscapedPath, result), device.getDeviceDescriptor());
    }

    /**
     * Get the {@link IRunUtil} object to use.
     * <p/>
     * Exposed so unit tests can mock.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    private static String buildRelPath(String... parts) {
        return ArrayUtil.join(FileListingService.FILE_SEPARATOR, (Object[]) parts);
    }

    private static String buildAbsPath(String... parts) {
        return FileListingService.FILE_SEPARATOR + buildRelPath(parts);
    }

    /**
     * Retrieves the set of files contained in given tests.zip/DATA directory.
     * <p/>
     * Exposed so unit tests can mock.
     *
     * @param hostDir the {@link File} directory, representing the local path extracted tests.zip
     *            contents 'DATA' sub-folder
     * @return array of {@link File}
     */
    File[] getTestsZipDataFiles(File hostDir, ITestDevice device) throws TargetSetupError {
        if (!hostDir.isDirectory()) {
            throw new TargetSetupError("Unrecognized tests.zip content: missing DATA folder",
                    device.getDeviceDescriptor());
        }
        File[] childFiles = hostDir.listFiles();
        if (childFiles == null || childFiles.length <= 0) {
            throw new TargetSetupError(
                    "Unrecognized tests.zip content: DATA folder has no content",
                    device.getDeviceDescriptor());
        }
        return childFiles;
    }

    /**
     * Indirection to {@link FileUtil#findDirsUnder(File, File)} to allow for unit testing.
     */
    Set<File> findDirs(File hostDir, File deviceRootPath) {
        return FileUtil.findDirsUnder(hostDir, deviceRootPath);
    }
}
