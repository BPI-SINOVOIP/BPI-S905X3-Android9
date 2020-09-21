/*
 * Copyright (C) 2010 The Android Open Source Project
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

package vogar.android;

import com.google.common.annotations.VisibleForTesting;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.jar.JarEntry;
import java.util.jar.JarInputStream;
import java.util.jar.JarOutputStream;
import vogar.Classpath;
import vogar.Dexer;
import vogar.HostFileCache;
import vogar.Language;
import vogar.Log;
import vogar.Md5Cache;
import vogar.ModeId;
import vogar.commands.Command;
import vogar.commands.Mkdir;
import vogar.util.Strings;



/**
 * Android SDK commands such as adb, aapt and dx.
 */
public class AndroidSdk {

    private static final String D8_COMMAND_NAME = "d8-compat-dx";
    private static final String DX_COMMAND_NAME = "dx";
    private static final String ARBITRARY_BUILD_TOOL_NAME = D8_COMMAND_NAME;

    private final Log log;
    private final Mkdir mkdir;
    private final File[] compilationClasspath;
    private final String androidJarPath;
    private final String desugarJarPath;
    private final Md5Cache dexCache;
    private final Language language;

    public static Collection<File> defaultExpectations() {
        return Collections.singletonList(new File("libcore/expectations/knownfailures.txt"));
    }

    /**
     * Create an {@link AndroidSdk}.
     *
     * <p>Searches the PATH used to run this and scans the file system in order to determine the
     * compilation class path and android jar path.
     */
    public static AndroidSdk createAndroidSdk(
            Log log, Mkdir mkdir, ModeId modeId, Language language) {
        List<String> path = new Command.Builder(log).args("which", ARBITRARY_BUILD_TOOL_NAME)
                .permitNonZeroExitStatus(true)
                .execute();
        if (path.isEmpty()) {
            throw new RuntimeException(ARBITRARY_BUILD_TOOL_NAME + " not found");
        }
        File buildTool = new File(path.get(0)).getAbsoluteFile();
        String buildToolDirString = getParentFileNOrLast(buildTool, 1).getName();

        List<String> adbPath = new Command.Builder(log)
                .args("which", "adb")
                .permitNonZeroExitStatus(true)
                .execute();

        File adb;
        if (!adbPath.isEmpty()) {
            adb = new File(adbPath.get(0));
        } else {
            adb = null;  // Could not find adb.
        }

        /*
         * Determine if we are running with a provided SDK or in the AOSP source tree.
         *
         * Android build tree (target):
         *  ${ANDROID_BUILD_TOP}/out/host/linux-x86/bin/aapt
         *  ${ANDROID_BUILD_TOP}/out/host/linux-x86/bin/adb
         *  ${ANDROID_BUILD_TOP}/out/host/linux-x86/bin/dx
         *  ${ANDROID_BUILD_TOP}/out/host/linux-x86/bin/desugar.jar
         *  ${ANDROID_BUILD_TOP}/out/target/common/obj/JAVA_LIBRARIES/core-libart_intermediates
         *      /classes.jar
         */

        File[] compilationClasspath;
        String androidJarPath;
        String desugarJarPath = null;

        // Accept that we are running in an SDK if the user has added the build-tools or
        // platform-tools to their path.
        boolean buildToolsPathValid = "build-tools".equals(getParentFileNOrLast(buildTool, 2)
                .getName());
        boolean isAdbPathValid = (adb != null) &&
                "platform-tools".equals(getParentFileNOrLast(adb, 1).getName());
        if (buildToolsPathValid || isAdbPathValid) {
            File sdkRoot = buildToolsPathValid
                    ? getParentFileNOrLast(buildTool, 3)  // if build tool path invalid then
                    : getParentFileNOrLast(adb, 2);  // adb must be valid.
            File newestPlatform = getNewestPlatform(sdkRoot);
            log.verbose("Using android platform: " + newestPlatform);
            compilationClasspath = new File[] { new File(newestPlatform, "android.jar") };
            androidJarPath = new File(newestPlatform.getAbsolutePath(), "android.jar")
                    .getAbsolutePath();
            log.verbose("using android sdk: " + sdkRoot);

            // There must be a desugar.jar in the build tool directory.
            desugarJarPath = buildToolDirString + "/desugar.jar";
            File desugarJarFile = new File(desugarJarPath);
            if (!desugarJarFile.exists()) {
                throw new RuntimeException("Could not find " + desugarJarPath);
            }
        } else if ("bin".equals(buildToolDirString)) {
            log.verbose("Using android source build mode to find dependencies.");
            String tmpJarPath = "prebuilts/sdk/current/android.jar";
            String androidBuildTop = System.getenv("ANDROID_BUILD_TOP");
            if (!com.google.common.base.Strings.isNullOrEmpty(androidBuildTop)) {
                tmpJarPath = androidBuildTop + "/prebuilts/sdk/current/android.jar";
            } else {
                log.warn("Assuming current directory is android build tree root.");
            }
            androidJarPath = tmpJarPath;

            String outDir = System.getenv("OUT_DIR");
            if (Strings.isNullOrEmpty(outDir)) {
                if (Strings.isNullOrEmpty(androidBuildTop)) {
                    outDir = ".";
                    log.warn("Assuming we are in android build tree root to find libraries.");
                } else {
                    log.verbose("Using ANDROID_BUILD_TOP to find built libraries.");
                    outDir = androidBuildTop;
                }
                outDir += "/out/";
            } else {
                log.verbose("Using OUT_DIR environment variable for finding built libs.");
                outDir += "/";
            }

            String hostOutDir = System.getenv("ANDROID_HOST_OUT");
            if (!Strings.isNullOrEmpty(hostOutDir)) {
                log.verbose("Using ANDROID_HOST_OUT to find host libraries.");
            } else {
                // Handle the case where lunch hasn't been run. Guess the architecture.
                log.warn("ANDROID_HOST_OUT not set. Assuming linux-x86");
                hostOutDir = outDir + "/host/linux-x86";
            }

            String desugarPattern = hostOutDir + "/framework/desugar.jar";
            File desugarJar = new File(desugarPattern);

            if (!desugarJar.exists()) {
                throw new RuntimeException("Could not find " + desugarPattern);
            }

            desugarJarPath = desugarJar.getPath();

            String pattern = outDir + "target/common/obj/JAVA_LIBRARIES/%s_intermediates/classes";
            if (modeId.isHost()) {
                pattern = outDir + "host/common/obj/JAVA_LIBRARIES/%s_intermediates/classes";
            }
            pattern += ".jar";

            String[] jarNames = modeId.getJarNames();
            compilationClasspath = new File[jarNames.length];
            for (int i = 0; i < jarNames.length; i++) {
                String jar = jarNames[i];
                compilationClasspath[i] = new File(String.format(pattern, jar));
            }
        } else {
            throw new RuntimeException("Couldn't derive Android home from "
                    + ARBITRARY_BUILD_TOOL_NAME);
        }

        return new AndroidSdk(log, mkdir, compilationClasspath, androidJarPath, desugarJarPath,
                new HostFileCache(log, mkdir), language);
    }

