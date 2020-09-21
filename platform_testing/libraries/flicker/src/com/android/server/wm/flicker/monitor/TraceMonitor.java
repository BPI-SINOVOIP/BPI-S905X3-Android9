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

package com.android.server.wm.flicker.monitor;

import static com.android.compatibility.common.util.SystemUtil.runShellCommand;

import android.os.Environment;
import android.os.RemoteException;

import com.android.internal.annotations.VisibleForTesting;

import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Locale;

/**
 * Base class for monitors containing common logic to read the trace
 * as a byte array and save the trace to another location.
 */
public abstract class TraceMonitor {
    public static final String TAG = "FLICKER";
    private static final String TRACE_DIR = "/data/misc/wmtrace/";
    private static final String OUTPUT_DIR =
            Environment.getExternalStorageDirectory().getPath();

    String traceFileName;

    abstract void start();

    abstract void stop();

    abstract boolean isEnabled() throws RemoteException;

    /**
     * Saves trace file to the external storage directory suffixing the name with the testtag
     * and iteration.
     *
     * Moves the trace file from the default location via a shell command since the test app
     * does not have security privileges to access /data/misc/wmtrace.
     * @param testTag suffix added to trace name used to identify trace
     * @param iteration suffix added to trace name used to identify trace
     * @return Path to saved trace file
     */
    public Path saveTraceFile(String testTag, int iteration) {
        Path traceFileCopy = getOutputTraceFilePath(testTag, iteration);
        String copyCommand = String.format(Locale.getDefault(), "mv %s%s %s", TRACE_DIR,
                traceFileName, traceFileCopy.toString());
        runShellCommand(copyCommand);
        return traceFileCopy;
    }

    @VisibleForTesting
    Path getOutputTraceFilePath(String testTag, int iteration) {
        return Paths.get(OUTPUT_DIR, traceFileName + "_" + testTag + "_" + iteration);
    }
}
