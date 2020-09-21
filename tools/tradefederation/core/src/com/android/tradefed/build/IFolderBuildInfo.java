/*
 * Copyright (C) 2010 The Android Open Source Project
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
 * A simple abstract {@link IBuildInfo} whose build artifacts are containing in a local filesystem
 * directory.
 */
public interface IFolderBuildInfo extends IBuildInfo {

    /**
     * Get the root folder that contains the build artifacts.
     *
     * @return the {@link File} directory.
     */
    public File getRootDir();

    /**
     * Set the root directory that contains the build artifacts.
     */
    public void setRootDir(File rootDir);

    /**
     * Deletes root directory and all its contents.
     */
    @Override
    public void cleanUp();
}
