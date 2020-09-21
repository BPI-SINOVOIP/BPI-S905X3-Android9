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
import com.android.tradefed.command.FatalHostError;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.io.IOException;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Stack;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;

/**
 * A helper class that maintains a local filesystem LRU cache of downloaded files.
 */
public class FileDownloadCache {

    private static final String LOG_TAG = "FileDownloadCache";

    private static final char REL_PATH_SEPARATOR = '/';

    /** fixed location of download cache. */
    private final File mCacheRoot;

    /**
     * The map of remote file paths to local files, stored in least-recently-used order.
     * <p/>
     * Used for performance reasons. Functionally speaking, this data structure is not needed,
     * since all info could be obtained from inspecting the filesystem.
     */
    private final Map<String, File> mCacheMap = new LinkedHashMap<String, File>();

    /** the lock for <var>mCacheMap</var> */
    private final ReentrantLock mCacheMapLock = new ReentrantLock();

    /** A map of remote file paths to locks. */
    private final Map<String, ReentrantLock> mFileLocks = new HashMap<String, ReentrantLock>();

    private long mCurrentCacheSize = 0;

    /** The approximate maximum allowed size of the local file cache. Default to 20 gig */
    private long mMaxFileCacheSize = 20L * 1024L * 1024L * 1024L;

    /**
     * Struct for a {@link File} and its remote relative path
     */
    private static class FilePair {
        final String mRelPath;
        final File mFile;

        FilePair(String relPath, File file) {
            mRelPath = relPath;
            mFile = file;
        }
    }

    /**
     * A {@link Comparator} for comparing {@link File}s based on {@link File#lastModified()}.
     */
    private static class FileTimeComparator implements Comparator<FilePair> {
        @Override
        public int compare(FilePair o1, FilePair o2) {
            Long timestamp1 = Long.valueOf(o1.mFile.lastModified());
            Long timestamp2 = o2.mFile.lastModified();
            return timestamp1.compareTo(timestamp2);
        }
    }

    /**
     * Create a {@link FileDownloadCache}, deleting any previous cache contents from disk.
     * <p/>
     * Assumes that the current process has exclusive access to the <var>cacheRoot</var> directory.
     * <p/>
     * Essentially, the LRU cache is a mirror of a given remote file path hierarchy.
     */
    FileDownloadCache(File cacheRoot) {
        mCacheRoot = cacheRoot;
        if (!mCacheRoot.exists()) {
            Log.d(LOG_TAG, String.format("Creating file cache at %s",
                    mCacheRoot.getAbsolutePath()));
            if (!mCacheRoot.mkdirs()) {
                throw new FatalHostError(String.format("Could not create cache directory at %s",
                        mCacheRoot.getAbsolutePath()));
            }
        } else {
            Log.d(LOG_TAG, String.format("Building file cache from contents at %s",
                    mCacheRoot.getAbsolutePath()));
            // create an unsorted list of all the files in mCacheRoot. Need to create list first
            // rather than inserting in Map directly because Maps cannot be sorted
            List<FilePair> cacheEntryList = new LinkedList<FilePair>();
            addFiles(mCacheRoot, new Stack<String>(), cacheEntryList);
            // now sort them based on file timestamp, to get them in LRU order
            Collections.sort(cacheEntryList, new FileTimeComparator());
            // now insert them into the map
            for (FilePair cacheEntry : cacheEntryList) {
                mCacheMap.put(cacheEntry.mRelPath, cacheEntry.mFile);
                mCurrentCacheSize += cacheEntry.mFile.length();
            }
            // this would be an unusual situation, but check if current cache is already too big
            if (mCurrentCacheSize > getMaxFileCacheSize()) {
                incrementAndAdjustCache(0);
            }
        }
    }

    /**
     * Recursive method for adding a directory's contents to the cache map
     * <p/>
     * cacheEntryList will contain results of all files found in cache, in no guaranteed order.
     *
     * @param dir the parent directory to search
     * @param relPathSegments the current filesystem path of <var>dir</var>, relative to
     *            <var>mCacheRoot</var>
     * @param cacheEntryList the list of files discovered
     */
    private void addFiles(File dir, Stack<String> relPathSegments,
            List<FilePair> cacheEntryList) {

        File[] fileList = dir.listFiles();
        if (fileList == null) {
            CLog.e("Unable to list files in cache dir %s", dir.getAbsolutePath());
            return;
        }
        for (File childFile : fileList) {
            if (childFile.isDirectory()) {
                relPathSegments.push(childFile.getName());
                addFiles(childFile, relPathSegments, cacheEntryList);
                relPathSegments.pop();
            } else if (childFile.isFile()) {
                StringBuffer relPath = new StringBuffer();
                for (String pathSeg : relPathSegments) {
                    relPath.append(pathSeg);
                    relPath.append(REL_PATH_SEPARATOR);
                }
                relPath.append(childFile.getName());
                cacheEntryList.add(new FilePair(relPath.toString(), childFile));
            } else {
                Log.w(LOG_TAG, String.format("Unrecognized file type %s in cache",
                        childFile.getAbsolutePath()));
            }
        }
    }

