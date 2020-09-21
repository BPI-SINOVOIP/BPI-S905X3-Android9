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

package com.android.tradefed.util;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.Arrays;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Utility class to get app version string from device.
 *
 * <p>Send dumpsys package command to device, then parse the return result string.
 */
public final class AppVersionFetcher {

    /** App version info types. */
    public enum AppVersionInfo {
        VERSION_CODE,
        VERSION_NAME
    }

    // Static immutable fields
    private static final String DUMP_PACKAGE_COMMAND = "dumpsys package";
    private static final Pattern VERSION_CODE_PATTERN =
            Pattern.compile("\\s+versionCode=(\\d+) .*");
    private static final Pattern VERSION_NAME_PATTERN = Pattern.compile("\\s+versionName=(.*?)");

    /**
     * Fetch application version string based on package name.
     *
     * @param device ITestDevice, device instance
     * @param packageName String, package name
     * @param info AppVersionInfo, app version info type
     * @return version string of the package
     */
    public static String fetch(ITestDevice device, String packageName, AppVersionInfo info)
            throws DeviceNotAvailableException {
        switch (info) {
            case VERSION_CODE:
                return fetchVersionCode(device, packageName);
            case VERSION_NAME:
                return fetchVersionName(device, packageName);
            default:
                throw new IllegalArgumentException("Unknown app version info");
        }
    }

    private static String fetchVersionName(ITestDevice device, String packageName)
            throws DeviceNotAvailableException {
        String shellCommand =
                String.format("%s %s | grep versionName=", DUMP_PACKAGE_COMMAND, packageName);
        String packageInfo = device.executeShellCommand(shellCommand);
        return parseVersionString(packageInfo, VERSION_NAME_PATTERN);
    }

    private static String fetchVersionCode(ITestDevice device, String packageName)
            throws DeviceNotAvailableException {
        String shellCommand =
                String.format("%s %s | grep versionCode=", DUMP_PACKAGE_COMMAND, packageName);
        String packageInfo = device.executeShellCommand(shellCommand);
        return parseVersionString(packageInfo, VERSION_CODE_PATTERN);
    }

    private static String parseVersionString(String packageInfo, Pattern pattern) {
        if (packageInfo == null) {
            throw new IllegalArgumentException("Shell command result is null");
        }
        String[] lines = packageInfo.split("\\r?\\n");
        if (lines.length > 1) {
            CLog.i("There are two versions:\n%s\nThe first one is picked.", Arrays.toString(lines));
        }
        Matcher matcher = pattern.matcher(lines[0]);
        if (!matcher.matches()) {
            throw new IllegalStateException("Invalid app version string: " + lines[0]);
        }
        return matcher.group(1);
    }
}
