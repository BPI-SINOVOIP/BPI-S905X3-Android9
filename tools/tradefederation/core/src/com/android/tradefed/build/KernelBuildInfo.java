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

import java.io.File;
import java.io.IOException;

/**
 * A {@link IBuildInfo} that represents a kernel build.
 */
public class KernelBuildInfo extends BuildInfo implements IKernelBuildInfo {
    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private final static String KERNEL_FILE = "kernel";

    private String mSha1 = null;
    private String mShortSha1 = null;
    private long mCommitTime = 0;

    /**
     * Creates a {@link KernelBuildInfo}.
     */
    public KernelBuildInfo() {
        super();
    }

    /**
     * Creates a {@link KernelBuildInfo}.
     *
     * @param sha1 the git sha1, used as the build id
     * @param shortSha1 the git short sha1
     * @param commitTime the git commit time
     * @param buildName the build name
     */
    public KernelBuildInfo(String sha1, String shortSha1, long commitTime, String buildName) {
        super(sha1, buildName);
        mSha1 = sha1;
        mShortSha1 = shortSha1;
        mCommitTime = commitTime;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File getKernelFile() {
        return getFile(KERNEL_FILE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getKernelVersion() {
        return getVersion(KERNEL_FILE);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setKernelFile(File kernelFile, String version) {
        setFile(KERNEL_FILE, kernelFile, version);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getSha1() {
        return mSha1;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSha1(String sha1) {
        mSha1 = sha1;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getShortSha1() {
        return mShortSha1;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setShortSha1(String shortSha1) {
        mShortSha1 = shortSha1;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long getCommitTime() {
        return mCommitTime;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setCommitTime(long time) {
        mCommitTime = time;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo clone() {
        KernelBuildInfo copy = new KernelBuildInfo(getSha1(), getShortSha1(),
                getCommitTime(), getBuildTargetName());
        copy.addAllBuildAttributes(this);
        try {
            copy.addAllFiles(this);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        copy.setBuildBranch(getBuildBranch());
        copy.setBuildFlavor(getBuildFlavor());

        return copy;
    }
}
