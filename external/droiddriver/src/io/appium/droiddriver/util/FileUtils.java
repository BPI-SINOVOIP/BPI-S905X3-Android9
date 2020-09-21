/*
 * Copyright (C) 2013 DroidDriver committers
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

package io.appium.droiddriver.util;

import android.annotation.TargetApi;
import android.os.Build;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;

import io.appium.droiddriver.exceptions.DroidDriverException;

/**
 * Internal helper methods for manipulating files.
 */
public class FileUtils {
  /**
   * Opens file at {@code path} to output. If any directories on {@code path} do
   * not exist, they will be created. The file will be readable and writable to
   * all.
   */
  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  public static BufferedOutputStream open(String path) throws FileNotFoundException {
    File file = getAbsoluteFile(path);

    Logs.log(Log.INFO, "opening file " + file.getAbsolutePath());
    BufferedOutputStream stream = new BufferedOutputStream(new FileOutputStream(file));
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
      file.setReadable(true /* readable */, false/* ownerOnly */);
      file.setWritable(true /* readable */, false/* ownerOnly */);
    }
    return stream;
  }

  /**
   * Returns a new file constructed using the absolute path of {@code path}.
   * Unlike {@link File#getAbsoluteFile()}, default parent is "java.io.tmpdir"
   * instead of "user.dir".
   * <p>
   * If any directories on {@code path} do not exist, they will be created.
   */
  public static File getAbsoluteFile(String path) {
    File file = new File(path);
    if (!file.isAbsolute()) {
      file = new File(System.getProperty("java.io.tmpdir"), path);
    }
    mkdirs(file.getParentFile());
    return file;
  }

  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  private static void mkdirs(File dir) {
    if (dir == null || dir.exists()) {
      return;
    }

    mkdirs(dir.getParentFile());
    if (!dir.mkdir()) {
      throw new DroidDriverException("failed to mkdir " + dir);
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
      dir.setReadable(true /* readable */, false/* ownerOnly */);
      dir.setWritable(true /* readable */, false/* ownerOnly */);
      dir.setExecutable(true /* executable */, false/* ownerOnly */);
    }
  }
}
