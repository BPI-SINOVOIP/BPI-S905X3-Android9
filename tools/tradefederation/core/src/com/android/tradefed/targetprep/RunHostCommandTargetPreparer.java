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

package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestLoggerReceiver;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Target preparer to run arbitrary host commands before and after running the test. */
@OptionClass(alias = "run-host-command")
public class RunHostCommandTargetPreparer extends BaseTargetPreparer
        implements ITargetCleaner, ITestLoggerReceiver {

    /** Placeholder to be replaced with real device serial number in commands */
    private static final String DEVICE_SERIAL_PLACEHOLDER = "$SERIAL";

    private static final String BG_COMMAND_LOG_PREFIX = "bg_command_log_";

    @Option(
        name = "host-setup-command",
        description =
                "Command to be run before the test. Can be repeated. "
                        + DEVICE_SERIAL_PLACEHOLDER
                        + " can be used as placeholder to be replaced "
                        + "with real device serial number at runtime."
    )
    private List<String> mSetUpCommands = new ArrayList<>();

    @Option(
        name = "host-teardown-command",
        description = "Command to be run after the test. Can be repeated."
    )
    private List<String> mTearDownCommands = new ArrayList<>();

    @Option(
        name = "host-background-command",
        description =
                "Background command to be run before the test. Can be repeated. "
                        + "They will be forced to terminate after the test. "
                        + DEVICE_SERIAL_PLACEHOLDER
                        + " can be used as placeholder to be replaced "
                        + "with real device serial number at runtime."
    )
    private List<String> mBgCommands = new ArrayList<>();

    @Option(
        name = "host-cmd-timeout",
        isTimeVal = true,
        description = "Timeout for each command specified."
    )
    private long mTimeout = 60000L;

    private List<Process> mBgProcesses = new ArrayList<>();
    private List<BgCommandLog> mBgCommandLogs = new ArrayList<>();
    private ITestLogger mLogger;

    /**
     * An interface simply wraps the OutputStream and InputStreamSource for the background command
     * log.
     */
    @VisibleForTesting
    interface BgCommandLog {
        OutputStream getOutputStream();

        InputStreamSource getInputStreamSource();

        String getName();
    }

    /** An implementation for BgCommandLog that is based on a file. */
    private static class BgCommandFileLog implements BgCommandLog {
        private File mFile;
        private OutputStream mOutputStream;
        private InputStreamSource mInputStreamSource;

        public BgCommandFileLog(File file) throws IOException {
            mFile = file;
            mOutputStream = new FileOutputStream(mFile);
            //The file will be deleted on cancel
            mInputStreamSource = new FileInputStreamSource(file, true);
        }

        @Override
        public OutputStream getOutputStream() {
            return mOutputStream;
        }

        @Override
        public InputStreamSource getInputStreamSource() {
            return mInputStreamSource;
        }

        @Override
        public String getName() {
            return mFile.getName();
        }
    }

    @Override
    public void setTestLogger(ITestLogger testLogger) {
        mLogger = testLogger;
    }

    /** {@inheritDoc} */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        replaceSerialNumber(mSetUpCommands, device);
        runCommandList(mSetUpCommands);

        try {
            mBgCommandLogs = createBgCommandLogs();
            replaceSerialNumber(mBgCommands, device);
            runBgCommandList(mBgCommands, mBgCommandLogs);
        } catch (IOException e) {
            throw new TargetSetupError(e.toString(), device.getDeviceDescriptor());
        }
    }

    /** {@inheritDoc} */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        replaceSerialNumber(mTearDownCommands, device);
        runCommandList(mTearDownCommands);

        // Terminate background commands after test finished
        for (Process process : mBgProcesses) {
            process.destroy();
        }

        // Upload log of each background command use external logger.
        for (BgCommandLog log : mBgCommandLogs) {
            try {
                log.getOutputStream().close();
                mLogger.testLog(log.getName(), LogDataType.TEXT, log.getInputStreamSource());
                log.getInputStreamSource().close(); //Also delete the file
            } catch (IOException exception) {
                CLog.e("Failed to close background command log output stream.", exception);
            }
        }
    }

    /**
     * Sequentially runs command from specified list.
     *
     * @param commands list of commands to run.
     */
    private void runCommandList(final List<String> commands) {
        for (final String command : commands) {
            final CommandResult result = getRunUtil().runTimedCmd(mTimeout, command.split("\\s+"));
            switch (result.getStatus()) {
                case SUCCESS:
                    CLog.i(
                            "Command %s finished successfully, stdout = [%s].",
                            command, result.getStdout());
                    break;
                case FAILED:
                    CLog.e(
                            "Command %s failed, stdout = [%s], stderr = [%s].",
                            command, result.getStdout(), result.getStderr());
                    break;
                case TIMED_OUT:
                    CLog.e("Command %s timed out.", command);
                    break;
                case EXCEPTION:
                    CLog.e("Exception occurred when running command %s.", command);
                    break;
            }
        }
    }

    /**
     * Sequentially runs background command from specified list.
     *
     * @param bgCommands list of commands to run.
     */
    private void runBgCommandList(
            final List<String> bgCommands, final List<BgCommandLog> bgCommandLogs)
            throws IOException {
        for (int i = 0; i < bgCommands.size(); i++) {
            String command = bgCommands.get(i);
            CLog.d("About to run host background command: %s", command);
            Process process =
                    getRunUtil()
                            .runCmdInBackground(
                                    Arrays.asList(command.split("\\s+")),
                                    bgCommandLogs.get(i).getOutputStream());
            if (process == null) {
                CLog.e("Failed to run command: %s", command);
                continue;
            }
            mBgProcesses.add(process);
        }
    }

    /**
     * For each command in the list, replace placeholder (if any) with real device serial number in
     * place.
     *
     * @param commands list of host commands
     * @param device device with which the host commands execute
     */
    private void replaceSerialNumber(final List<String> commands, ITestDevice device) {
        for (int i = 0; i < commands.size(); i++) {
            String command =
                    commands.get(i).replace(DEVICE_SERIAL_PLACEHOLDER, device.getSerialNumber());
            commands.set(i, command);
        }
    }

    /**
     * Gets instance of {@link IRunUtil}.
     *
     * @return instance of {@link IRunUtil}.
     */
    @VisibleForTesting
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Create a BgCommandLog object that is based on a temporary file for each background command
     *
     * @return A list of BgCommandLog object matching each command
     * @throws IOException
     */
    @VisibleForTesting
    List<BgCommandLog> createBgCommandLogs() throws IOException {
        List<BgCommandLog> bgCommandLogs = new ArrayList<>();
        for (String command : mBgCommands) {
            File file = FileUtil.createTempFile(BG_COMMAND_LOG_PREFIX, ".txt");
            CLog.d("Redirect output to %s for command: %s", file.getAbsolutePath(), command);
            bgCommandLogs.add(new BgCommandFileLog(file));
        }
        return bgCommandLogs;
    }
}
