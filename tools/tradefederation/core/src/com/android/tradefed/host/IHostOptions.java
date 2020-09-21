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

import com.android.tradefed.build.IBuildProvider;
import com.android.tradefed.targetprep.DeviceFlashPreparer;

import java.io.File;

/**
 * Host options holder interface.
 * This interface is used to access host-wide options.
 */
public interface IHostOptions {

    /**
     * Returns the max number of concurrent flashing to allow. Used by {@link DeviceFlashPreparer}.
     *
     * @return the concurrent flasher limit.
     */
    Integer getConcurrentFlasherLimit();

    /**
     * Returns the max number of concurrent downloads allowed. Used by {@link IBuildProvider} that
     * downloads remote builds.
     */
    Integer getConcurrentDownloadLimit();

    /**
     * Returns the path that fastboot should use as temporary folder
     *
     * @return
     */
    File getFastbootTmpDir();

    /**
     * Returns the path used for storing downloaded artifacts
     *
     * @return
     */
    File getDownloadCacheDir();
}
