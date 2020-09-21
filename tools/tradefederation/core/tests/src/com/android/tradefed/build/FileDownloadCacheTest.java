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

import static org.junit.Assert.*;

import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;

import org.easymock.EasyMock;
import org.easymock.IAnswer;
import org.easymock.IExpectationSetters;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

/** Unit tests for {@link FileDownloadCache}. */
@RunWith(JUnit4.class)
public class FileDownloadCacheTest {

    private static final String REMOTE_PATH = "foo/path";
    private static final String DOWNLOADED_CONTENTS = "downloaded contents";

    private IFileDownloader mMockDownloader;

    private File mCacheDir;
    private FileDownloadCache mCache;
    private boolean mFailCopy = false;

    @Before
    public void setUp() throws Exception {
        mMockDownloader = EasyMock.createMock(IFileDownloader.class);
        mCacheDir = FileUtil.createTempDir("unittest");
        mCache = new FileDownloadCache(mCacheDir);
    }

    @After
    public void tearDown() throws Exception {
        mCache.empty();
        FileUtil.recursiveDelete(mCacheDir);
    }

    /** Test basic case for {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)}. */
    @Test
    public void testFetchRemoteFile() throws Exception {
        setDownloadExpections();
        EasyMock.replay(mMockDownloader);
        assertFetchRemoteFile();
        EasyMock.verify(mMockDownloader);
    }

    /**
     * Test {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)} when file can be
     * retrieved from cache.
     */
    @Test
    public void testFetchRemoteFile_cacheHit() throws Exception {
        setDownloadExpections();
        EasyMock.replay(mMockDownloader);
        assertFetchRemoteFile();
        // now retrieve file again
        assertFetchRemoteFile();
        // verify only one download call occurred
        EasyMock.verify(mMockDownloader);
    }

    /**
     * Test {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)} when cache grows
     * larger than max
     */
    @Test
    public void testFetchRemoteFile_cacheSizeExceeded() throws Exception {
        final String remotePath2 = "anotherpath";
        // set cache size to be small
        mCache.setMaxCacheSize(DOWNLOADED_CONTENTS.length() + 1);
        setDownloadExpections(remotePath2);
        setDownloadExpections();
        EasyMock.replay(mMockDownloader);
        assertFetchRemoteFile(remotePath2);
        // now retrieve another file, which will exceed size of cache
        assertFetchRemoteFile();
        assertNotNull(mCache.getCachedFile(REMOTE_PATH));
        assertNull(mCache.getCachedFile(remotePath2));
        EasyMock.verify(mMockDownloader);
    }

    /**
     * Test {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)} when download fails
     */
    @Test
    public void testFetchRemoteFile_downloadFailed() throws Exception {
        mMockDownloader.downloadFile(EasyMock.eq(REMOTE_PATH),
                (File)EasyMock.anyObject());
        EasyMock.expectLastCall().andThrow(new BuildRetrievalError("download error"));
        EasyMock.replay(mMockDownloader);
        try {
            mCache.fetchRemoteFile(mMockDownloader, REMOTE_PATH);
            fail("BuildRetrievalError not thrown");
        } catch (BuildRetrievalError e) {
            // expected
        }
        assertNull(mCache.getCachedFile(REMOTE_PATH));
        EasyMock.verify(mMockDownloader);
    }

    /**
     * Test {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)} when download fails
     * with RuntimeException.
     */
    @Test
    public void testFetchRemoteFile_downloadFailed_Runtime() throws Exception {
        mMockDownloader.downloadFile(EasyMock.eq(REMOTE_PATH), (File) EasyMock.anyObject());
        EasyMock.expectLastCall().andThrow(new RuntimeException("download error"));
        EasyMock.replay(mMockDownloader);
        try {
            mCache.fetchRemoteFile(mMockDownloader, REMOTE_PATH);
            fail("RuntimeException not thrown");
        } catch (RuntimeException e) {
            // expected
        }
        assertNull(mCache.getCachedFile(REMOTE_PATH));
        EasyMock.verify(mMockDownloader);
    }

    /**
     * Test {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)} when copy of a cached
     * file is missing
     */
    @Test
    public void testFetchRemoteFile_cacheMissing() throws Exception {
        // perform successful download
        setDownloadExpections(REMOTE_PATH).times(2);
        EasyMock.replay(mMockDownloader);
        assertFetchRemoteFile(REMOTE_PATH);
        // now be sneaky and delete the cachedFile, so copy will fail
        File cachedFile = mCache.getCachedFile(REMOTE_PATH);
        assertNotNull(cachedFile);
        cachedFile.delete();
        File file = null;
        try {
            file = mCache.fetchRemoteFile(mMockDownloader, REMOTE_PATH);
            // file should have been updated in cache.
            assertNotNull(file);
            assertNotNull(mCache.getCachedFile(REMOTE_PATH));
            EasyMock.verify(mMockDownloader);
        } finally {
            FileUtil.deleteFile(file);
        }
    }

    /**
     * Test {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)} when copy of a cached
     * file fails
     */
    @Test
    public void testFetchRemoteFile_copyFailed() throws Exception {
        mCache =
                new FileDownloadCache(mCacheDir) {
                    @Override
                    File copyFile(String remotePath, File cachedFile) throws BuildRetrievalError {
                        if (mFailCopy) {
                            FileUtil.deleteFile(cachedFile);
                        }
                        return super.copyFile(remotePath, cachedFile);
                    }
                };
        // perform successful download
        setDownloadExpections(REMOTE_PATH);
        EasyMock.replay(mMockDownloader);
        assertFetchRemoteFile(REMOTE_PATH);
        mFailCopy = true;
        try {
            mCache.fetchRemoteFile(mMockDownloader, REMOTE_PATH);
            fail("BuildRetrievalError not thrown");
        } catch (BuildRetrievalError e) {
            // expected
        }
        // file should be removed from cache
        assertNull(mCache.getCachedFile(REMOTE_PATH));
        EasyMock.verify(mMockDownloader);
    }

    /**
     * Perform one fetchRemoteFile call and verify contents for default remote path
     */
    private void assertFetchRemoteFile() throws BuildRetrievalError, IOException {
        assertFetchRemoteFile(REMOTE_PATH);
    }

    /**
     * Perform one fetchRemoteFile call and verify contents
     */
    private void assertFetchRemoteFile(String remotePath) throws BuildRetrievalError, IOException {
        // test downloading file not in cache
        File fileCopy = mCache.fetchRemoteFile(mMockDownloader, remotePath);
        try {
            assertNotNull(mCache.getCachedFile(remotePath));
            String contents = StreamUtil.getStringFromStream(new FileInputStream(fileCopy));
            assertEquals(DOWNLOADED_CONTENTS, contents);
        } finally {
            fileCopy.delete();
        }
    }

    /**
     * Set EasyMock expectations for a downloadFile call for default remote path
     */
    private void setDownloadExpections() throws BuildRetrievalError {
        setDownloadExpections(REMOTE_PATH);
    }

    /** Set EasyMock expectations for a downloadFile call */
    private IExpectationSetters<Object> setDownloadExpections(String remotePath)
            throws BuildRetrievalError {
        IAnswer<Object> downloadAnswer = new IAnswer<Object>() {
            @Override
            public Object answer() throws Throwable {
                File fileArg =  (File) EasyMock.getCurrentArguments()[1];
                FileUtil.writeToFile("downloaded contents", fileArg);
                return null;
            }
        };
        mMockDownloader.downloadFile(EasyMock.eq(remotePath),
                EasyMock.<File>anyObject());
        return EasyMock.expectLastCall().andAnswer(downloadAnswer);
    }
}
