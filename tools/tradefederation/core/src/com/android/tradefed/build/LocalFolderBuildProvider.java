/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;

import java.io.File;

/**
 * A {@link IBuildProvider} that constructs a {@link IFolderBuildInfo} based on a provided local
 * path
 */
@OptionClass(alias = "local-folder")
public class LocalFolderBuildProvider extends StubBuildProvider {

    private static final String FOLDER_OPTION_NAME = "folder-path";

    @Option(name = FOLDER_OPTION_NAME, description =
            "the local filesystem path to build root directory to test.",
            importance = Importance.IF_UNSET)
    private File mLocalFolder = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild() throws BuildRetrievalError {
        if (mLocalFolder == null) {
            throw new IllegalArgumentException(String.format("missing --%s option",
                    FOLDER_OPTION_NAME));
        }
        if (!mLocalFolder.exists()) {
            throw new IllegalArgumentException(String.format("path '%s' does not exist. " +
                    "Please provide a valid folder via --%s",
                    mLocalFolder.getAbsolutePath(), FOLDER_OPTION_NAME));
        }
        // utilize parent build provider to set build id, test target name etc attributes if
        // desired
        IBuildInfo parentBuild = super.getBuild();
        IFolderBuildInfo folderBuild = new FolderBuildInfo((BuildInfo)parentBuild);
        folderBuild.setRootDir(mLocalFolder);
        return folderBuild;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void buildNotTested(IBuildInfo info) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void cleanUp(IBuildInfo info) {
        // ignore
    }
}
