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

package com.android.server.cts;

import android.service.procstats.ProcessStatsProto;
import android.service.procstats.ProcessStatsServiceDumpProto;

/**
 * Test proto dump of procstats
 *
 * $ make -j32 CtsIncidentHostTestCases
 * $ cts-tradefed run cts-dev -m CtsIncidentHostTestCases \
 * -t com.android.server.cts.ProcStatsProtoTest
 */
public class ProcStatsProtoTest extends ProtoDumpTestCase {

    private static final String DEVICE_SIDE_TEST_APK = "CtsProcStatsProtoApp.apk";
    private static final String DEVICE_SIDE_TEST_PACKAGE = "com.android.server.cts.procstats";
    private static final String TEST_APP_TAG = "ProcstatsAppRunningTest";
    private static final String TEST_APP_LOG = "Procstats app is running";
    private static final int WAIT_MS = 1000;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        getDevice().executeShellCommand("dumpsys procstats --clear");
    }

    @Override
    protected void tearDown() throws Exception {
        getDevice().uninstallPackage(DEVICE_SIDE_TEST_PACKAGE);
        super.tearDown();
    }

    /**
     * Test that procstats dump is reasonable, it installs procstats app and
     * starts the activity, then asserts the procstats dump contains the package.
     *
     * @throws Exception
     */
    public void testDump() throws Exception {
        installPackage(DEVICE_SIDE_TEST_APK, /* grantPermissions= */ true);
        int retries = 3;
        do {
            getDevice().executeShellCommand(
                "am start -n com.android.server.cts.procstats/.SimpleActivity");
        } while (!checkLogcatForText(TEST_APP_TAG, TEST_APP_LOG, WAIT_MS) && retries-- > 0);

        final ProcessStatsServiceDumpProto dump = getDump(ProcessStatsServiceDumpProto.parser(),
                "dumpsys procstats --proto");

        int N = dump.getProcstatsNow().getProcessStatsCount();
        boolean containsTestApp = false;
        for (int i = 0; i < N; i++) {
            ProcessStatsProto ps = dump.getProcstatsNow().getProcessStats(i);
            if (DEVICE_SIDE_TEST_PACKAGE.equals(ps.getProcess())) {
                containsTestApp = true;
            }
        }

        assertTrue(N > 0);
        assertTrue(containsTestApp); // found test app in procstats dump
    }
}
