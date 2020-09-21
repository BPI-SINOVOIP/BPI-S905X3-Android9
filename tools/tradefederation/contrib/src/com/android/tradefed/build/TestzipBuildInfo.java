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
import java.io.IOException;

/**
 * A {@link IBuildInfo} that represents an Android application and its test package(s).
 */
public class TestzipBuildInfo extends BuildInfo implements ITestzipBuildInfo {

    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private static final String TESTDIR_IMAGE_NAME = "testsdir";

    /**
     * Creates a {@link TestzipBuildInfo}.
     *
     * @param buildId the unique build id
     * @param buildName the build name
     */
    public TestzipBuildInfo(String buildId, String buildName) {
        super(buildId, buildName);
    }

    /**
     * @see BuildInfo#BuildInfo(BuildInfo)
     */
    public TestzipBuildInfo(BuildInfo buildToCopy) {
        super(buildToCopy);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getTestzipDir() {
        return getFile(TESTDIR_IMAGE_NAME);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getTestzipDirVersion() {
        return getVersion(TESTDIR_IMAGE_NAME);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestzipDir(File testsDir, String version) {
        setFile(TESTDIR_IMAGE_NAME, testsDir, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo clone() {
        AppBuildInfo copy = new AppBuildInfo(getBuildId(), getBuildTargetName());
        copy.addAllBuildAttributes(this);
        try {
            copy.addAllFiles(this);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        copy.setBuildBranch(getBuildBranch());
        copy.setBuildFlavor(getBuildFlavor());

        return copy;
    }
}
