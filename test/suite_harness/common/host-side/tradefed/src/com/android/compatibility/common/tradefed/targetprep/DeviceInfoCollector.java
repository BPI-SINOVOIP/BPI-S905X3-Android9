/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.compatibility.common.tradefed.targetprep;

import com.android.compatibility.common.tradefed.testtype.CompatibilityTest;
import com.android.compatibility.common.util.DeviceInfo;
import com.android.compatibility.common.util.DevicePropertyInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestLoggerReceiver;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;
import java.util.Map.Entry;

/**
 * An {@link ApkInstrumentationPreparer} that collects device info.
 */
public class DeviceInfoCollector extends ApkInstrumentationPreparer implements ITestLoggerReceiver {

    private static final String ABI = "ro.product.cpu.abi";
    private static final String ABI2 = "ro.product.cpu.abi2";
    private static final String ABIS = "ro.product.cpu.abilist";
    private static final String ABIS_32 = "ro.product.cpu.abilist32";
    private static final String ABIS_64 = "ro.product.cpu.abilist64";
    private static final String BOARD = "ro.product.board";
    private static final String BRAND = "ro.product.brand";
    private static final String DEVICE = "ro.product.device";
    private static final String FINGERPRINT = "ro.build.fingerprint";
    private static final String ID = "ro.build.id";
    private static final String MANUFACTURER = "ro.product.manufacturer";
    private static final String MODEL = "ro.product.model";
    private static final String PRODUCT = "ro.product.name";
    private static final String REFERENCE_FINGERPRINT = "ro.build.reference.fingerprint";
    private static final String SERIAL = "ro.serialno";
    private static final String TAGS = "ro.build.tags";
    private static final String TYPE = "ro.build.type";
    private static final String VERSION_BASE_OS = "ro.build.version.base_os";
    private static final String VERSION_RELEASE = "ro.build.version.release";
    private static final String VERSION_SDK = "ro.build.version.sdk";
    private static final String VERSION_SECURITY_PATCH = "ro.build.version.security_patch";
    private static final String VERSION_INCREMENTAL = "ro.build.version.incremental";

    private static final String PREFIX_TAG = "cts:build_";

    @Option(name = CompatibilityTest.SKIP_DEVICE_INFO_OPTION,
            shortName = 'd',
            description = "Whether device info collection should be skipped")
    private boolean mSkipDeviceInfo = false;

    @Option(name= "src-dir", description = "The directory to copy to the results dir")
    private String mSrcDir;

    @Option(name = "dest-dir", description = "The directory under the result to store the files")
    private String mDestDir = DeviceInfo.RESULT_DIR_NAME;

    @Deprecated
    @Option(name = "temp-dir", description = "The directory containing host-side device info files")
    private String mTempDir;

    private ITestLogger mLogger;

    public DeviceInfoCollector() {
        mWhen = When.BEFORE;
    }

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        DevicePropertyInfo devicePropertyInfo = new DevicePropertyInfo(ABI, ABI2, ABIS, ABIS_32,
                ABIS_64, BOARD, BRAND, DEVICE, FINGERPRINT, ID, MANUFACTURER, MODEL, PRODUCT,
                REFERENCE_FINGERPRINT, SERIAL, TAGS, TYPE, VERSION_BASE_OS, VERSION_RELEASE,
                VERSION_SDK, VERSION_SECURITY_PATCH, VERSION_INCREMENTAL);

        // add device properties to the result with a prefix tag for each key
        for (Entry<String, String> entry :
                devicePropertyInfo.getPropertytMapWithPrefix(PREFIX_TAG).entrySet()) {
            buildInfo.addBuildAttribute(
                    entry.getKey(), nullToEmpty(device.getProperty(entry.getValue())));
        }
        if (mSkipDeviceInfo) {
            return;
        }
        run(device, buildInfo);
        File deviceInfoDir = null;
        try {
            deviceInfoDir = FileUtil.createTempDir(DeviceInfo.RESULT_DIR_NAME);
            if (device.pullDir(mSrcDir, deviceInfoDir)) {
                for (File deviceInfoFile : deviceInfoDir.listFiles()) {
                    try (FileInputStreamSource source = new FileInputStreamSource(deviceInfoFile)) {
                        mLogger.testLog(deviceInfoFile.getName(), LogDataType.TEXT, source);
                    }
                }
            } else {
                CLog.e("Failed to pull device-info files from device %s", device.getSerialNumber());
            }
        } catch (IOException e) {
            CLog.e("Failed to pull device-info files from device %s", device.getSerialNumber());
            CLog.e(e);
        } finally {
            FileUtil.recursiveDelete(deviceInfoDir);
        }
    }

    @Override
    public void setTestLogger(ITestLogger testLogger) {
        mLogger = testLogger;
    }

    private static String nullToEmpty(String value) {
        return value == null ? "" : value;
    }
}
