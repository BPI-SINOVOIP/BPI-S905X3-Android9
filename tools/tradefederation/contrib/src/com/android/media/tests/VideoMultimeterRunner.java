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

package com.android.media.tests;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Semaphore;

/**
 * A harness that test video playback with multiple devices and reports result.
 */
public class VideoMultimeterRunner extends VideoMultimeterTest
    implements IDeviceTest, IRemoteTest {

    @Option(name = "robot-util-path", description = "path for robot control util",
            importance = Importance.ALWAYS, mandatory = true)
    String mRobotUtilPath = "/tmp/robot_util.sh";

    @Option(name = "device-map", description =
            "Device serials map to location and audio input. May be repeated",
            importance = Importance.ALWAYS)
    Map<String, String> mDeviceMap = new HashMap<String, String>();

    @Option(name = "calibration-map", description =
            "Device serials map to calibration values. May be repeated",
            importance = Importance.ALWAYS)
    Map<String, String> mCalibrationMap = new HashMap<String, String>();

    static final long ROBOT_TIMEOUT_MS = 60 * 1000;

    static final Semaphore runToken = new Semaphore(1);

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        long durationMs = 0;
        TestDescription testId = new TestDescription(getClass()
                .getCanonicalName(), RUN_KEY);

        listener.testRunStarted(RUN_KEY, 0);
        listener.testStarted(testId);

        long testStartTime = System.currentTimeMillis();
        Map<String, String> metrics = new HashMap<String, String>();

        try {
            CLog.v("Waiting to acquire run token");
            runToken.acquire();

            String deviceSerial = getDevice().getSerialNumber();

            String calibrationValue = (mCalibrationMap.containsKey(deviceSerial) ?
                    mCalibrationMap.get(deviceSerial) : null);
            if (mDebugWithoutHardware
                    || (moveArm(deviceSerial) && setupTestEnv(calibrationValue))) {
                runMultimeterTest(listener, metrics);
            } else {
                listener.testFailed(testId, "Failed to set up environment");
            }
        } catch (InterruptedException e) {
            CLog.d("Acquire run token interrupted");
            listener.testFailed(testId, "Failed to acquire run token");
        } finally {
            runToken.release();
            listener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(metrics));
            durationMs = System.currentTimeMillis() - testStartTime;
            listener.testRunEnded(durationMs, TfMetricProtoUtil.upgradeConvert(metrics));
        }
    }

    protected boolean moveArm(String deviceSerial) {
        if (mDeviceMap.containsKey(deviceSerial)) {
            CLog.v("Moving robot arm to device " + deviceSerial);
            CommandResult cr = getRunUtil().runTimedCmd(
                    ROBOT_TIMEOUT_MS, mRobotUtilPath, mDeviceMap.get(deviceSerial));
            CLog.v(cr.getStdout());
            return true;
        } else {
            CLog.e("Cannot find device in map, test failed");
            return false;
        }
    }
}
