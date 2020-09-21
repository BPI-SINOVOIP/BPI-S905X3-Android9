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
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;

/** A {@link ITargetPreparer} that wipes userdata */
@OptionClass(alias = "device-wiper")
public class DeviceWiper extends BaseTargetPreparer {

    @Option(name = "use-erase", description =
            "instruct wiper to use fastboot erase instead of format")
    protected boolean mUseErase = false;

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException {
        if (isDisabled()) {
            return;
        }
        CLog.i("Wiping device");
        device.rebootIntoBootloader();
        if (mUseErase) {
            doErase(device);
        } else {
            doFormat(device);
        }
        device.executeFastbootCommand("reboot");
        device.waitForDeviceAvailable();
    }

    private void doFormat(ITestDevice device) throws DeviceNotAvailableException, TargetSetupError {
        CLog.d("Attempting fastboot wiping");
        CommandResult r = device.executeLongFastbootCommand("-w");
        if (r.getStatus() != CommandStatus.SUCCESS) {
            throw new TargetSetupError(String.format("fastboot wiping failed: %s", r.getStderr()),
                    device.getDeviceDescriptor());
        }
    }

    private void doErase(ITestDevice device) throws DeviceNotAvailableException, TargetSetupError {
        performFastbootOp(device, "erase", "cache");
        performFastbootOp(device, "erase", "userdata");
    }

    private void performFastbootOp(ITestDevice device, String op, String partition)
            throws DeviceNotAvailableException, TargetSetupError {
        CLog.d("Attempting fastboot %s %s", op, partition);
        CommandResult r = device.executeLongFastbootCommand(op, partition);
        if (r.getStatus() != CommandStatus.SUCCESS) {
            throw new TargetSetupError(String.format("%s %s failed: %s", op, partition,
                    r.getStderr()), device.getDeviceDescriptor());
        }
    }
}