    @VisibleForTesting
    AndroidSdk(Log log, Mkdir mkdir, File[] compilationClasspath, String androidJarPath,
               String desugarJarPath, HostFileCache hostFileCache, Language language) {
        this.log = log;
        this.mkdir = mkdir;
        this.compilationClasspath = compilationClasspath;
        this.androidJarPath = androidJarPath;
        this.desugarJarPath = desugarJarPath;
        this.dexCache = new Md5Cache(log, "dex", hostFileCache);
        this.language = language;
    }

    // Goes up N levels in the filesystem hierarchy. Return the last file that exists if this goes
    // past /.
    private static File getParentFileNOrLast(File f, int n) {
        File lastKnownExists = f;
        for (int i = 0; i < n; i++) {
            File parentFile = lastKnownExists.getParentFile();
            if (parentFile == null) {
                return lastKnownExists;
            }
            lastKnownExists = parentFile;
        }
        return lastKnownExists;
    }

    /**
     * Returns the platform directory that has the highest API version. API
     * platform directories are named like "android-9" or "android-11".
     */
    private static File getNewestPlatform(File sdkRoot) {
        File newestPlatform = null;
        int newestPlatformVersion = 0;
        File[] platforms = new File(sdkRoot, "platforms").listFiles();
        if (platforms != null) {
            for (File platform : platforms) {
                try {
                    int version =
                            Integer.parseInt(platform.getName().substring("android-".length()));
                    if (version > newestPlatformVersion) {
                        newestPlatform = platform;
                        newestPlatformVersion = version;
                    }
                } catch (NumberFormatException ignore) {
                    // Ignore non-numeric preview versions like android-Honeycomb
                }
            }
        }
        if (newestPlatform == null) {
            throw new IllegalStateException("Cannot find newest platform in " + sdkRoot);
        }
        return newestPlatform;
    }

