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

package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.ArrayList;
import java.util.List;

/**
 * A {@link ITargetPreparer} for removing an apk from the system partition before a test run. Note
 * that the device is rebooted before actual test execution, so the system partition remains
 * read-only during the test run.
 */
public class RemoveSystemAppPreparer extends BaseTargetPreparer {
    @Option(name = "uninstall-system-app",
            description = "the name of the file to remove (extension included)")
    private List<String> mFiles= new ArrayList<String>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException {

        device.remountSystemWritable();
        for (String file : mFiles) {
            CLog.d("Removing system app %s from /system/app", file);
            device.executeShellCommand(String.format("rm /system/app/%s", file));
            CLog.d("Removing system app %s from /system/priv-app", file);
            device.executeShellCommand(String.format("rm /system/priv-app/%s", file));
        }

        // Reboot the device to put /system back into read-only
        device.reboot();
        device.waitForDeviceAvailable();
    }

}

