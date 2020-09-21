/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.sl4a.Sl4aClient;

import java.io.File;
import java.io.IOException;
import java.util.HashSet;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Utility functions for calling BluetoothInstrumentation on device
 * <p>
 * Device side BluetoothInstrumentation code can be found in AOSP at:
 * <code>frameworks/base/core/tests/bluetoothtests</code>
 *
 */
public class BluetoothUtils {

    private static final String BT_INSTR_CMD = "am instrument -w -r -e command %s "
            + "com.android.bluetooth.tests/android.bluetooth.BluetoothInstrumentation";
    private static final String SUCCESS_INSTR_OUTPUT = "INSTRUMENTATION_RESULT: result=SUCCESS";
    private static final String BT_GETADDR_HEADER = "INSTRUMENTATION_RESULT: address=";
    private static final long BASE_RETRY_DELAY_MS = 60 * 1000;
    private static final int MAX_RETRIES = 3;
    private static final Pattern BONDED_MAC_HEADER =
            Pattern.compile("INSTRUMENTATION_RESULT: device-\\d{2}=(.*)$");
    private static final String BT_STACK_CONF = "/etc/bluetooth/bt_stack.conf";
    public static final String BTSNOOP_API = "bluetoothConfigHciSnoopLog";
    public static final String BTSNOOP_CMD = "setprop persist.bluetooth.btsnoopenable ";
    public static final String BTSNOOP_ENABLE_CMD = BTSNOOP_CMD + "true";
    public static final String BTSNOOP_DISABLE_CMD = BTSNOOP_CMD + "false";
    public static final String GOLD_BTSNOOP_LOG_PATH = "/data/misc/bluetooth/logs/btsnoop_hci.log";
    public static final String O_BUILD = "O";

    /**
     * Convenience method to execute BT instrumentation command and return output
     *
     * @param device
     * @param command a command string sent over to BT instrumentation, currently supported:
     *                 enable, disable, unpairAll, getName, getAddress, getBondedDevices; refer to
     *                 AOSP source for more details
     * @return output of BluetoothInstrumentation
     * @throws DeviceNotAvailableException
     */
    public static String runBluetoothInstrumentation(ITestDevice device, String command)
            throws DeviceNotAvailableException {
        CollectingOutputReceiver receiver = new CollectingOutputReceiver();
        device.executeShellCommand(String.format(BT_INSTR_CMD, command), receiver);
        String output = receiver.getOutput();
        CLog.v("bluetooth instrumentation sub command: %s\noutput:\n", command);
        CLog.v(output);
        return output;
    }

    public static boolean runBluetoothInstrumentationWithRetry(ITestDevice device, String command)
        throws DeviceNotAvailableException {
        for (int retry = 0; retry < MAX_RETRIES; retry++) {
            String output = runBluetoothInstrumentation(device, command);
            if (output.contains(SUCCESS_INSTR_OUTPUT)) {
                return true;
            }
            RunUtil.getDefault().sleep(retry * BASE_RETRY_DELAY_MS);
        }
        return false;
    }

    /**
     * Retries clearing of BT pairing with linear backoff
     * @param device
     * @throws DeviceNotAvailableException
     */
    public static boolean unpairWithRetry(ITestDevice device)
            throws DeviceNotAvailableException {
        return runBluetoothInstrumentationWithRetry(device, "unpairAll");
    }

    /**
     * Retrieves BT mac of the given device
     *
     * @param device
     * @return BT mac or null if not found
     * @throws DeviceNotAvailableException
     */
    public static String getBluetoothMac(ITestDevice device) throws DeviceNotAvailableException {
        String lines[] = runBluetoothInstrumentation(device, "getAddress").split("\\r?\\n");
        for (String line : lines) {
            line = line.trim();
            if (line.startsWith(BT_GETADDR_HEADER)) {
                return line.substring(BT_GETADDR_HEADER.length());
            }
        }
        return null;
    }

    /**
     * Enables bluetooth on the given device
     *
     * @param device
     * @return True if enable is successful, false otherwise
     * @throws DeviceNotAvailableException
     */
    public static boolean enable(ITestDevice device)
            throws DeviceNotAvailableException {
        return runBluetoothInstrumentationWithRetry(device, "enable");
    }

    /**
     * Disables bluetooth on the given device
     *
     * @param device
     * @return True if disable is successful, false otherwise
     * @throws DeviceNotAvailableException
     */
    public static boolean disable(ITestDevice device)
            throws DeviceNotAvailableException {
        return runBluetoothInstrumentationWithRetry(device, "disable");
    }

    /**
     * Returns bluetooth mac addresses of devices that the given device has bonded with
     *
     * @param device
     * @return bluetooth mac addresses
     * @throws DeviceNotAvailableException
     */
    public static Set<String> getBondedDevices(ITestDevice device)
            throws DeviceNotAvailableException {
        String lines[] = runBluetoothInstrumentation(device, "getBondedDevices").split("\\r?\\n");
        return parseBondedDeviceInstrumentationOutput(lines);
    }

