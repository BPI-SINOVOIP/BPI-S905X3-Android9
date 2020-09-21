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

import com.android.annotations.VisibleForTesting;
import com.android.ddmlib.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.Vector;
import java.util.concurrent.TimeUnit;
import java.util.function.Predicate;

public class CmdUtil {
    public static final int MAX_RETRY_COUNT = 10;
    static final int DELAY_BETWEEN_RETRY_IN_SECS = 3;
    static final long FRAMEWORK_START_TIMEOUT = 1000 * 60 * 2; // 2 minutes.
    static final String BOOTCOMPLETE_PROP = "dev.bootcomplete";

    // An interface to wait with delay. Used for testing purpose.
    public interface ISleeper { public void sleep(int seconds) throws InterruptedException; }
    private ISleeper mSleeper = null;

    /**
     * Helper method to retry given cmd until the expected results are satisfied.
     * An example usage it to retry 'lshal' until the expected hal service appears.
     *
     * @param device testing device.
     * @param cmd the command string to be executed on device.
     * @param predicate function that checks the exit condition.
     * @return true if the exit condition is satisfied, false otherwise.
     * @throws DeviceNotAvailableException
     */
    public boolean waitCmdResultWithDelay(ITestDevice device, String cmd,
            Predicate<String> predicate) throws DeviceNotAvailableException {
        for (int count = 0; count < MAX_RETRY_COUNT; count++) {
            if (validateCmdSuccess(device, cmd, predicate)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Helper method to retry a given set of commands, cmds.
     *
     * @param device testing device.
     * @param cmds a vector of the command strings to be executed on device.
     * @param validation_cmd the validation command string to be executed on device.
     * @param predicate function that checks the exit condition.
     * @return true if the exit condition is satisfied, false otherwise.
     * @throws DeviceNotAvailableException
     */
    public boolean retry(ITestDevice device, Vector<String> cmds, String validation_cmd,
            Predicate<String> predicate) throws DeviceNotAvailableException {
        if (cmds.isEmpty()) {
            CLog.w("retry() called but cmd is an epmty vector.");
            return false;
        }
        for (int count = 0; count < MAX_RETRY_COUNT; count++) {
            for (String cmd : cmds) {
                CLog.i("Running a command: %s", cmd);
                String out = device.executeShellCommand(cmd);
                CLog.i("Command output: %s", out);
            }
            if (validateCmdSuccess(device, validation_cmd, predicate)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Helper method to retry a given cmd.
     *
     * @param device testing device.
     * @param cmd the command string to be executed on device.
     * @param validation_cmd the validation command string to be executed on device.
     * @param predicate function that checks the exit condition.
     * @return true if the exit condition is satisfied, false otherwise.
     * @throws DeviceNotAvailableException
     */
    public boolean retry(ITestDevice device, String cmd, String validation_cmd,
            Predicate<String> predicate) throws DeviceNotAvailableException {
        return retry(device, cmd, validation_cmd, predicate, MAX_RETRY_COUNT);
    }

    /**
     * Helper method to retry a given cmd.
     *
     * @param device testing device.
     * @param cmd the command string to be executed on device.
     * @param validation_cmd the validation command string to be executed on device.
     * @param predicate function that checks the exit condition.
     * @param retry_count the max number of times to try
     * @return true if the exit condition is satisfied, false otherwise.
     * @throws DeviceNotAvailableException
     */
    public boolean retry(ITestDevice device, String cmd, String validation_cmd,
            Predicate<String> predicate, int retry_count) throws DeviceNotAvailableException {
        for (int count = 0; count < retry_count; count++) {
            CLog.i("Running a command: %s", cmd);
            device.executeShellCommand(cmd);
            if (validateCmdSuccess(device, validation_cmd, predicate)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Validates the device status and waits if the validation fails.
     *
     * @param device testing device.
     * @param cmd the command string to be executed on device.
     * @param predicate function that checks the exit condition.
     * @return true if the exit condition is satisfied, false otherwise.
     * @throws DeviceNotAvailableException
     */
    protected boolean validateCmdSuccess(ITestDevice device, String cmd,
            Predicate<String> predicate) throws DeviceNotAvailableException {
        if (cmd == null) {
            CLog.w("validateCmdSuccess() called but cmd is null");
            return false;
        }
        String out = device.executeShellCommand(cmd);
        CLog.i("validation cmd output: %s", out);
        if (out != null && predicate.test(out)) {
            CLog.i("Exit condition satisfied.");
            return true;
        } else {
            CLog.i("Exit condition not satisfied. Waiting for %s more seconds.",
                    DELAY_BETWEEN_RETRY_IN_SECS);
            try {
                if (mSleeper != null) {
                    mSleeper.sleep(DELAY_BETWEEN_RETRY_IN_SECS);
                } else {
                    TimeUnit.SECONDS.sleep(DELAY_BETWEEN_RETRY_IN_SECS);
                }
            } catch (InterruptedException ex) {
                /* pass */
            }
        }
        return false;
    }

    /**
     * Restarts the Andriod framework and waits for the device boot completion.
     *
     * @param device the test device instance.
     * @throws DeviceNotAvailableException
     */
    public void restartFramework(ITestDevice device) throws DeviceNotAvailableException {
        device.executeShellCommand("stop");
        setSystemProperty(device, BOOTCOMPLETE_PROP, "0");
        device.executeShellCommand("start");
        device.waitForDeviceAvailable(FRAMEWORK_START_TIMEOUT);
    }

    /**
     * Gets a sysprop from the device.
     *
     * @param device the test device instance.
     * @param name the name of a sysprop.
     * @return the device sysprop value.
     * @throws DeviceNotAvailableException
     */
    public String getSystemProperty(ITestDevice device, String name)
            throws DeviceNotAvailableException {
        CollectingOutputReceiver receiver = new CollectingOutputReceiver();
        device.executeShellCommand(String.format("getprop %s", name), receiver);
        return receiver.getOutput();
    }

    /**
     * Sets a sysprop on the device.
     *
     * @param device the test device instance.
     * @param name the name of a sysprop.
     * @param value the value of a sysprop.
     * @throws DeviceNotAvailableException
     */
    public void setSystemProperty(ITestDevice device, String name, String value)
            throws DeviceNotAvailableException {
        device.executeShellCommand(String.format("setprop %s %s", name, value));
    }

    @VisibleForTesting
    void setSleeper(ISleeper sleeper) {
        mSleeper = sleeper;
    }
}
