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

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.List;

/** {@link GCSBucketUtil} functional test. */
@RunWith(JUnit4.class)
public class GCSBucketUtilFuncTest {

    private static final String BUCKET_NAME_PREFIX = "tradefed_function_test";
    private static final String FILE_NAME = "a_host_config.xml";
    private static final String FILE_CONTENT = "Hello World!";
    private static final String PROJECT_ID = "google.com:tradefed-cluster-staging";
    private static final long TIMEOUT = 10000;
    private static GCSBucketUtil sBucket;

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        File tempFile = FileUtil.createTempFile(BUCKET_NAME_PREFIX, "");

        try {
            sBucket = new GCSBucketUtil(tempFile.getName());
        } finally {
            FileUtil.deleteFile(tempFile);
        }

        sBucket.makeBucket(PROJECT_ID);
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        sBucket.remove("/", true);
    }

    @Before
    public void setUp() throws IOException {
        sBucket.setBotoConfig(null);
        sBucket.setBotoPath(null);
        sBucket.setRecursive(false);
        sBucket.setTimeoutMs(TIMEOUT);
    }

    @After
    public void tearDown() throws Exception {
        sBucket.setRecursive(true);
        sBucket.remove("/*", true);
    }

    @Test
    public void testStringUpload() throws Exception {
        Path path = Paths.get(FILE_NAME);
        sBucket.pushString(FILE_CONTENT, path);
        Assert.assertEquals(FILE_CONTENT, sBucket.pullContents(path));
    }

    @Test
    public void testStringUploadThenDownLoad() throws Exception {
        Path path = Paths.get(FILE_NAME);
        sBucket.pushString(FILE_CONTENT, path);
        Assert.assertEquals(FILE_CONTENT, sBucket.pullContents(path));
    }

    @Test
    public void testDownloadMultiple() throws Exception {
        List<String> expectedFiles = Arrays.asList("A", "B", "C");
        File tmpDir = FileUtil.createTempDir(BUCKET_NAME_PREFIX);

        for(String file : expectedFiles) {
            sBucket.pushString(FILE_CONTENT, Paths.get(file));
        }

        sBucket.setRecursive(true);
        sBucket.pull(Paths.get("/"), tmpDir);

        File tmpDirBucket = new File(tmpDir, sBucket.getBucketName());

        List<String> actualFiles = Arrays.asList(tmpDirBucket.list());
        for(String expected : expectedFiles) {
            if(actualFiles.indexOf(expected) == -1) {
                Assert.fail(String.format("Could not find file %s in %s [have: %s]", expected,
                        tmpDirBucket, actualFiles));
            }
        }

        FileUtil.recursiveDelete(tmpDir);
    }

    @Test
    public void TestUploadDownload() throws IOException {
        File tempSrc = FileUtil.createTempFile(sBucket.getBucketName(), "src");
        File tempDst = FileUtil.createTempFile(sBucket.getBucketName(), "dst");

        try {
            FileUtil.writeToFile(FILE_CONTENT, tempDst);

            sBucket.push(tempSrc, Paths.get(FILE_NAME));
            sBucket.pull(Paths.get(FILE_NAME), tempDst);

            Assert.assertTrue("File contents should match",
                    FileUtil.compareFileContents(tempSrc, tempDst));
        } finally {
            FileUtil.deleteFile(tempSrc);
            FileUtil.deleteFile(tempDst);
        }
    }

    @Test
    public void testDownloadFile_notExist() throws Exception {
        try {
            sBucket.pullContents(Paths.get("non_exist_file"));
            Assert.fail("Should throw IOExcepiton.");
        } catch (IOException e) {
            // Expect IOException
        }
    }

    @Test
    public void testRemoveFile_notExist() throws Exception {
        try {
            sBucket.remove("non_exist_file");
            Assert.fail("Should throw IOExcepiton.");
        } catch (IOException e) {
            // Expect IOException
        }
    }

    @Test
    public void testRemoveFile_notExist_force() throws Exception {
        sBucket.remove("non_exist_file", true);
    }

    @Test
    public void testBotoPathCanBeSet() throws Exception {
        sBucket.setBotoPath("/dev/null");
        sBucket.setBotoConfig("/dev/null");
        try {
            testStringUpload();
            Assert.fail("Should throw IOExcepiton.");
        } catch (IOException e) {
            // expected
        }
    }

}
