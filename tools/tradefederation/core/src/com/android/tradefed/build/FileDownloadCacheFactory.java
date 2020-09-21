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
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * A factory for creating {@link FileDownloadCache}
 */
public class FileDownloadCacheFactory {

    // use the "singleton inner class" pattern
    // http://en.wikipedia.org/wiki/Singleton_pattern#The_solution_of_Bill_Pugh
    private static class SingletonHolder {
        public static final FileDownloadCacheFactory INSTANCE = new FileDownloadCacheFactory();
    }

    private Map<String, FileDownloadCache> mCacheObjectMap = Collections.synchronizedMap(
            new HashMap<String, FileDownloadCache>());

    /**
     * Get the singleton instance of FileDownloadCacheFactory
     */
    public static FileDownloadCacheFactory getInstance() {
        return SingletonHolder.INSTANCE;
    }

    /**
     * Retrieve the {@link FileDownloadCache} with the given cache directory, creating if necessary.
     * <p/>
     * Note that the cache assumes that this process has exclusive access to the <var>cacheDir</var>
     * directory. If multiple TF processes will be run on the same machine, they MUST each use
     * unique cache directories.
     *
     * @param cacheDir the local filesystem directory to use as a cache
     * @return the {@link FileDownloadCache} for given cacheDir
     */
    public synchronized FileDownloadCache getCache(File cacheDir) {
        FileDownloadCache cache = mCacheObjectMap.get(cacheDir.getAbsolutePath());
        if (cache == null) {
            cache = new FileDownloadCache(cacheDir);
            mCacheObjectMap.put(cacheDir.getAbsolutePath(), cache);
        }
        return cache;
    }
}
