/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tradefed.util;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.ProcessHelper;
import com.android.tradefed.util.RunInterruptedException;
import com.android.tradefed.util.RunUtil;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * A helper class for executing VTS python scripts.
 */
public class VtsPythonRunnerHelper {
    static final String OS_NAME = "os.name";
    static final String WINDOWS = "Windows"; // Not officially supported OS.
    static final String LINUX = "Linux";
    static final String PYTHONPATH = "PYTHONPATH";
    static final String VIRTUAL_ENV_PATH = "VIRTUALENVPATH";
    static final String VTS = "vts";
    static final String ANDROID_BUILD_TOP = "ANDROID_BUILD_TOP";
    static final long TEST_ABORT_TIMEOUT_MSECS = 1000 * 15;

    private IBuildInfo mBuildInfo = null;
    private String mPythonVersion = "";
    private IRunUtil mRunUtil = null;
    private String mPythonPath = null;

    public VtsPythonRunnerHelper(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
        mPythonPath = buildPythonPath();
        mRunUtil = new RunUtil();
        mRunUtil.setEnvVariable(PYTHONPATH, mPythonPath);
        mRunUtil.setEnvVariable("VTS", "1");
    }

    /**
     * Create a {@link ProcessHelper} from mRunUtil.
     *
     * @param cmd the command to run.
     * @throws IOException if fails to start Process.
     */
    protected ProcessHelper createProcessHelper(String[] cmd) throws IOException {
        return new ProcessHelper(mRunUtil.runCmdInBackground(cmd));
    }

    /**
     * Run VTS Python runner and handle interrupt from TradeFed.
     *
     * @param cmd: the command to start VTS Python runner.
     * @param commandResult: the object containing the command result.
     * @param timeout: command timeout value.
     * @return null if the command terminates or times out; a message string if the command is
     * interrupted by TradeFed.
     */
    public String runPythonRunner(String[] cmd, CommandResult commandResult, long timeout) {
        ProcessHelper process;
        try {
            process = createProcessHelper(cmd);
        } catch (IOException e) {
            CLog.e(e);
            commandResult.setStatus(CommandStatus.EXCEPTION);
            commandResult.setStdout("");
            commandResult.setStderr("");
            return null;
        }

        String interruptMessage;
        try {
            CommandStatus commandStatus;
            try {
                commandStatus = process.waitForProcess(timeout);
                interruptMessage = null;
            } catch (RunInterruptedException e) {
                CLog.e("Python process is interrupted.");
                commandStatus = CommandStatus.TIMED_OUT;
                interruptMessage = (e.getMessage() != null ? e.getMessage() : "");
            }
            if (process.isRunning()) {
                CLog.e("Cancel Python process and wait %d seconds.",
                        TEST_ABORT_TIMEOUT_MSECS / 1000);
                try {
                    process.closeStdin();
                    // Wait for the process to clean up and ignore the CommandStatus.
                    // Propagate RunInterruptedException if this is interrupted again.
                    process.waitForProcess(TEST_ABORT_TIMEOUT_MSECS);
                } catch (IOException e) {
                    CLog.e("Fail to cancel Python process.");
                }
            }
            commandResult.setStatus(commandStatus);
        } finally {
            process.cleanUp();
        }
        commandResult.setStdout(process.getStdout());
        commandResult.setStderr(process.getStderr());
        return interruptMessage;
    }

    /**
     * This method returns whether the OS is Windows.
     */
    private static boolean isOnWindows() {
        return System.getProperty(OS_NAME).contains(WINDOWS);
    }

    /**
     * This method builds the python path based on the following values:
     * 1) System environment $PYTHONPATH.
     * 2) VTS testcase root directory (e.g. android-vts/testcases/).
     * 3) Value passed in buildInfo attribute PYTHONPATH (for tests started
     *    using PythonVirtualenvPreparer).
     * 4) System environment $ANDROID_BUILD_TOP/test.
     *
     * @throws RuntimeException.
     */
    private String buildPythonPath() throws RuntimeException {
        StringBuilder sb = new StringBuilder();
        String separator = File.pathSeparator;
        if (System.getenv(PYTHONPATH) != null) {
            sb.append(separator);
            sb.append(System.getenv(PYTHONPATH));
        }

        // to get the path for android-vts/testcases/ which keeps the VTS python code under vts.
        if (mBuildInfo != null) {
            CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuildInfo);

            File testDir = null;
            try {
                testDir = buildHelper.getTestsDir();
            } catch (FileNotFoundException e) {
                /* pass */
            }
            if (testDir != null) {
                sb.append(separator);
                String testCaseDataDir = testDir.getAbsolutePath();
                sb.append(testCaseDataDir);
            } else if (mBuildInfo.getFile(VTS) != null) {
                sb.append(separator);
                sb.append(mBuildInfo.getFile(VTS).getAbsolutePath()).append("/..");
            }

            // for when one uses PythonVirtualenvPreparer.
            if (mBuildInfo.getFile(PYTHONPATH) != null) {
                sb.append(separator);
                sb.append(mBuildInfo.getFile(PYTHONPATH).getAbsolutePath());
            }
        }

        if (System.getenv(ANDROID_BUILD_TOP) != null) {
            sb.append(separator);
            sb.append(System.getenv(ANDROID_BUILD_TOP)).append("/test");
        }
        if (sb.length() == 0) {
            throw new RuntimeException("Could not find python path on host machine");
        }
        return sb.substring(1);
    }

    /**
     * This method gets the python binary.
     */
    public String getPythonBinary() {
        boolean isWindows = isOnWindows();
        String python = (isWindows ? "python.exe" : "python" + mPythonVersion);
        if (mBuildInfo != null) {
            File venvDir = mBuildInfo.getFile(VIRTUAL_ENV_PATH);
            if (venvDir != null) {
                String binDir = (isWindows ? "Scripts" : "bin");
                File pythonBinaryFile =
                        new File(venvDir.getAbsolutePath(), binDir + File.separator + python);
                String pythonBinPath = pythonBinaryFile.getAbsolutePath();
                if (pythonBinaryFile.exists()) {
                    CLog.i("Python path " + pythonBinPath + ".\n");
                    return pythonBinPath;
                }
                CLog.e(python + " doesn't exist under the "
                        + "created virtualenv dir (" + pythonBinPath + ").\n");
            } else {
                CLog.e(VIRTUAL_ENV_PATH + " not available in BuildInfo. "
                        + "Please use VtsPythonVirtualenvPreparer tartget preparer.\n");
            }
        }

        CommandResult c = mRunUtil.runTimedCmd(1000, (isWindows ? "where" : "which"), python);
        String pythonBin = c.getStdout().trim();
        if (pythonBin.length() == 0) {
            throw new RuntimeException("Could not find python binary on host "
                    + "machine");
        }
        return pythonBin;
    }

    /**
     * Gets mPythonPath.
     */
    public String getPythonPath() {
        return mPythonPath;
    }

    /**
     * Sets mPythonVersion.
     */
    public void setPythonVersion(String pythonVersion) {
        mPythonVersion = pythonVersion;
    }

    /**
     * Sets mRunUtil, should only be used in tests.
     */
    public void setRunUtil(IRunUtil runUtil) {
        mRunUtil = runUtil;
    }
}
