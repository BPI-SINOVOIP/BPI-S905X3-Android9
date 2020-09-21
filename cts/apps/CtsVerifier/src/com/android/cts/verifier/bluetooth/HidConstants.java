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

package com.android.cts.verifier.bluetooth;

public class HidConstants {
    public static final byte ID_KEYBOARD = 1;
    public static final byte ID_MOUSE = 2;

    public static final byte[] HIDD_REPORT_DESC = {
            (byte) 0x05,
            (byte) 0x01, // Usage page (Generic Desktop)
            (byte) 0x09,
            (byte) 0x06, // Usage (Keyboard)
            (byte) 0xA1,
            (byte) 0x01, // Collection (Application)
            (byte) 0x85,
            ID_KEYBOARD, //    Report ID
            (byte) 0x05,
            (byte) 0x07, //       Usage page (Key Codes)
            (byte) 0x19,
            (byte) 0xE0, //       Usage minimum (224)
            (byte) 0x29,
            (byte) 0xE7, //       Usage maximum (231)
            (byte) 0x15,
            (byte) 0x00, //       Logical minimum (0)
            (byte) 0x25,
            (byte) 0x01, //       Logical maximum (1)
            (byte) 0x75,
            (byte) 0x01, //       Report size (1)
            (byte) 0x95,
            (byte) 0x08, //       Report count (8)
            (byte) 0x81,
            (byte) 0x02, //       Input (Data, Variable, Absolute) ; Modifier byte
            (byte) 0x75,
            (byte) 0x08, //       Report size (8)
            (byte) 0x95,
            (byte) 0x01, //       Report count (1)
            (byte) 0x81,
            (byte) 0x01, //       Input (Constant)                 ; Reserved byte
            (byte) 0x75,
            (byte) 0x08, //       Report size (8)
            (byte) 0x95,
            (byte) 0x06, //       Report count (6)
            (byte) 0x15,
            (byte) 0x00, //       Logical Minimum (0)
            (byte) 0x25,
            (byte) 0x65, //       Logical Maximum (101)
            (byte) 0x05,
            (byte) 0x07, //       Usage page (Key Codes)
            (byte) 0x19,
            (byte) 0x00, //       Usage Minimum (0)
            (byte) 0x29,
            (byte) 0x65, //       Usage Maximum (101)
            (byte) 0x81,
            (byte) 0x00, //       Input (Data, Array)              ; Key array (6 keys)
            (byte) 0xC0, // End Collection
            (byte) 0x05,
            (byte) 0x01, // Usage Page (Generic Desktop)
            (byte) 0x09,
            (byte) 0x02, // Usage (Mouse)
            (byte) 0xA1,
            (byte) 0x01, // Collection (Application)
            (byte) 0x85,
            ID_MOUSE, //    Report ID
            (byte) 0x09,
            (byte) 0x01, //    Usage (Pointer)
            (byte) 0xA1,
            (byte) 0x00, //    Collection (Physical)
            (byte) 0x05,
            (byte) 0x09, //       Usage Page (Buttons)
            (byte) 0x19,
            (byte) 0x01, //       Usage minimum (1)
            (byte) 0x29,
            (byte) 0x03, //       Usage maximum (3)
            (byte) 0x15,
            (byte) 0x00, //       Logical minimum (0)
            (byte) 0x25,
            (byte) 0x01, //       Logical maximum (1)
            (byte) 0x75,
            (byte) 0x01, //       Report size (1)
            (byte) 0x95,
            (byte) 0x03, //       Report count (3)
            (byte) 0x81,
            (byte) 0x02, //       Input (Data, Variable, Absolute)
            (byte) 0x75,
            (byte) 0x05, //       Report size (5)
            (byte) 0x95,
            (byte) 0x01, //       Report count (1)
            (byte) 0x81,
            (byte) 0x01, //       Input (constant)                 ; 5 bit padding
            (byte) 0x05,
            (byte) 0x01, //       Usage page (Generic Desktop)
            (byte) 0x09,
            (byte) 0x30, //       Usage (X)
            (byte) 0x09,
            (byte) 0x31, //       Usage (Y)
            (byte) 0x09,
            (byte) 0x38, //       Usage (Wheel)
            (byte) 0x15,
            (byte) 0x81, //       Logical minimum (-127)
            (byte) 0x25,
            (byte) 0x7F, //       Logical maximum (127)
            (byte) 0x75,
            (byte) 0x08, //       Report size (8)
            (byte) 0x95,
            (byte) 0x03, //       Report count (3)
            (byte) 0x81,
            (byte) 0x06, //       Input (Data, Variable, Relative)
            (byte) 0xC0, //    End Collection
            (byte) 0xC0 // End Collection
    };
}
