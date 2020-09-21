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

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import java.util.HashSet;
import java.util.Set;

/**
 * Host-side utility class for detecting system features
 */
public class FeatureUtil {

    public static final String LEANBACK_FEATURE = "android.software.leanback";
    public static final String LOW_RAM_FEATURE = "android.hardware.ram.low";
    public static final String TELEPHONY_FEATURE = "android.hardware.telephony";
    public static final String TV_FEATURE = "android.hardware.type.television";
    public static final String WATCH_FEATURE = "android.hardware.type.watch";
    public static final String FEATURE_MICROPHONE = "android.hardware.microphone";

    /** Returns true if the device has a given system feature */
    public static boolean hasSystemFeature(ITestDevice device, String feature)
            throws DeviceNotAvailableException {
        return device.hasFeature(feature);
    }

    /** Returns true if the device has any feature in a given collection of system features */
    public static boolean hasAnySystemFeature(ITestDevice device, String... features)
            throws DeviceNotAvailableException {
        for (String feature : features) {
            if (device.hasFeature(feature)) {
                return true;
            }
        }
        return false;
    }

    /** Returns true if the device has all features in a given collection of system features */
    public static boolean hasAllSystemFeatures(ITestDevice device, String... features)
            throws DeviceNotAvailableException {
        for (String feature : features) {
            if (!device.hasFeature(feature)) {
                return false;
            }
        }
        return true;
    }

    /** Returns all system features of the device */
    public static Set<String> getAllFeatures(ITestDevice device)
            throws DeviceNotAvailableException {
        Set<String> allFeatures = new HashSet<String>();
        String output = device.executeShellCommand("pm list features");
        for (String feature : output.split("[\\r?\\n]+")) {
            allFeatures.add(feature.substring("feature:".length()));
        }
        return allFeatures;
    }

    /** Returns true if the device has feature TV_FEATURE or feature LEANBACK_FEATURE */
    public static boolean isTV(ITestDevice device) throws DeviceNotAvailableException {
        return hasAnySystemFeature(device, TV_FEATURE, LEANBACK_FEATURE);
    }

    /** Returns true if the device has feature WATCH_FEATURE */
    public static boolean isWatch(ITestDevice device) throws DeviceNotAvailableException {
        return hasSystemFeature(device, WATCH_FEATURE);
    }

    /** Returns true if the device is a low ram device:
     *  1. API level &gt;= O
     *  2. device has feature LOW_RAM_FEATURE
     */
    public static boolean isLowRam(ITestDevice device) throws DeviceNotAvailableException {
        return ApiLevelUtil.isAtLeast(device, VersionCodes.O) &&
                hasSystemFeature(device, LOW_RAM_FEATURE);
    }

    /** Returns true if the device has feature TELEPHONY_FEATURE */
    public static boolean hasTelephony(ITestDevice device) throws DeviceNotAvailableException {
        return hasSystemFeature(device, TELEPHONY_FEATURE);
    }

    /** Returns true if the device has feature FEATURE_MICROPHONE */
    public static boolean hasMicrophone(ITestDevice device) throws DeviceNotAvailableException {
        return hasSystemFeature(device, FEATURE_MICROPHONE);
    }
}
