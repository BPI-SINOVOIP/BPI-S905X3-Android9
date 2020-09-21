/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * An {@link IBuildInfo} that contains otatools artifacts.
 */
public class OtatoolsBuildInfo extends BuildInfo {
    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private static final String SECURITY_DIR_NAME = "otatools_security";
    private static final String BIN_DIR_NAME = "otatools_bin";
    private static final String FRAMEWORK_DIR_NAME = "otatools_framework";
    private static final String RELEASETOOLS_DIR_NAME = "otatools_releasetools";

    /**
     * Creates a {@link OtatoolsBuildInfo}
     */
    public OtatoolsBuildInfo(String buildId, String buildTargetName) {
        super(buildId, buildTargetName);
    }

    /**
     * Add /build/target/product/security to this file map
     */
    public void setSecurityDir(File dir, String version) {
        setFile(SECURITY_DIR_NAME, dir, version);
    }

    /**
     * Get the {@link File} representing the /build/target/product/security directory.
     */
    public File getSecurityDir() {
        return getFile(SECURITY_DIR_NAME);
    }

    /**
     * Add /bin to this file map
     */
    public void setBinDir(File dir, String version) {
        setFile(BIN_DIR_NAME, dir, version);
    }

    /**
     * Get the {@link File} representing the /bin directory.
     */
    public File getBinDir() {
        return getFile(BIN_DIR_NAME);
    }

    /**
     * Add /framework to this file map
     */
    public void setFrameworkDir(File dir, String version) {
        setFile(FRAMEWORK_DIR_NAME, dir, version);
    }

    /**
     * Get the {@link File} representing the /framework directory.
     */
    public File getFrameworkDir() {
        return getFile(FRAMEWORK_DIR_NAME);
    }

    /**
     * Add /releasetools to this file map
     */
    public void setReleasetoolsDir(File dir, String version) {
        setFile(RELEASETOOLS_DIR_NAME, dir, version);
    }

    /**
     * Get the {@link File} representing the /releasetools directory.
     */
    public File getReleasetoolsDir() {
        return getFile(RELEASETOOLS_DIR_NAME);
    }
}
