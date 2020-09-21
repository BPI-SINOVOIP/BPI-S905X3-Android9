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

import java.io.File;

/**
 *  * A {@link IBuildInfo} that represents an Android application and its test zip.
 */
public interface ITestzipBuildInfo extends IBuildInfo {

    /**
     * Get the local path to the extracted tests.zip file contents.
     */
    public File getTestzipDir();

    /**
     * Set local path to the extracted tests.zip file contents.
     *
     * @param testsZipFile
     */
    public void setTestzipDir(File testsZipFile, String version);

    /**
     * Get the extracted tests.zip version.
     */
    public String getTestzipDirVersion();
}
