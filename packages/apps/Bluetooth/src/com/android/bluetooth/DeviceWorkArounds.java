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

package com.android.bluetooth;

/**
 * @hide
 */
public final class DeviceWorkArounds {
    public static final String PCM_CARKIT = "9C:DF:03";
    public static final String FORD_SYNC_CARKIT = "00:1E:AE";
    public static final String HONDA_CARKIT = "64:D4:BD";
    public static final String SYNC_CARKIT = "D0:39:72";
    public static final String BREZZA_ZDI_CARKIT = "28:a1:83";
    public static final String MERCEDES_BENZ_CARKIT = "00:26:e8";

    /**
     * @hide
     */
    public static boolean addressStartsWith(String bdAddr, String carkitAddr) {
        return bdAddr.toLowerCase().startsWith(carkitAddr.toLowerCase());
    }
}
