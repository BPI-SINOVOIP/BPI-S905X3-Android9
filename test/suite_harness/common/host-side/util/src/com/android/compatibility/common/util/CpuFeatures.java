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
package com.android.compatibility.common.util;
import com.android.tradefed.device.CollectingOutputReceiver;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import java.util.regex.Pattern;

/**
 * Host-side utility class for reading properties and gathering information for testing
 * Android device compatibility.
 */
public class CpuFeatures {

    private static final String UNAME_OPTION_MACHINE_TYPE = "-m";
    private static final String UNAME_OPTION_KERNEL_RELEASE = "-r";

    private static String uname(ITestDevice device, String option) throws DeviceNotAvailableException {
        CollectingOutputReceiver Out = new CollectingOutputReceiver();
        device.executeShellCommand("uname " + option, Out);
        return Out.getOutput().trim();
    }

    /**
     * Return true if architecture is arm64.
     */
    public static boolean isArm64(ITestDevice device) throws DeviceNotAvailableException {

        return uname(device, UNAME_OPTION_MACHINE_TYPE).contains("aarch64");
    }

    /**
     * Return true if architecture is arm32.
     */
    public static boolean isArm32(ITestDevice device) throws DeviceNotAvailableException {

        return uname(device, UNAME_OPTION_MACHINE_TYPE).contains("armv7");
    }

    /**
     * Return true if architecture is x86.
     */
    public static boolean isX86(ITestDevice device) throws DeviceNotAvailableException {

        return uname(device, UNAME_OPTION_MACHINE_TYPE).contains("x86");
    }

    /**
     * Return true kernel if version is less than input values.
     */
    public static boolean kernelVersionLessThan(ITestDevice device, int major, int minor)
            throws DeviceNotAvailableException {

        String[] kernelVersion = uname(device, UNAME_OPTION_KERNEL_RELEASE).split(Pattern.quote("."));
        int deviceMajor = Integer.parseInt(kernelVersion[0]);
        int deviceMinor = Integer.parseInt(kernelVersion[1]);

        return (major < deviceMajor) || ((major == deviceMajor) && (minor < deviceMinor));
    }
}
