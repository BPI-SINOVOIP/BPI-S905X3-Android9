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
package com.android.tradefed.device;

/**
 * Common constant definitions for device side property names
 */
public class DeviceProperties {

    /** property name for device board */
    public static final String BOARD = "ro.product.board";
    /** proprty name to indicate device variant (e.g. flo vs dev) */
    public static final String VARIANT = "ro.product.vendor.device";
    /** Legacy property name to indicate device variant (e.g. flo vs dev) */
    public static final String VARIANT_LEGACY = "ro.product.device";
    /** proprty name to indicate SDK version */
    public static final String SDK_VERSION = "ro.build.version.sdk";
}
