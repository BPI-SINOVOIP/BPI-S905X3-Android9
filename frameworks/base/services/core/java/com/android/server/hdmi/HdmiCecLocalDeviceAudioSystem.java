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
package com.android.server.hdmi;

import android.hardware.hdmi.HdmiDeviceInfo;
import android.os.SystemProperties;
import android.hardware.hdmi.IHdmiControlCallback;
import java.util.List;
import android.hardware.hdmi.HdmiControlManager;
import com.android.server.hdmi.HdmiAnnotations.ServiceThreadOnly;
import com.android.internal.annotations.GuardedBy;
import com.android.server.hdmi.Constants.LocalActivePort;
import android.hardware.hdmi.IHdmiControlCallback;
import com.android.internal.annotations.VisibleForTesting;
import android.os.RemoteException;
import android.util.Slog;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.stream.Collectors;
import java.util.ArrayList;
import android.util.SparseArray;
import com.android.server.hdmi.DeviceDiscoveryAction.DeviceDiscoveryCallback;
import com.android.internal.util.IndentingPrintWriter;
/**
 * Represent a logical device of type {@link HdmiDeviceInfo#DEVICE_AUDIO_SYSTEM} residing in
 * Android system.
 */
public class HdmiCecLocalDeviceAudioSystem extends HdmiCecLocalDevice {

    private static final String TAG = "HdmiCecLocalDeviceAudioSystem";
    @GuardedBy("mLock")
    @LocalActivePort
    private int mRoutingPort = Constants.CEC_SWITCH_HOME;

    protected HdmiCecLocalDeviceAudioSystem(HdmiControlService service) {
        super(service, HdmiDeviceInfo.DEVICE_AUDIO_SYSTEM);
    }

    // This records the current input of the device.
    // When device is switched to ARC input, mRoutingPort does not record it
    // since it's not an HDMI port used for Routing Control.
    // mLocalActivePort will record whichever input we switch to to keep tracking on
    // the current input status of the device.
    // This can help prevent duplicate switching and provide status information.
    @GuardedBy("mLock")
    @LocalActivePort
    protected int mLocalActivePort = Constants.CEC_SWITCH_HOME;

    // Copy of mDeviceInfos to guarantee thread-safety.
    @GuardedBy("mLock")
    private List<HdmiDeviceInfo> mSafeAllDeviceInfos = Collections.emptyList();

    // Map-like container of all cec devices.
    // device id is used as key of container.
    private final SparseArray<HdmiDeviceInfo> mDeviceInfos = new SparseArray<>();

    @Override
    @HdmiAnnotations.ServiceThreadOnly
    protected void onAddressAllocated(int logicalAddress, int reason) {
        assertRunOnServiceThread();
        mService.sendCecCommand(HdmiCecMessageBuilder.buildReportPhysicalAddressCommand(
                mAddress, mService.getPhysicalAddress(), mDeviceType));
        mService.sendCecCommand(HdmiCecMessageBuilder.buildDeviceVendorIdCommand(
                mAddress, mService.getVendorId()));
        clearDeviceInfoList();
        launchDeviceDiscovery();
        startQueuedActions();
    }

    @Override
    @HdmiAnnotations.ServiceThreadOnly
    protected int getPreferredAddress() {
        assertRunOnServiceThread();
        return SystemProperties.getInt(Constants.PROPERTY_PREFERRED_ADDRESS_AUDIO_SYSTEM,
                Constants.ADDR_UNREGISTERED);
    }

    @Override
    @HdmiAnnotations.ServiceThreadOnly
    protected void setPreferredAddress(int addr) {
        assertRunOnServiceThread();
        SystemProperties.set(Constants.PROPERTY_PREFERRED_ADDRESS_AUDIO_SYSTEM,
                String.valueOf(addr));
    }

    /**
     * Set {@link #mRoutingPort} to a specific {@link LocalActivePort} to record the current active
     * CEC Routing Control related port.
     *
     * @param portId The portId of the new routing port.
     */
    @VisibleForTesting
    protected void setRoutingPort(@LocalActivePort int portId) {
        synchronized (mLock) {
            mRoutingPort = portId;
        }
    }

    /**
     * Get {@link #mRoutingPort}. This is useful when the device needs to route to the last valid
     * routing port.
     */
    @LocalActivePort
    protected int getRoutingPort() {
        synchronized (mLock) {
            return mRoutingPort;
        }
    }

