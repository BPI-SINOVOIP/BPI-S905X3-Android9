/*
 * Copyright (C) 2017 The Android Open Source Project
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

/** Target preparer that restarts the system server without rebooting the device. */
@OptionClass(alias = "restart-system-server")
public class RestartSystemServerTargetPreparer extends BaseTargetPreparer
        implements ITargetCleaner {
    @Option(name = "restart-setup", description = "Restart system server on setup.")
    private boolean mRestartBefore = true;

    @Option(name = "restart-teardown", description = "Restart system server on tear down.")
    private boolean mRestartAfter = false;

    // Exposed for testing.
    static final String KILL_SERVER_COMMAND = "pkill system_server";

    private void restartSystemServer(ITestDevice device) throws DeviceNotAvailableException {
        device.enableAdbRoot();
        device.executeShellCommand("setprop dev.bootcomplete 0");
        device.executeShellCommand(KILL_SERVER_COMMAND);
        device.waitForDeviceAvailable();
    }

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        if (!mRestartBefore) {
            return;
        }
        restartSystemServer(device);
    }

    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (!mRestartAfter) {
            return;
        }
        restartSystemServer(device);
    }
}
