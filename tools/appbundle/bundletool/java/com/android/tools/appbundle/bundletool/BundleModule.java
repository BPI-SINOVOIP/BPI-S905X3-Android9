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

import com.google.common.collect.ImmutableList;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;

/** Represents a single module inside App Bundle. */
public class BundleModule {

  private AppBundle parent;
  private String name;
  private List<ZipEntry> entries;

  private BundleModule(String name, AppBundle parent, List<ZipEntry> entries) {
    this.parent = parent;
    this.name = name;
    this.entries = entries;
  }

  public String getName() {
    return name;
  }

  public AppBundle getParent() {
    return parent;
  }

  public List<ZipEntry> getEntries() {
    return ImmutableList.copyOf(entries);
  }

  /** Builder for BundleModule. */
  public static class Builder {
    private List<ZipEntry> entries;
    private String name;
    private AppBundle parent;

    public Builder(String name, AppBundle parent) {
      this.name = name;
      this.parent = parent;
      this.entries = new ArrayList<>();
    }

    public Builder addZipEntry(ZipEntry entry) {
      this.entries.add(entry);
      return this;
    }

    public BundleModule build() {
      return new BundleModule(name, parent, entries);
    }
  }
}
