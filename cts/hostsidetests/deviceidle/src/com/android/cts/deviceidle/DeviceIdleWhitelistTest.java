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

package com.android.cts.deviceidle;

import static org.junit.Assert.*;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.After;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests that it is possible to remove apps from the system whitelist
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class DeviceIdleWhitelistTest extends BaseHostJUnit4Test {

    private static final String DEVICE_IDLE_COMMAND_PREFIX = "cmd deviceidle sys-whitelist ";
    private static final String RESET_SYS_WHITELIST_COMMAND = "cmd deviceidle sys-whitelist reset";
    private static final String SHOW_SYS_WHITELIST_COMMAND = DEVICE_IDLE_COMMAND_PREFIX;

    private List<String> mOriginalSystemWhitelist;

    @Before
    public void setUp() throws Exception {
        getDevice().executeShellCommand(RESET_SYS_WHITELIST_COMMAND);
        mOriginalSystemWhitelist = getSystemWhitelist();
        if (mOriginalSystemWhitelist.size() < 1) {
            LogUtil.CLog.w("No packages found in system whitelist");
            Assume.assumeTrue(false);
        }
    }

    @After
    public void tearDown() throws Exception {
        getDevice().executeShellCommand(RESET_SYS_WHITELIST_COMMAND);
    }

    @Test
    public void testRemoveFromSysWhitelist() throws Exception {
        final String packageToRemove = mOriginalSystemWhitelist.get(0);
        getDevice().executeShellCommand(DEVICE_IDLE_COMMAND_PREFIX + "-" + packageToRemove);
        final List<String> newWhitelist = getSystemWhitelist();
        assertFalse("Package " + packageToRemove + " not removed from whitelist",
                newWhitelist.contains(packageToRemove));
    }

    @Test
    public void testRemovesPersistedAcrossReboots() throws Exception {
        for (int i = 0; i < mOriginalSystemWhitelist.size(); i+=2) {
            // remove odd indexed packages from the whitelist
            getDevice().executeShellCommand(
                    DEVICE_IDLE_COMMAND_PREFIX + "-" + mOriginalSystemWhitelist.get(i));
        }
        final List<String> whitelistBeforeReboot = getSystemWhitelist();
        Thread.sleep(10_000); // write to disk happens after 5 seconds
        getDevice().reboot();
        Thread.sleep(5_000); // to make sure service is initialized
        final List<String> whitelistAfterReboot = getSystemWhitelist();
        assertEquals(whitelistBeforeReboot.size(), whitelistAfterReboot.size());
        for (int i = 0; i < whitelistBeforeReboot.size(); i++) {
            assertTrue(whitelistAfterReboot.contains(whitelistBeforeReboot.get(i)));
        }
    }

    private List<String> getSystemWhitelist() throws DeviceNotAvailableException {
        final String output = getDevice().executeShellCommand(SHOW_SYS_WHITELIST_COMMAND).trim();
        final List<String> packages = new ArrayList<>();
        for (String line : output.split("\n")) {
            final int i = line.indexOf(',');
            packages.add(line.substring(0, i));
        }
        return packages;
    }
}
