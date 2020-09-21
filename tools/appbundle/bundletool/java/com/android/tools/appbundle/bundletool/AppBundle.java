/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tools.appbundle.bundletool;

import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Maps;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/** Represents an app bundle. */
public class AppBundle {

  private ZipFile bundleFile;
  private Map<String, BundleModule> modules;

  public AppBundle(ZipFile bundleFile) {
    this.bundleFile = bundleFile;
    this.modules = new HashMap<>();
    open();
  }

  private void open() {
    Map<String, BundleModule.Builder> moduleBuilders = new HashMap<>();
    Enumeration<? extends ZipEntry> entries = bundleFile.entries();
    while (entries.hasMoreElements()) {
      ZipEntry entry = entries.nextElement();
      Path path = Paths.get(entry.getName());
      if (path.getNameCount() > 1) {
        String moduleName = path.getName(0).toString();
        BundleModule.Builder moduleBuilder =
            moduleBuilders.computeIfAbsent(
                moduleName, name -> new BundleModule.Builder(name, this));
        moduleBuilder.addZipEntry(entry);
      }
    }
    modules.putAll(Maps.transformValues(moduleBuilders, BundleModule.Builder::build));
  }

  public Map<String, BundleModule> getModules() {
    return ImmutableMap.copyOf(modules);
  }

  public BundleModule getModule(String moduleName) {
    return modules.get(moduleName);
  }

  public InputStream getEntryInputStream(ZipEntry entry) throws IOException {
    return bundleFile.getInputStream(entry);
  }
}
