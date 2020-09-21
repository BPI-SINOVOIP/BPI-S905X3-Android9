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
package com.android.tradefed.sandbox;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;

import java.io.File;

/** Class that can receive and provide options to a {@link ISandbox}. */
@OptionClass(alias = "sandbox", global_namespace = false)
public final class SandboxOptions {

    public static final String TF_LOCATION = "tf-location";
    public static final String SANDBOX_BUILD_ID = "sandbox-build-id";

    @Option(
        name = TF_LOCATION,
        description = "The path to the Tradefed binary of the version to use for the sandbox."
    )
    private File mTfVersion = null;

    @Option(
        name = SANDBOX_BUILD_ID,
        description =
                "Provide the build-id to force the sandbox version of Tradefed to be."
                        + "Mutually exclusive with the tf-location option."
    )
    private String mBuildId = null;

    /**
     * Returns the provided directories containing the Trade Federation version to use for
     * sandboxing the run.
     */
    public File getSandboxTfDirectory() {
        return mTfVersion;
    }

    /** Returns the build-id forced for the sandbox to be used during the run. */
    public String getSandboxBuildId() {
        return mBuildId;
    }
}
