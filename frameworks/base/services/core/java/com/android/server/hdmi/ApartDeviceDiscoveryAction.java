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

package com.android.server.hdmi;

import android.hardware.hdmi.HdmiDeviceInfo;
import android.util.Slog;

import com.android.internal.util.Preconditions;
import com.android.server.hdmi.HdmiControlService.DevicePollingCallback;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

/**
 * Feature action that handles device discovery sequences.
 * Device discovery is launched when device is woken from "Standby" state
 * or enabled "Control for Hdmi" from disabled state.
 *
 * <p>Device discovery goes through the following steps.
 * <ol>
 *   <li>Poll all non-local devices by sending &lt;Polling Message&gt;
 *   <li>Gather "Physical address" and "device type" of all acknowledged devices
 *   <li>Gather "OSD (display) name" of all acknowledge devices
 *   <li>Gather "Vendor id" of all acknowledge devices
 * </ol>
 * We attempt to get OSD name/vendor ID up to 5 times in case the communication fails.
 */
class ApartDeviceDiscoveryAction extends DeviceDiscoveryAction {
    private static final String TAG = "ApartDeviceDiscoveryAction";

    private static final class DiscoveryDeviceInfo extends DeviceInfo {
        protected DiscoveryDeviceInfo(int logicalAddress) {
            super(logicalAddress);
        }
        private int mState = STATE_WAITING_FOR_DEVICE_POLLING;
        private int mTimeoutRetry = 0;
        private boolean mProcessed = false;

            @Override
        public String toString() {
            StringBuffer s = new StringBuffer();
            s.append("mLogicalAddress:").append(mLogicalAddress).append(" ");
            s.append("mState:").append(mState).append(" ");
            s.append("mProcessed:").append(mProcessed);
            return s.toString();
        }
    }

    /**
     * Constructor.
     *
     * @param source an instance of {@link HdmiCecLocalDevice}.
     */
    ApartDeviceDiscoveryAction(HdmiCecLocalDevice source, DeviceDiscoveryCallback callback) {
        super(source, callback);
    }

    @Override
    boolean start() {
        mDevices.clear();
        mState = STATE_WAITING_FOR_DEVICE_POLLING;
        Slog.v(TAG, "start and pollDevices");

        pollDevices(new DevicePollingCallback() {
            @Override
            public void onPollingFinished(List<Integer> ackedAddress) {
                //We should make sure the action is removed
                if (STATE_NONE == mState) {
                    Slog.e(TAG, "action has been removed.");
                    return;
                }
                if (ackedAddress.isEmpty()) {
                    Slog.v(TAG, "No device is detected.");
                    wrapUpAndFinish(null);
                    return;
                }

                Slog.v(TAG, "Device detected: " + ackedAddress);
                processDevices(ackedAddress);
            }
        }, Constants.POLL_ITERATION_REVERSE_ORDER
            | Constants.POLL_STRATEGY_REMOTES_DEVICES, HdmiConfig.DEVICE_POLLING_RETRY);
        return true;
    }

    private void processDevices(List<Integer> addresses) {
        for (Integer i : addresses) {
            DiscoveryDeviceInfo info = new DiscoveryDeviceInfo(i);
            mDevices.add(info);
            startPhysicalAddressStage(info);
        }
    }

    private void startPhysicalAddressStage(DiscoveryDeviceInfo device) {
        Slog.v(TAG, "Start [Physical Address Stage]:" + device);
        device.mState = STATE_WAITING_FOR_PHYSICAL_ADDRESS;
        checkAndProceedStage(device);
    }

    private void queryPhysicalAddress(DiscoveryDeviceInfo device) {
        if (!verifyValidLogicalAddress(device.mLogicalAddress)) {
            checkAndProceedStage(device);
            return;
        }

        mActionTimer.clearTimerMessage();

        // Check cache first and send request if not exist.
        if (mayProcessMessageIfCached(device.mLogicalAddress, Constants.MESSAGE_REPORT_PHYSICAL_ADDRESS)) {
            return;
        }
        sendCommand(HdmiCecMessageBuilder.buildGivePhysicalAddress(getSourceAddress(), device.mLogicalAddress));
        addTimer(device.mState, device.mLogicalAddress, HdmiConfig.TIMEOUT_MS);
    }

    private void startOsdNameStage(DiscoveryDeviceInfo device) {
        Slog.v(TAG, "Start [Osd Name Stage]:" + device);
        device.mProcessed = false;
        device.mState = STATE_WAITING_FOR_OSD_NAME;

        checkAndProceedStage(device);
    }

