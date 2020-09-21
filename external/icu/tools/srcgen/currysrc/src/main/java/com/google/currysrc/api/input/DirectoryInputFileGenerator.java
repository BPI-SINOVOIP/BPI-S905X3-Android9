/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.google.currysrc.api.input;

import com.google.common.collect.Lists;
import com.google.currysrc.api.input.InputFileGenerator;

import java.io.File;
import java.util.List;

/**
 * Generates a set of .java input files beneath a given base directory.
 */
public final class DirectoryInputFileGenerator implements InputFileGenerator {

  private final File baseDir;

  public DirectoryInputFileGenerator(File baseDir) {
    if (baseDir == null) {
      throw new NullPointerException("Null baseDir");
    }
    this.baseDir = baseDir;
  }

  @Override
  public Iterable<? extends File> generate() {
    List<File> files = Lists.newArrayList();
    collectFiles(baseDir, files);
    return files;
  }

  private void collectFiles(File baseDir, List<File> files) {
    if (!baseDir.exists()) {
      throw new IllegalArgumentException("Not found: " + baseDir.getAbsolutePath());
    }
    if (!baseDir.isDirectory()) {
      throw new IllegalArgumentException("Not a directory: " + baseDir.getAbsolutePath());
    }
    for (File file : baseDir.listFiles()) {
      if (file.isDirectory()) {
        collectFiles(file, files);
      } else if (file.getName().endsWith(".java")) {
        files.add(file);
      } else {
        System.out.println("Ignoring file: " + file);
      }
    }
  }
}
