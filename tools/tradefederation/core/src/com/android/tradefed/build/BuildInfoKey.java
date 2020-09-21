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
package com.android.tradefed.build;

/** Class holding enumeration related to build information queries. */
public class BuildInfoKey {

    /**
     * Enum describing all the known file types that can be queried through {@link
     * IBuildInfo#getFile(BuildInfoFileKey)}.
     */
    public enum BuildInfoFileKey {
        DEVICE_IMAGE("device"),
        USERDATA_IMAGE("userdata"),
        TESTDIR_IMAGE("testsdir"),
        BASEBAND_IMAGE("baseband"),
        BOOTLOADER_IMAGE("bootloader"),
        OTA_IMAGE("ota"),
        MKBOOTIMG_IMAGE("mkbootimg"),
        RAMDISK_IMAGE("ramdisk"),

        // Externally linked files in the testsdir:
        // ANDROID_HOST_OUT_TESTCASES and ANDROID_TARGET_OUT_TESTCASES are linked in the tests dir
        // of the build info.
        TARGET_LINKED_DIR("target_testcases"),
        HOST_LINKED_DIR("host_testcases");

        private final String mFileKey;

        private BuildInfoFileKey(String fileKey) {
            mFileKey = fileKey;
        }

        public String getFileKey() {
            return mFileKey;
        }

        @Override
        public String toString() {
            return mFileKey;
        }
    }
}
