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
package com.android.tradefed.util;

import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;

import org.apache.commons.compress.archivers.zip.ZipArchiveEntry;
import org.apache.commons.compress.archivers.zip.ZipFile;

import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Object holding the bugreport files references, compatible of flat bugreport and zipped bugreport
 * (bugreportz).
 */
public class Bugreport implements Closeable {
    private File mBugreport;
    private boolean mIsZipped;

    public Bugreport(File bugreportFile, boolean isZipped) {
        mBugreport = bugreportFile;
        mIsZipped = isZipped;
    }

    /**
     * Return true if it's a zipped bugreport, false otherwise.
     */
    public boolean isZipped() {
        return mIsZipped;
    }

    /**
     * Helper to log the Bugreport whether its zipped or not.
     *
     * @param dataName the name of the data once logged.
     * @param logger a {@link ITestLogger} to receive the log.
     */
    public void log(String dataName, ITestLogger logger) {
        LogDataType type = isZipped() ? LogDataType.BUGREPORTZ : LogDataType.BUGREPORT;
        try (InputStreamSource source = new FileInputStreamSource(mBugreport)) {
            logger.testLog(dataName, type, source);
        }
    }

    /**
     * Return a {@link File} pointing to the bugreport main file. For a flat bugreport, it returns
     * the flat bugreport itself. For a zipped bugreport, it returns the main entry file.
     * The returned file is a copy and should be appropriately managed by the user.
     */
    public File getMainFile() {
        if (mBugreport == null) {
            return null;
        }
        if (!mIsZipped) {
            return mBugreport;
        } else {
            File mainEntry = null;
            try {
                try (ZipFile zip = new ZipFile(mBugreport)) {
                    // We get the main_entry.txt that contains the bugreport name.
                    mainEntry = ZipUtil2.extractFileFromZip(zip, "main_entry.txt");
                    if (mainEntry == null) {
                        CLog.w("main_entry.txt was not found inside the bugreport");
                        return null;
                    }
                    String bugreportName = FileUtil.readStringFromFile(mainEntry).trim();
                    CLog.d("bugreport name: '%s'", bugreportName);
                    return ZipUtil2.extractFileFromZip(zip, bugreportName);
                }
            } catch (IOException e) {
                CLog.e("Error while unzipping bugreportz");
                CLog.e(e);
            } finally {
                FileUtil.deleteFile(mainEntry);
            }
        }
        return null;
    }

    /**
     * Returns the list of files contained inside the zipped bugreport. Null if it's not a zipped
     * bugreport.
     */
    public List<String> getListOfFiles() {
        if (mBugreport == null) {
            return null;
        }
        List<String> list = new ArrayList<>();
        if (!mIsZipped) {
            return null;
        }
        try (ZipFile zipBugreport = new ZipFile(mBugreport)) {
            for (ZipArchiveEntry entry : Collections.list(zipBugreport.getEntries())) {
                list.add(entry.getName());
            }
        } catch (IOException e) {
            CLog.e("Error reading the list of files in the bugreport");
            CLog.e(e);
        }
        return list;
    }

    /**
     * Return the {@link File} associated with the name in the bugreport. Null if not found or if
     * name is null. Non zipped bugreport always return null.
     * The returned file is a copy and should be appropriately managed by the user.
     */
    public File getFileByName(String name) {
        if (mBugreport == null || name == null) {
            return null;
        }
        if (mIsZipped) {
            return extractFileBugreport(name);
        }
        return null;
    }

    /**
     * Helper to extract and return a file from the zipped bugreport.
     */
    private File extractFileBugreport(String name) {
        File bugreport = null;
        try (ZipFile zip = new ZipFile(mBugreport)) {
            bugreport = ZipUtil2.extractFileFromZip(zip, name);
            return bugreport;
        } catch (IOException e) {
            CLog.e("Error while unzipping bugreportz");
            CLog.e(e);
        }
        return null;
    }

    /**
     * Clean up the files held by the bugreport object. Must be called when the object is not used
     * anymore.
     */
    @Override
    public void close() throws IOException {
        FileUtil.deleteFile(mBugreport);
    }
}