    /**
     * Get {@link #mLocalActivePort}. This is useful when device needs to know the current active
     * port.
     */
    @LocalActivePort
    protected int getLocalActivePort() {
        synchronized (mLock) {
            return mLocalActivePort;
        }
    }

    /**
     * Set {@link #mLocalActivePort} to a specific {@link LocalActivePort} to record the current
     * active port.
     *
     * <p>It does not have to be a Routing Control related port. For example it can be
     * set to {@link Constants#CEC_SWITCH_ARC} but this port is System Audio related.
     *
     * @param activePort The portId of the new active port.
     */
    protected void setLocalActivePort(@LocalActivePort int activePort) {
        synchronized (mLock) {
            mLocalActivePort = activePort;
        }
    }

    @Override
    protected int findKeyReceiverAddress() {
        if (getActiveSource().isValid()) {
            return getActiveSource().logicalAddress;
        }
        HdmiDeviceInfo info = getDeviceInfoByPath(getActivePath());
        if (info != null) {
            return info.getLogicalAddress();
        }
        return Constants.ADDR_INVALID;
    }

    @ServiceThreadOnly
    final HdmiDeviceInfo getDeviceInfoByPath(int path) {
        assertRunOnServiceThread();
        for (HdmiDeviceInfo info : getSafeCecDevicesLocked()) {
            if (info.getPhysicalAddress() == path) {
                return info;
            }
        }
        return null;
    }

    @Override
    @ServiceThreadOnly
    protected boolean handleReportPhysicalAddress(HdmiCecMessage message) {
        assertRunOnServiceThread();
        int path = HdmiUtils.twoBytesToInt(message.getParams());
        int address = message.getSource();
        int type = message.getParams()[2];

        // Ignore if [Device Discovery Action] is going on.
        if (hasAction(DeviceDiscoveryAction.class)) {
            Slog.i(TAG, "Ignored while Device Discovery Action is in progress: " + message);
            return true;
        }

        // Update the device info with TIF, note that the same device info could have added in
        // device discovery and we do not want to override it with default OSD name. Therefore we
        // need the following check to skip redundant device info updating.
        HdmiDeviceInfo oldDevice = getCecDeviceInfo(address);
        if (oldDevice == null || oldDevice.getPhysicalAddress() != path) {
            addDeviceInfo(new HdmiDeviceInfo(
                address, path, mService.pathToPortId(path), type,
                Constants.UNKNOWN_VENDOR_ID, HdmiUtils.getDefaultDeviceName(address)));
            // if we are adding a new device info, send out a give osd name command
            // to update the name of the device in TIF
            mService.sendCecCommand(
                HdmiCecMessageBuilder.buildGiveOsdNameCommand(mAddress, address));
            return true;
        }

        Slog.w(TAG, "Device info exists. Not updating on Physical Address.");
        return true;
    }
    /**
    * Add a new {@link HdmiDeviceInfo}. It returns old device info which has the same
     * logical address as new device info's.
     *
     * @param deviceInfo a new {@link HdmiDeviceInfo} to be added.
     * @return {@code null} if it is new device. Otherwise, returns old {@HdmiDeviceInfo}
     *         that has the same logical address as new one has.
     */
    @ServiceThreadOnly
    @VisibleForTesting
    protected HdmiDeviceInfo addDeviceInfo(HdmiDeviceInfo deviceInfo) {
        assertRunOnServiceThread();
        HdmiDeviceInfo oldDeviceInfo = getCecDeviceInfo(deviceInfo.getLogicalAddress());
        if (oldDeviceInfo != null) {
            removeDeviceInfo(deviceInfo.getId());
        }
        mDeviceInfos.append(deviceInfo.getId(), deviceInfo);
        updateSafeDeviceInfoList();
        return oldDeviceInfo;
    }

    /**
     * Remove a device info corresponding to the given {@code logicalAddress}.
     * It returns removed {@link HdmiDeviceInfo} if exists.
     *
     * @param id id of device to be removed
     * @return removed {@link HdmiDeviceInfo} it exists. Otherwise, returns {@code null}
     */
    @ServiceThreadOnly
    private HdmiDeviceInfo removeDeviceInfo(int id) {
        assertRunOnServiceThread();
        HdmiDeviceInfo deviceInfo = mDeviceInfos.get(id);
        if (deviceInfo != null) {
            mDeviceInfos.remove(id);
        }
        updateSafeDeviceInfoList();
        return deviceInfo;
    }

