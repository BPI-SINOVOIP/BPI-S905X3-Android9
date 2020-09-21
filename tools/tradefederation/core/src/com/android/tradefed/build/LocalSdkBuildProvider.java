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
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;

import java.io.File;

/**
 * A {@link IBuildProvider} that constructs a {@link ISdkBuildInfo} based on a provided local path
 */
@OptionClass(alias = "local-sdk")
public class LocalSdkBuildProvider implements IBuildProvider {

    private static final String SDK_OPTION_NAME = "sdk-path";

    @Option(name = SDK_OPTION_NAME, description =
            "the local filesystem path to sdk root directory to test.",
            importance = Importance.IF_UNSET)
    private File mLocalSdkPath = getSdkFromBuildEnv();

    @Option(name = "test-path", description =
            "the local filesystem path to a test directory.",
            importance = Importance.IF_UNSET)
    private File mLocalTestPath = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild() throws BuildRetrievalError {
        ISdkBuildInfo sdkBuild = new SdkBuildInfo("0", "sdk");
        if (mLocalSdkPath == null) {
            throw new IllegalArgumentException(String.format("missing --%s option",
                    SDK_OPTION_NAME));
        }
        if (!mLocalSdkPath.exists()) {
            throw new IllegalArgumentException(String.format("SDK path '%s' does not exist. " +
                    "Please provide a valid sdk via --%s, or ensure you are running " +
                    "tradefed in a Android build env with a built SDK",
                    mLocalSdkPath.getAbsolutePath(), SDK_OPTION_NAME));
        }
        // allow a null test-build-path
        sdkBuild.setSdkDir(mLocalSdkPath);
        sdkBuild.setTestsDir(mLocalTestPath);
        return sdkBuild;
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

    /**
     * Return the output sdk location if running in an Android source tree build environment,
     * Otherwise return null.
     * <p/>
     * Assumes built SDK will be in following path:
     * <p/>
     * <i>
     * ${ANDROID_BUILD_TOP}/out/host/{os.name}-x86/sdk/android-sdk_eng.{user.name}_{os.name}-x86
     * </i>
     * @return the {@link File} to the output sdk location.
     */
    private File getSdkFromBuildEnv() {
        String buildRoot = System.getenv("ANDROID_BUILD_TOP");

        if (buildRoot != null) {
            String osName = System.getProperty("os.name");
            String osPath = null;
            if (osName.contains("Mac")) {
                osPath = "darwin-x86";
            } else if (osName.contains("Linux")) {
                osPath = "linux-x86";
            } else {
                CLog.w(String.format("Cannot determine sdk build path due to unrecognized os '%s'",
                        osName));
                return null;
            }
            String userName = System.getProperty("user.name");
            String path = FileUtil.getPath(buildRoot, "out", "host", osPath, "sdk",
                    String.format("android-sdk_eng.%s_%s", userName, osPath));
            return new File(path);
        }
        return null;
    }
}