    /** Parses instrumentation output into mac addresses */
    static Set<String> parseBondedDeviceInstrumentationOutput(String[] lines) {
        Set<String> ret = new HashSet<>();
        for (String line : lines) {
            Matcher m = BONDED_MAC_HEADER.matcher(line.trim());
            if (m.find()) {
                ret.add(m.group(1));
            }
        }
        return ret;
    }

    /**
     * Confirm branch version if it is Gold or not based on build alias
     *
     * @param device Test device to check
     * @throws DeviceNotAvailableException
     */
    private static boolean isGoldAndAbove(ITestDevice device) throws DeviceNotAvailableException {
        // TODO: Use API level once it is bumped up
        int apiLevel = device.getApiLevel();
        if (!"REL".equals(device.getProperty("ro.build.version.codename"))) {
            apiLevel++;
        }
        return apiLevel >= 25;
    }

    /**
     * Enable btsnoop logging by sl4a call
     *
     * @param device
     * @return success or not
     * @throws DeviceNotAvailableException
     */
    public static boolean enableBtsnoopLogging(ITestDevice device)
            throws DeviceNotAvailableException {
        if (isGoldAndAbove(device)) {
            device.executeShellCommand(BTSNOOP_ENABLE_CMD);
            disable(device);
            enable(device);
            return true;
        }
        return enableBtsnoopLogging(device, null);
    }

    /**
     * Enable btsnoop logging by sl4a call
     *
     * @param device
     * @param sl4aApkFile sl4a.apk file location, null if it has been installed
     * @return success or not
     * @throws DeviceNotAvailableException
     */
    public static boolean enableBtsnoopLogging(ITestDevice device, File sl4aApkFile)
            throws DeviceNotAvailableException {
        Sl4aClient client = new Sl4aClient(device, sl4aApkFile);
        return toggleBtsnoopLogging(client, true);
    }

    /**
     * Disable btsnoop logging by sl4a call
     *
     * @param device
     * @return success or not
     * @throws DeviceNotAvailableException
     */
    public static boolean disableBtsnoopLogging(ITestDevice device)
            throws DeviceNotAvailableException {
        if (isGoldAndAbove(device)) {
            device.executeShellCommand(BTSNOOP_DISABLE_CMD);
            disable(device);
            enable(device);
            return true;
        }
        return disableBtsnoopLogging(device, null);
    }

    /**
     * Disable btsnoop logging by sl4a call
     *
     * @param device
     * @param sl4aApkFile sl4a.apk file location, null if it has been installed
     * @return success or not
     * @throws DeviceNotAvailableException
     */
    public static boolean disableBtsnoopLogging(ITestDevice device, File sl4aApkFile)
            throws DeviceNotAvailableException {
        Sl4aClient client = new Sl4aClient(device, sl4aApkFile);
        return toggleBtsnoopLogging(client, false);
    }

    public static boolean toggleBtsnoopLogging(Sl4aClient client, boolean onOff)
            throws DeviceNotAvailableException {
        try {
            client.startSl4A();
            client.rpcCall(BTSNOOP_API, onOff);
            return true;
        } catch (IOException | RuntimeException e) {
            CLog.e(e);
            return false;
        } finally {
            client.close();
        }
    }

    /**
     * Upload snoop log file for test results
     */
    public static void uploadLogFiles(ITestInvocationListener listener, ITestDevice device,
            String type, int iteration) throws DeviceNotAvailableException {
        File logFile = null;
        InputStreamSource logSource = null;
        String fileName = getBtSnoopLogFilePath(device);
        if (fileName != null) {
            try {
                logFile = device.pullFile(fileName);
                if (logFile != null) {
                    CLog.d(
                            "Sending %s %d byte file %s into the logosphere!",
                            type, logFile.length(), logFile);
                    logSource = new FileInputStreamSource(logFile);
                    listener.testLog(
                            String.format("%s_btsnoop_%d", type, iteration),
                            LogDataType.UNKNOWN,
                            logSource);
                }
            } finally {
                FileUtil.deleteFile(logFile);
                StreamUtil.cancel(logSource);
            }
        } else {
            return;
        }
    }

    /**
     * Delete snoop log file from device
     */
    public static void cleanLogFile(ITestDevice device) throws DeviceNotAvailableException {
        String fileName = getBtSnoopLogFilePath(device);
        if (fileName != null) {
            device.executeShellCommand(String.format("rm %s", fileName));
        } else {
            CLog.e("Not able to delete BT snoop log, file not found");
        }
    }

    /**
     * Get bt snoop log file path from bt_stack.config file
     *
     * @param device
     * @return THe file name for bt_snoop_log or null if it is not found
     */
    public static String getBtSnoopLogFilePath(ITestDevice device)
            throws DeviceNotAvailableException {
        if (isGoldAndAbove(device)) {
            return GOLD_BTSNOOP_LOG_PATH;
        }
        String snoopfileSetting =
                device.executeShellCommand(
                        String.format("cat %s | grep BtSnoopFileName", BT_STACK_CONF));
        String[] settingItems = snoopfileSetting.split("=");
        if (settingItems.length > 1) {
            return settingItems[1].trim();
        } else {
            CLog.e(String.format("Not able to local BT snoop log, '%s'", snoopfileSetting));
            return null;
        }
    }
}
