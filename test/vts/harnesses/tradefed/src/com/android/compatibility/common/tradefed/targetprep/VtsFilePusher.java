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

package com.android.compatibility.common.tradefed.targetprep;

import com.android.compatibility.common.tradefed.build.VtsCompatibilityInvocationHelper;
import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.PushFilePreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;

import java.io.File;
import java.util.TreeSet;
import java.util.Collection;
import java.lang.Class;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileNotFoundException;

/**
 * Pushes specified testing artifacts from Compatibility repository.
 */
@OptionClass(alias = "vts-file-pusher")
public class VtsFilePusher extends PushFilePreparer implements IAbiReceiver {
    @Option(name="push-group", description=
            "A push group name. Must be a .push file under tools/vts-tradefed/res/push_groups/. "
                    + "May be under a relative path. May be repeated. Duplication of file specs "
                    + "is ok. Will throw a TargetSetupError if a file push fails.")
    private Collection<String> mPushSpecGroups = new TreeSet<>();

    @Option(name="push-group-cleanup", description = "Whether files in push group "
            + "should be cleaned up from device after test. Note that preparer does not verify "
            + "that files/directories have been deleted. Default value: true.")
    private boolean mPushGroupCleanup = true;

    @Option(name="push-group-remount-system", description="Whether to remounts system "
            + "partition to be writable before push or clean up default files. "
            + "Default value: false")
    private boolean mPushGroupRemount = false;

    @Option(name = "append-bitness", description = "Append the ABI's bitness to the filename.")
    private boolean mAppendBitness = false;

    private static final String DIR_PUSH_GROUPS = "vts/tools/vts-tradefed/res/push_groups";
    static final String PUSH_GROUP_FILE_EXTENSION = ".push";

    private Collection<String> mFilesPushed = new TreeSet<>();
    private IAbi mAbi;
    private VtsCompatibilityInvocationHelper mInvocationHelper;

    /**
     * Load file push specs from .push files as a collection of Strings
     * @param buildInfo
     * @return a collection of push spec strings
     * @throws TargetSetupError if load fails
     */
    private Collection<String> loadFilePushGroups(IBuildInfo buildInfo) throws TargetSetupError {
        Collection<String> result = new TreeSet<>();
        File testDir;
        try {
            testDir = mInvocationHelper.getTestsDir();
        } catch(FileNotFoundException e) {
            throw new TargetSetupError(e.getMessage());
        }

        for (String group: mPushSpecGroups) {
            TreeSet<String> stack = new TreeSet<>();
            result.addAll(loadFilePushGroup(group, testDir.getAbsolutePath(), stack));
        }
        return result;
    }

    /**
     * Recursively load file push specs from a push group file into a collection of strings.
     *
     * @param specFile, push group file name with .push extension. Can contain relative directory.
     * @param testsDir, the directory where test data files are stored.
     */
    private Collection<String> loadFilePushGroup(String specFileName, String testsDir,
            Collection<String> stack) throws TargetSetupError {
        Collection<String> result = new TreeSet<>();

        String relativePath = new File(
                DIR_PUSH_GROUPS, specFileName).getPath();
        File specFile = null;

        try {
            specFile = new File(testsDir, relativePath);
        } catch(Exception e) {
            throw new TargetSetupError(e.getMessage());
        }

        try (BufferedReader br = new BufferedReader(new FileReader(specFile))) {
            String line;
            while ((line = br.readLine()) != null) {
                String spec = line.trim();
                if (spec.contains("->")) {
                    result.add(spec);
                } else if (spec.contains(PUSH_GROUP_FILE_EXTENSION)) {
                    if (!stack.contains(spec)) {
                        stack.add(spec);
                        result.addAll(loadFilePushGroup(spec, testsDir, stack));
                        stack.remove(spec);
                    }
                } else if (spec.length() > 0) {
                    throw new TargetSetupError("Unknown file push spec: " + spec);
                }
            }
        } catch(Exception e) {
            throw new TargetSetupError(e.getMessage());
        }

        return result;
    }

    /**
     * Push file groups if configured in .xml file.
     * @param device
     * @param buildInfo
     * @throws TargetSetupError, DeviceNotAvailableException
     */
    private void pushFileGroups(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, DeviceNotAvailableException {
        if (mPushGroupRemount) {
            device.remountSystemWritable();
        }

        for (String pushspec : loadFilePushGroups(buildInfo)) {
            String[] pair = pushspec.split("->");

            if (pair.length != 2) {
                throw new TargetSetupError(
                        String.format("Failed to parse push spec '%s'", pushspec));
            }

            File src = new File(pair[0]);

            if (!src.isAbsolute()) {
                src = resolveRelativeFilePath(buildInfo, pair[0]);
            }

            Class cls = this.getClass();

            if (!src.exists()) {
                Log.w(cls.getSimpleName(), String.format(
                        "Skipping push spec in push group whose source does not exist: %s",
                        pushspec));
                continue;
            }

            Log.d(cls.getSimpleName(),
                    String.format("Trying to push file from local to remote: %s", pushspec));

            if ((src.isDirectory() && !device.pushDir(src, pair[1])) ||
                    (!device.pushFile(src, pair[1]))) {
                mFilesPushed = null;
                throw new TargetSetupError(String.format("Failed to push local '%s' to remote '%s'",
                        pair[0], pair[1]));
            } else {
                mFilesPushed.add(pair[1]);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        device.enableAdbRoot();
        mInvocationHelper = new VtsCompatibilityInvocationHelper();
        pushFileGroups(device, buildInfo);

        super.setUp(device, buildInfo);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {

        if (!(e instanceof DeviceNotAvailableException) && mPushGroupCleanup && mFilesPushed != null) {
            device.enableAdbRoot();
            if (mPushGroupRemount) {
                device.remountSystemWritable();
            }
            for (String devicePath : mFilesPushed) {
                device.executeShellCommand("rm -r " + devicePath);
            }
        }

        super.tearDown(device, buildInfo, e);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IAbi getAbi() {
        return mAbi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File resolveRelativeFilePath(IBuildInfo buildInfo, String fileName) {
        try {
            File f = new File(mInvocationHelper.getTestsDir(),
                    String.format("%s%s", fileName, mAppendBitness ? mAbi.getBitness() : ""));
            CLog.logAndDisplay(LogLevel.INFO, "Copying from %s", f.getAbsolutePath());
            return f;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        return null;
    }
}

