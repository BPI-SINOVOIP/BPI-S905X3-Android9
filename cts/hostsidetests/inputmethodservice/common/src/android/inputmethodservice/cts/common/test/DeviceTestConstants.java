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

package android.inputmethodservice.cts.common.test;

/**
 * Constants of CtsInputMethodServiceDeviceTests.apk that contains tests on device side and
 * related activities for test.
 */
public final class DeviceTestConstants {

    // This is constants holding class, can't instantiate.
    private DeviceTestConstants() {}

    /** Package name of the APK. */
    public static final String PACKAGE = "android.inputmethodservice.cts.devicetest";

    /** APK file name. */
    public static final String APK = "CtsInputMethodServiceDeviceTests.apk";

    /** Device test activity name. */
    public static final String TEST_ACTIVITY_CLASS =
            "android.inputmethodservice.cts.devicetest.InputMethodServiceTestActivity";

    /**
     * Device test class name and methods name.
     */
    public static final String TEST_CLASS =
           "android.inputmethodservice.cts.devicetest.InputMethodServiceDeviceTest";
    public static final String TEST_CREATE_IME1 = "testCreateIme1";
    public static final String TEST_SWITCH_IME1_TO_IME2 = "testSwitchIme1ToIme2";
    public static final String TEST_SWITCH_INPUTMETHOD = "testSwitchInputMethod";
    public static final String TEST_SWITCH_NEXT_INPUT = "testSwitchToNextInputMethod";
    public static final String TEST_SWITCH_PREVIOUS_INPUT = "switchToPreviousInputMethod";
    public static final String TEST_INPUT_UNBINDS_ON_IME_STOPPED = "testInputUnbindsOnImeStopped";
    public static final String TEST_INPUT_UNBINDS_ON_APP_STOPPED = "testInputUnbindsOnAppStopped";
}
