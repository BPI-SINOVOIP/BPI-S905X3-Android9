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
package com.android.tradefed.device;

import com.android.ddmlib.IDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;

/**
 * Implementation of a {@link ITestDevice} for a full stack android device connected via
 * adb connect.
 * Assume the device serial will be in the format <hostname>:<portnumber> in adb.
 */
public class RemoteAndroidDevice extends TestDevice {
    public static final long WAIT_FOR_ADB_CONNECT = 2 * 60 * 1000;

    protected static final long RETRY_INTERVAL_MS = 5000;
    protected static final int MAX_RETRIES = 5;
    protected static final long DEFAULT_SHORT_CMD_TIMEOUT = 20 * 1000;

    private static final String ADB_SUCCESS_CONNECT_TAG = "connected to";
    private static final String ADB_ALREADY_CONNECTED_TAG = "already";
    private static final String ADB_CONN_REFUSED = "Connection refused";

    /**
     * Creates a {@link RemoteAndroidDevice}.
     *
     * @param device the associated {@link IDevice}
     * @param stateMonitor the {@link IDeviceStateMonitor} mechanism to use
     * @param allocationMonitor the {@link IDeviceMonitor} to inform of allocation state changes.
     */
    public RemoteAndroidDevice(IDevice device, IDeviceStateMonitor stateMonitor,
            IDeviceMonitor allocationMonitor) {
        super(device, stateMonitor, allocationMonitor);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void postAdbRootAction() throws DeviceNotAvailableException {
        // attempt to reconnect first to make sure we didn't loose the connection because of
        // adb root.
        adbTcpConnect(getHostName(), getPortNum());
        waitForAdbConnect(WAIT_FOR_ADB_CONNECT);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void postAdbUnrootAction() throws DeviceNotAvailableException {
        // attempt to reconnect first to make sure we didn't loose the connection because of
        // adb unroot.
        adbTcpConnect(getHostName(), getPortNum());
        waitForAdbConnect(WAIT_FOR_ADB_CONNECT);
    }

    /**
     * Return the hostname associated with the device. Extracted from the serial.
     */
    public String getHostName() {
        if (!checkSerialFormatValid()) {
            throw new RuntimeException(
                    String.format("Serial Format is unexpected: %s "
                            + "should look like <hostname>:<port>", getSerialNumber()));
        }
        return getSerialNumber().split(":")[0];
    }

    /**
     * Return the port number asociated with the device. Extracted from the serial.
     */
    public String getPortNum() {
        if (!checkSerialFormatValid()) {
            throw new RuntimeException(
                    String.format("Serial Format is unexpected: %s "
                            + "should look like <hostname>:<port>", getSerialNumber()));
        }
        return getSerialNumber().split(":")[1];
    }

    /**
     * Check if the format of the serial is as expected <hostname>:port
     * @return true if the format is valid, false otherwise.
     */
    private boolean checkSerialFormatValid() {
        String[] serial =  getSerialNumber().split(":");
        if (serial.length == 2) {
            try {
                Integer.parseInt(serial[1]);
                return true;
            } catch (NumberFormatException nfe) {
                return false;
            }
        }
        return false;
    }

    /**
     * Helper method to adb connect to a given tcp ip Android device
     *
     * @param host the hostname/ip of a tcp/ip Android device
     * @param port the port number of a tcp/ip device
     * @return true if we successfully connected to the device, false
     *         otherwise.
     */
    public boolean adbTcpConnect(String host, String port) {
        for (int i = 0; i < MAX_RETRIES; i++) {
            CommandResult result = getRunUtil().runTimedCmd(DEFAULT_SHORT_CMD_TIMEOUT, "adb",
                    "connect", String.format("%s:%s", host, port));
            if (CommandStatus.SUCCESS.equals(result.getStatus()) &&
                result.getStdout().contains(ADB_SUCCESS_CONNECT_TAG)) {
                CLog.d("adb connect output: status: %s stdout: %s stderr: %s",
                        result.getStatus(), result.getStdout(), result.getStderr());

                // It is possible to get a positive result without it being connected because of
                // the ssh bridge. Retrying to get confirmation, and expecting "already connected".
                if(confirmAdbTcpConnect(host, port)) {
                    return true;
                }
            } else if (CommandStatus.SUCCESS.equals(result.getStatus()) &&
                    result.getStdout().contains(ADB_CONN_REFUSED)) {
                // If we find "Connection Refused", we bail out directly as more connect won't help
                return false;
            }
            CLog.d("adb connect output: status: %s stdout: %s stderr: %s, retrying.",
                    result.getStatus(), result.getStdout(), result.getStderr());
            getRunUtil().sleep((i + 1) * RETRY_INTERVAL_MS);
        }
        return false;
    }

    private boolean confirmAdbTcpConnect(String host, String port) {
        CommandResult resultConfirmation =
                getRunUtil().runTimedCmd(DEFAULT_SHORT_CMD_TIMEOUT, "adb", "connect",
                String.format("%s:%s", host, port));
        if (CommandStatus.SUCCESS.equals(resultConfirmation.getStatus()) &&
                resultConfirmation.getStdout().contains(ADB_ALREADY_CONNECTED_TAG)) {
            CLog.d("adb connect confirmed:\nstdout: %s\nsterr: %s",
                    resultConfirmation.getStdout(), resultConfirmation.getStderr());
            return true;
        } else {
            CLog.d("adb connect confirmation failed:\nstatus:%s\nstdout: %s\nsterr: %s",
                    resultConfirmation.getStatus(), resultConfirmation.getStdout(),
                    resultConfirmation.getStderr());
        }
        return false;
    }

    /**
     * Helper method to adb disconnect from a given tcp ip Android device
     *
     * @param host the hostname/ip of a tcp/ip Android device
     * @param port the port number of a tcp/ip device
     * @return true if we successfully disconnected to the device, false
     *         otherwise.
     */
    public boolean adbTcpDisconnect(String host, String port) {
        CommandResult result = getRunUtil().runTimedCmd(DEFAULT_SHORT_CMD_TIMEOUT, "adb",
                "disconnect",
                String.format("%s:%s", host, port));
        return CommandStatus.SUCCESS.equals(result.getStatus());
    }

    /**
     * Check if the adb connection is enabled.
     */
    public void waitForAdbConnect(final long waitTime) throws DeviceNotAvailableException {
        CLog.i("Waiting %d ms for adb connection.", waitTime);
        long startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < waitTime) {
            if (confirmAdbTcpConnect(getHostName(), getPortNum())) {
                CLog.d("Adb connection confirmed.");
                return;
            }
            getRunUtil().sleep(RETRY_INTERVAL_MS);
        }
        throw new DeviceNotAvailableException(
                String.format("No adb connection after %sms.", waitTime), getSerialNumber());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isEncryptionSupported() {
        // Prevent device from being encrypted since we won't have a way to decrypt on Remote
        // devices since fastboot cannot be use remotely
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getMacAddress() {
        return null;
    }
}
