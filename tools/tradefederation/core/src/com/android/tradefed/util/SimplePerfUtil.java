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
package com.android.tradefed.util;

import com.android.ddmlib.IShellOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import org.junit.Assert;

import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * Utility class to dispatch simple command and collect results
 * @see <a href="https://android.googlesource.com/platform/system/extras/+/master/simpleperf/">
 * Introduction of simpleperf</a>
 */
public class SimplePerfUtil {

    private ITestDevice mDevice;

    private SimplePerfType mType;

    private List<String> mArguList;
    private StringBuilder mCachedCommandPrepend;

    private SimplePerfUtil(ITestDevice device, SimplePerfType type) {
        mDevice = device;
        mType = type;
        mCachedCommandPrepend = null;
    }

    /**
     * SimplePerfUtil Constructor
     * <p/>
     * Caller must define device and simpleperf type when initializing instance
     *
     * @param device {@link ITestDevice} test device
     * @param type {@link SimplePerfType} indicates which simpleperf mode
     * @return a newly created SimplePerfUtil instance
     */
    public static SimplePerfUtil newInstance(ITestDevice device, SimplePerfType type)
            throws  NullPointerException {
        if (device == null || type == null) {
            throw new NullPointerException();
        }
        return new SimplePerfUtil(device, type);
    }

    /**
     * Get argument for simpleperf command
     *
     * @return list of subcommand and arguments (nullable)
     */
    public List<String> getArgumentList() {
        return mArguList;
    }

    /**
     * Set argument on simpleperf command
     *
     * @param arguList list of subcommand and arguments
     */
    public void setArgumentList(List<String> arguList) {
        mArguList = arguList;
        mCachedCommandPrepend = null;
    }

    /**
     * Executes the given adb shell command, with simpleperf wrapped around
     * <p/>
     * Simpleperf result will be parsed and return to caller
     *
     * @param command command to run on device
     * @return {@link SimplePerfResult} object contains all result information
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered
     */
    public SimplePerfResult executeCommand(String command) throws DeviceNotAvailableException {
        String output = mDevice.executeShellCommand(commandStringPreparer(command));
        Assert.assertNotNull("ExecuteShellCommand returns null", output);
        return SimplePerfStatResultParser.parseRawOutput(output);
    }

    /**
     * Executes the given adb shell command, with simpleperf wrapped around
     * <p/>
     * It is caller's responsibility to parse simpleperf result through receiver
     *
     * @param command command to run on device
     * @param receiver {@link IShellOutputReceiver} object to direct shell output to
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered
     */
    public void executeCommand(String command, IShellOutputReceiver receiver)
            throws DeviceNotAvailableException {
        mDevice.executeShellCommand(commandStringPreparer(command), receiver);
    }

    /**
     * Executes the given adb shell command, with simpleperf wrapped around
     * <p/>
     * It is caller's responsibility to parse simpleperf result through receiver
     *
     * @param command command to run on device
     * @param receiver {@link IShellOutputReceiver} object to direct shell output to
     * @param maxTimeToOutputShellResponse the maximum amount of time during which the command is
     * allowed to not output any response; unit as specified in <code>timeUnit</code>
     * @param timeUnit timeUnit unit for <code>maxTimeToOutputShellResponse</code>, see {@link
     * TimeUnit}
     * @param retryAttempts the maximum number of times to retry command if it fails due to a
     * exception. DeviceNotResponsiveException will be thrown if <var>retryAttempts</var> are
     * performed without success.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     * recovered
     */
    public void executeCommand(String command, IShellOutputReceiver receiver,
            long maxTimeToOutputShellResponse, TimeUnit timeUnit, int retryAttempts)
            throws DeviceNotAvailableException {
        mDevice.executeShellCommand(commandStringPreparer(command), receiver,
                maxTimeToOutputShellResponse, timeUnit, retryAttempts);
    }

    protected String commandStringPreparer(String command) {
        if (command == null) {
            command = "";
        }
        return commandPrependPreparer().toString() + command;
    }

    private StringBuilder commandPrependPreparer() {
        if (mCachedCommandPrepend == null) {
            StringBuilder sb = new StringBuilder("simpleperf ");
            sb.append(mType.toString()).append(" ");
            if (mArguList != null) {
                for (String argu : mArguList) {
                    sb.append(argu).append(" ");
                }
            }
            mCachedCommandPrepend = sb;
        }
        return mCachedCommandPrepend;
    }

    /**
     * Enum of simpleperf command options
     */
    public enum SimplePerfType {
        STAT("stat"),
        REPORT("report"),
        RECORD("record"),
        LIST("list");

        private final String text;

        SimplePerfType(final String text) {
            this.text = text;
        }

        @Override
        public String toString() {
            return text;
        }
    }

}
