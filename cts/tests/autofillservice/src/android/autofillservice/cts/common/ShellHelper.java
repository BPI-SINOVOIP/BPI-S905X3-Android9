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

package android.autofillservice.cts.common;

import androidx.annotation.NonNull;
import android.support.test.InstrumentationRegistry;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;

import com.android.compatibility.common.util.SystemUtil;

/**
 * Provides Shell-based utilities such as running a command.
 */
public final class ShellHelper {

    private static final String TAG = "ShellHelper";

    /**
     * Runs a Shell command, returning a trimmed response.
     */
    @NonNull
    public static String runShellCommand(@NonNull String template, Object...args) {
        final String command = String.format(template, args);
        Log.d(TAG, "runShellCommand(): " + command);
        try {
            final String result = SystemUtil
                    .runShellCommand(InstrumentationRegistry.getInstrumentation(), command);
            return TextUtils.isEmpty(result) ? "" : result.trim();
        } catch (Exception e) {
            throw new RuntimeException("Command '" + command + "' failed: ", e);
        }
    }

    /**
     * Tap on the view center, it may change window focus.
     */
    public static void tap(View view) {
        final int[] xy = new int[2];
        view.getLocationOnScreen(xy);
        final int viewWidth = view.getWidth();
        final int viewHeight = view.getHeight();
        final int x = (int) (xy[0] + (viewWidth / 2.0f));
        final int y = (int) (xy[1] + (viewHeight / 2.0f));

        runShellCommand("input touchscreen tap %d %d", x, y);
    }


    private ShellHelper() {
        throw new UnsupportedOperationException("contain static methods only");
    }
}
