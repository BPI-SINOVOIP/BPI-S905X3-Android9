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
package com.android.devicehealthchecks;

import android.platform.test.annotations.GlobalPresubmit;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests used for basic device health validation after the device boot is completed. This test class
 * can be used to add more tests in the future for additional basic device health validation after
 * the device boot is completed. This test is used for global presubmit, any dropbox label checked
 * showing failures must be resolved immediately, or have corresponding tests filtered out. */
@GlobalPresubmit
@RunWith(AndroidJUnit4.class)
public class CrashCheck extends CrashCheckBase {

    @Test
    public void system_server_crash() {
        checkCrash("system_server_crash");
    }

    @Test
    public void system_server_native_crash() {
        checkCrash("system_server_native_crash");
    }

    @Test
    public void system_server_anr() {
        checkCrash("system_server_anr");
    }

    @Test
    public void system_app_crash() {
        checkCrash("system_app_crash");
    }

    @Test
    public void system_app_native_crash() {
        checkCrash("system_app_native_crash");
    }

    @Test
    public void system_app_anr() {
        checkCrash("system_app_anr");
    }

    @Test
    public void system_tombstone() {
        checkCrash("SYSTEM_TOMBSTONE");
    }
}
