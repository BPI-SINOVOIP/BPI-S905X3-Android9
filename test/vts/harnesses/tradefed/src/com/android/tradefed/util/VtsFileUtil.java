/*
 * Copyright (C) 2018 The Android Open Source Project
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

import com.android.tradefed.log.LogUtil.CLog;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Paths;

public class VtsFileUtil extends FileUtil {
    /**
     * Save a resource file to a directory.
     *
     * TODO(yuexima): This method is to be removed when it becomes available in FileUtil.
     *
     * @param resourceStream a {link InputStream} object to the resource to be saved.
     * @param destDir a {@link File} object of a directory to where the resource file will be saved.
     * @param targetFileName a {@link String} for the name of the file to be saved to.
     * @return a {@link File} object of the file saved.
     * @throws IOException if the file failed to be saved.
     */
    public static File saveResourceFile(
            InputStream resourceStream, File destDir, String targetFileName) throws IOException {
        FileWriter writer = null;
        File file = Paths.get(destDir.getAbsolutePath(), targetFileName).toFile();
        try {
            writer = new FileWriter(file);
            StreamUtil.copyStreamToWriter(resourceStream, writer);
            return file;
        } catch (IOException e) {
            CLog.e("IOException while saving resource %s/%s", destDir, targetFileName);
            deleteFile(file);
            throw e;
        } finally {
            if (writer != null) {
                writer.close();
            }
            if (resourceStream != null) {
                resourceStream.close();
            }
        }
    }
}