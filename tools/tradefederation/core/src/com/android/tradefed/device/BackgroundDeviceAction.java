/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache
 * License, Version 2.0 (the "License");
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
import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

/**
 * Runs a command on a given device repeating as necessary until the action is canceled.
 * <p>
 * When the class is run, the command is run on the device in a separate thread and the output is
 * collected in a temporary host file.
 * </p><p>
 * This is done so:
 * </p><ul>
 * <li>if device goes permanently offline during a test, the log data is retained.</li>
 * <li>to capture more data than may fit in device's circular log.</li>
 * </ul>
 */
public class BackgroundDeviceAction extends Thread {
    private static final long ONLINE_POLL_INTERVAL_MS = 10 * 1000;
    private IShellOutputReceiver mReceiver;
    private ITestDevice mTestDevice;
    private String mCommand;
    private String mDescriptor;
    private boolean mIsCancelled;
    private int mLogStartDelay;

    /**
     * Creates a {@link BackgroundDeviceAction}
     *
     * @param command the command to run
     * @param descriptor the description of the command. For logging only.
     * @param device the device to run the command on
     * @param receiver the receiver for collecting the output of the command
     * @param startDelay the delay to wait after the device becomes online
     */
    public BackgroundDeviceAction(String command, String descriptor, ITestDevice device,
            IShellOutputReceiver receiver, int startDelay) {
        super("BackgroundDeviceAction-" + command);
        mCommand = command;
        mDescriptor = descriptor;
        mTestDevice = device;
        mReceiver = receiver;
        mLogStartDelay = startDelay;
        // don't keep VM open if this thread is still running
        setDaemon(true);
    }

    /**
     * {@inheritDoc}
     * <p>
     * Repeats the command until canceled.
     * </p>
     */
    @Override
    public void run() {
        String separator = String.format(
                "\n========== beginning of new [%s] output ==========\n", mDescriptor);
        while (!isCancelled()) {
            if (mLogStartDelay > 0) {
                CLog.d("Sleep for %d before starting %s for %s.", mLogStartDelay, mDescriptor,
                        mTestDevice.getSerialNumber());
                getRunUtil().sleep(mLogStartDelay);
            }
            blockUntilOnlineNoThrow();
            // check again if the operation has been cancelled after the wait for online
            if (isCancelled()) {
                break;
            }
            CLog.d("Starting %s for %s.", mDescriptor, mTestDevice.getSerialNumber());
            mReceiver.addOutput(separator.getBytes(), 0, separator.length());
            try {
                mTestDevice.getIDevice().executeShellCommand(mCommand, mReceiver,
                        0, TimeUnit.MILLISECONDS);
            } catch (AdbCommandRejectedException | IOException |
                    ShellCommandUnresponsiveException | TimeoutException e) {
                waitForDeviceRecovery(e.getClass().getName());
            }
        }
    }

    /**
     * If device goes offline for any reason, the recovery will be triggered from the main
     * so we just have to block until it recovers or invocation fails for device unavailable.
     * @param exceptionType
     */
    protected void waitForDeviceRecovery(String exceptionType) {
        CLog.d("%s while running %s on %s. May see duplicated content in log.", exceptionType,
                mDescriptor, mTestDevice.getSerialNumber());
        blockUntilOnlineNoThrow();
    }

    /**
     * Cancels the command.
     */
    public synchronized void cancel() {
        mIsCancelled = true;
        interrupt();
    }

    /**
     * If the command is cancelled.
     */
    public synchronized boolean isCancelled() {
        return mIsCancelled;
    }

    /**
     * Get the {@link RunUtil} instance to use.
     * <p/>
     * Exposed for unit testing.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    private void blockUntilOnlineNoThrow() {
        CLog.d("Waiting for device %s online before starting.", mTestDevice.getSerialNumber());
        while (!isCancelled()) {
            // For stub device we wait no matter what to avoid flooding with logs.
            if ((mTestDevice.getIDevice() instanceof StubDevice)
                    || !TestDeviceState.ONLINE.equals(mTestDevice.getDeviceState())) {
                getRunUtil().sleep(ONLINE_POLL_INTERVAL_MS);
            } else {
                CLog.d("Device %s now online.", mTestDevice.getSerialNumber());
                break;
            }
        }
    }
}
