/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.device;

import com.android.ddmlib.FileListingService;
import com.android.ddmlib.FileListingService.FileEntry;

import java.util.Collection;

/**
* Interface definition that provides simpler, mockable contract to
* {@link FileEntry} methods.
* <p/>
* TODO: move this into ddmlib
*/
public interface IFileEntry {

    /**
     * Wrapper for {@link FileEntry#getFullEscapedPath()}.
     */
    public String getFullEscapedPath();

    /**
     * Wrapper for {@link FileEntry#getFullPath()}.
     */
    public String getFullPath();

    /**
     * Wrapper for {@link FileEntry#isDirectory()}.
     */
    public boolean isDirectory();

    /**
     * Finds a child {@link IFileEntry} with given name.
     * <p/>
     * Basically a wrapper for {@link FileEntry#findChild(String)} that
     * will also first search the cached children for file with given name, and if not found,
     * refresh the cached child file list and attempt again.
     *
     * @throws DeviceNotAvailableException
     */
    public IFileEntry findChild(String name) throws DeviceNotAvailableException;

    /**
     * Wrapper for {@link FileEntry#isAppFileName()}.
     */
    public boolean isAppFileName();

    /**
     * Wrapper for {@link FileEntry#getName()}.
     */
    public String getName();

    /**
     * Wrapper for {@link FileEntry#getTime()}.
     */
    public String getTime();

    /**
     * Wrapper for {@link FileEntry#getDate()}.
     */
    public String getDate();

    /**
     * Wrapper for {@link FileEntry#getPermissions()}.
     */
    public String getPermissions();

    /**
     * Returns the children of a {@link IFileEntry}.
     * <p/>
     * Basically a synchronous wrapper for
     * {@link FileListingService#getChildren(FileEntry, boolean, FileListingService.IListingReceiver)}
     *
     * @param useCache <code>true</code> if the cached children should be returned if available.
     *            <code>false</code> if a new ls command should be forced.
     * @return list of sub files
     * @throws DeviceNotAvailableException
     */
    public Collection<IFileEntry> getChildren(boolean useCache) throws DeviceNotAvailableException;

    /**
     * Return reference to the ddmlib {@link FileEntry}.
     */
    public FileEntry getFileEntry();

}
