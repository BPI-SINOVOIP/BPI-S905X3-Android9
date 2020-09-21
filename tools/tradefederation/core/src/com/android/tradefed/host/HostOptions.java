/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tradefed.host;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;

import java.io.File;

/**
 * Host options holder class.
 * This class is used to store host-wide options.
 */
@OptionClass(alias = "host_options", global_namespace = false)
public class HostOptions implements IHostOptions {

    @Option(name = "concurrent-flasher-limit", description =
            "The maximum number of concurrent flashers (may be useful to avoid memory constraints)")
    private Integer mConcurrentFlasherLimit = 1;

    @Option(
        name = "concurrent-download-limit",
        description =
                "The maximum number of concurrent downloads (may be useful to avoid network "
                        + "constraints)"
    )
    private Integer mConcurrentDownloadLimit = null;

    @Option(
        name = "fastboot-tmpdir",
        description = "The location of temporary directory used by fastboot"
    )
    private File mFastbootTmpDir = null;

    @Option(name = "download-cache-dir", description = "the directory for caching downloaded "
            + "flashing files. Should be on the same filesystem as java.io.tmpdir.  Consider "
            + "changing the java.io.tmpdir property if you want to move downloads to a different "
            + "filesystem.")
    private File mDownloadCacheDir = new File(System.getProperty("java.io.tmpdir"), "lc_cache");

    /**
     * {@inheritDoc}
     */
    @Override
    public Integer getConcurrentFlasherLimit() {
        return mConcurrentFlasherLimit;
    }

    /** {@inheritDoc} */
    @Override
    public Integer getConcurrentDownloadLimit() {
        return mConcurrentDownloadLimit;
    }

    /** {@inheritDoc} */
    @Override
    public File getFastbootTmpDir() {
        return mFastbootTmpDir;
    }

    @Override
    public File getDownloadCacheDir() {
        return mDownloadCacheDir;
    }
}
