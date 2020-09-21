/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;

/**
 * An {@link ITargetPreparer} that writes build info meta data into a specified file.
 *
 * <p>The file is written in simple key-value pair format; each line of the file has:<br>
 * <code>key=value</code><br>
 * where <code>key</code> is a meta data field from {@link IBuildInfo}
 *
 * <p>Currently, only build id is written.
 */
@OptionClass(alias = "build-info-recorder")
public class BuildInfoRecorder extends BaseTargetPreparer {

    @Option(name = "build-info-file", description = "when specified, build info will be written "
            + "into the file specified. Any existing file will be overwritten.")
    private File mBuildInfoFile = null;

    /*
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (mBuildInfoFile != null) {
            try {
                String alias = buildInfo.getBuildAttributes().get("build_alias");
                if (alias == null) {
                    alias = buildInfo.getBuildId();
                }
                FileUtil.writeToFile(String.format("%s=%s\n%s=%s\n",
                        "build_id", buildInfo.getBuildId(),
                        "build_alias", alias),
                        mBuildInfoFile);
            } catch (IOException ioe) {
                CLog.e("Exception while writing build info into %s",
                        mBuildInfoFile.getAbsolutePath());
                CLog.e(ioe);
            }
        }
    }

}
