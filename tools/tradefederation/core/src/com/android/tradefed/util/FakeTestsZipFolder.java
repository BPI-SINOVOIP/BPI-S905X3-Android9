/*
 * Copyright (C) 2012 The Android Open Source Project
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

import java.io.File;
import java.io.IOException;
import java.util.Map;

/**
 * A testing fixture that creates a fake unzipped tests folder based on a list of content.
 *
 * The folder structure is configured based on a list of file names or folder names, as provided
 * to the constructor. {@link FakeTestsZipFolder#cleanUp()} should be called after the folder is
 * no longer needed.
 *
 */
public class FakeTestsZipFolder {

    public enum ItemType {
        FILE, DIRECTORY,
    }

    private Map<String, ItemType> mItems;
    private File mBase;
    private File mData;

    /**
     * Create a fake unzipped tests folder backed by empty files
     *
     * @param items list of items to include in the fake unzipped folder. key of
     *            the map shall be the relative path of the item, value of the
     *            entry shall indicate if the entry should be backed by an empty
     *            file or a folder
     */
    public FakeTestsZipFolder(Map<String, ItemType> items) {
        mItems = items;
    }

    /**
     * Create fake unzipped tests folder as indicated by the manifest of items
     *
     * @return false if failed to create any item
     * @throws IOException
     */
    public boolean createItems() throws IOException {
        mBase = File.createTempFile("tf_", "_test_zip");
        mBase.delete();
        mData = new File(mBase, "DATA");
        if (!mData.mkdirs()) {
            return false;
        }
        boolean failed = false;
        for (String fileName : mItems.keySet()) {
            File file = new File(mData, fileName);
            ItemType type = mItems.get(fileName);
            if (ItemType.DIRECTORY.equals(type)) {
                if (!file.mkdirs()) {
                    failed = true;
                    break;
                }
            } else {
                File p = file.getParentFile();
                if (!p.exists()) {
                    if (!p.mkdirs()) {
                        failed = true;
                        break;
                    }
                }
                if (!file.createNewFile()) {
                    failed = true;
                    break;
                }
            }
        }
        if (failed) {
            // attempt to clean up
            cleanUp();
        }
        return !failed;
    }

    /**
     * Delete the entire fake unzipped test folder
     */
    public void cleanUp() {
        FileUtil.recursiveDelete(mBase);
    }

    protected File getDataFolder() {
        return mData;
    }

    /**
     * Returns the base of the fake unzipped folder This would be a replacement
     * of root folder where a real tests zip is expanded
     */
    public File getBasePath() {
        return mBase;
    }
}
