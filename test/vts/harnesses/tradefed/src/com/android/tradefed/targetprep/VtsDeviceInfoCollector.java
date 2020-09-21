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

package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.ArrayUtil;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Collects device info. This's a fork of DeviceInfoCollector and is forked in order to simplify the
 * change deployment process and reduce the deployment time, which are critical for VTS services.
 */
public class VtsDeviceInfoCollector implements ITargetPreparer, ITargetCleaner {
    // TODO(trong): remove "cts:" prefix, will need a custom ResultReporter.
    private static final Map<String, String> BUILD_KEYS = new HashMap<>();
    private static final Map<String, String> BUILD_LEGACY_PROPERTIES = new HashMap<>();
    private static final long REBOOT_TIMEOUT = 1000 * 60 * 2; // 2 minutes.

    @Option(name = "disable-framework", description = "Initialize device by stopping framework.")
    private boolean mDisableFramework = false;

    static {
        BUILD_KEYS.put("cts:build_id", "ro.build.id");
        BUILD_KEYS.put("cts:build_product", "ro.product.name");
        BUILD_KEYS.put("cts:build_device", "ro.product.device");
        BUILD_KEYS.put("cts:build_board", "ro.product.board");
        BUILD_KEYS.put("cts:build_manufacturer", "ro.product.manufacturer");
        BUILD_KEYS.put("cts:build_brand", "ro.product.brand");
        BUILD_KEYS.put("cts:build_model", "ro.product.model");
        BUILD_KEYS.put("cts:build_type", "ro.build.type");
        BUILD_KEYS.put("cts:build_tags", "ro.build.tags");
        BUILD_KEYS.put("cts:build_fingerprint", "ro.vendor.build.fingerprint");
        BUILD_KEYS.put("cts:build_abi", "ro.product.cpu.abi");
        BUILD_KEYS.put("cts:build_abi2", "ro.product.cpu.abi2");
        BUILD_KEYS.put("cts:build_abis", "ro.product.cpu.abilist");
        BUILD_KEYS.put("cts:build_abis_32", "ro.product.cpu.abilist32");
        BUILD_KEYS.put("cts:build_abis_64", "ro.product.cpu.abilist64");
        BUILD_KEYS.put("cts:build_serial", "ro.serialno");
        BUILD_KEYS.put("cts:build_version_release", "ro.build.version.release");
        BUILD_KEYS.put("cts:build_version_sdk", "ro.build.version.sdk");
        BUILD_KEYS.put("cts:build_version_base_os", "ro.build.version.base_os");
        BUILD_KEYS.put("cts:build_version_security_patch", "ro.build.version.security_patch");
        BUILD_KEYS.put("cts:build_reference_fingerprint", "ro.build.reference.fingerprint");
        BUILD_KEYS.put("cts:build_system_fingerprint", "ro.build.fingerprint");
        BUILD_KEYS.put("cts:build_vendor_fingerprint", "ro.vendor.build.fingerprint");
        BUILD_KEYS.put("cts:build_vendor_manufacturer", "ro.product.vendor.manufacturer");
        BUILD_KEYS.put("cts:build_vendor_model", "ro.product.vendor.model");

        BUILD_LEGACY_PROPERTIES.put(
                "ro.product.vendor.manufacturer", "ro.vendor.product.manufacturer");
        BUILD_LEGACY_PROPERTIES.put("ro.product.vendor.model", "ro.vendor.product.model");
    }

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        for (Entry<String, String> entry : BUILD_KEYS.entrySet()) {
            String propertyValue = device.getProperty(entry.getValue());
            if ((propertyValue == null || propertyValue.length() == 0)
                    && BUILD_LEGACY_PROPERTIES.containsKey(entry.getValue())) {
                propertyValue = device.getProperty(BUILD_LEGACY_PROPERTIES.get(entry.getValue()));
            }
            buildInfo.addBuildAttribute(entry.getKey(), ArrayUtil.join(",", propertyValue));
        }

        if (mDisableFramework) {
            // Set the default device runtime state to stopped.
            device.executeShellCommand("stop");
            device.executeShellCommand("setprop sys.boot_completed 0");
        }
    }

    /** {@inheritDoc} */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (mDisableFramework) {
            // Restore the framework at test run completion.
            long startTime = System.currentTimeMillis();
            device.waitForDeviceOnline(REBOOT_TIMEOUT);
            device.executeShellCommand("start");
            if (!device.waitForBootComplete(
                        REBOOT_TIMEOUT + startTime - System.currentTimeMillis())) {
                throw new DeviceNotAvailableException("Framework irrecoverable after testing.");
            }
        }
    }
}
