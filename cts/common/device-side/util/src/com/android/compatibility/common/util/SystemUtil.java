/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.compatibility.common.util;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.ActivityManager;
import android.app.ActivityManager.MemoryInfo;
import android.app.Instrumentation;
import android.content.Context;
import android.os.ParcelFileDescriptor;
import android.os.StatFs;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.function.Predicate;

public class SystemUtil {
    private static final String TAG = "CtsSystemUtil";

    public static long getFreeDiskSize(Context context) {
        final StatFs statFs = new StatFs(context.getFilesDir().getAbsolutePath());
        return (long)statFs.getAvailableBlocks() * statFs.getBlockSize();
    }

    public static long getFreeMemory(Context context) {
        final MemoryInfo info = new MemoryInfo();
        ((ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE)).getMemoryInfo(info);
        return info.availMem;
    }

    public static long getTotalMemory(Context context) {
        final MemoryInfo info = new MemoryInfo();
        ((ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE)).getMemoryInfo(info);
        return info.totalMem;
    }

    /**
     * Executes a shell command using shell user identity, and return the standard output in string
     * <p>Note: calling this function requires API level 21 or above
     * @param instrumentation {@link Instrumentation} instance, obtained from a test running in
     * instrumentation framework
     * @param cmd the command to run
     * @return the standard output of the command
     * @throws Exception
     */
    public static String runShellCommand(Instrumentation instrumentation, String cmd)
            throws IOException {
        Log.v(TAG, "Running command: " + cmd);
        if (cmd.startsWith("pm grant ") || cmd.startsWith("pm revoke ")) {
            throw new UnsupportedOperationException("Use UiAutomation.grantRuntimePermission() "
                    + "or revokeRuntimePermission() directly, which are more robust.");
        }
        ParcelFileDescriptor pfd = instrumentation.getUiAutomation().executeShellCommand(cmd);
        byte[] buf = new byte[512];
        int bytesRead;
        FileInputStream fis = new ParcelFileDescriptor.AutoCloseInputStream(pfd);
        StringBuffer stdout = new StringBuffer();
        while ((bytesRead = fis.read(buf)) != -1) {
            stdout.append(new String(buf, 0, bytesRead));
        }
        fis.close();
        return stdout.toString();
    }

    /**
     * Simpler version of {@link #runShellCommand(Instrumentation, String)}.
     */
    public static String runShellCommand(String cmd) {
        try {
            return runShellCommand(InstrumentationRegistry.getInstrumentation(), cmd);
        } catch (IOException e) {
            fail("Failed reading command output: " + e);
            return "";
        }
    }

    /**
     * Same as {@link #runShellCommand(String)}, with optionally
     * check the result using {@code resultChecker}.
     */
    public static String runShellCommand(String cmd, Predicate<String> resultChecker) {
        final String result = runShellCommand(cmd);
        if (resultChecker != null) {
            assertTrue("Assertion failed. Command was: " + cmd + "\n"
                    + "Output was:\n" + result,
                    resultChecker.test(result));
        }
        return result;
    }

    /**
     * Same as {@link #runShellCommand(String)}, but fails if the output is not empty.
     */
    public static String runShellCommandForNoOutput(String cmd) {
        final String result = runShellCommand(cmd);
        assertTrue("Command failed. Command was: " + cmd + "\n"
                + "Didn't expect any output, but the output was:\n" + result,
                result.length() == 0);
        return result;
    }

    /**
     * Run a command and print the result on logcat.
     */
    public static void runCommandAndPrintOnLogcat(String logtag, String cmd) {
        Log.i(logtag, "Executing: " + cmd);
        final String output = runShellCommand(cmd);
        for (String line : output.split("\\n", -1)) {
            Log.i(logtag, line);
        }
    }

    /**
     * Run a command and return the section matching the patterns.
     *
     * @see TextUtils#extractSection
     */
    public static String runCommandAndExtractSection(String cmd,
            String extractionStartRegex, boolean startInclusive,
            String extractionEndRegex, boolean endInclusive) {
        return TextUtils.extractSection(runShellCommand(cmd), extractionStartRegex, startInclusive,
                extractionEndRegex, endInclusive);
    }
}
