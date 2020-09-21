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

import static com.google.common.base.Preconditions.checkArgument;

import com.android.tools.appbundle.bundletool.utils.FlagParser;
import com.google.auto.value.AutoValue;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Optional;

/** Command responsible for building an App Bundle module. */
@AutoValue
public abstract class BuildModuleCommand {

  public static final String COMMAND_NAME = "build-module";

  private static final String OUTPUT_FLAG = "output";
  private static final String MANIFEST_FLAG = "manifest";
  private static final String MANIFEST_DIR_FLAG = "manifest-dir";
  private static final String DEX_FLAG = "dex";
  private static final String DEX_DIR_FLAG = "dex-dir";
  private static final String RESOURCES_DIR_FLAG = "resources-dir";
  private static final String ASSETS_DIR_FLAG = "assets-dir";
  private static final String NATIVE_DIR_FLAG = "native-dir";

  abstract Path getOutputPath();

  abstract Optional<Path> getManifestPath();

  abstract Optional<Path> getManifestDirPath();

  abstract Optional<Path> getDexPath();

  abstract Optional<Path> getDexDirPath();

  abstract Optional<Path> getResourcesDirPath();

  abstract Optional<Path> getAssetsDirPath();

  abstract Optional<Path> getNativeDirPath();

  public static Builder builder() {
    return new AutoValue_BuildModuleCommand.Builder();
  }

  /** Builder for the {@link BuildModuleCommand} */
  @AutoValue.Builder
  public abstract static class Builder {
    abstract Builder setOutputPath(Path outputPath);

    abstract Builder setManifestPath(Path manifestPath);

    abstract Builder setManifestDirPath(Path manifestDirPath);

    abstract Builder setDexPath(Path dexPath);

    abstract Builder setDexDirPath(Path dexDirPath);

    abstract Builder setResourcesDirPath(Path resourcesDirPath);

    abstract Builder setAssetsDirPath(Path assetsDirPath);

    abstract Builder setNativeDirPath(Path nativeDirPath);

    abstract BuildModuleCommand build();
  }

  static BuildModuleCommand fromFlags(FlagParser flagParser) {
    Builder builder =
        builder().setOutputPath(Paths.get(flagParser.getRequiredFlagValue(OUTPUT_FLAG)));
    flagParser.getFlagValueAsPath(MANIFEST_FLAG).ifPresent(builder::setManifestPath);
    flagParser.getFlagValueAsPath(MANIFEST_DIR_FLAG).ifPresent(builder::setManifestDirPath);
    flagParser.getFlagValueAsPath(DEX_FLAG).ifPresent(builder::setDexPath);
    flagParser.getFlagValueAsPath(DEX_DIR_FLAG).ifPresent(builder::setDexDirPath);
    flagParser.getFlagValueAsPath(RESOURCES_DIR_FLAG).ifPresent(builder::setResourcesDirPath);
    flagParser.getFlagValueAsPath(ASSETS_DIR_FLAG).ifPresent(builder::setAssetsDirPath);
    flagParser.getFlagValueAsPath(NATIVE_DIR_FLAG).ifPresent(builder::setNativeDirPath);

    return builder.build();
  }

  public void execute() {
    validateInput();


  }

  private void validateInput() {
    checkArgument(
        getManifestPath().isPresent() || getManifestDirPath().isPresent(),
        "One of --%s or --%s is required.",
        MANIFEST_FLAG,
        MANIFEST_DIR_FLAG);
    checkArgument(
        !getManifestPath().isPresent() || !getManifestDirPath().isPresent(),
        "Cannot set both --%s and --%s flags.",
        MANIFEST_FLAG,
        MANIFEST_DIR_FLAG);
    checkArgument(
        !getDexPath().isPresent() || !getDexDirPath().isPresent(),
        "Cannot set both --%s and --%s flags.",
        DEX_FLAG,
        DEX_DIR_FLAG);

    checkArgument(!Files.exists(getOutputPath()), "File %s already exists.", getOutputPath());
    checkFileExistsAndReadable(getManifestPath());
    checkDirectoryExists(getManifestDirPath());
    checkFileExistsAndReadable(getDexPath());
    checkDirectoryExists(getDexDirPath());
    checkDirectoryExists(getResourcesDirPath());
    checkDirectoryExists(getAssetsDirPath());
    checkDirectoryExists(getNativeDirPath());
  }

  private static void checkFileExistsAndReadable(Optional<Path> pathOptional) {
    if (pathOptional.isPresent()) {
      Path path = pathOptional.get();
      checkArgument(Files.exists(path), "File '%s' was not found.", path);
      checkArgument(Files.isReadable(path), "File '%s' is not readable.", path);
    }
  }

  private static void checkDirectoryExists(Optional<Path> pathOptional) {
    if (pathOptional.isPresent()) {
      Path path = pathOptional.get();
      checkArgument(Files.exists(path), "Directory '%s' was not found.", path);
      checkArgument(Files.isDirectory(path), "'%s' is not a directory.");
    }
  }

  public static void help() {
    System.out.println(
        String.format(
            "bundletool %s --output=<path/to/module.zip> "
                + "[--%s=<path/to/AndroidManifest.flat>|--%s=<path/to/manifest-dir/>] "
                + "[--%s=<path/to/classes.dex>|--%s=<path/to/dex-dir/>] "
                + "[--%s=<path/to/res/>] "
                + "[--%s=<path/to/assets/>] "
                + "[--%s=<path/to/lib/>] ",
            COMMAND_NAME,
            MANIFEST_FLAG,
            MANIFEST_DIR_FLAG,
            DEX_FLAG,
            DEX_DIR_FLAG,
            RESOURCES_DIR_FLAG,
            ASSETS_DIR_FLAG,
            NATIVE_DIR_FLAG));
    System.out.println();
    System.out.println(
        "Builds a module as a zip from an app's project. Note that the resources and the "
            + "AndroidManifest.xml must already have been compiled with aapt2.");
    System.out.println();
    System.out.println("--output: Path to the zip file to build.");
    System.out.printf(
        "--%s: Path to the AndroidManifest.flat compiled by aapt2. Use --%s if there "
            + "are more than one.\n",
        MANIFEST_FLAG, MANIFEST_DIR_FLAG);
    System.out.printf(
        "--%s: Path to the directory containing multiple Android manifests compiled by aapt2. "
            + "A file named 'manifest-targeting.xml' must be present in the directory "
            + "describing the targeting of each manifest present.\n",
        MANIFEST_DIR_FLAG);
    System.out.printf(
        "--%s: Path to the dex file. Use --%s if there are more than one.\n",
        DEX_FLAG, DEX_DIR_FLAG);
    System.out.printf(
        "--%s: Path to the directory containing multiple dex files. Unless all dex files must "
            + "be included in the generated APKs (for MultiDex), a file named "
            + "'dex-targeting.xml' must be present in the directory describing the targeting "
            + "of the different dex files.\n",
        DEX_DIR_FLAG);
    System.out.printf(
        "--%s: Path to the directory containing the resources file(s). A file named "
            + "'resources.flat' must be present in that directory corresponding to the output "
            + "of the aapt2 compilation of the resources.\n",
        RESOURCES_DIR_FLAG);
    System.out.printf("--%s: Path to the directory containing the assets.\n", ASSETS_DIR_FLAG);
    System.out.printf(
        "--%s: Path to the directory containing the native libraries.\n", NATIVE_DIR_FLAG);
  }
}