    /** Acquires the lock for a file. */
    protected void lockFile(String remoteFilePath) {
        ReentrantLock fileLock;

        synchronized (mFileLocks) {
            fileLock = mFileLocks.get(remoteFilePath);
            if (fileLock == null) {
                fileLock = new ReentrantLock();
                mFileLocks.put(remoteFilePath, fileLock);
            }
        }
        fileLock.lock();
    }

    /**
     * Acquire the lock for a file only if it is not held by another thread.
     *
     * @return true if the lock was acquired, and false otherwise.
     */
    protected boolean tryLockFile(String remoteFilePath) {
        synchronized (mFileLocks) {
            ReentrantLock fileLock = mFileLocks.get(remoteFilePath);
            if (fileLock == null) {
                fileLock = new ReentrantLock();
                mFileLocks.put(remoteFilePath, fileLock);
            }
            try {
                return fileLock.tryLock(0, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                return false;
            }
        }
    }

    /** Attempt to release a lock for a file. */
    protected void unlockFile(String remoteFilePath) {
        synchronized (mFileLocks) {
            ReentrantLock fileLock = mFileLocks.get(remoteFilePath);
            if (fileLock != null) {
                if (!fileLock.hasQueuedThreads()) {
                    mFileLocks.remove(remoteFilePath);
                }
                fileLock.unlock();
            }
        }
    }

    /**
     * Set the maximum size of the local file cache.
     *
     * <p>Cache will not be adjusted immediately if set to a smaller size than current, but will
     * take effect on next file download.
     *
     * @param numBytes
     */
    public void setMaxCacheSize(long numBytes) {
        // for simplicity, get global lock
        mCacheMapLock.lock();
        mMaxFileCacheSize = numBytes;
        mCacheMapLock.unlock();
    }

    /**
     * Returns a local file corresponding to the given <var>remotePath</var>
     * <p/>
     * The local {@link File} will be copied from the cache if it exists, otherwise will be
     * downloaded via the given {@link IFileDownloader}.
     *
     * @param downloader the {@link IFileDownloader}
     * @param remotePath the remote file.
     * @return a local {@link File} containing contents of remotePath
     * @throws BuildRetrievalError if file could not be retrieved
     */
    public File fetchRemoteFile(IFileDownloader downloader, String remotePath)
            throws BuildRetrievalError {
        boolean download = false;
        File cachedFile, copyFile;

        lockFile(remotePath);
        try {
            mCacheMapLock.lock();
            try {
                cachedFile = mCacheMap.remove(remotePath);
                if (cachedFile == null) {
                    download = true;
                    String localRelativePath = convertPath(remotePath);
                    cachedFile = new File(mCacheRoot, localRelativePath);
                }
                mCacheMap.put(remotePath, cachedFile);
            } finally {
                mCacheMapLock.unlock();
            }

            try {
                if (download || !cachedFile.exists()) {
                    cachedFile.getParentFile().mkdirs();
                    downloadFile(downloader, remotePath, cachedFile);
                } else {
                    Log.d(
                            LOG_TAG,
                            String.format(
                                    "Retrieved remote file %s from cached file %s",
                                    remotePath, cachedFile.getAbsolutePath()));
                }
                copyFile = copyFile(remotePath, cachedFile);
            } catch (BuildRetrievalError | RuntimeException e) {
                // cached file is likely incomplete, delete it.
                deleteCacheEntry(remotePath);
                throw e;
            }

            // Only the thread that first downloads the file should increment the cache.
            if (download) {
               incrementAndAdjustCache(cachedFile.length());
            }
        } finally {
            unlockFile(remotePath);
        }
        return copyFile;
    }

    /** Do the actual file download, clean up on exception is done by the caller. */
    private void downloadFile(IFileDownloader downloader, String remotePath, File cachedFile)
            throws BuildRetrievalError {
        Log.d(LOG_TAG, String.format("Downloading %s to cache", remotePath));
        downloader.downloadFile(remotePath, cachedFile);
    }

    @VisibleForTesting
    File copyFile(String remotePath, File cachedFile) throws BuildRetrievalError {
        // attempt to create a local copy of cached file with sane name
        File hardlinkFile = null;
        try {
            hardlinkFile = FileUtil.createTempFileForRemote(remotePath, null);
            hardlinkFile.delete();
            CLog.d("Creating hardlink '%s' to '%s'", hardlinkFile.getAbsolutePath(),
                    cachedFile.getAbsolutePath());
            FileUtil.hardlinkFile(cachedFile, hardlinkFile);
            return hardlinkFile;
        } catch (IOException e) {
            FileUtil.deleteFile(hardlinkFile);
            // cached file might be corrupt or incomplete, delete it
            FileUtil.deleteFile(cachedFile);
            throw new BuildRetrievalError(String.format("Failed to copy cached file %s",
                    cachedFile), e);
        }
    }

    /**
     * Convert remote relative path into an equivalent local path
     * @param remotePath
     * @return the local relative path
     */
    private String convertPath(String remotePath) {
        if (FileDownloadCache.REL_PATH_SEPARATOR != File.separatorChar) {
            return remotePath.replace(FileDownloadCache.REL_PATH_SEPARATOR , File.separatorChar);
        } else {
            // no conversion necessary
            return remotePath;
        }
    }

    /**
     * Adjust file cache size to mMaxFileCacheSize if necessary by deleting old files
     */
    private void incrementAndAdjustCache(long length) {
        mCacheMapLock.lock();
        try {
            mCurrentCacheSize += length;
            Iterator<String> keyIterator = mCacheMap.keySet().iterator();
            while (mCurrentCacheSize > getMaxFileCacheSize() && keyIterator.hasNext()) {
                String remotePath = keyIterator.next();
                // Only delete the file if it is not being used by another thread.
                if (tryLockFile(remotePath)) {
                    try {
                        File file = mCacheMap.get(remotePath);
                        mCurrentCacheSize -= file.length();
                        file.delete();
                        keyIterator.remove();
                    } finally {
                        unlockFile(remotePath);
                    }
                } else {
                    CLog.i(
                            String.format(
                                    "File %s is being used by another invocation. Skipping.",
                                    remotePath));
                }
            }
            // audit cache size
            if (mCurrentCacheSize < 0) {
                // should never happen
                Log.e(LOG_TAG, "Cache size is less than 0!");
                // TODO: throw fatal error?
            } else if (mCurrentCacheSize > getMaxFileCacheSize()) {
                // May occur if the cache is configured to be too small or if mCurrentCacheSize is
                // accounting for non-existent files.
                Log.w(LOG_TAG, "File cache is over-capacity.");
            }
        } finally {
            mCacheMapLock.unlock();
        }
    }

    /**
     * Returns the cached file for given remote path, or <code>null</code> if no cached file exists.
     * <p/>
     * Exposed for unit testing
     *
     * @param remoteFilePath the remote file path
     * @return the cached {@link File} or <code>null</code>
     */
     File getCachedFile(String remoteFilePath) {
        mCacheMapLock.lock();
        try {
            return mCacheMap.get(remoteFilePath);
        } finally {
            mCacheMapLock.unlock();
        }
     }

    /**
     * Empty the cache, deleting all files.
     * <p/>
     * exposed for unit testing
     */
     void empty() {
        long currentMax = getMaxFileCacheSize();
        // reuse incrementAndAdjustCache to clear cache, by setting cache cap to 0
        setMaxCacheSize(0L);
        incrementAndAdjustCache(0);
        setMaxCacheSize(currentMax);
    }

    /**
     * Retrieve the oldest remotePath from cache.
     * <p/>
     * Exposed for unit testing
     *
     * @return the remote path or <code>null</null> if cache is empty
     */
    String getOldestEntry() {
        mCacheMapLock.lock();
        try {
            if (!mCacheMap.isEmpty()) {
                return mCacheMap.keySet().iterator().next();
            } else {
                return null;
            }
        } finally {
            mCacheMapLock.unlock();
        }
    }

    /**
     * Get the current max size of file cache.
     * <p/>
     * exposed for unit testing.
     *
     * @return the mMaxFileCacheSize
     */
    long getMaxFileCacheSize() {
        return mMaxFileCacheSize;
    }

    /**
     * Allow deleting an entry from the cache. In case the entry is invalid or corrupted.
     */
    public void deleteCacheEntry(String remoteFilePath) {
        lockFile(remoteFilePath);
        try {
            mCacheMapLock.lock();
            try {
                File file = mCacheMap.remove(remoteFilePath);
                if (file != null) {
                    FileUtil.recursiveDelete(file);
                } else {
                    CLog.i("No cache entry to delete for %s", remoteFilePath);
                }
            } finally {
                mCacheMapLock.unlock();
            }
        } finally {
            unlockFile(remoteFilePath);
        }
    }
}
