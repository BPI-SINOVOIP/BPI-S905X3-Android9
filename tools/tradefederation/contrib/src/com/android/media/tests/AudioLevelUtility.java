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
package com.android.media.tests;

import com.android.ddmlib.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.concurrent.TimeUnit;

/** Class to provide audio level utility functions for a test device */
public class AudioLevelUtility {

    public static int extractDeviceHeadsetLevelFromAdbShell(ITestDevice device)
            throws DeviceNotAvailableException {

        final String ADB_SHELL_DUMPSYS_AUDIO = "dumpsys audio";
        final String STREAM_MUSIC = "- STREAM_MUSIC:";
        final String HEADSET = "(headset): ";

        final CollectingOutputReceiver receiver = new CollectingOutputReceiver();

        device.executeShellCommand(
                ADB_SHELL_DUMPSYS_AUDIO, receiver, 300, TimeUnit.MILLISECONDS, 1);
        final String shellOutput = receiver.getOutput();
        if (shellOutput == null || shellOutput.isEmpty()) {
            return -1;
        }

        int audioLevel = -1;
        int pos = shellOutput.indexOf(STREAM_MUSIC);
        if (pos != -1) {
            pos = shellOutput.indexOf(HEADSET, pos);
            if (pos != -1) {
                final int start = pos + HEADSET.length();
                final int stop = shellOutput.indexOf(",", start);
                if (stop != -1) {
                    final String audioLevelStr = shellOutput.substring(start, stop);
                    try {
                        audioLevel = Integer.parseInt(audioLevelStr);
                    } catch (final NumberFormatException e) {
                        CLog.e(e.getMessage());
                        audioLevel = 1;
                    }
                }
            }
        }

        return audioLevel;
    }
}
