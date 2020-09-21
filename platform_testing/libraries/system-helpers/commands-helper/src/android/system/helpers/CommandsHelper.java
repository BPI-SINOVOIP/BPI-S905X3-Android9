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
package android.system.helpers;

import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;
import android.util.Log;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;

/**
 * Implement common helper for executing shell commands on device
 */
public class CommandsHelper {
    private static final String TAG = CommandsHelper.class.getSimpleName();
    private static CommandsHelper sInstance = null;
    private Instrumentation mInstrumentation = null;

    private static final String LINE_SEPARATORS = "\\r?\\n";


    private CommandsHelper(Instrumentation instrumentation) {
        mInstrumentation = instrumentation;
    }

    /**
     * @deprecated Should use {@link CommandsHelper#getInstance(Instrumentation)} instead.
     */
    @Deprecated
    public static CommandsHelper getInstance() {
        if (sInstance == null) {
            sInstance = new CommandsHelper(InstrumentationRegistry.getInstrumentation());
        }
        return sInstance;
    }

    public static CommandsHelper getInstance(Instrumentation instrumentation) {
        if (sInstance == null) {
            sInstance = new CommandsHelper(instrumentation);
        } else {
            sInstance.injectInstrumentation(instrumentation);
        }
        return sInstance;
    }

    /**
     * Injects instrumentation into this helper.
     *
     * @param instrumentation the instrumentation to use with this instance
     */
    public void injectInstrumentation(Instrumentation instrumentation) {
        mInstrumentation = instrumentation;
    }

    /**
     * Executes a shell command on device, and return the standard output in string.
     * @param command the command to run
     * @return the standard output of the command, or empty string if
     * failed without throwing an IOException
     */
    public String executeShellCommand(String command) {
        try {
            return UiDevice.getInstance(mInstrumentation).executeShellCommand(command);
        } catch (IOException e) {
            // ignore
            Log.e(TAG, String.format("The shell command failed to run: %s exception: %s",
                    command, e.getMessage()));
            return "";
        }
    }

    /**
     * Executes a shell command on device, and split the multi-line output into collection
     * @param command the command to run
     * @param separatorChars the line separator
     * @return the List of strings from the standard output of the command
     */
    public List<String> executeShellCommandAndSplitOutput(String command,
            final String separatorChars) {
        return Arrays.asList(executeShellCommand(command).split(separatorChars));
    }

    /**
     * Convenience version of {@link #executeShellCommand} for use without having a reference to
     * CommandsHelper.
     * @param command the command to run
     */
    @Deprecated
    public static String execute(String command) {
        return getInstance().executeShellCommand(command);
    }

    /**
     * Convenience version of {@link #executeShellCommandAndSplitOutput} for use
     * without having a reference to CommandsHelper.
     * @param command the command to run
     */
    @Deprecated
    public static List<String> executeAndSplitLines(String command) {
        return getInstance().executeShellCommandAndSplitOutput(command, LINE_SEPARATORS);
    }
}
