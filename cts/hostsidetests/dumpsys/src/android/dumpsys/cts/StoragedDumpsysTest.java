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

package android.dumpsys.cts;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;

import java.io.BufferedReader;
import java.io.StringReader;
import java.util.Date;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.text.SimpleDateFormat;

/**
 * Test to check the format of the dumps of the processstats test.
 */
public class StoragedDumpsysTest extends BaseDumpsysTest {
    private static final String DEVICE_SIDE_TEST_APK = "CtsStoragedTestApp.apk";
    private static final String DEVICE_SIDE_TEST_PACKAGE = "com.android.server.cts.storaged";

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        getDevice().uninstallPackage(DEVICE_SIDE_TEST_PACKAGE);
    }

    private String getCgroupFromLog(String log) {
        Pattern pattern = Pattern.compile("cgroup:([^\\s]+)", Pattern.MULTILINE);
        Matcher matcher = pattern.matcher(log);
        if (matcher.find()) {
            return matcher.group(1);
        }
        return null;
    }

    private String getCurrentLogcatDate() throws Exception {
        long timestampMs = getDevice().getDeviceDate();
        return new SimpleDateFormat("MM-dd HH:mm:ss.SSS")
            .format(new Date(timestampMs));
    }

    /**
     * Tests the output of "dumpsys storaged --force --hours 0.01".
     *
     * @throws Exception
     */
    public void testStoragedOutput() throws Exception {
        String result = mDevice.executeShellCommand("stat /proc/uid_io/stats");
        if(result.contains("No such file or directory")) {
            return;
        }

        if (mDevice.getAppPackageInfo(DEVICE_SIDE_TEST_APK) != null) {
            getDevice().uninstallPackage(DEVICE_SIDE_TEST_PACKAGE);
        }

        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mCtsBuild);
        mDevice.installPackage(buildHelper.getTestFile(DEVICE_SIDE_TEST_APK), true);

        mDevice.executeShellCommand("dumpsys storaged --force");

        String logcatDate = getCurrentLogcatDate();

        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE,
                "com.android.server.cts.storaged.StoragedTest",
                "testBackgroundIO");
        String log = mDevice.executeAdbCommand(
                "logcat", "-v", "brief", "-d", "-t", logcatDate,
                "SimpleIOService:I", "*:S");
        String serviceCgroup = getCgroupFromLog(log);
        if (serviceCgroup != null && serviceCgroup.equals("/top")) {
            System.out.println("WARNING: Service was not in the correct cgroup; ActivityManager may be unresponsive.");
        }

        runDeviceTests(DEVICE_SIDE_TEST_PACKAGE,
                "com.android.server.cts.storaged.StoragedTest",
                "testForegroundIO");

        String output = mDevice.executeShellCommand("dumpsys storaged --force --hours 0.01");
        assertNotNull(output);
        assertTrue(output.length() > 0);

        boolean hasTestIO = false;
        try (BufferedReader reader = new BufferedReader(
                new StringReader(output))) {

            String line;
            String[] parts;
            while ((line = reader.readLine()) != null) {
                if (line.isEmpty()) {
                    continue;
                }

                if (line.contains(",")) {
                    parts = line.split(",");
                    assertTrue(parts.length == 2);
                    if (!parts[0].isEmpty()) {
                        assertInteger(parts[0]);
                    }
                    assertInteger(parts[1]);
                    continue;
                }

                parts = line.split(" ");
                assertTrue(parts.length == 9);
                for (int i = 1; i < parts.length; i++) {
                    assertInteger(parts[i]);
                }

                if (parts[0].equals(DEVICE_SIDE_TEST_PACKAGE)) {
                    if (Integer.parseInt(parts[6]) >= 8192 && Integer.parseInt(parts[8]) == 0) {
                        System.out.print("WARNING: Background I/O was attributed to the "
                                + "foreground. This could indicate a broken or malfunctioning "
                                + "ActivityManager or UsageStatsService.\n");
                    } else {
                        assertTrue((Integer.parseInt(parts[6]) >= 4096 && Integer.parseInt(parts[8]) >= 4096) ||
                                    Integer.parseInt(parts[8]) >= 8192);
                    }
                    hasTestIO = true;
                }
            }

            assertTrue(hasTestIO);
        }
    }
}
