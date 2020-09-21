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

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * No-op empty implementation of a {@link IBuildProvider}.
 * <p/>
 * Will provide an empty {@link BuildInfo} with the provided values from options.
 */
@OptionClass(alias="stub")
public class StubBuildProvider implements IBuildProvider {

    @Option(name="build-id", description="build id to supply.")
    private String mBuildId = "0";

    @Option(name="build-target", description="build target name to supply.")
    private String mBuildTargetName = "stub";

    @Option(name="branch", description="build branch name to supply.")
    private String mBranch = null;

    @Option(name="build-flavor", description="build flavor name to supply.")
    private String mBuildFlavor = null;

    @Option(name = "build-os", description = "build os name to supply.")
    private String mBuildOs = null;

    @Option(name="build-attribute", description="build attributes to supply.")
    private Map<String, String> mBuildAttributes = new HashMap<String,String>();

    @Option(
        name = "return-null",
        description = "force the stub provider to return a null build. Used for testing."
    )
    private boolean mReturnNull = false;

    @Option(
        name = "throw-build-error",
        description = "force the stub provider to throw a BuildRetrievalError. Used for testing."
    )
    private boolean mThrowError = false;

    /** Standard platforms */
    private static final Set<String> STANDARD_PLATFORMS =
            new HashSet<String>(Arrays.asList("linux", "mac"));

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild() throws BuildRetrievalError {
        if (mReturnNull) {
            CLog.d("Returning a null build.");
            return null;
        }
        if (mThrowError) {
            throw new BuildRetrievalError("stub failed to get build.");
        }
        CLog.d("skipping build provider step");
        BuildInfo stubBuild = new BuildInfo(mBuildId, mBuildTargetName);
        stubBuild.setBuildBranch(mBranch);
        stubBuild.setBuildFlavor(mBuildFlavor);

        String buildTarget = mBuildFlavor;
        if (!STANDARD_PLATFORMS.contains(mBuildOs)) {
            buildTarget += "_" + mBuildOs;
        }
        stubBuild.addBuildAttribute("build_target", buildTarget);

        for (Map.Entry<String, String> attributeEntry : mBuildAttributes.entrySet()) {
            stubBuild.addBuildAttribute(attributeEntry.getKey(), attributeEntry.getValue());
        }
        return stubBuild;
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
