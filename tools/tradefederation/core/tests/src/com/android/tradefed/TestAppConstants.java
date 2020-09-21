/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed;

/**
 * Constants for the device test-apps used to perform functional tests.
 */
public class TestAppConstants {

    public static final String TESTAPP_PACKAGE = "com.android.tradefed.testapp";
    public static final String TESTAPP_CLASS = "com.android.tradefed.testapp.OnDeviceTest";
    public static final int TOTAL_TEST_CLASS_TESTS = 4;
    public static final int TOTAL_TEST_CLASS_PASSED_TESTS = 1;
    public static final String PASSED_TEST_METHOD = "testPassed";
    public static final String FAILED_TEST_METHOD = "testFailed";
    public static final String CRASH_TEST_METHOD = "testCrash";
    public static final String TIMEOUT_TEST_METHOD = "testNeverEnding";
    public static final String ANR_TEST_METHOD = "testAnr";

    public static final String CRASH_ON_INIT_TEST_CLASS =
        "com.android.tradefed.testapp.CrashOnInitTest";
    public static final String CRASH_ON_INIT_TEST_METHOD = "testNeverRun";
    public static final String HANG_ON_INIT_TEST_CLASS =
        "com.android.tradefed.testapp.HangOnInitTest";

    public static final String CRASH_ACTIVITY = String.format("%s/.CrashMeActivity",
            TESTAPP_PACKAGE);

    public static final String UITESTAPP_PACKAGE = "com.android.tradefed.uitestapp";
    public static final int UI_TOTAL_TESTS = 4;

    private TestAppConstants() {
    }
}
