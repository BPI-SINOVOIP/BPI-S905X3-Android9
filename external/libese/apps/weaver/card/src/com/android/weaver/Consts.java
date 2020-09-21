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

package com.android.weaver;

// Keep in sync with native code
public class Consts {
    public static final byte SLOT_ID_BYTES = 4; // 32-bit
    public static final byte SLOT_KEY_BYTES = 16; // 128-bit
    public static final byte SLOT_VALUE_BYTES = 16; // 128-bit

    // Read errors (first byte of response)
    public static final byte READ_SUCCESS = 0x00;
    public static final byte READ_WRONG_KEY = 0x7f;
    public static final byte READ_BACK_OFF = 0x76;

    // Errors
    public static final short SW_INVALID_SLOT_ID = 0x6a86;

    // Instructions
    public static final byte INS_GET_NUM_SLOTS = 0x02;
    public static final byte INS_WRITE = 0x4;
    public static final byte INS_READ = 0x6;
    public static final byte INS_ERASE_VALUE = 0x8;
    public static final byte INS_ERASE_ALL = 0xa;
}
