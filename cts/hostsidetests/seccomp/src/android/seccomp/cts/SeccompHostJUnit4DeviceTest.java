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

package android.seccomp.cts;

import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test that collects test results from test package android.seccomp.cts.app.
 *
 * When this test builds, it also builds a support APK containing
 * {@link android.seccomp.cts.app.SeccompDeviceTest}, the results of which are
 * collected from the hostside and reported accordingly.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class SeccompHostJUnit4DeviceTest extends BaseHostJUnit4Test {

    private static final String TEST_PKG = "android.seccomp.cts.app";
    private static final String TEST_CLASS = TEST_PKG + "." + "SeccompDeviceTest";
    private static final String TEST_APP = "CtsSeccompDeviceApp.apk";

    private static final String TEST_CTS_SYSCALL_BLOCKED = "testCTSSyscallBlocked";
    private static final String TEST_CTS_SYSCALL_ALLOWED = "testCTSSyscallAllowed";

    @Before
    public void setUp() throws Exception {
        installPackage(TEST_APP);
    }

    @Test
    public void testCTSSyscallBlocked() throws Exception {
        Assert.assertTrue(runDeviceTests(TEST_PKG, TEST_CLASS, TEST_CTS_SYSCALL_BLOCKED));
    }

    @Test
    public void testCTSSyscallAllowed() throws Exception {
        Assert.assertTrue(runDeviceTests(TEST_PKG, TEST_CLASS, TEST_CTS_SYSCALL_ALLOWED));
    }

    @After
    public void tearDown() throws Exception {
        uninstallPackage(getDevice(), TEST_PKG);
    }

}
