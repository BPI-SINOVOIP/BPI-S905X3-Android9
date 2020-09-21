/*
 * Copyright (C) 2015 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.caliper.platform.dalvik;

import static com.google.common.base.Preconditions.checkState;

import com.google.caliper.platform.Platform;
import com.google.common.base.Preconditions;
import com.google.common.base.Predicate;
import com.google.common.base.Predicates;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;
import com.google.common.base.Joiner;

import java.io.File;
import java.util.Collection;
import java.util.Collections;
import java.util.Map;

/**
 * An abstraction of the Dalvik (aka Android) platform.
 *
 * <p>Although this talks about dalvik it actually works with ART too.
 */
public final class DalvikPlatform extends Platform {

  // By default, use dalvikvm.
  // However with --vm (by calling customVmHomeDir) we can change this.
  private String vmExecutable = "dalvikvm";
  private String vmAndroidRoot = System.getenv("ANDROID_ROOT");

  public DalvikPlatform() {
    super(Type.DALVIK);

    if (vmAndroidRoot == null) {
      // Shouldn't happen unless running it on the host and someone forgot to set it.
      // On an actual device, ANDROID_ROOT is always set.
      vmAndroidRoot = "/system";
    }
  }

  @Override
  public File vmExecutable(File vmHome) {
    File bin = new File(vmHome, "bin");
    Preconditions.checkState(bin.exists() && bin.isDirectory(),
        "Could not find %s under android root %s", bin, vmHome);
    String executableName = vmExecutable;
    File dalvikvm = new File(bin, executableName);
    if (!dalvikvm.exists() || dalvikvm.isDirectory()) {
      throw new IllegalStateException(
          String.format("Cannot find %s binary in %s", executableName, bin));
    }

    return dalvikvm;
  }

  @Override
  public ImmutableSet<String> commonInstrumentVmArgs() {
    return ImmutableSet.of();
  }

  @Override
  public ImmutableSet<String> workerProcessArgs() {
    if (vmExecutable.equals("app_process")) {
      // app_process expects a parent_dir argument before the main class name, e.g.
      // app_process -cp /path/to/dex/file /system/bin foo.bar.Main
      return ImmutableSet.of(vmAndroidRoot + "/bin");
    }
    return ImmutableSet.of();
  }

  @Override
  protected String workerClassPath() {
    // TODO(user): Find a way to get the class path programmatically from the class loader.
    return System.getProperty("java.class.path");
  }

  /**
   * Construct the set of arguments that specify the classpath which
   * is passed to the worker.
   *
   * <p>
   * Use {@code -Djava.class.path=$classpath} for dalvik because it is supported
   * by dalvikvm and also by app_process (which doesn't recognize "-cp args").
   * </p>
   */
  @Override
  public ImmutableList<String> workerClassPathArgs() {
    String classPathArgs = String.format("-Djava.class.path=%s", workerClassPath());
    return ImmutableList.of(classPathArgs);
  }

  @Override
  public Collection<String> inputArguments() {
    return Collections.emptyList();
  }

  @Override
  public Predicate<String> vmPropertiesToRetain() {
    return Predicates.alwaysFalse();
  }

  @Override
  public void checkVmProperties(Map<String, String> options) {
    checkState(options.isEmpty());
  }

  @Override
  public File customVmHomeDir(Map<String, String> vmGroupMap, String vmConfigName) {
    // Support a handful of specific commands:
    switch (vmConfigName) {
      case "app_process":   // run with Android framework code already initialized.
      case "dalvikvm":      // same as not using --vm (selects 64-bit on 64-bit, 32-bit on 32-bit)
      case "dalvikvm32":    // 32-bit specific dalvikvm (e.g. if running on 64-bit device)
      case "dalvikvm64":    // 64-bit specific dalvikvm (which is already default on 64-bit)
      case "art":           // similar to dalvikvm but goes through a script with better settings.
      {
        // Usually passed as vmHome to vmExecutable, but we don't get that passed here.
        String vmHome = vmAndroidRoot;

        // Do not return the binary here. We return the new vmHome used by #vmExecutable.
        // Remember that the home directory was changed, and accordingly update the executable name.
        vmExecutable = vmConfigName;

        File androidRootFile = new File(vmHome);

        if (!androidRootFile.exists()) {
          throw new IllegalStateException(String.format("%s does not exist", androidRootFile));
        } else if (!androidRootFile.isDirectory()) {
          throw new IllegalStateException(String.format("%s is not a directory", androidRootFile));
        }

        return androidRootFile;
      }
    }

    // Unknown vm, throw an exception.

    Joiner.MapJoiner mapJoiner = Joiner.on(',').withKeyValueSeparator("=");
    String mapString = (vmGroupMap == null) ? "<null>" : mapJoiner.join(vmGroupMap);

    if (vmConfigName == null) {
      vmConfigName = "<null>";
    }

    throw new UnsupportedOperationException(
            "Running with a custom Dalvik VM is not currently supported (group map = '" + mapString
            + "', vmConfigName = '" + vmConfigName + "')");
  }
}