    public static Collection<File> defaultSourcePath() {
        return filterNonExistentPathsFrom("libcore/support/src/test/java",
                                          "external/mockwebserver/src/main/java/");
    }

    private static Collection<File> filterNonExistentPathsFrom(String... paths) {
        ArrayList<File> result = new ArrayList<File>();
        String buildRoot = System.getenv("ANDROID_BUILD_TOP");
        for (String path : paths) {
            File file = new File(buildRoot, path);
            if (file.exists()) {
                result.add(file);
            }
        }
        return result;
    }

    public File[] getCompilationClasspath() {
        return compilationClasspath;
    }

    /**
     * Converts all the .class files on 'classpath' into a dex file written to 'output'.
     *
     * @param multidex could the output be more than 1 dex file?
     * @param output the File for the classes.dex that will be generated as a result of this call.
     * @param outputTempDir a temporary directory which can store intermediate files generated.
     * @param classpath a list of files/directories containing .class files that are
     *                  merged together and converted into the output (dex) file.
     * @param dependentCp classes that are referenced in classpath but are not themselves on the
     *                    classpath must be listed in dependentCp, this is required to be able
     *                    resolve all class dependencies. The classes in dependentCp are <i>not</i>
     *                    included in the output dex file.
     * @param dexer Which dex tool to use
     */
    public void dex(boolean multidex, File output, File outputTempDir,
            Classpath classpath, Classpath dependentCp, Dexer dexer) {
        mkdir.mkdirs(output.getParentFile());

        String classpathSubKey = dexCache.makeKey(classpath);
        String cacheKey = null;
        if (classpathSubKey != null) {
            String multidexSubKey = "mdex=" + multidex;
            cacheKey = dexCache.makeKey(classpathSubKey, multidexSubKey);
            boolean cacheHit = dexCache.getFromCache(output, cacheKey);
            if (cacheHit) {
                log.verbose("dex cache hit for " + classpath);
                return;
            }
        }

        // Call desugar first to remove invoke-dynamic LambdaMetaFactory usage,
        // which ART doesn't support.
        List<String> desugarOutputFilePaths = desugar(outputTempDir, classpath, dependentCp);

        /*
         * We pass --core-library so that we can write tests in the
         * same package they're testing, even when that's a core
         * library package. If you're actually just using this tool to
         * execute arbitrary code, this has the unfortunate
         * side-effect of preventing "dx" from protecting you from
         * yourself.
         *
         * Memory options pulled from build/core/definitions.mk to
         * handle large dx input when building dex for APK.
         */

        Command.Builder builder = new Command.Builder(log);
        switch (dexer) {
            case DX:
                builder.args(DX_COMMAND_NAME);
                break;
            case D8:
                builder.args(D8_COMMAND_NAME);
                break;
        }
        builder.args("-JXms16M")
                .args("-JXmx1536M")
                .args("--min-sdk-version=" + language.getMinApiLevel());
        if (multidex) {
            builder.args("--multi-dex");
        }
        builder.args("--dex")
                .args("--output=" + output)
                .args("--core-library")
                .args(desugarOutputFilePaths);
        builder.execute();

        if (dexer == Dexer.D8 && output.toString().endsWith(".jar")) {
            try {
                fixD8JarOutput(output, desugarOutputFilePaths);
            } catch (IOException e) {
                throw new RuntimeException("Error while fixing d8 output", e);
            }
        }
        dexCache.insert(cacheKey, output);
    }

    /**
     * Produces an output file like dx does. dx generates jar files containing all resources present
     * in the input files.
     * d8-compat-dx only produces a jar file containing dex and none of the input resources, and
     * will produce no file at all if there are no .class files to process.
     */
    private static void fixD8JarOutput(File output, List<String> inputs) throws IOException {
        List<String> filesToMerge = new ArrayList<>(inputs);

        // JarOutputStream is not keen on appending entries to existing file so we move the output
        // files if it already exists.
        File outputCopy = null;
        if (output.exists()) {
            outputCopy = new File(output.toString() + ".copy");
            output.renameTo(outputCopy);
            filesToMerge.add(outputCopy.toString());
        }

        byte[] buffer = new byte[4096];
        try (JarOutputStream outputJar = new JarOutputStream(new FileOutputStream(output))) {
            for (String fileToMerge : filesToMerge) {
                copyJarContentExcludingClassFiles(buffer, fileToMerge, outputJar);
            }
        } finally {
            if (outputCopy != null) {
                outputCopy.delete();
            }
        }
    }

