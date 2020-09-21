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

import android.content.pm.FeatureInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.test.InstrumentationRegistry;

import java.util.HashSet;
import java.util.Set;

/**
 * Device-side utility class for detecting system features
 */
public class FeatureUtil {

    public static final String LEANBACK_FEATURE = "android.software.leanback";
    public static final String LOW_RAM_FEATURE = "android.hardware.ram.low";
    public static final String TELEPHONY_FEATURE = "android.hardware.telephony";
    public static final String TV_FEATURE = "android.hardware.type.television";
    public static final String WATCH_FEATURE = "android.hardware.type.watch";


    /** Returns true if the device has a given system feature */
    public static boolean hasSystemFeature(String feature) {
        return getPackageManager().hasSystemFeature(feature);
    }

    /** Returns true if the device has any feature in a given collection of system features */
    public static boolean hasAnySystemFeature(String... features) {
        PackageManager pm = getPackageManager();
        for (String feature : features) {
            if (pm.hasSystemFeature(feature)) {
                return true;
            }
        }
        return false;
    }

    /** Returns true if the device has all features in a given collection of system features */
    public static boolean hasAllSystemFeatures(String... features) {
        PackageManager pm = getPackageManager();
        for (String feature : features) {
            if (!pm.hasSystemFeature(feature)) {
                return false;
            }
        }
        return true;
    }

    /** Returns all system features of the device */
    public static Set<String> getAllFeatures() {
        Set<String> allFeatures = new HashSet<String>();
        for (FeatureInfo fi : getPackageManager().getSystemAvailableFeatures()) {
            allFeatures.add(fi.name);
        }
        return allFeatures;
    }

    /** Returns true if the device has feature TV_FEATURE or feature LEANBACK_FEATURE */
    public static boolean isTV() {
        return hasAnySystemFeature(TV_FEATURE, LEANBACK_FEATURE);
    }

    /** Returns true if the device has feature WATCH_FEATURE */
    public static boolean isWatch() {
        return hasSystemFeature(WATCH_FEATURE);
    }

    /** Returns true if the device is a low ram device:
     *  1. API level &gt;= O
     *  2. device has feature LOW_RAM_FEATURE
     */
    public static boolean isLowRam() {
        return ApiLevelUtil.isAtLeast(Build.VERSION_CODES.O) &&
                hasSystemFeature(LOW_RAM_FEATURE);
    }

    private static PackageManager getPackageManager() {
        return InstrumentationRegistry.getInstrumentation().getTargetContext().getPackageManager();
    }

    /** Returns true if the device has feature TELEPHONY_FEATURE */
    public static boolean hasTelephony() {
        return hasSystemFeature(TELEPHONY_FEATURE);
    }

    /** Returns true if the device has feature FEATURE_MICROPHONE */
    public static boolean hasMicrophone() {
        return hasSystemFeature(getPackageManager().FEATURE_MICROPHONE);
    }
}
