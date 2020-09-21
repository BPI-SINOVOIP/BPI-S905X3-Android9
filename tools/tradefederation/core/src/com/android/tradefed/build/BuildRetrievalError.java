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

/**
 * A fatal error occurred while retrieving the build for testing.
 */
public class BuildRetrievalError extends Exception {

    private static final long serialVersionUID = -1636070073516175229L;
    private IBuildInfo mBuildInfo = new BuildInfo();

    /**
     * Constructs a new {@link BuildRetrievalError} with a meaningful error message.
     *
     * @param reason a error message describing the cause of the error
     */
    public BuildRetrievalError(String reason) {
        super(reason);
    }

    /**
     * Constructs a new {@link BuildRetrievalError} with a meaningful error message, and a
     * cause.
     *
     * @param reason a detailed error message.
     * @param cause a {@link Throwable} capturing the original cause of the ProvideBuildError
     */
    public BuildRetrievalError(String reason, Throwable cause) {
        super(reason, cause);
    }

    /**
     * Constructs a new {@link BuildRetrievalError} with a meaningful error message, a
     * cause, and build details.
     *
     * @param reason a detailed error message.
     * @param cause a {@link Throwable} capturing the original cause of the ProvideBuildError
     * @param build details about the build that was attempted to be retrieved
     */
    public BuildRetrievalError(String reason, Throwable cause, IBuildInfo build) {
        super(reason, cause);
        mBuildInfo = build;
    }

    /**
     * Return details about the build that was attempted to be retrieved.
     * <p/>
     * The returned {@link IBuildInfo} will never be null but it may be missing data such as
     * build_id, etc
     *
     * @return the {@link IBuildInfo}
     */
    public IBuildInfo getBuildInfo() {
        return mBuildInfo;
    }

    /**
     * Set the build info.
     *
     * @param build the {@link IBuildInfo}
     */
    public void setBuildInfo(IBuildInfo build) {
        mBuildInfo = build;
    }
}
