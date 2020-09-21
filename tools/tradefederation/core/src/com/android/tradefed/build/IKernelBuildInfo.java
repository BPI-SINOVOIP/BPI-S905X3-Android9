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

/**
 * A {@link IBuildInfo} that represents a kernel build.
 */
public interface IKernelBuildInfo extends IBuildInfo {

    /**
     * Get the kernel file.
     *
     * @return the kernel file
     */
    public File getKernelFile();

    /**
     * Get the kernel file version.
     *
     * @return the kernel file version
     */
    public String getKernelVersion();

    /**
     * Set the kernel file
     *
     * @param kernelFile
     */
    public void setKernelFile(File kernelFile, String version);

    /**
     * Get the git commit time.
     *
     * @return the git commit time in seconds since the Unix epoch.
     */
    public long getCommitTime();

    /**
     * Sets the git commit time for the change.
     *
     * @param time the time in seconds since the Unix epoch.
     */
    public void setCommitTime(long time);

    /**
     * Gets the git sha1.
     *
     * @return the git sha1
     */
    public String getSha1();

    /**
     * Sets the git sha1.
     *
     * @param sha1 the git sha1
     */
    public void setSha1(String sha1);

    /**
     * Gets the git short sha1.
     *
     * @return the git short sha1
     */
    public String getShortSha1();

    /**
     * Sets the git short sha1.
     *
     * @param shortSha1 the git short sha1
     */
    public void setShortSha1(String shortSha1);
}
