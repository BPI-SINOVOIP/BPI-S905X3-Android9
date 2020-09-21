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
package com.android.tradefed.build;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

/**
 * A wrapper class for a {@link IBuildInfo}, that contains helper methods to retrieve device
 * platform build information.
 * <p/>
 * Intended to be use for "unbundled" aka not device builds {@link IBuildInfo}, that desire
 * metadata about what device the build was run on.
 */
public class DeviceBuildDescriptor {

    public static final String DEVICE_BUILD_ID = "device_build_id";
    public static final String DEVICE_BUILD_ALIAS = "device_build_alias";
    public static final String DEVICE_BUILD_FLAVOR = "device_build_flavor";
    public static final String DEVICE_DESC = "device_description";
    public static final String DEVICE_PRODUCT = "device_product";

    private final IBuildInfo mBuild;

    public DeviceBuildDescriptor(IBuildInfo build) {
        mBuild = build;
    }

    /**
     * Determines if given {@link IBuildInfo} contains device build metadata
     *
     * @param build
     * @return True if the {@link IBuildInfo} contains the device build metadata, false otherwise
     */
    public static boolean describesDeviceBuild(IBuildInfo build) {
        return build.getBuildAttributes().containsKey(DEVICE_BUILD_ID);
    }

    /**
     * Gets the device build ID. Maps to the ro.build.incremental.id property on device.
     */
    public String getDeviceBuildId() {
        return mBuild.getBuildAttributes().get(DEVICE_BUILD_ID);
    }

    /**
     * Gets the device build alias. Maps to the ro.build.id property on device. Typically follows
     * format IMM76.
     */
    public String getDeviceBuildAlias() {
        return mBuild.getBuildAttributes().get(DEVICE_BUILD_ALIAS);
    }

    /**
     * Gets the device build flavor eg yakju-userdebug.
     */
    public String getDeviceBuildFlavor() {
        return mBuild.getBuildAttributes().get(DEVICE_BUILD_FLAVOR);
    }

    /**
     * Gets a description of the device and build. This is typically a more end-user friendly
     * description compared with {@link #getDeviceBuildAlias()} and {@link #getDeviceBuildFlavor()}
     * but with the possible penalty of being less precise.
     * eg. it wouldn't be possible to distinguish the GSM (yakju) and CDMA (mysid) variants of
     * Google Galaxy Nexus using this string.
     */
    public String getDeviceUserDescription() {
        return mBuild.getBuildAttributes().get(DEVICE_DESC);
    }

    /**
     * Get the product and variant of the device, in product:variant format.
     */
    public String getDeviceProduct() {
        return mBuild.getBuildAttributes().get(DEVICE_PRODUCT);
    }

    /**
     * Inserts attributes from device into build.
     *
     * @param device
     * @throws DeviceNotAvailableException
     */
    public static void injectDeviceAttributes(ITestDevice device, IBuildInfo b)
            throws DeviceNotAvailableException {
        b.addBuildAttribute(DEVICE_BUILD_ID, device.getBuildId());
        b.addBuildAttribute(DEVICE_BUILD_ALIAS, device.getBuildAlias());
        String buildFlavor = String.format("%s-%s", device.getProperty("ro.product.name"),
                device.getProperty("ro.build.type"));
        b.addBuildAttribute(DEVICE_BUILD_FLAVOR, buildFlavor);
        b.addBuildAttribute(DEVICE_DESC, generateDeviceDesc(device));
        b.addBuildAttribute(DEVICE_PRODUCT, generateDeviceProduct(device));
    }

    /**
     * Generate the device description string from device properties.
     * <p/>
     * Description should follow this format: eg
     * Google Galaxy Nexus 4.2
     *
     * @param device
     * @return The device description string
     * @throws DeviceNotAvailableException
     */
    public static String generateDeviceDesc(ITestDevice device)
            throws DeviceNotAvailableException {
        // brand is typically all lower case. Capitalize it
        String brand =  device.getProperty("ro.product.brand");
        if (brand.length() > 1) {
            brand = String.format("%s%s", brand.substring(0, 1).toUpperCase(), brand.substring(1));
        }
        return String.format("%s %s %s", brand, device.getProperty("ro.product.model"),
                device.getProperty("ro.build.version.release"));
    }

    /**
     * Query the product and variant of the device, in product:variant format.
     * @throws DeviceNotAvailableException
     */
    public static String generateDeviceProduct(ITestDevice device)
            throws DeviceNotAvailableException {
        return String.format("%s:%s", device.getProductType(), device.getProductVariant());
    }
}
