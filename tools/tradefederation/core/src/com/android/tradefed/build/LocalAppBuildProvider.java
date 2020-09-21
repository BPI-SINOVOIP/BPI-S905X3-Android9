/*
 * Copyright (C) 2012 The Android Open Source Project
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
import java.util.ArrayList;
import java.util.Collection;

/**
 * A {@link IBuildProvider} that constructs a {@link IAppBuildInfo} based on a provided local
 * path
 */
@OptionClass(alias = "local-app")
public class LocalAppBuildProvider extends StubBuildProvider {

    private static final String APP_OPTION_NAME = "app-path";

    @Option(name = APP_OPTION_NAME, description =
            "the local filesystem path to apk.",
            importance = Importance.IF_UNSET, mandatory=true)
    private Collection<File> mApkPaths = new ArrayList<File>();

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild() throws BuildRetrievalError {
        // utilize parent build provider to set build id, test target name etc attributes if
        // desired
        IBuildInfo parentBuild = super.getBuild();
        IAppBuildInfo appBuild = new AppBuildInfo((BuildInfo)parentBuild);
        for (File apkPath : mApkPaths) {
            if (!apkPath.exists()) {
                throw new IllegalArgumentException(String.format("path '%s' does not exist. "
                        + "Please provide a valid file via --%s", apkPath.getAbsolutePath(),
                        APP_OPTION_NAME));
            }
            appBuild.addAppPackageFile(apkPath, parentBuild.getBuildId());
        }
        return appBuild;
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
