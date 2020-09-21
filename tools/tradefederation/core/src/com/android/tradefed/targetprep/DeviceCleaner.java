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
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.RunUtil;

/** Performs reboot or format as cleanup action after test, and optionally turns screen off */
@OptionClass(alias = "device-cleaner")
public class DeviceCleaner extends BaseTargetPreparer implements ITargetCleaner {

    public static enum CleanupAction {
        /** no cleanup action */
        NONE,
        /** reboot the device as post test cleanup */
        REBOOT,
        /** format userdata and cache partitions as post test cleanup */
        FORMAT,
    }

    public static enum PostCleanupAction {
        /** no post cleanup action */
        NONE,
        /** turns screen off after the cleanup action */
        SCREEN_OFF,
        /** turns off screen and stops runtime after the cleanup action */
        SCREEN_OFF_AND_STOP,
    }

    private static final int MAX_SCREEN_OFF_RETRY = 5;
    private static final int SCREEN_OFF_RETRY_DELAY_MS = 2 * 1000;

    @Option(name = "cleanup-action",
            description = "Type of action to perform as a post test cleanup; options are: "
            + "NONE, REBOOT or FORMAT; defaults to NONE")
    private CleanupAction mCleanupAction = CleanupAction.NONE;

    /**
     * @deprecated use --post-cleanup SCREEN_OFF option instead.
     */
    @Deprecated
    @Option(name = "screen-off", description = "After cleanup action, "
            + "if screen should be turned off; defaults to false; "
            + "[deprecated] use --post-cleanup SCREEN_OFF instead")
    private boolean mScreenOff = false;

    @Option(name = "post-cleanup",
            description = "Type of action to perform after the cleanup action;"
            + "this will override the deprecated screen-off action if specified")
    private PostCleanupAction mPostCleanupAction = PostCleanupAction.NONE;

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        // no op since this is a target cleaner
    }

    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (e instanceof DeviceFailedToBootError) {
            CLog.w("boot failure: attempting to stop runtime instead of cleanup");
            try {
                device.executeShellCommand("stop");
            } catch (DeviceUnresponsiveException due) {
                CLog.w("boot failure: ignored DeviceUnresponsiveException during forced cleanup");
            }
        } else {
            clean(device);
        }
    }

    /**
     * Execute cleanup action followed by post cleanup action
     */
    protected void clean(ITestDevice device) throws DeviceNotAvailableException {
        if (TestDeviceState.ONLINE.equals(device.getDeviceState())) {
            switch (mCleanupAction) {
                case NONE:
                    // do nothing here
                    break;
                case REBOOT:
                    device.reboot();
                    break;
                case FORMAT:
                    device.rebootIntoBootloader();
                    device.executeLongFastbootCommand("-w");
                    device.executeFastbootCommand("reboot");
                    device.waitForDeviceAvailable();
                    break;
            }
            if (mScreenOff && mPostCleanupAction == PostCleanupAction.NONE) {
                mPostCleanupAction = PostCleanupAction.SCREEN_OFF;
            }
            // perform post cleanup action
            switch (mPostCleanupAction) {
                case NONE:
                    // do nothing here
                    break;
                case SCREEN_OFF:
                    turnScreenOff(device);
                    break;
                case SCREEN_OFF_AND_STOP:
                    turnScreenOff(device);
                    device.executeShellCommand("stop");
                    break;
            }
        }
    }

    private void turnScreenOff(ITestDevice device) throws DeviceNotAvailableException {
        String output = device.executeShellCommand("dumpsys power");
        int retries = 1;
        // screen on semantics have changed in newest API platform, checking for both signatures
        // to detect screen on state
        while (output.contains("mScreenOn=true") || output.contains("mInteractive=true")) {
            // KEYCODE_POWER = 26
            device.executeShellCommand("input keyevent 26");
            // due to framework initialization, device may not actually turn off screen
            // after boot, recheck screen status with linear backoff
            RunUtil.getDefault().sleep(SCREEN_OFF_RETRY_DELAY_MS * retries);
            output = device.executeShellCommand("dumpsys power");
            retries++;
            if (retries > MAX_SCREEN_OFF_RETRY) {
                CLog.w(String.format("screen still on after %d retries", retries));
                break;
            }
        }
    }
}