    @ServiceThreadOnly
    HdmiDeviceInfo getCecDeviceInfo(int logicalAddress) {
        assertRunOnServiceThread();
        return mDeviceInfos.get(HdmiDeviceInfo.idForCecDevice(logicalAddress));
    }

    @ServiceThreadOnly
    private void updateSafeDeviceInfoList() {
        assertRunOnServiceThread();
        List<HdmiDeviceInfo> copiedDevices = HdmiUtils.sparseArrayToList(mDeviceInfos);
        synchronized (mLock) {
            mSafeAllDeviceInfos = copiedDevices;
        }
    }

    @GuardedBy("mLock")
    List<HdmiDeviceInfo> getSafeCecDevicesLocked() {
        ArrayList<HdmiDeviceInfo> infoList = new ArrayList<>();
        for (HdmiDeviceInfo info : mSafeAllDeviceInfos) {
            infoList.add(info);
        }
        return infoList;
    }

    @ServiceThreadOnly
    void doManualPortSwitching(int portId, IHdmiControlCallback callback) {
        if (!mService.isValidPortId(portId)) {
            invokeCallback(callback, HdmiControlManager.RESULT_TARGET_NOT_AVAILABLE);
            return;
        }
        if (portId == getLocalActivePort()) {
            invokeCallback(callback, HdmiControlManager.RESULT_SUCCESS);
            return;
        }
        if (!mService.isControlEnabled()) {
            setRoutingPort(portId);
            setLocalActivePort(portId);
            invokeCallback(callback, HdmiControlManager.RESULT_INCORRECT_MODE);
            return;
        }
        int oldPath = getRoutingPort() != Constants.CEC_SWITCH_HOME
                ? mService.portIdToPath(getRoutingPort())
                : getDeviceInfo().getPhysicalAddress();
        int newPath = mService.portIdToPath(portId);
        if (oldPath == newPath) {
            return;
        }
        setRoutingPort(portId);
        setLocalActivePort(portId);
        setActivePath(newPath);
        HdmiCecMessage routingChange =
                HdmiCecMessageBuilder.buildRoutingChange(mAddress, oldPath, newPath);
        mService.sendCecCommand(routingChange);
    }

    @ServiceThreadOnly
    void invokeCallback(IHdmiControlCallback callback, int result) {
        assertRunOnServiceThread();
        if (callback == null) {
            return;
        }
        try {
            callback.onComplete(result);
        } catch (RemoteException e) {
            Slog.e(TAG, "Invoking callback failed:" + e);
        }
    }

    int getPortId(int physicalAddress) {
        return mService.pathToPortId(physicalAddress);
    }

    @ServiceThreadOnly
    private void launchDeviceDiscovery() {
        assertRunOnServiceThread();
        if (hasAction(DeviceDiscoveryAction.class)) {
            Slog.i(TAG, "Device Discovery Action is in progress. Restarting.");
            removeAction(DeviceDiscoveryAction.class);
        }
        DeviceDiscoveryAction action = new DeviceDiscoveryAction(this,
            new DeviceDiscoveryCallback() {
                @Override
                public void onDeviceDiscoveryDone(List<HdmiDeviceInfo> deviceInfos) {
                    for (HdmiDeviceInfo info : deviceInfos) {
                        addDeviceInfo(info);
                    }
                }
                @Override
                public void onDeviceDiscovered(HdmiDeviceInfo deviceInfo) {
                }
            });
        addAndStartAction(action);
    }

    // Clear all device info.
    @ServiceThreadOnly
    private void clearDeviceInfoList() {
        assertRunOnServiceThread();
        for (HdmiDeviceInfo info : HdmiUtils.sparseArrayToList(mDeviceInfos)) {
            if (info.getPhysicalAddress() == mService.getPhysicalAddress()) {
                continue;
            }
        }
        mDeviceInfos.clear();
        updateSafeDeviceInfoList();
    }

    @Override
    protected void dump(final IndentingPrintWriter pw) {
        pw.println("HdmiCecLocalDeviceAudioSystem:");
        pw.increaseIndent();
        HdmiUtils.dumpSparseArray(pw, "mDeviceInfos:", mDeviceInfos);
        pw.decreaseIndent();
        super.dump(pw);
    }
}
