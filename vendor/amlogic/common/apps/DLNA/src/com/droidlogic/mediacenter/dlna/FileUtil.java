/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.mediacenter.dlna;

import java.io.File;
import java.io.IOException;

import android.os.Environment;
import android.util.Log;

public class FileUtil {
    private static final String TAG = "FileUtil";

    public static File getCacheFile(String imageUri) {
        File cacheFile = null;
        File nandDir = Environment.getExternalStorageDirectory();
        String fileName = getFileName(imageUri);
        // Log.d(TAG,"File Name path:"+imageUri+" FileName:"+fileName);
        File dir = new File(nandDir.getAbsoluteFile(), AsynImageLoader.CACHE_DIR);
        if (!dir.exists()) {
            dir.mkdirs();
        }
        cacheFile = new File(dir, fileName);
        // Log.d(TAG,"exists:" + cacheFile.exists() + ",dir:" + dir + ",file:" +
        // fileName);
        return cacheFile;
    }

    public static void delCacheFile() {
        File cacheFile = null;
        File nandDir = Environment.getExternalStorageDirectory();
        File dir = new File(nandDir.getAbsoluteFile(), AsynImageLoader.CACHE_DIR);
        if (dir.exists()) {
            deleteAllDir(dir);
        }

    }

    private static void deleteAllDir(File dir) {
        if (dir.isDirectory() && !dir.delete()) {
            for (File f : dir.listFiles()) {
                if (f.isDirectory()) {
                    deleteAllDir(f);
                } else {
                    f.delete();
                }
            }
        }
        dir.delete();
    }

    public static String getFileName(String path) {
        int index = path.lastIndexOf("/");
        return path.substring(index + 1);
    }
}