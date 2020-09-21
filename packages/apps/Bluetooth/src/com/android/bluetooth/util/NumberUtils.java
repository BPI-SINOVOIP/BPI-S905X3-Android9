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

package com.android.bluetooth.util;

/**
 * Utility for parsing numbers in Bluetooth.
 */
public class NumberUtils {

    /**
     * Convert a byte to unsigned int.
     */
    public static int unsignedByteToInt(byte b) {
        return b & 0xFF;
    }

    /**
     * Convert a little endian byte array to integer.
     */
    public static int littleEndianByteArrayToInt(byte[] bytes) {
        int length = bytes.length;
        if (length == 0) {
            return 0;
        }
        int result = 0;
        for (int i = length - 1; i >= 0; i--) {
            int value = unsignedByteToInt(bytes[i]);
            result += (value << (i * 8));
        }
        return result;
    }
}