    private void queryOsdName(DiscoveryDeviceInfo device) {
        if (!verifyValidLogicalAddress(device.mLogicalAddress)) {
            checkAndProceedStage(device);
            return;
        }

        mActionTimer.clearTimerMessage();

        if (mayProcessMessageIfCached(device.mLogicalAddress, Constants.MESSAGE_SET_OSD_NAME)) {
            return;
        }
        sendCommand(HdmiCecMessageBuilder.buildGiveOsdNameCommand(getSourceAddress(), device.mLogicalAddress));
        addTimer(device.mState, device.mLogicalAddress, HdmiConfig.TIMEOUT_MS);
    }

    private void startVendorIdStage(DiscoveryDeviceInfo device) {
        Slog.v(TAG, "Start [Vendor Id Stage]:" + device);

        device.mProcessed = false;
        device.mState = STATE_WAITING_FOR_VENDOR_ID;

        checkAndProceedStage(device);
    }

    private void queryVendorId(DiscoveryDeviceInfo device) {
        if (!verifyValidLogicalAddress(device.mLogicalAddress)) {
            checkAndProceedStage(device);
            return;
        }

        mActionTimer.clearTimerMessage();

        if (mayProcessMessageIfCached(device.mLogicalAddress, Constants.MESSAGE_DEVICE_VENDOR_ID)) {
            return;
        }
        sendCommand(
                HdmiCecMessageBuilder.buildGiveDeviceVendorIdCommand(getSourceAddress(), device.mLogicalAddress));
        addTimer(device.mState, device.mLogicalAddress, HdmiConfig.TIMEOUT_MS);
    }

    @Override
    boolean processCommand(HdmiCecMessage cmd) {
        DiscoveryDeviceInfo device = getDevice(cmd.getSource());
        if (null == device) {
            return false;
        }
        switch (device.mState) {
            case STATE_WAITING_FOR_PHYSICAL_ADDRESS:
                if (cmd.getOpcode() == Constants.MESSAGE_REPORT_PHYSICAL_ADDRESS) {
                    handleReportPhysicalAddress(cmd, device);
                    return true;
                }
                return false;
            case STATE_WAITING_FOR_OSD_NAME:
                if (cmd.getOpcode() == Constants.MESSAGE_SET_OSD_NAME) {
                    handleSetOsdName(cmd, device);
                    return true;
                } else if ((cmd.getOpcode() == Constants.MESSAGE_FEATURE_ABORT) &&
                        ((cmd.getParams()[0] & 0xFF) == Constants.MESSAGE_GIVE_OSD_NAME)) {
                    handleSetOsdName(cmd, device);
                    return true;
                }
                return false;
            case STATE_WAITING_FOR_VENDOR_ID:
                if (cmd.getOpcode() == Constants.MESSAGE_DEVICE_VENDOR_ID) {
                    handleVendorId(cmd, device);
                    return true;
                } else if ((cmd.getOpcode() == Constants.MESSAGE_FEATURE_ABORT) &&
                        ((cmd.getParams()[0] & 0xFF) == Constants.MESSAGE_GIVE_DEVICE_VENDOR_ID)) {
                    handleVendorId(cmd, device);
                    return true;
                }
                return false;
            case STATE_WAITING_FOR_DEVICE_POLLING:
                // Fall through.
            case STATE_FINISHED:
            default:
                return false;
        }
    }

    private void checkAndProceedStage(DiscoveryDeviceInfo device) {
        if (null == device) {
            return;
        }

        if (!mDevices.contains(device)) {
            wrapUpAndFinish(null);
            return;
        }

        if (device.mProcessed) {
            // If finished current stage, move on to next stage.
            switch (device.mState) {
                case STATE_WAITING_FOR_PHYSICAL_ADDRESS:
                    startOsdNameStage(device);
                    return;
                case STATE_WAITING_FOR_OSD_NAME:
                    startVendorIdStage(device);
                    return;
                case STATE_WAITING_FOR_VENDOR_ID:
                    wrapUpAndFinish(device);
                    return;
                case STATE_FINISHED:
                    return;
                default:
                    return;
            }
        } else {
            sendQueryCommand(device);
        }
    }

