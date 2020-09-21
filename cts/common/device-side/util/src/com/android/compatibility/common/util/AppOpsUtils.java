/*
 * Copyright (C) 2018 Google Inc.
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

import static android.app.AppOpsManager.MODE_ALLOWED;
import static android.app.AppOpsManager.MODE_DEFAULT;
import static android.app.AppOpsManager.MODE_ERRORED;
import static android.app.AppOpsManager.MODE_IGNORED;

import android.app.AppOpsManager;
import android.support.test.InstrumentationRegistry;

import java.io.IOException;

/**
 * Utilities for controlling App Ops settings, and testing whether ops are logged.
 */
public class AppOpsUtils {

    /**
     * Resets a package's app ops configuration to the device default. See AppOpsManager for the
     * default op settings.
     *
     * <p>
     * It's recommended to call this in setUp() and tearDown() of your test so the test starts and
     * ends with a reproducible default state, and so doesn't affect other tests.
     *
     * <p>
     * Some app ops are configured to be non-resettable, which means that the state of these will
     * not be reset even when calling this method.
     */
    public static String reset(String packageName) throws IOException {
        return runCommand("appops reset " + packageName);
    }

    /**
     * Sets the app op mode (e.g. allowed, denied) for a single package and operation.
     */
    public static String setOpMode(String packageName, String opStr, int mode)
            throws IOException {
        String modeStr;
        switch (mode) {
            case MODE_ALLOWED:
                modeStr = "allow";
                break;
            case MODE_ERRORED:
                modeStr = "deny";
                break;
            case MODE_IGNORED:
                modeStr = "ignore";
                break;
            case MODE_DEFAULT:
                modeStr = "default";
                break;
            default:
                throw new IllegalArgumentException("Unexpected app op type");
        }
        String command = "appops set " + packageName + " " + opStr + " " + modeStr;
        return runCommand(command);
    }

    /**
     * Returns whether an allowed operation has been logged by the AppOpsManager for a
     * package. Operations are noted when the app attempts to perform them and calls e.g.
     * {@link AppOpsManager#noteOperation}.
     *
     * @param opStr The public string constant of the operation (e.g. OPSTR_READ_SMS).
     */
    public static boolean allowedOperationLogged(String packageName, String opStr)
            throws IOException {
        return getOpState(packageName, opStr).contains(" time=");
    }

    /**
     * Returns whether an allowed operation has been logged by the AppOpsManager for a
     * package. Operations are noted when the app attempts to perform them and calls e.g.
     * {@link AppOpsManager#noteOperation}.
     *
     * @param opStr The public string constant of the operation (e.g. OPSTR_READ_SMS).
     */
    public static boolean rejectedOperationLogged(String packageName, String opStr)
            throws IOException {
        return getOpState(packageName, opStr).contains(" rejectTime=");
    }

    /**
     * Returns the app op state for a package. Includes information on when the operation was last
     * attempted to be performed by the package.
     *
     * Format: "SEND_SMS: allow; time=+23h12m54s980ms ago; rejectTime=+1h10m23s180ms"
     */
    private static String getOpState(String packageName, String opStr) throws IOException {
        return runCommand("appops get " + packageName + " " + opStr);
    }

    private static String runCommand(String command) throws IOException {
        return SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(), command);
    }
}