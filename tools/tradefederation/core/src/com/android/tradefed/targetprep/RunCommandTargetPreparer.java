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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.BackgroundDeviceAction;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.RunUtil;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

@OptionClass(alias = "run-command")
public class RunCommandTargetPreparer extends BaseTargetPreparer implements ITargetCleaner {

    @Option(name = "run-command", description = "adb shell command to run")
    private List<String> mCommands = new ArrayList<String>();

    @Option(name = "run-bg-command", description = "Command to run repeatedly in the"
            + " device background. Can be repeated to run multiple commands"
            + " in the background.")
    private List<String> mBgCommands = new ArrayList<String>();

    @Option(name = "hide-bg-output", description = "if true, don't log background command output")
    private boolean mHideBgOutput = false;

    @Option(name = "teardown-command", description = "adb shell command to run at teardown time")
    private List<String> mTeardownCommands = new ArrayList<String>();

    @Option(name = "delay-after-commands",
            description = "Time to delay after running commands, in msecs")
    private long mDelayMsecs = 0;

    @Option(name = "run-command-timeout",
            description = "Timeout for execute shell command",
            isTimeVal = true)
    private long mRunCmdTimeout = 0;

    @Option(
        name = "use-shell-v2",
        description =
                "Whether or not to use the shell v2 execution which provides status and output "
                        + "for the shell command."
    )
    private boolean mUseShellV2 = false;

    private Map<BackgroundDeviceAction, CollectingOutputReceiver> mBgDeviceActionsMap =
            new HashMap<>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException {
        if (isDisabled()) return;

        for (String bgCmd : mBgCommands) {
            CollectingOutputReceiver receiver = new CollectingOutputReceiver();
            BackgroundDeviceAction mBgDeviceAction =
                    new BackgroundDeviceAction(bgCmd, bgCmd, device, receiver, 0);
            mBgDeviceAction.start();
            mBgDeviceActionsMap.put(mBgDeviceAction, receiver);
        }

        for (String cmd : mCommands) {
            CLog.d("About to run setup command on device %s: %s", device.getSerialNumber(), cmd);
            CommandResult result;
            if (!mUseShellV2) {
                // Shell v1 without command status.
                if (mRunCmdTimeout > 0) {
                    CollectingOutputReceiver receiver = new CollectingOutputReceiver();
                    device.executeShellCommand(
                            cmd, receiver, mRunCmdTimeout, TimeUnit.MILLISECONDS, 0);
                    CLog.v("cmd: '%s', returned:\n%s", cmd, receiver.getOutput());
                } else {
                    String output = device.executeShellCommand(cmd);
                    CLog.v("cmd: '%s', returned:\n%s", cmd, output);
                }
            } else {
                // Shell v2 with command status checks
                if (mRunCmdTimeout > 0) {
                    result =
                            device.executeShellV2Command(
                                    cmd, mRunCmdTimeout, TimeUnit.MILLISECONDS, 0);
                } else {
                    result = device.executeShellV2Command(cmd);
                }
                // Ensure the command ran successfully.
                if (!CommandStatus.SUCCESS.equals(result.getStatus())) {
                    throw new TargetSetupError(
                            String.format(
                                    "Failed to run '%s' without error. stdout: '%s'\nstderr: '%s'",
                                    cmd, result.getStdout(), result.getStderr()),
                            device.getDeviceDescriptor());
                }
                CLog.v("cmd: '%s', returned:\n%s", cmd, result.getStdout());
            }
        }

        CLog.d("Sleeping %d msecs on device %s", mDelayMsecs, device.getSerialNumber());
        RunUtil.getDefault().sleep(mDelayMsecs);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (isDisabled()) return;

        for (Map.Entry<BackgroundDeviceAction, CollectingOutputReceiver> bgAction :
                mBgDeviceActionsMap.entrySet()) {
            if (!mHideBgOutput) {
                CLog.d("Background command output : %s", bgAction.getValue().getOutput());
            }
            bgAction.getKey().cancel();
        }

        for (String cmd : mTeardownCommands) {
            CLog.d("About to run tearDown command on device %s: %s", device.getSerialNumber(),
                    cmd);
            String output = device.executeShellCommand(cmd);
            CLog.v("tearDown cmd: '%s', returned:\n%s", cmd, output);
        }

    }
}

