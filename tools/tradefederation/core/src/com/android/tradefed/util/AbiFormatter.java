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

package com.android.tradefed.util;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Utility class for abi.
 */
public class AbiFormatter {

    private static final String PRODUCT_CPU_ABILIST_KEY = "ro.product.cpu.abilist";
    private static final String PRODUCT_CPU_ABI_KEY = "ro.product.cpu.abi";
    public static final String FORCE_ABI_STRING = "force-abi";
    public static final String FORCE_ABI_DESCRIPTION = "The abi to use, can be either 32 or 64.";

    /**
     * Special marker to be used as a placeholder in strings, that can be then
     * replaced with the help of {@link #formatCmdForAbi}.
     */
    static final String ABI_REGEX = "\\|#ABI(\\d*)#\\|";

    /**
     * Helper method that formats a given string to include abi specific
     * values to it by replacing a given marker.
     *
     * @param str {@link String} to format which includes special markers |
     *            {@value #ABI_REGEX} to be replaced
     * @param abi {@link String} of the abi we desire to run on.
     * @return formatted string.
     */
    public static String formatCmdForAbi(String str, String abi) {
        // If the abi is not set or null, do nothing. This is to maintain backward compatibility.
        if (str == null) {
            return null;
        }
        if (abi == null) {
            return str.replaceAll(ABI_REGEX, "");
        }
        StringBuffer sb = new StringBuffer();
        Matcher m = Pattern.compile(ABI_REGEX).matcher(str);
        while (m.find()) {
            if (m.group(1).equals(abi)) {
                m.appendReplacement(sb, "");
            } else {
                m.appendReplacement(sb, abi);
            }
        }
        m.appendTail(sb);
        return sb.toString();
    }

    /**
     * Helper method to get the default abi name for the given bitness
     *
     * @param device
     * @param bitness
     * @return the default abi name for the given abi. Returns null if something went wrong.
     * @throws DeviceNotAvailableException
     */
    public static String getDefaultAbi(ITestDevice device, String bitness)
            throws DeviceNotAvailableException {
        String []abis = getSupportedAbis(device, bitness);
        if (abis != null && abis.length > 0 && abis[0] != null && abis[0].length() > 0) {
            return abis[0];
        }
        return null;
    }

    /**
     * Helper method to get the list of supported abis for the given bitness
     *
     * @param device
     * @param bitness 32 or 64 or empty string
     * @return the supported abi list of that bitness
     * @throws DeviceNotAvailableException
     */
    public static String[] getSupportedAbis(ITestDevice device, String bitness)
            throws DeviceNotAvailableException {
        String abiList = device.getProperty(PRODUCT_CPU_ABILIST_KEY + bitness);
        if (abiList != null && !abiList.isEmpty()) {
            String []abis = abiList.split(",");
            if (abis.length > 0) {
                return abis;
            }
        }
        // fallback plan for before lmp, the bitness is ignored
        return new String[]{device.getProperty(PRODUCT_CPU_ABI_KEY)};
    }
}