    private static void copyJarContentExcludingClassFiles(byte[] buffer, String inputJarName,
            JarOutputStream outputJar) throws IOException {

        try (JarInputStream inputJar = new JarInputStream(new FileInputStream(inputJarName))) {
            for (JarEntry entry = inputJar.getNextJarEntry();
                    entry != null;
                    entry = inputJar.getNextJarEntry()) {
                if (entry.getName().endsWith(".class")) {
                    continue;
                }
                outputJar.putNextEntry(entry);
                int length;
                while ((length = inputJar.read(buffer)) >= 0) {
                    if (length > 0) {
                        outputJar.write(buffer, 0, length);
                    }
                }
                outputJar.closeEntry();
            }
        }
    }

    // Runs desugar on classpath as the input with dependentCp as the classpath_entry.
    // Returns the generated output list of files.
    private List<String> desugar(File outputTempDir, Classpath classpath, Classpath dependentCp) {
        Command.Builder builder = new Command.Builder(log)
                .args("java", "-jar", desugarJarPath);

        // Ensure that libcore is on the bootclasspath for desugar,
        // otherwise it tries to use the java command's bootclasspath.
        for (File f : compilationClasspath) {
            builder.args("--bootclasspath_entry", f.getPath());
        }

        // Desugar needs to actively resolve classes that the original inputs
        // were compiled against. Dx does not; so it doesn't use dependentCp.
        for (File f : dependentCp.getElements()) {
            builder.args("--classpath_entry", f.getPath());
        }

        builder.args("--core_library")
                .args("--min_sdk_version", language.getMinApiLevel());

        // Build the -i (input) and -o (output) arguments.
        // Every input from classpath corresponds to a new output temp file into
        // desugarTempDir.
        File desugarTempDir;
        {
            // Generate a temporary list of files that correspond to the 'classpath';
            // desugar will then convert the files in 'classpath' into 'desugarClasspath'.
            if (!outputTempDir.isDirectory()) {
                throw new AssertionError(
                        "outputTempDir must be a directory: " + outputTempDir.getPath());
            }

            String desugarTempDirPath = outputTempDir.getPath() + "/desugar";
            desugarTempDir = new File(desugarTempDirPath);
            desugarTempDir.mkdirs();
            if (!desugarTempDir.exists()) {
                throw new AssertionError(
                        "desugarTempDir; failed to create " + desugarTempDirPath);
            }
        }

        // Create unique file names to support non-unique classpath base names.
        //
        // For example:
        //
        // Classpath("/x/y.jar:/z/y.jar:/a/b.jar") ->
        // Output Files("${tmp}/0y.jar:${tmp}/1y.jar:${tmp}/2b.jar")
        int uniqueCounter = 0;
        List<String> desugarOutputFilePaths = new ArrayList<String>();

        for (File desugarInput : classpath.getElements()) {
            String tmpName = uniqueCounter + desugarInput.getName();
            ++uniqueCounter;

            String desugarOutputPath = desugarTempDir.getPath() + "/" + tmpName;
            desugarOutputFilePaths.add(desugarOutputPath);

            builder.args("-i", desugarInput.getPath())
                    .args("-o", desugarOutputPath);
        }

        builder.execute();

        return desugarOutputFilePaths;
    }

    public void packageApk(File apk, File manifest) {
        new Command(log, "aapt",
                "package",
                "-F", apk.getPath(),
                "-M", manifest.getPath(),
                "-I", androidJarPath,
                "--version-name", "1.0",
                "--version-code", "1").execute();
    }

    public void addToApk(File apk, File dex) {
        new Command(log, "aapt", "add", "-k", apk.getPath(), dex.getPath()).execute();
    }

    public void install(File apk) {
        new Command(log, "adb", "install", "-r", apk.getPath()).execute();
    }

    public void uninstall(String packageName) {
        new Command.Builder(log)
                .args("adb", "uninstall", packageName)
                .permitNonZeroExitStatus(true)
                .execute();
    }
}
