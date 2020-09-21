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
package com.android.tradefed.targetprep;

import com.android.ddmlib.FileListingService;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

/**
 * A {@link ITargetPreparer} that pushes one or more files/dirs from a {@link
 * IDeviceBuildInfo#getTestsDir()} folder onto device.
 *
 * <p>This preparer will look in alternate directories if the tests zip does not exist or does not
 * contain the required apk. The search will go in order from the last alternative dir specified to
 * the first.
 */
@OptionClass(alias = "tests-zip-file")
public class TestFilePushSetup extends BaseTargetPreparer {

    @Option(name = "test-file-name", description =
            "the relative path of a test zip file/directory to install on device. Can be repeated.",
            importance = Importance.IF_UNSET)
    private Collection<String> mTestPaths = new ArrayList<String>();

    @Option(name = "throw-if-not-found", description =
            "Throw exception if the specified file is not found.")
    private boolean mThrowIfNoFile = true;

    @Option(name = "alt-dir",
            description = "Alternate directory to look for the apk if the apk is not in the tests "
                    + "zip file. For each alternate dir, will look in // and //DATA. Can be "
                    + "repeated. Look for apks in last alt-dir first.")
    private List<File> mAltDirs = new ArrayList<>();

    @Option(name = "alt-dir-behavior", description = "The order of alternate directory to be used "
            + "when searching for files to push")
    private AltDirBehavior mAltDirBehavior = AltDirBehavior.FALLBACK;

    /**
     * Adds a file to the list of items to push
     *
     * @param fileName
     */
    protected void addTestFileName(String fileName) {
        mTestPaths.add(fileName);
    }

    /**
     * Retrieves the list of files to be pushed from test zip onto device
     */
    protected Collection<String> getTestFileNames() {
        return mTestPaths;
    }

    protected void clearTestFileName() {
        mTestPaths.clear();
    }

    /**
     * Resolve the host side path based on testing artifact information inside build info.
     *
     * @param buildInfo build artifact information
     * @param fileName filename of artifacts to push
     * @return a {@link File} representing the physical file/path on host
     */
    protected File getLocalPathForFilename(IBuildInfo buildInfo, String fileName,
            ITestDevice device) throws TargetSetupError {
        List<File> dirs = new ArrayList<>();
        for (File dir : mAltDirs) {
            dirs.add(dir);
            dirs.add(FileUtil.getFileForPath(dir, "DATA"));
        }
        // reverse the order so ones provided via command line last can be searched first
        Collections.reverse(dirs);

        List<File> expandedTestDirs = new ArrayList<>();
        if (buildInfo instanceof IDeviceBuildInfo) {
            File testsDir = ((IDeviceBuildInfo)buildInfo).getTestsDir();
            if (testsDir != null && testsDir.exists()) {
                expandedTestDirs.add(FileUtil.getFileForPath(testsDir, "DATA"));
            }
        }
        if (mAltDirBehavior == AltDirBehavior.FALLBACK) {
            // alt dirs are appended after build artifact dirs
            expandedTestDirs.addAll(dirs);
            dirs = expandedTestDirs;
        } else if (mAltDirBehavior == AltDirBehavior.OVERRIDE) {
            dirs.addAll(expandedTestDirs);
        } else {
            throw new TargetSetupError("Missing handler for alt-dir-behavior: " + mAltDirBehavior,
                    device.getDeviceDescriptor());
        }
        if (dirs.isEmpty()) {
            throw new TargetSetupError(
                    "Provided buildInfo does not contain a valid tests directory and no " +
                    "alternative directories were provided", device.getDeviceDescriptor());
        }

        for (File dir : dirs) {
            File testAppFile = new File(dir, fileName);
            if (testAppFile.exists()) {
                return testAppFile;
            }
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (!(buildInfo instanceof IDeviceBuildInfo)) {
            throw new IllegalArgumentException(String.format("Provided buildInfo is not a %s",
                    IDeviceBuildInfo.class.getCanonicalName()));
        }
        if (mTestPaths.size() == 0) {
            CLog.d("No test files to push, skipping");
            return;
        }
        int filePushed = 0;
        for (String fileName : mTestPaths) {
            File localFile = getLocalPathForFilename(buildInfo, fileName, device);
            if (localFile == null) {
                if (mThrowIfNoFile) {
                    throw new TargetSetupError(String.format(
                            "Could not find test file %s directory in extracted tests.zip",
                            fileName), device.getDeviceDescriptor());
                } else {
                    CLog.w(String.format(
                            "Could not find test file %s directory in extracted tests.zip, but" +
                            "will continue test setup as throw-if-not-found is set to false",
                            fileName));
                    continue;
                }
            }
            String remoteFileName = getDevicePathFromUserData(fileName);
            CLog.d("Pushing file: %s -> %s", localFile.getAbsoluteFile(), remoteFileName);
            if (localFile.isDirectory()) {
                device.pushDir(localFile, remoteFileName);
            } else if (localFile.isFile()) {
                device.pushFile(localFile, remoteFileName);
            }
            // there's no recursive option for 'chown', best we can do here
            device.executeShellCommand(String.format("chown system.system %s", remoteFileName));
            filePushed++;
        }
        if (filePushed == 0 && mThrowIfNoFile) {
            throw new TargetSetupError("No file is pushed from tests.zip",
                    device.getDeviceDescriptor());
        }
    }

    protected void setThrowIfNoFile(boolean throwIfNoFile) {
        mThrowIfNoFile = throwIfNoFile;
    }

    /**
     * Set an alternate directory. Exposed for testing.
     */
    protected void setAltDir(File altDir) {
        mAltDirs.add(altDir);
    }

    /**
     * Set the alternative directory search behavior. Exposed for testing.
     */
    protected void setAltDirBehavior(AltDirBehavior behavior) {
        mAltDirBehavior = behavior;
    }

    static String getDevicePathFromUserData(String path) {
        return ArrayUtil.join(FileListingService.FILE_SEPARATOR,
                "", FileListingService.DIRECTORY_DATA, path);
    }
}
