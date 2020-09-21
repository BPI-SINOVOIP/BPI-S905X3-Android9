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

import static java.nio.file.StandardCopyOption.REPLACE_EXISTING;

import android.os.Environment;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * Captures screen contents and saves it as a mp4 video file.
 */
public class ScreenRecorder {
    private static final String TAG = "FLICKER";
    private static final String OUTPUT_DIR =
            Environment.getExternalStorageDirectory().getPath();
    @VisibleForTesting
    static final Path DEFAULT_OUTPUT_PATH = Paths.get(OUTPUT_DIR ,  "transition.mp4");
    private Thread recorderThread;

    @VisibleForTesting
    static Path getPath(String testTag) {
        return Paths.get(OUTPUT_DIR, testTag + ".mp4");
    }

    private static Path getPath(String testTag, int iteration) {
        return Paths.get(OUTPUT_DIR, testTag + "_" + Integer.toString(iteration) + ".mp4");
    }

    public void start() {
        String command = "screenrecord " + DEFAULT_OUTPUT_PATH;
        recorderThread = new Thread(() -> {
            try {
                Runtime.getRuntime().exec(command);
            } catch (IOException e) {
                Log.e(TAG, "Error executing " + command, e);
            }
        });
        recorderThread.start();
    }

    public void stop() {
        runShellCommand("killall -s 2 screenrecord");
        try {
            recorderThread.join();
        } catch (InterruptedException e) {
            // ignore
        }
    }

    public Path save(String testTag, int iteration) throws IOException {
        return Files.move(DEFAULT_OUTPUT_PATH, getPath(testTag, iteration),
                REPLACE_EXISTING);
    }

    public Path save(String testTag) throws IOException {
        return Files.move(DEFAULT_OUTPUT_PATH, getPath(testTag), REPLACE_EXISTING);
    }
}
