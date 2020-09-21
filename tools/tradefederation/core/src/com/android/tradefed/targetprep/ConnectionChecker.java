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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.RunUtil;

/** Target preparer that waits until an ip address is asigned to any of the specified interfaces. */
@OptionClass(alias = "connection-checker")
public class ConnectionChecker extends BaseTargetPreparer {

    @Option(name="max-wait", description="How long to wait for the device to connect, in seconds")
    private long mTimeout = 600;

    @Option(name="poll-interval", description="How often to poll the device, in seconds")
    private long mInterval = 30;

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        long startTime = System.currentTimeMillis();
        long timeout = mTimeout * 1000;
        while (!device.checkConnectivity()) {
            if (System.currentTimeMillis() - startTime > timeout) {
                throw new TargetSetupError("Device did not connect to the network",
                        device.getDeviceDescriptor());
            }
            RunUtil.getDefault().sleep(mInterval * 1000);
        }
    }
}
