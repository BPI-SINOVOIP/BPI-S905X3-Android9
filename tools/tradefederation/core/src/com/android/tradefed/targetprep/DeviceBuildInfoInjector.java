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

import com.android.tradefed.build.DeviceBuildDescriptor;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

/**
 * A {@link ITargetPreparer} that inserts {@link DeviceBuildDescriptor} metadata into the {@link
 * IBuildInfo}.
 *
 * <p>Intended to be used for "unbundled" build types that want test reporters to use metadata about
 * what device platform test was run on.
 */
@OptionClass(alias = "device-build-injector")
public class DeviceBuildInfoInjector extends BaseTargetPreparer {

    @Option(name = "override-device-build-id", description =
            "the device buid id to inject.")
    private String mOverrideDeviceBuildId = null;

    @Option(name = "override-device-build-alias", description =
            "the device buid alias to inject.")
    private String mOverrideDeviceBuildAlias = null;

    @Option(name = "override-device-build-flavor", description =
            "the device build flavor to inject.")
    private String mOverrideDeviceBuildFlavor = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (mOverrideDeviceBuildId != null) {
            buildInfo.addBuildAttribute(DeviceBuildDescriptor.DEVICE_BUILD_ID,
                    mOverrideDeviceBuildId);
        } else {
            buildInfo.addBuildAttribute(DeviceBuildDescriptor.DEVICE_BUILD_ID, device.getBuildId());
        }
        if (mOverrideDeviceBuildAlias != null) {
            buildInfo.addBuildAttribute(DeviceBuildDescriptor.DEVICE_BUILD_ALIAS,
                    mOverrideDeviceBuildAlias);
        } else {
            buildInfo.addBuildAttribute(DeviceBuildDescriptor.DEVICE_BUILD_ALIAS,
                    device.getBuildAlias());
        }
        if (mOverrideDeviceBuildFlavor != null){
            buildInfo.addBuildAttribute(DeviceBuildDescriptor.DEVICE_BUILD_FLAVOR,
                    mOverrideDeviceBuildFlavor);
        } else {
            String buildFlavor = String.format("%s-%s", device.getProperty("ro.product.name"),
                    device.getProperty("ro.build.type"));
            buildInfo.addBuildAttribute(DeviceBuildDescriptor.DEVICE_BUILD_FLAVOR, buildFlavor);
        }
        buildInfo.addBuildAttribute(DeviceBuildDescriptor.DEVICE_DESC,
                DeviceBuildDescriptor.generateDeviceDesc(device));
        buildInfo.addBuildAttribute(DeviceBuildDescriptor.DEVICE_PRODUCT,
                DeviceBuildDescriptor.generateDeviceProduct(device));
    }
}
