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

package com.android.monkey;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.util.clockwork.ClockworkUtils;

import java.util.ArrayList;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.List;
import java.util.Map;

/** Runner for paired stress tests which use the monkey command. */
public class MonkeyPairedBase extends MonkeyBase implements IMultiDeviceTest {

    @Option(name = "companion-recurring-command", description = "recurring shell command "
            + "on companion")
    private String mCompanionRecurringCommand = null;

    @Option(name = "companion-recurring-interval", description = "interval between recurring "
            + "command in seconds")
    private int mCompanionRecurringInterval = 25;

    private ITestDevice mCompanion;
    private List<ITestDevice> mDeviceList = new ArrayList<>();
    private ScheduledExecutorService mScheduler;

    /**
     * Fetches the companion device allocated for the primary device
     *
     * @return the allocated companion device
     * @throws RuntimeException if no companion device has been allocated
     */
    protected ITestDevice getCompanion() {
        return mCompanion;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (mCompanionRecurringCommand != null) {
            scheduleRecurringCommand();
        }
        try {
            super.run(listener);
        } finally {
            stopRecurringCommand();
        }
    }

    protected void scheduleRecurringCommand() {
        mScheduler = Executors.newScheduledThreadPool(1);
        mScheduler.scheduleAtFixedRate(new Runnable() {
            @Override
            public void run() {
                try {
                    getCompanion().executeShellCommand(mCompanionRecurringCommand);
                } catch (DeviceNotAvailableException e) {
                    CLog.e("Recurring command failed on %s (%s)", getCompanion().getSerialNumber(),
                            mCompanionRecurringCommand);
                }
            }
        }, mCompanionRecurringInterval, mCompanionRecurringInterval, TimeUnit.SECONDS);
    }

    protected void stopRecurringCommand() {
        mScheduler.shutdownNow();
        try {
            mScheduler.awaitTermination(mCompanionRecurringInterval, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            CLog.e("Could not terminate recurring command on %s (%s)",
                    getCompanion().getSerialNumber(), mCompanionRecurringCommand);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
        ClockworkUtils cwUtils = new ClockworkUtils();
        mCompanion = cwUtils.setUpMultiDevice(deviceInfos, mDeviceList);
    }
}