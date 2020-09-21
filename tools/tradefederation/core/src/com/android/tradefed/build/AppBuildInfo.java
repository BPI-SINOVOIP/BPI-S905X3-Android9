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

import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * A {@link IBuildInfo} that represents an Android application and its test package(s).
 */
public class AppBuildInfo extends BuildInfo implements IAppBuildInfo {

    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private List<VersionedFile> mAppPackageFiles = new ArrayList<VersionedFile>();

    /**
     * Creates a {@link AppBuildInfo}.
     *
     * @param buildId the unique build id
     * @param buildName the build name
     */
    public AppBuildInfo(String buildId, String buildName) {
        super(buildId, buildName);
    }

    /**
     * @see BuildInfo#BuildInfo(BuildInfo)
     */
    public AppBuildInfo(BuildInfo buildToCopy) {
        super(buildToCopy);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<VersionedFile> getAppPackageFiles() {
        List<VersionedFile> listCopy = new ArrayList<VersionedFile>(mAppPackageFiles.size());
        listCopy.addAll(mAppPackageFiles);
        return listCopy;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAppPackageFile(File appPackageFile, String version) {
        mAppPackageFiles.add(new VersionedFile(appPackageFile, version));
    }

    /**
     * Removes all temporary files
     */
    @Override
    public void cleanUp() {
        for (VersionedFile appPackageFile : mAppPackageFiles) {
            appPackageFile.getFile().delete();
        }
        mAppPackageFiles.clear();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo clone() {
        AppBuildInfo copy = new AppBuildInfo(getBuildId(), getBuildTargetName());
        copy.addAllBuildAttributes(this);
        try {
            for (VersionedFile origVerFile : mAppPackageFiles) {
                // Only using createTempFile to create a unique dest filename
                File origFile = origVerFile.getFile();
                File copyFile = FileUtil.createTempFile(FileUtil.getBaseName(origFile.getName()),
                        FileUtil.getExtension(origFile.getName()));
                copyFile.delete();
                FileUtil.hardlinkFile(origFile, copyFile);
                copy.addAppPackageFile(copyFile, origVerFile.getVersion());
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        copy.setBuildBranch(getBuildBranch());
        copy.setBuildFlavor(getBuildFlavor());

        return copy;
    }
}
