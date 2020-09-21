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

package com.googlecode.android_scripting;

import android.annotation.NonNull;

import com.google.common.base.Strings;

public class ConvertUtils {
    /**
     * Converts a String of comma separated bytes to a byte array
     *
     * @param value The value to convert
     * @return the byte array
     */
    public static byte[] convertStringToByteArray(String value) {
        if (value.equals("")) {
            return new byte[0];
        }
        String[] parseString = value.split(",");
        byte[] byteArray = new byte[parseString.length];
        if (byteArray.length > 0) {
            for (int i = 0; i < parseString.length; i++) {
                int val = Integer.valueOf(parseString[i].trim());
                if (val < 0 || val > 255)
                    throw new java.lang.NumberFormatException("Val must be between 0 and 255");
                byteArray[i] = (byte)val;
            }
        }
        return byteArray;
    }

    /**
     * Converts a byte array to a comma separated String
     *
     * @param byteArray
     * @return comma separated string of bytes
     */
    public static String convertByteArrayToString(byte[] byteArray) {
        String ret = "";
        if (byteArray != null) {
            for (int i = 0; i < byteArray.length; i++) {
                if ((i + 1) != byteArray.length) {
                    ret = ret + Integer.valueOf((byteArray[i]&0xFF)) + ",";
                }
                else {
                    ret = ret + Integer.valueOf((byteArray[i]&0xFF));
                }
            }
        }
        return ret;
    }

    /**
     * An interface for
     * @param <T1> The type of the caller.
     * @param <T2> The returned type of the call.
     */
    public interface IFunction<T1, T2> {
        T2 call(T1 t1);
    }

    /**
     * Calls a given function on an object, and returns a NonNull String of the return value.
     * @param obj The object to get the string data from.
     * @param function The function or method to call.
     * @param <T1> The type of the object.
     * @param <T2> The type of the function return type.
     * @return A string guaranteed not to be null.
     */
    public static <T1, T2> String toNonNullString(T1 obj, @NonNull IFunction<T1, T2> function) {
        if (obj == null) {
            return "";
        } else {
            return toNonNullString(function.call(obj));
        }
    }

    /**
     * Returns toString() or an empty string if {@code obj} or result is null.
     * @param obj The object to call toString() on.
     * @param <T> The type of the object.
     * @return A string guaranteed not to be null.
     */
    public static <T> String toNonNullString(T obj) {
        if (obj == null) {
            return "";
        } else {
            return Strings.nullToEmpty(obj.toString());
        }
    }
}
