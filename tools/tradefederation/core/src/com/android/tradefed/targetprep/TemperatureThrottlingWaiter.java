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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.RunUtil;

/** An {@link ITargetPreparer} that waits until device's temperature gets down to target */
@OptionClass(alias = "temperature-throttle-waiter")
public class TemperatureThrottlingWaiter extends BaseTargetPreparer {

    @Option(name = "poll-interval",
            description = "Interval in seconds, to poll for device temperature; defaults to 30s")
    private long mPollIntervalSecs = 30;

    @Option(name = "max-wait", description = "Max wait time in seconds, for device cool down "
            + "to target temperature; defaults to 20 minutes")
    private long mMaxWaitSecs = 60 * 20;

    @Option(name = "abort-on-timeout", description = "If test should be aborted if device is still "
        + " above expected temperature; defaults to false")
    private boolean mAbortOnTimeout = false;

    @Option(name = "post-idle-wait", description = "Additional time to wait in seconds, after "
        + "temperature has reached to target; defaults to 120s")
    private long mPostIdleWaitSecs = 120;

    public static final String DEVICE_TEMPERATURE_FILE_PATH_NAME = "device-temperature-file-path";

    @Option(
        name = DEVICE_TEMPERATURE_FILE_PATH_NAME,
        description =
                "Name of file that contains device"
                        + "temperature. Example: /sys/class/hwmon/hwmon1/device/msm_therm"
    )
    private String mDeviceTemperatureFilePath = null;

    @Option(name = "target-temperature", description = "Target Temperature that device should have;"
        + "defaults to 30c")
    private int mTargetTemperature = 30;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (mDeviceTemperatureFilePath == null) {
            return;
        }
        long start = System.currentTimeMillis();
        long maxWaitMs = mMaxWaitSecs * 1000;
        long intervalMs = mPollIntervalSecs * 1000;
        int deviceTemperature = Integer.MAX_VALUE;
        while (true) {
            // get device temperature
            deviceTemperature = getDeviceTemperature(device, mDeviceTemperatureFilePath);
            if (deviceTemperature > mTargetTemperature) {
                CLog.d("Temperature is still high actual %d/expected %d",
                        deviceTemperature, mTargetTemperature);
            } else {
                CLog.i("Total time elapsed to get to %dc : %ds", mTargetTemperature,
                        (System.currentTimeMillis() - start) / 1000);
                break; // while loop
            }
            if ((System.currentTimeMillis() - start) > maxWaitMs) {
                CLog.w("Temperature is still high, actual %d/expected %d; waiting after %ds",
                        deviceTemperature, mTargetTemperature, maxWaitMs);
                if (mAbortOnTimeout) {
                    throw new TargetSetupError(String.format("Temperature is still high after wait "
                            + "timeout; actual %d/expected %d", deviceTemperature,
                            mTargetTemperature), device.getDeviceDescriptor());
                }
                break; // while loop
            }
            RunUtil.getDefault().sleep(intervalMs);
        }
        // extra idle time after reaching the targetl to stable the system
        RunUtil.getDefault().sleep(mPostIdleWaitSecs * 1000);
        CLog.d("Done waiting, total time elapsed: %ds",
                (System.currentTimeMillis() - start) / 1000);
    }

    /**
     * @param device
     * @param fileName : filename where device temperature is stored
     * @throws DeviceNotAvailableException
     */
    protected int getDeviceTemperature (ITestDevice device, String fileName)
            throws DeviceNotAvailableException, TargetSetupError {
        int deviceTemp = Integer.MAX_VALUE;
        String result = device.executeShellCommand(
                String.format("cat %s", fileName)).trim();
        CLog.i(String.format("Temperature file output : %s", result));
        // example output : Result:30 Raw:7f6f
        if (result == null || result.contains("No such file or directory")) {
            throw new TargetSetupError(String.format("File %s doesn't exist", fileName),
                    device.getDeviceDescriptor());
        } else if (!result.toLowerCase().startsWith("result:")) {
            throw new TargetSetupError(
                    String.format("file content is not as expected. Content : %s", result),
                    device.getDeviceDescriptor());
        }

        try {
            deviceTemp = Integer.parseInt(result.split(" ")[0].split(":")[1].trim());
        } catch (NumberFormatException numEx) {
            CLog.e(String.format("Temperature is not of right format %s", numEx.getMessage()));
            throw numEx;
        }

        return deviceTemp;
    }
}