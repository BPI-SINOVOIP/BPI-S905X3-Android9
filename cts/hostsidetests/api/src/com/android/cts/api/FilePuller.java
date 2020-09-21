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

package com.android.cts.api;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import com.android.ddmlib.IShellOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

/**
 * Pulls files from a non-rooted device
 */
class FilePuller {
    private final ITestDevice device;
    private final Path hostDir;

    FilePuller(ITestDevice device) {
        this.device = device;
        try {
            hostDir = Files.createTempDirectory("pulled_files");
            hostDir.toFile().deleteOnExit();
        } catch (IOException e) {
            throw new RuntimeException("Cannot create directory for pulled files", e);
        }
    }

    void clean() {
        hostDir.toFile().delete();
    }

    PulledFile pullFromDevice(String path, String name) {
        try {
            File outputFile = new File(hostDir.toFile(), name);
            FileOutputStream outputStream = new FileOutputStream(outputFile);

            // For files on vendor partition, `adb shell pull` does not work on non-rooted device,
            // due to the permission. Thus using `cat` to copy file content to outside of the
            // device, which might a little bit slower than adb pull, but should be acceptable for
            // testing.
            device.executeShellCommand(String.format("cat %s", path),
                    new IShellOutputReceiver() {

                        @Override
                        public void addOutput(byte[] data, int offset, int len) {
                            try {
                                outputStream.write(data, offset, len);
                            } catch (IOException e) {
                                throw new RuntimeException("Error pulling file " + path, e);
                            }
                        }

                        @Override
                        public void flush() {
                            try {
                                outputStream.close();
                            } catch (IOException e) {
                                throw new RuntimeException("Error saving file " + path,
                                        e);
                            }
                        }

                        @Override
                        public boolean isCancelled() {
                            // don't cancel at any time.
                            return false;
                        }
                    });
            return new PulledFile(outputFile, path);
        } catch (DeviceNotAvailableException e) {
            throw new RuntimeException("Cannot connect to the device", e);
        } catch (IOException e) {
            throw new RuntimeException("Failed to pull file " + path, e);
        }
    }
}
