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

import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import java.io.File;

/**
 * Unit tests for {@link KernelBuildInfo}.
 */
public class KernelBuildInfoTest extends TestCase {

    private IKernelBuildInfo mBuildInfo;
    private File mKernelFile;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mBuildInfo = new KernelBuildInfo("kernel", "ker", 1, "build");
        mKernelFile = FileUtil.createTempFile("kernel", null);
        FileUtil.writeToFile("filedata", mKernelFile);
        mBuildInfo.setKernelFile(mKernelFile, "1");
    }

    @Override
    protected void tearDown() throws Exception {
        FileUtil.deleteFile(mKernelFile);
        super.tearDown();
    }

    /**
     * Test method for {@link KernelBuildInfo#clone()}.
     */
    public void testClone() throws Exception {
        IKernelBuildInfo copy = (KernelBuildInfo) mBuildInfo.clone();

        try {
            assertEquals(mBuildInfo.getBuildBranch(), copy.getBuildBranch());
            assertEquals(mBuildInfo.getBuildFlavor(), copy.getBuildFlavor());
            assertEquals(mBuildInfo.getBuildId(), copy.getBuildId());
            assertEquals(mBuildInfo.getBuildTargetName(), copy.getBuildTargetName());
            assertEquals(mBuildInfo.getCommitTime(), copy.getCommitTime());
            assertEquals(mBuildInfo.getShortSha1(), copy.getShortSha1());
            assertEquals(mBuildInfo.getTestTag(), copy.getTestTag());

            assertFalse(mKernelFile.getAbsolutePath().equals(copy.getKernelFile()));
            assertTrue(FileUtil.compareFileContents(mKernelFile, copy.getKernelFile()));
        } finally {
            copy.cleanUp();
        }
    }

    /**
     * Test method for {@link KernelBuildInfo#cleanUp()}.
     */
    public void testCleanUp() {
        assertTrue(mBuildInfo.getKernelFile().exists());
        mBuildInfo.cleanUp();
        assertNull(mBuildInfo.getKernelFile());
        assertFalse(mKernelFile.exists());
    }
}
