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
import com.android.tradefed.util.ZipUtil;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.StringReader;
import java.util.Properties;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Provides a {@link IBuildInfo} based on a local OTA zip file.
 */
public class OtaZipfileBuildProvider implements IBuildProvider {

    @Option(name = "ota-path", description = "path to the OTA zipfile",
            importance = Importance.IF_UNSET)
    private String mOtaZipPath = null;

    @Override
    public IBuildInfo getBuild() throws BuildRetrievalError {
        Properties buildProp = null;
        try {
            buildProp = new Properties();
            buildProp.load(new StringReader(getBuildPropContents()));
        } catch (IOException e) {
            throw new BuildRetrievalError(
                    "Error processing build.prop contents from file: " + getOtaPath(), e);
        }
        String bid = buildProp.getProperty("ro.build.version.incremental");
        IDeviceBuildInfo buildInfo = new DeviceBuildInfo(bid, bid);
        buildInfo.setOtaPackageFile(new File(getOtaPath()), bid);
        return buildInfo;

    }

    /**
     * Package-private for testing
     *
     * @return an {@link InputStream} of the contents of the system/build.prop
     *         file inside the OTA zipfile
     * @throws BuildRetrievalError
     */
    String getBuildPropContents() throws BuildRetrievalError {
        ZipFile otaZip = null;
        try {
            otaZip = new ZipFile(getOtaPath());
            ZipEntry buildPropEntry = otaZip.getEntry("system/build.prop");
            if (buildPropEntry == null) {
                // no build.prop in the zip file
                throw new BuildRetrievalError(
                        "Couldn't find a build.prop file in OTA zip file " + getOtaPath());
            }
            StringBuilder body = new StringBuilder();
            BufferedReader reader = new BufferedReader(
                    new InputStreamReader(otaZip.getInputStream(buildPropEntry)));
            String line = reader.readLine();
            while (line != null) {
                body.append(line).append("\n");
                line = reader.readLine();
            }

            return body.toString();
        } catch (IOException e) {
            throw new BuildRetrievalError("Failure while getting build.prop from OTA zipfile", e);
        } finally {
            ZipUtil.closeZip(otaZip);
        }
    }

    @Override
    public void buildNotTested(IBuildInfo info) {
        // ignore
    }

    @Override
    public void cleanUp(IBuildInfo info) {
        // ignore
    }

    String getOtaPath() {
        if (mOtaZipPath == null || mOtaZipPath.isEmpty()) {
            throw new IllegalArgumentException("Please pass ota-path");
        }
        return mOtaZipPath;
    }
}
