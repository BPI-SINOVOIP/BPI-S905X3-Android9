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
package com.android.tradefed.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.build.IBuildInfo;

import org.easymock.EasyMock;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.InputStream;
import java.nio.file.Paths;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Unit tests for {@link TestMapping}. */
@RunWith(JUnit4.class)
public class TestMappingTest {

    private static final String TEST_DATA_DIR = "testdata";
    private static final String TEST_MAPPING = "TEST_MAPPING";
    private static final String TEST_MAPPINGS_ZIP = "test_mappings.zip";

    /** Test for {@link TestMapping#getTests()} implementation. */
    @Test
    public void testparseTestMapping() throws Exception {
        File tempDir = null;
        File testMappingFile = null;

        try {
            tempDir = FileUtil.createTempDir("test_mapping");
            String srcFile = File.separator + TEST_DATA_DIR + File.separator + "test_mapping_1";
            InputStream resourceStream = this.getClass().getResourceAsStream(srcFile);
            testMappingFile = FileUtil.saveResourceFile(resourceStream, tempDir, TEST_MAPPING);
            List<TestMapping.TestInfo> tests =
                    new TestMapping(testMappingFile.toPath()).getTests("presubmit");
            assertEquals(1, tests.size());
            assertEquals("test1", tests.get(0).getName());
            tests = new TestMapping(testMappingFile.toPath()).getTests("postsubmit");
            assertEquals(3, tests.size());
            assertEquals("test2", tests.get(0).getName());
            TestMapping.TestOption option = tests.get(0).getOptions().get(0);
            assertEquals("instrumentation-arg", option.getName());
            assertEquals(
                    "annotation=android.platform.test.annotations.Presubmit", option.getValue());
            assertEquals("instrument", tests.get(1).getName());
            tests = new TestMapping(testMappingFile.toPath()).getTests("othertype");
            assertEquals(1, tests.size());
            assertEquals("test3", tests.get(0).getName());
        } finally {
            FileUtil.recursiveDelete(tempDir);
        }
    }

    /** Test for {@link TestMapping#getTests()} throw exception for malformated json file. */
    @Test(expected = RuntimeException.class)
    public void testparseTestMapping_BadJson() throws Exception {
        File tempDir = null;

        try {
            tempDir = FileUtil.createTempDir("test_mapping");
            File testMappingFile = Paths.get(tempDir.getAbsolutePath(), TEST_MAPPING).toFile();
            FileUtil.writeToFile("bad format json file", testMappingFile);
            List<TestMapping.TestInfo> tests =
                    new TestMapping(testMappingFile.toPath()).getTests("presubmit");
        } finally {
            FileUtil.recursiveDelete(tempDir);
        }
    }

    /** Test for {@link TestMapping#getTests()} for loading tests from test_mappings.zip. */
    @Test
    public void testGetTests() throws Exception {
        File tempDir = null;
        try {
            tempDir = FileUtil.createTempDir("test_mapping");

            File srcDir = FileUtil.createTempDir("src", tempDir);
            String srcFile = File.separator + TEST_DATA_DIR + File.separator + "test_mapping_1";
            InputStream resourceStream = this.getClass().getResourceAsStream(srcFile);
            FileUtil.saveResourceFile(resourceStream, srcDir, TEST_MAPPING);
            File subDir = FileUtil.createTempDir("sub_dir", srcDir);
            srcFile = File.separator + TEST_DATA_DIR + File.separator + "test_mapping_2";
            resourceStream = this.getClass().getResourceAsStream(srcFile);
            FileUtil.saveResourceFile(resourceStream, subDir, TEST_MAPPING);

            File zipFile = Paths.get(tempDir.getAbsolutePath(), TEST_MAPPINGS_ZIP).toFile();
            ZipUtil.createZip(srcDir, zipFile);
            IBuildInfo mockBuildInfo = EasyMock.createMock(IBuildInfo.class);
            EasyMock.expect(mockBuildInfo.getFile(TEST_MAPPINGS_ZIP)).andReturn(zipFile);

            EasyMock.replay(mockBuildInfo);
            Set<TestMapping.TestInfo> tests = TestMapping.getTests(mockBuildInfo, "presubmit");

            assertEquals(2, tests.size());
            Set<String> names = new HashSet<String>();
            for (TestMapping.TestInfo test : tests) {
                names.add(test.getName());
            }
            assertTrue(names.contains("suite/stub1"));
            assertTrue(names.contains("test1"));
        } finally {
            FileUtil.recursiveDelete(tempDir);
        }
    }
}
