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

import java.io.File;

/**
 * A wrapper class that provides {@link FileDownloadCache} facilities while implementing the
 * {@link IFileDownloader} interface.
 * <p/>
 * Useful for cases where you want to abstract the use of the cache from callers.
 */
public class FileDownloadCacheWrapper implements IFileDownloader {

    private final FileDownloadCache mCache;
    private final IFileDownloader mDelegateDownloader;

    public FileDownloadCacheWrapper(File cacheDir, IFileDownloader delegateDownloader) {
        mCache = FileDownloadCacheFactory.getInstance().getCache(cacheDir);
        mDelegateDownloader = delegateDownloader;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File downloadFile(String remoteFilePath) throws BuildRetrievalError {
        return mCache.fetchRemoteFile(mDelegateDownloader, remoteFilePath);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void downloadFile(String remotePath, File destFile) throws BuildRetrievalError {
        throw new UnsupportedOperationException();
    }
}
