/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.tradefed.testtype.python;

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.SubprocessTestResultsParser;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Host test meant to run a python binary file from the Android Build system (Soong) */
@OptionClass(alias = "python-host")
public class PythonBinaryHostTest
        implements IRemoteTest, IDeviceTest, IBuildReceiver, IInvocationContextReceiver {

    protected static final String PYTHON_OUTPUT = "python-output";

    @Option(name = "par-file-name", description = "The binary names inside the build info to run.")
    private Set<String> mBinaryNames = new HashSet<>();

    @Option(
        name = "python-binaries",
        description = "The full path to a runnable python binary. Can be repeated."
    )
    private Set<File> mBinaries = new HashSet<>();

    @Option(
        name = "test-timeout",
        description = "Timeout for a single par file to terminate.",
        isTimeVal = true
    )
    private long mTestTimeout = 20 * 1000L;

    private ITestDevice mDevice;
    private IBuildInfo mBuildInfo;
    private IInvocationContext mContext;

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    @Override
    public void setInvocationContext(IInvocationContext invocationContext) {
        mContext = invocationContext;
    }

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        List<File> pythonFilesList = findParFiles();
        for (File pyFile : pythonFilesList) {
            if (!pyFile.exists()) {
                CLog.d(
                        "ignoring %s which doesn't look like a test file.",
                        pyFile.getAbsolutePath());
                continue;
            }
            pyFile.setExecutable(true);
            runSinglePythonFile(listener, pyFile);
        }
    }

    private List<File> findParFiles() {
        File testsDir = null;
        if (mBuildInfo instanceof IDeviceBuildInfo) {
            testsDir = ((IDeviceBuildInfo) mBuildInfo).getTestsDir();
        }
        List<File> files = new ArrayList<>();
        for (String parFileName : mBinaryNames) {
            File res = null;
            // search tests dir
            if (testsDir != null) {
                res = FileUtil.findFile(testsDir, parFileName);
            }

            // TODO: is there other places to search?
            if (res == null) {
                throw new RuntimeException(
                        String.format("Couldn't find a par file %s", parFileName));
            }
            files.add(res);
        }
        files.addAll(mBinaries);
        return files;
    }

    private void runSinglePythonFile(ITestInvocationListener listener, File pyFile) {
        List<String> commandLine = new ArrayList<>();
        commandLine.add(pyFile.getAbsolutePath());
        // Run with -q (quiet) to avoid extraneous outputs
        commandLine.add("-q");
        // If we have a physical device, pass it to the python test by serial
        if (!(getDevice().getIDevice() instanceof StubDevice)) {
            // TODO: support multi-device python tests?
            commandLine.add("-s");
            commandLine.add(getDevice().getSerialNumber());
        }
        CommandResult result =
                getRunUtil().runTimedCmd(mTestTimeout, commandLine.toArray(new String[0]));
        PythonForwarder forwarder = new PythonForwarder(listener, pyFile.getName());
        if (!CommandStatus.SUCCESS.equals(result.getStatus())) {
            // If the binary finishes we an error code, it could simply be a test failure, but if
            // it does not even have a TEST_RUN_STARTED tag, then we probably have binary setup
            // issue.
            if (!result.getStderr().contains("TEST_RUN_STARTED")) {
                throw new RuntimeException(
                        String.format(
                                "Something went wrong when running the python binary: %s",
                                result.getStderr()));
            }
        }
        SubprocessTestResultsParser parser = new SubprocessTestResultsParser(forwarder, mContext);
        File resultFile = null;
        try {
            resultFile = FileUtil.createTempFile("python-res", ".txt");
            FileUtil.writeToFile(result.getStderr(), resultFile);
            try (FileInputStreamSource data = new FileInputStreamSource(resultFile)) {
                listener.testLog(PYTHON_OUTPUT, LogDataType.TEXT, data);
            }
            parser.parseFile(resultFile);
        } catch (IOException e) {
            throw new RuntimeException(e);
        } finally {
            FileUtil.deleteFile(resultFile);
            StreamUtil.close(parser);
        }
    }

    @VisibleForTesting
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /** Result forwarder to replace the run name by the binary name. */
    public class PythonForwarder extends ResultForwarder {

        private String mRunName;

        /** Ctor with the run name using the binary name. */
        public PythonForwarder(ITestInvocationListener listener, String name) {
            super(listener);
            mRunName = name;
        }

        @Override
        public void testRunStarted(String runName, int testCount) {
            // Replace run name
            super.testRunStarted(mRunName, testCount);
        }
    }
}
