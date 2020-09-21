/*
 * Copyright (C) 2018 The Android Open Source Project
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

import com.android.ddmlib.AdbCommandRejectedException;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.MultiLineReceiver;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.log.LogUtil.CLog;

import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class BatteryTemperature implements IBatteryTemperature {

    /** Parse the output of "adb shell dumpsys battery" to collect the temperature. */
    class DumpsysBatteryTemperatureReceiver extends MultiLineReceiver {
        /*
         * The temperature is printed as numeric value in tenths of a degree, i.e.
         * 512 indicates 51.2°C.
         */
        private static final int DUMPSYS_BATTERY_TEMP_SCALE = 10;

        /* The temperature is listed on a single line, like "temperature: 512" */
        private static final String BATTERY_REGEX = "temperature: ([0-9]+)";

        private int mDeviceBatteryTemp = 0;

        public int getDeviceBatteryTemp() {
            return mDeviceBatteryTemp;
        }

        @Override
        public boolean isCancelled() {
            return false;
        }

        @Override
        public void processNewLines(String[] lines) {
            for (String line : lines) {
                Pattern p = Pattern.compile(BATTERY_REGEX);
                Matcher m = p.matcher(line);
                if (m.find()) {
                    String tempString = m.group(1);
                    try {
                        mDeviceBatteryTemp = Integer.parseInt(tempString);
                        mDeviceBatteryTemp /= DUMPSYS_BATTERY_TEMP_SCALE;
                    } catch (NumberFormatException e) {
                        CLog.w("Failed to parse battery temperature: %s", line);
                    }
                }
            }
        }
    }

    @Override
    public Integer getBatteryTemperature(IDevice device) {
        DumpsysBatteryTemperatureReceiver receiver = new DumpsysBatteryTemperatureReceiver();

        try {
            device.executeShellCommand("dumpsys battery", receiver);
        } catch (TimeoutException
                | AdbCommandRejectedException
                | ShellCommandUnresponsiveException
                | IOException e) {
            return 0;
        }

        return receiver.getDeviceBatteryTemp();
    }
}
