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
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Implementation of a {@link IFileEntry}.
 */
class FileEntryWrapper implements IFileEntry {

    private final NativeDevice mTestDevice;
    private final FileListingService.FileEntry mFileEntry;
    private Map<String, IFileEntry> mChildMap = null;

    /**
     * Creates a {@link FileEntryWrapper}.
     *
     * @param testDevice the {@link TestDevice} to use
     * @param entry the corresponding {@link FileEntry} to wrap
     */
    FileEntryWrapper(NativeDevice testDevice, FileEntry entry) {
        mTestDevice = testDevice;
        mFileEntry = entry;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getFullEscapedPath() {
        return mFileEntry.getFullEscapedPath();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getFullPath() {
        return mFileEntry.getFullPath();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IFileEntry findChild(String name) throws DeviceNotAvailableException {
        if (mChildMap == null || !mChildMap.containsKey(name)) {
            mChildMap = buildChildrenMap();
        }
        return mChildMap.get(name);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isDirectory() {
        return mFileEntry.isDirectory();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isAppFileName() {
        return mFileEntry.isAppFileName();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getName() {
        return mFileEntry.getName();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Collection<IFileEntry> getChildren(boolean useCache) throws DeviceNotAvailableException {
        if (!useCache || mChildMap == null) {
            mChildMap = buildChildrenMap();
        }
        return mChildMap.values();
    }

    private Map<String, IFileEntry> buildChildrenMap() throws DeviceNotAvailableException {
        FileEntry[] childEntrys = mTestDevice.getFileChildren(mFileEntry);
        Map<String, IFileEntry> childMap = new HashMap<String, IFileEntry>(childEntrys.length);
        for (FileEntry entry : childEntrys) {
            childMap.put(entry.getName(), new FileEntryWrapper(mTestDevice, entry));
        }
        return childMap;
    }

    /**
     * Helper method that tries to a find the descendant of a {@link IFileEntry}
     *
     * @param fileEntry the starting point
     * @param childSegments the relative path of the {@link IFileEntry} to find
     * @return the {@link IFileEntry}, or <code>null</code> if it cannot be found
     * @throws DeviceNotAvailableException
     */
    static IFileEntry getDescendant(IFileEntry fileEntry, List<String> childSegments)
            throws DeviceNotAvailableException {
        IFileEntry child = fileEntry;
        for (String childName: childSegments) {
            if (childName.length() > 0) {
                child = child.findChild(childName);
                if (child == null) {
                    return null;
                }
            }
        }
        return child;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public FileEntry getFileEntry() {
        return mFileEntry;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getTime() {
        return mFileEntry.getTime();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getDate() {
        return mFileEntry.getDate();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPermissions() {
        return mFileEntry.getPermissions();
    }
}
