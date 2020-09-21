/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.tradefed.targetprep;

import com.android.tradefed.testtype.DeviceTestCase;


/**
 * Functional tests for {@link DefaultTestsZipInstaller}.
 * <p/>
 * Note: this test is destructive, and shouldn't be included in the default suite.
 */
public class DefaultTestsZipInstallerFuncTest extends DeviceTestCase {
    private DefaultTestsZipInstaller mZipInstaller = new DefaultTestsZipInstaller("media");

    /**
     * Simple test to ensure wipe data succeeds
     * @throws Exception
     */
    public void testDeleteData() throws Exception {
        getDevice().enableAdbRoot();
        getDevice().executeShellCommand("stop");
        try {
            mZipInstaller.deleteData(getDevice());
        } finally {
            getDevice().reboot();
        }
    }
}
