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

import com.android.ddmlib.Log;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.easymock.IAnswer;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Longer running, concurrency based tests for {@link FileDownloadCache}.
 */
public class FileDownloadCacheFuncTest extends TestCase {

    private static final String REMOTE_PATH = "path";
    private static final String DOWNLOADED_CONTENTS = "downloaded contents";
    protected static final String LOG_TAG = "FileDownloadCacheFuncTest";

    private IFileDownloader mMockDownloader;

    private FileDownloadCache mCache;
    private File mTmpDir;
    private List<File> mReturnedFiles;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockDownloader = EasyMock.createStrictMock(IFileDownloader.class);
        mTmpDir = FileUtil.createTempDir("functest");
        mCache = new FileDownloadCache(mTmpDir);
        mReturnedFiles = new ArrayList<File>(2);
    }

    @Override
    protected void tearDown() throws Exception {
        for (File file : mReturnedFiles) {
            file.delete();
        }
        FileUtil.recursiveDelete(mTmpDir);
        super.tearDown();
    }

    /**
     * Test {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)} being called
     * concurrently by two separate threads.
     */
    @SuppressWarnings("unchecked")
    public void testFetchRemoteFile_concurrent() throws Exception {
        // Simulate a relatively slow file download
        IAnswer<Object> slowDownloadAnswer = new IAnswer<Object>() {
            @Override
            public Object answer() throws Throwable {
                Thread.sleep(500);
                File fileArg =  (File) EasyMock.getCurrentArguments()[1];
                FileUtil.writeToFile(DOWNLOADED_CONTENTS, fileArg);
                return null;
            }
        };
        // Download is only called once, second thread will wait on synchronized until the download
        // is done, then link the downloaded file.
        mMockDownloader.downloadFile(EasyMock.eq(REMOTE_PATH), EasyMock.<File>anyObject());
        EasyMock.expectLastCall().andAnswer(slowDownloadAnswer);
        EasyMock.replay(mMockDownloader);
        Thread downloadThread1 = createDownloadThread(mMockDownloader, REMOTE_PATH);
        downloadThread1.setName("FileDownloadCacheFuncTest#testFetchRemoteFile_concurrent-1");
        Thread downloadThread2 = createDownloadThread(mMockDownloader, REMOTE_PATH);
        downloadThread2.setName("FileDownloadCacheFuncTest#testFetchRemoteFile_concurrent-2");
        downloadThread1.start();
        downloadThread2.start();
        downloadThread1.join();
        downloadThread2.join();
        assertNotNull(mCache.getCachedFile(REMOTE_PATH));
        assertEquals(2, mReturnedFiles.size());
        // returned files should be identical in content, but be different files
        assertTrue(!mReturnedFiles.get(0).equals(mReturnedFiles.get(1)));
        assertEquals(DOWNLOADED_CONTENTS, StreamUtil.getStringFromStream(new FileInputStream(
                mReturnedFiles.get(0))));
        assertEquals(DOWNLOADED_CONTENTS, StreamUtil.getStringFromStream(new FileInputStream(
                mReturnedFiles.get(1))));
        EasyMock.verify(mMockDownloader);
    }

    /**
     * Test {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)} being called
     * concurrently by multiple threads trying to download different files.
     */
    public void testFetchRemoteFile_multiConcurrent() throws Exception {
        IFileDownloader mockDownloader1 = Mockito.mock(IFileDownloader.class);
        IFileDownloader mockDownloader2 = Mockito.mock(IFileDownloader.class);
        IFileDownloader mockDownloader3 = Mockito.mock(IFileDownloader.class);
        String remotePath1 = "path1";
        String remotePath2 = "path2";
        String remotePath3 = "path3";

        // Block first download, but allow other downloads to pass.
        final AtomicBoolean startedDownload = new AtomicBoolean(false);
        final AtomicBoolean blockDownload = new AtomicBoolean(true);
        mCache.setMaxCacheSize(DOWNLOADED_CONTENTS.length() + 1);
        Answer<Void> blockedAnswer =
                new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        if (!startedDownload.get()) {
                            startedDownload.set(true);
                            while (blockDownload.get()) {
                                RunUtil.getDefault().sleep(10);
                            }
                        }
                        File fileArg = (File) invocation.getArguments()[1];
                        FileUtil.writeToFile(DOWNLOADED_CONTENTS, fileArg);
                        return null;
                    }
                };

        // Download is called once per files since they are different files.
        Mockito.doAnswer(blockedAnswer)
                .when(mockDownloader1)
                .downloadFile(Mockito.eq(remotePath1), Mockito.any());
        Mockito.doAnswer(blockedAnswer)
                .when(mockDownloader2)
                .downloadFile(Mockito.eq(remotePath2), Mockito.any());
        Mockito.doAnswer(blockedAnswer)
                .when(mockDownloader3)
                .downloadFile(Mockito.eq(remotePath3), Mockito.any());

        Thread downloadThread1 = createDownloadThread(mockDownloader1, remotePath1);
        downloadThread1.setName("FileDownloadCacheFuncTest#testFetchRemoteFile_multiConcurrent-1");
        Thread downloadThread2 = createDownloadThread(mockDownloader2, remotePath2);
        downloadThread2.setName("FileDownloadCacheFuncTest#testFetchRemoteFile_multiConcurrent-2");
        Thread downloadThread3 = createDownloadThread(mockDownloader3, remotePath3);
        downloadThread3.setName("FileDownloadCacheFuncTest#testFetchRemoteFile_multiConcurrent-3");

        // Start first thread, and wait for download to begin
        downloadThread1.start();
        while (!startedDownload.get()) {
            RunUtil.getDefault().sleep(10);
        }

        // Start the other threads, which should run to completion. The cache should be adjusted,
        // but the file in the first thread should not be deleted since it is still being
        // downloaded.
        downloadThread2.start();
        downloadThread3.start();
        downloadThread2.join(2000);
        downloadThread3.join(2000);
        assertFalse(downloadThread2.isAlive());
        assertFalse(downloadThread3.isAlive());
        assertNotNull(mCache.getCachedFile(remotePath1));

        // Complete download of first thread, and let the thread run to completion. What files are
        // left in the cache can depend on implementation, so we're not testing for it.
        blockDownload.set(false);
        downloadThread1.join(2000);
        assertFalse(downloadThread1.isAlive());

        Mockito.verify(mockDownloader1).downloadFile(Mockito.eq(remotePath1), Mockito.any());
        Mockito.verify(mockDownloader2).downloadFile(Mockito.eq(remotePath2), Mockito.any());
        Mockito.verify(mockDownloader3).downloadFile(Mockito.eq(remotePath3), Mockito.any());
    }

    /**
     * Test {@link FileDownloadCache#fetchRemoteFile(IFileDownloader, String)} being called
     * concurrently by multiple threads trying to download the same file, with one thread failing.
     */
    @SuppressWarnings("unchecked")
    public void testFetchRemoteFile_concurrentFail() throws Exception {
        // Block first download, and later raise an error, but allow other downloads to pass.
        final AtomicBoolean startedDownload = new AtomicBoolean(false);
        final AtomicBoolean throwException = new AtomicBoolean(false);
        IAnswer<Object> blockedDownloadAnswer =
                new IAnswer<Object>() {
                    @Override
                    public Object answer() throws Throwable {
                        if (!startedDownload.get()) {
                            startedDownload.set(true);
                            while (!throwException.get()) {
                                Thread.sleep(10);
                            }
                            throw new BuildRetrievalError("download error");
                        }
                        File fileArg = (File) EasyMock.getCurrentArguments()[1];
                        FileUtil.writeToFile(DOWNLOADED_CONTENTS, fileArg);
                        return null;
                    }
                };

        // Download should be called twice. The first call will result in an error, and the second
        // will run to completion.
        mMockDownloader.downloadFile(EasyMock.eq(REMOTE_PATH), EasyMock.<File>anyObject());
        EasyMock.expectLastCall().andAnswer(blockedDownloadAnswer).times(2);

        // Disable thread safety, otherwise the first call will block the rest.
        EasyMock.makeThreadSafe(mMockDownloader, false);
        EasyMock.replay(mMockDownloader);

        Thread downloadThread1 = createDownloadThread(mMockDownloader, REMOTE_PATH);
        downloadThread1.setName("FileDownloadCacheFuncTest#testFetchRemoteFile_concurrentFail-1");
        Thread downloadThread2 = createDownloadThread(mMockDownloader, REMOTE_PATH);
        downloadThread2.setName("FileDownloadCacheFuncTest#testFetchRemoteFile_concurrentFail-2");
        Thread downloadThread3 = createDownloadThread(mMockDownloader, REMOTE_PATH);
        downloadThread3.setName("FileDownloadCacheFuncTest#testFetchRemoteFile_concurrentFail-3");

        // Start first thread, and wait for download to begin
        downloadThread1.start();
        while (!startedDownload.get()) {
            Thread.sleep(10);
        }

        // Start second thread, and allow it to attempt to obtain the file lock.
        downloadThread2.start();
        Thread.sleep(100);

        // Throw a BuildRetrievalError, and allow both threads to run to completion.
        throwException.set(true);
        downloadThread1.join(2000);
        downloadThread2.join(2000);
        assertFalse(downloadThread1.isAlive());
        assertFalse(downloadThread2.isAlive());
        assertNotNull(mCache.getCachedFile(REMOTE_PATH));

        // A third attempt to retrive the file should not result in another download call.
        downloadThread3.start();
        downloadThread3.join(2000);

        EasyMock.verify(mMockDownloader);
    }

    /** Verify the cache is built from disk contents on creation */
    public void testConstructor_createCache() throws Exception {
        // create cache contents on disk
        File cacheRoot = FileUtil.createTempDir("constructorTest");
        try {
            final String filecontents = "these are the file contents";
            File file1 = new File(cacheRoot, REMOTE_PATH);
            FileUtil.writeToFile(filecontents, file1);
            // this is lame, but sleep for a small amount to ensure nestedFile has later timestamp
            // TODO: use mock File instead
            Thread.sleep(1000);
            File nestedDir = new File(cacheRoot, "aa");
            nestedDir.mkdir();
            File nestedFile = new File(nestedDir, "anotherpath");
            FileUtil.writeToFile(filecontents, nestedFile);

            FileDownloadCache cache = new FileDownloadCache(cacheRoot);
            assertNotNull(cache.getCachedFile(REMOTE_PATH));
            assertNotNull(cache.getCachedFile("aa/anotherpath"));
            assertEquals(REMOTE_PATH, cache.getOldestEntry());
        } finally {
            FileUtil.recursiveDelete(cacheRoot);
        }
    }

    /**
     * Test scenario where an already too large cache is built from disk contents.
     */
    public void testConstructor_cacheExceeded() throws Exception {
        File cacheRoot = FileUtil.createTempDir("testConstructor_cacheExceeded");
        try {
            // create a couple existing files in cache
            final String filecontents = "these are the file contents";
            final File file1 = new File(cacheRoot, REMOTE_PATH);
            FileUtil.writeToFile(filecontents, file1);
            // sleep for a small amount to ensure file2 has later timestamp
            // TODO: use mock File instead
            Thread.sleep(1000);
            final File file2 = new File(cacheRoot, "anotherpath");
            FileUtil.writeToFile(filecontents, file2);

            new FileDownloadCache(cacheRoot) {
                @Override
                long getMaxFileCacheSize() {
                    return file2.length() + 1;
                }
            };
            // expect cache to be cleaned on startup, with oldest file1 deleted, but newest file
            // retained
            assertFalse(file1.exists());
            assertTrue(file2.exists());

        } finally {
            FileUtil.recursiveDelete(cacheRoot);
        }
    }

    /** Utility method to create thread that calls fetchRemoteFile. */
    private Thread createDownloadThread(IFileDownloader downloader, String remotePath) {
        return new Thread() {
            @Override
            public void run() {
                try {
                    mReturnedFiles.add(mCache.fetchRemoteFile(downloader, remotePath));
                } catch (BuildRetrievalError e) {
                    Log.e(LOG_TAG, e);
                }
            }
        };
    }
}
