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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.ddmlib.IDevice;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * A {@link ITargetPreparer} which creates and pushes a simple key/value config file to the device.
 */
@OptionClass(alias = "key-value-config")
public class KeyValueConfigPreparer extends BaseTargetPreparer {

    @Option(name = "path", description = "The path of the config file on the device",
            mandatory = true)
    private String mPath = null;

    @Option(name = "config", description = "The key/value pairs of the config")
    private Map<String, String> mKeys = new HashMap<String, String>();

    @Option(name = "separator", description = "The separator used between key and value")
    private String mSep = "=";

    @Option(name = "interpolate", description = "Interpolate path variable")
    private boolean mInterpolate = false;

    /**
     * {@inheritDoc}
     * @throws TargetSetupError
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws DeviceNotAvailableException,
            TargetSetupError {
        if (mPath == null) {
            throw new TargetSetupError("Option path must be specified",
                    device.getDeviceDescriptor());
        }

        StringBuilder config = new StringBuilder();

        for (Entry<String, String> entry : mKeys.entrySet()) {
            config.append(String.format("%s%s%s\n", entry.getKey(), mSep, entry.getValue()));
        }

        String content = config.toString();

        if (mInterpolate) {
            final String externalStorageString = "${EXTERNAL_STORAGE}";
            if (content.contains(externalStorageString)) {
                final String externalStoragePath =
                        device.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
                content = content.replace(externalStorageString, externalStoragePath);
            }
        }

        device.pushString(content, mPath);
    }
}
