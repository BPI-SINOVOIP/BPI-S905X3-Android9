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

import com.android.tools.appbundle.bundletool.utils.FlagParser;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.zip.ZipException;
import java.util.zip.ZipFile;

/** Implementation of the command to generate module splits. */
public class SplitModuleCommand implements Command {

  private final String bundleLocation;
  private final String outputDirectory;
  private final String moduleName;

  public static final String COMMAND_NAME = "split-module";

  private static final String BUNDLE_LOCATION_FLAG = "bundle";
  private static final String OUTPUT_DIRECTORY_FLAG = "output";
  private static final String MODULE_FLAG = "module";

  public SplitModuleCommand(FlagParser parsedFlags) {
    bundleLocation = parsedFlags.getRequiredFlagValue(BUNDLE_LOCATION_FLAG);
    outputDirectory = parsedFlags.getRequiredFlagValue(OUTPUT_DIRECTORY_FLAG);
    moduleName = parsedFlags.getRequiredFlagValue(MODULE_FLAG);
  }

  @Override
  public void execute() throws ExecutionException {
    try {
      AppBundle appBundle = new AppBundle(new ZipFile(bundleLocation));
      BundleModule module = appBundle.getModule(moduleName);
      if (module == null) {
        throw new ExecutionException(
            String.format("Cannot find the %s module in the bundle", moduleName));
      }
      splitModule(moduleName, module, outputDirectory);
    } catch (ZipException e) {
      throw new ExecutionException("Zip error while opening the bundle " + e.getMessage(), e);
    } catch (FileNotFoundException e) {
      throw new ExecutionException("Bundle file not found", e);
    } catch (IOException e) {
      throw new ExecutionException("I/O error while processing the bundle " + e.getMessage(), e);
    }
  }

  private void splitModule(String moduleName, BundleModule module, String outputDirectory) {
    throw new UnsupportedOperationException("Not implemented");
  }

  public static void help() {
    System.out.printf(
        "bundletool %s --%s=[bundle.zip] --%s=[module-name] --%s=[output-dir]\n",
        BUNDLE_LOCATION_FLAG, MODULE_FLAG, OUTPUT_DIRECTORY_FLAG, COMMAND_NAME);
    System.out.println("Generates module splits for the given module of the bundle.");
    System.out.println("For now, one split is generated containing all module's resources.");
    System.out.println();
    System.out.printf("--%s: the zip file containing an App Bundle.\n", BUNDLE_LOCATION_FLAG);
    System.out.printf("--%s: module for which generate the splits.\n", MODULE_FLAG);
    System.out.printf(
        "--%s: the directory where the module zip files should be written to.\n",
        OUTPUT_DIRECTORY_FLAG);
  }
}
