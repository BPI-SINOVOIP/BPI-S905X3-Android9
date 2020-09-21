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

import com.android.ddmlib.CollectingOutputReceiver;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.concurrent.TimeUnit;

/**
 * A {@link ITargetPreparer} that pushes files from tests zip onto device, mark them as executable
 * and invokes the binary or script on device. See also {@link TestFilePushSetup}.
 */
@OptionClass(alias = "push-file-invoker")
public class PushFileInvoker extends TestFilePushSetup {

    @Option(name = "script-timeout",
            description = "Timeout for script execution, can be any valid time string",
            isTimeVal = true)
    private long mScriptTimeout = 2 * 60 * 1000;

    @Option(name = "execute-as-root",
            description = "Execute the pushed files with root identity. Note that this requires "
                    + "'su' on device, typically only available on debug builds.")
    private boolean mExecuteAsRoot = true;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        if (isDisabled()) {
            CLog.i("Performance setup script disabled.");
            return;
        }
        // call super to push files first
        super.setUp(device, buildInfo);
        for (String file : getTestFileNames()) {
            String devicePath = String.format("/data/%s", file);
            if (!device.doesFileExist(devicePath)) {
                CLog.w("Ignoring non-existent path %s", devicePath);
                continue;
            }
            if (device.isDirectory(devicePath)) {
                CLog.w("%s is a directory, skipping", devicePath);
                continue;
            }
            // first ensure that file is executable
            device.executeShellCommand(String.format("chmod 755 %s", devicePath));
            // next decide if we need `su`
            if (mExecuteAsRoot) {
                devicePath = String.format("su root %s", devicePath);
            }
            CollectingOutputReceiver receiver = new CollectingOutputReceiver();
            device.executeShellCommand(devicePath, receiver,
                    mScriptTimeout, TimeUnit.MILLISECONDS, 0);
            CLog.d("Executed perf setup script\noutput:\n%s\nEND", receiver.getOutput());
            device.waitForDeviceAvailable();
        }
    }
}
