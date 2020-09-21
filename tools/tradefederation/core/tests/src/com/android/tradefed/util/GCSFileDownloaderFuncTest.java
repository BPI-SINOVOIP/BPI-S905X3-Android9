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

import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

/** {@link GCSFileDownloader} functional test. */
@RunWith(JUnit4.class)
public class GCSFileDownloaderFuncTest {

    private static final String GSUTIL = "gsutil";
    private static final String BUCKET_NAME_PREFIX = "tradefed_function_test";
    private static final String FILE_NAME = "a_host_config.xml";
    private static final String FILE_CONTENT = "Hello World!";
    private static final String PROJECT_ID = "google.com:tradefed-cluster-staging";
    private static final long TIMEOUT = 10000;
    private static String sBucketName;
    private static String sBucketUrl;

    private GCSFileDownloader mDownloader;

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        File tempFile = FileUtil.createTempFile(BUCKET_NAME_PREFIX, "");
        sBucketName = tempFile.getName();
        FileUtil.deleteFile(tempFile);
        sBucketUrl = "gs://" + sBucketName;
        CommandResult cr =
                RunUtil.getDefault()
                        .runTimedCmd(TIMEOUT, GSUTIL, "mb", "-p", PROJECT_ID, sBucketUrl);
        Assert.assertEquals(
                String.format(
                        "Filed to create bucket %s %s: %s",
                        PROJECT_ID, sBucketName, cr.getStderr()),
                CommandStatus.SUCCESS,
                cr.getStatus());
        cr =
                RunUtil.getDefault()
                        .runTimedCmdWithInput(
                                TIMEOUT,
                                FILE_CONTENT,
                                GSUTIL,
                                "cp",
                                "-",
                                String.format("gs://%s/%s", sBucketName, FILE_NAME));
        Assert.assertEquals(
                String.format(
                        "Filed to create file %s %s: %s", sBucketName, FILE_NAME, cr.getStderr()),
                CommandStatus.SUCCESS,
                cr.getStatus());
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        CommandResult cr =
                RunUtil.getDefault()
                        .runTimedCmd(
                                TIMEOUT, GSUTIL, "rm", String.format("gs://%s/*", sBucketName));
        Assert.assertEquals(
                String.format("Filed to clear bucket %s: %s", sBucketName, cr.getStderr()),
                CommandStatus.SUCCESS,
                cr.getStatus());
        cr = RunUtil.getDefault().runTimedCmd(TIMEOUT, GSUTIL, "rb", "-f", sBucketUrl);
        Assert.assertEquals(
                String.format("Filed to delete bucket %s: %s", sBucketName, cr.getStderr()),
                CommandStatus.SUCCESS,
                cr.getStatus());
    }

    @Before
    public void setUp() {
        mDownloader = new GCSFileDownloader();
    }

    @Test
    public void testDownloadFile() throws Exception {
        InputStream inputStream = mDownloader.downloadFile(sBucketName, FILE_NAME);
        String content = StreamUtil.getStringFromStream(inputStream);
        Assert.assertEquals(FILE_CONTENT, content);
    }

    @Test
    public void testDownloadFile_notExist() throws Exception {
        try {
            mDownloader.downloadFile(sBucketName, "non_exist_file");
            Assert.fail("Should throw IOExcepiton.");
        } catch (IOException e) {
            // Expect IOException
        }
    }
}
