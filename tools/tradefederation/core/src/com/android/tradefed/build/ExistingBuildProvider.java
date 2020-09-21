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


/**
 * A {@link IBuildProvider} that returns an already constructed {@link IBuildInfo}.
 */
public class ExistingBuildProvider implements IBuildProvider {

    private final IBuildInfo mBuildInfo;
    private final IBuildProvider mParentProvider;
    private boolean mBuildMarkedNotTested = false;

    /**
     * Creates a {@link ExistingBuildProvider}.
     *
     * @param buildInfo the existing build to provide
     * @param parentProvider the original {@link IBuildProvider} that created the {@link IBuildInfo}
     *            Needed to pass along {@link IBuildProvider#buildNotTested(IBuildInfo)} events.
     */
    public ExistingBuildProvider(IBuildInfo buildInfo, IBuildProvider parentProvider) {
        mBuildInfo = buildInfo;
        mParentProvider = parentProvider;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild() throws BuildRetrievalError {
        return mBuildInfo;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void buildNotTested(IBuildInfo info) {
        if (!mBuildMarkedNotTested) {
            mBuildMarkedNotTested = true;
            mParentProvider.buildNotTested(info);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void cleanUp(IBuildInfo info) {
        mParentProvider.cleanUp(info);
    }
}
