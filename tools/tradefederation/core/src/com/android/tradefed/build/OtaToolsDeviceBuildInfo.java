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

import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;

/**
 * An {@link OtaDeviceBuildInfo} that also contains an otatools directory.
 */
public class OtaToolsDeviceBuildInfo extends OtaDeviceBuildInfo {
    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private File mOtaToolsDir;

    /**
     * Construct based on an {@link OtaDeviceBuildInfo}.
     * @param parent
     */
    public OtaToolsDeviceBuildInfo(OtaDeviceBuildInfo parent) {
        setBaselineBuild(parent.mBaselineBuild);
        setOtaBuild(parent.mOtaBuild);
    }

    public void setOtaTools(File otaTools) {
        mOtaToolsDir = otaTools;
    }

    public File getOtaTools() {
        return mOtaToolsDir;
    }

    // Allow the OTA package to be overwritten
    @Override
    public void setOtaPackageFile(File file, String version) {
        ((DeviceBuildInfo)mBaselineBuild)
            .getVersionedFileMap().put("ota", new VersionedFile(file, version));
        ((DeviceBuildInfo)mOtaBuild)
            .getVersionedFileMap().put("ota", new VersionedFile(file, version));
    }

    @Override
    public void cleanUp() {
        FileUtil.recursiveDelete(mOtaToolsDir);
        super.cleanUp();
    }

    @Override
    public IBuildInfo clone() {
        try {
            OtaToolsDeviceBuildInfo clone = new OtaToolsDeviceBuildInfo(
                    (OtaDeviceBuildInfo)super.clone());
            File toolsCopy = FileUtil.createTempDir("otatools");
            FileUtil.recursiveHardlink(mOtaToolsDir, toolsCopy);
            clone.setOtaTools(toolsCopy);
            return clone;
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}

