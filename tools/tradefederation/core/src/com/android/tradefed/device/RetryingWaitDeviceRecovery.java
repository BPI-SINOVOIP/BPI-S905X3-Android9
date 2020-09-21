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
package com.android.tradefed.device;

import com.android.tradefed.config.Option;
import com.android.tradefed.log.LogUtil.CLog;

/**
 * A {@link WaitDeviceRecovery} which retries its recovery step either indefinitely
 * or for a certain number of iterations.
 */
public class RetryingWaitDeviceRecovery extends WaitDeviceRecovery {

    @Option(name = "max-wait-iter",
            description = "maximum number of retries for device recovery, 0 for unlimited")
    private int mMaxIters = 0;

    @Override
    public void recoverDevice(IDeviceStateMonitor monitor, boolean recoverUntilOnline) {
        int iter = 0;
        while (iter < mMaxIters || mMaxIters == 0) {
            try {
                super.recoverDevice(monitor, recoverUntilOnline);
                return;
            } catch (DeviceNotAvailableException e) {
                CLog.i("Wait attempt %d failed, trying again. Max iterations: %d",
                        ++iter, mMaxIters);
            }
        }
    }
}