    private void handleReportPhysicalAddress(HdmiCecMessage cmd, DiscoveryDeviceInfo device) {
        byte params[] = cmd.getParams();
        device.mPhysicalAddress = HdmiUtils.twoBytesToInt(params);
        device.mPortId = getPortId(device.mPhysicalAddress);
        device.mDeviceType = params[2] & 0xFF;
        device.mDisplayName = HdmiUtils.getDefaultDeviceName(device.mDeviceType);

        Slog.d(TAG, "device has reported physical address " + device);
        // This is to manager CEC device separately in case they don't have address.
        if (mIsTvDevice) {
            tv().updateCecSwitchInfo(device.mLogicalAddress, device.mDeviceType,
                device.mPhysicalAddress);
        }
        device.mTimeoutRetry = 0;
        device.mProcessed = true;
        checkAndProceedStage(device);
    }

    private void handleSetOsdName(HdmiCecMessage cmd, DiscoveryDeviceInfo device) {
        String displayName = null;
        try {
            if (cmd.getOpcode() == Constants.MESSAGE_FEATURE_ABORT) {
                displayName = HdmiUtils.getDefaultDeviceName(device.mLogicalAddress);
            } else {
                displayName = new String(cmd.getParams(), "US-ASCII");
            }
        } catch (UnsupportedEncodingException e) {
            Slog.w(TAG, "Failed to decode display name: " + cmd.toString());
            // If failed to get display name, use the default name of device.
            displayName = HdmiUtils.getDefaultDeviceName(device.mLogicalAddress);
        }
        device.mDisplayName = displayName;
        device.mProcessed = true;
        device.mTimeoutRetry = 0;
        checkAndProceedStage(device);
    }

    private void handleVendorId(HdmiCecMessage cmd, DiscoveryDeviceInfo device) {
        if (cmd.getOpcode() != Constants.MESSAGE_FEATURE_ABORT) {
            byte[] params = cmd.getParams();
            int vendorId = HdmiUtils.threeBytesToInt(params);
            device.mVendorId = vendorId;
        }

        device.mProcessed = true;
        device.mTimeoutRetry = 0;
        checkAndProceedStage(device);;
    }

    private void wrapUpAndFinish(DiscoveryDeviceInfo device) {
        if (device != null) {
            device.mState = STATE_FINISHED;
            mProcessedDeviceCount++;
            HdmiDeviceInfo cecDeviceInfo = device.toHdmiDeviceInfo();
            mCallback.onDeviceDiscovered(cecDeviceInfo);
            // Process any commands buffered while device discovery action was in progress.
            if (mIsTvDevice) {
                tv().processDelayedMessages(device.mLogicalAddress);
            }
        }
        Slog.v(TAG, "wrapUpAndFinish DeviceInfo: " + device + " processed " + mProcessedDeviceCount + " devices " + mDevices.size());

        if (mProcessedDeviceCount == mDevices.size()) {
            Slog.v(TAG, "wrapUpAndFinish over");
            super.wrapUpAndFinish();
            finish();
            return;
        }
    }

    private void sendQueryCommand(DiscoveryDeviceInfo device) {
        switch (device.mState) {
            case STATE_WAITING_FOR_PHYSICAL_ADDRESS:
                queryPhysicalAddress(device);
                return;
            case STATE_WAITING_FOR_OSD_NAME:
                queryOsdName(device);
                return;
            case STATE_WAITING_FOR_VENDOR_ID:
                queryVendorId(device);
            default:
                return;
        }
    }

    @Override
    void handleTimerEvent(int state, int logicAddress) {
        if (STATE_NONE == mState) {
            Slog.e(TAG, "handleTimerEvent action has been removed");
            return;
        }
        DiscoveryDeviceInfo device = getDevice(logicAddress);
        if (null == device) {
            Slog.e(TAG, "handleTimerEvent no device of " + logicAddress);
            return;
        }

        if (device.mState == STATE_NONE || device.mState != state) {
            return;
        }

        if (++device.mTimeoutRetry < HdmiConfig.TIMEOUT_RETRY) {
            sendQueryCommand(device);
            return;
        }
        Slog.v(TAG, "Timeout[State=" + device.mState + ", Device=" + device);
        mDevices.remove(device);
        checkAndProceedStage(device);
    }

    private DiscoveryDeviceInfo getDevice(int logicAddress) {
        for (DeviceInfo device : mDevices) {
            if (device.mLogicalAddress == logicAddress) {
                return (DiscoveryDeviceInfo)device;
            }
        }
        return null;
    }

}
