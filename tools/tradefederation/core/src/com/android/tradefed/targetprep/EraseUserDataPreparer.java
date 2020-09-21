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

import java.util.ArrayList;
import java.util.Collection;

/** A {@link ITargetPreparer} that wipes user data on the device. */
@OptionClass(alias = "erase-user-data")
public class EraseUserDataPreparer extends BaseTargetPreparer {

    @Option(name="wipe-skip-list", description=
            "list of /data subdirectories to NOT wipe when doing UserDataFlashOption.TESTS_ZIP")
        private Collection<String> mDataWipeSkipList = new ArrayList<String>();

    @Option(name="wait-for-available",
            description="Wait until device is available for testing before performing erase")
    private boolean mWaitForAvailable = false;

    private ITestsZipInstaller mTestsZipInstaller = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (isDisabled()) {
            return;
        }
        if (mWaitForAvailable) {
            device.waitForDeviceAvailable();
        }
        
        device.enableAdbRoot();
        if (mTestsZipInstaller == null) {
            if (mDataWipeSkipList.isEmpty()) {
                // Maintain backwards compatibility
                // TODO: deprecate and remove
                mDataWipeSkipList.add("media");
            }
            mTestsZipInstaller = new DefaultTestsZipInstaller(mDataWipeSkipList);
        }
        mTestsZipInstaller.deleteData(device);
        device.reboot();
    }
}
