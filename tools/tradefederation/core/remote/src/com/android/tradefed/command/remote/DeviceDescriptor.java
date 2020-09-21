/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.tradefed.command.remote;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.IDevice.DeviceState;
import com.android.tradefed.device.DeviceAllocationState;

/**
 * A class containing information describing a device under test.
 */
public class DeviceDescriptor {

    private final String mSerial;
    private final boolean mIsStubDevice;
    private final DeviceState mDeviceState;
    private final DeviceAllocationState mState;
    private final String mProduct;
    private final String mProductVariant;
    private final String mSdkVersion;
    private final String mBuildId;
    private final String mBatteryLevel;
    private final String mDeviceClass;
    private final String mMacAddress;
    private final String mSimState;
    private final String mSimOperator;
    private final IDevice mIDevice;

    public DeviceDescriptor(String serial, boolean isStubDevice, DeviceAllocationState state,
            String product, String productVariant, String sdkVersion, String buildId,
            String batteryLevel) {
        this(serial, isStubDevice, state, product, productVariant, sdkVersion, buildId,
                batteryLevel, "", "", "", "");
    }

    public DeviceDescriptor(String serial, boolean isStubDevice, DeviceAllocationState state,
            String product, String productVariant, String sdkVersion, String buildId,
            String batteryLevel, String deviceClass, String macAddress, String simState,
            String simOperator) {
        this(
                serial,
                isStubDevice,
                null,
                state,
                product,
                productVariant,
                sdkVersion,
                buildId,
                batteryLevel,
                deviceClass,
                macAddress,
                simState,
                simOperator,
                null);
    }

    public DeviceDescriptor(
            String serial,
            boolean isStubDevice,
            DeviceState deviceState,
            DeviceAllocationState state,
            String product,
            String productVariant,
            String sdkVersion,
            String buildId,
            String batteryLevel,
            String deviceClass,
            String macAddress,
            String simState,
            String simOperator,
            IDevice idevice) {
        mSerial = serial;
        mIsStubDevice = isStubDevice;
        mDeviceState = deviceState;
        mState = state;
        mProduct = product;
        mProductVariant = productVariant;
        mSdkVersion = sdkVersion;
        mBuildId = buildId;
        mBatteryLevel = batteryLevel;
        mDeviceClass = deviceClass;
        mMacAddress = macAddress;
        mSimState = simState;
        mSimOperator = simOperator;
        mIDevice = idevice;
    }

    public String getSerial() {
        return mSerial;
    }

    public boolean isStubDevice() {
        return mIsStubDevice;
    }

    public DeviceState getDeviceState() {
        return mDeviceState;
    }

    public DeviceAllocationState getState() {
        return mState;
    }

    public String getProduct() {
        return mProduct;
    }

    public String getProductVariant() {
        return mProductVariant;
    }

    public String getDeviceClass() {
        return mDeviceClass;
    }

    /*
     * This version maps to the ro.build.version.sdk property.
     */
    public String getSdkVersion() {
        return mSdkVersion;
    }

    public String getBuildId() {
        return mBuildId;
    }

    public String getBatteryLevel() {
        return mBatteryLevel;
    }

    public String getMacAddress() {
        return mMacAddress;
    }

    public String getSimState() {
        return mSimState;
    }

    public String getSimOperator() {
        return mSimOperator;
    }

    public String getProperty(String name) {
        if (mIDevice == null) {
            throw new UnsupportedOperationException("this descriptor does not have IDevice");
        }
        return mIDevice.getProperty(name);
    }

    /**
     * Provides a description with serials, product and build id
     */
    @Override
    public String toString() {
        return String.format("[%s %s:%s %s]", mSerial, mProduct, mProductVariant, mBuildId);
    }
}
