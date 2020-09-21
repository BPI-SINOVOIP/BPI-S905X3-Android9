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

package com.android.cts.api;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Stream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.junit.Test;
import org.junit.runner.RunWith;

import com.android.compatibility.common.util.PropertyUtil;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.DeviceTestCase;

/**
 * Ensures that java modules in vendor partition on the device are not using any non-approved APIs
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class UnofficialApisUsageTest extends DeviceTestCase {
    public final static boolean DEBUG = true;
    private ITestDevice device;
    private FilePuller filePuller;

    @Override
    protected void setUp() throws Exception {
        device = getDevice();
        filePuller = new FilePuller(device);
    }

    @Override
    protected void tearDown() throws Exception {
        filePuller.clean();
    }

    private static class JavaModule {
        enum Type {
            JAR, APK,
        }

        public final String name;
        public final String path;
        public final Type type;

        private JavaModule(String name, String path, Type type) {
            this.name = name;
            this.path = path;
            this.type = type;
        }

        public static JavaModule newPackageFromLine(String line) {
            // package:/path/to/apk=com.foo.bar
            line = line.split(":")[1]; // filter-out "package:" prefix
            int separatorPos = line.lastIndexOf('=');
            String path = line.substring(0, separatorPos);
            String name = line.substring(separatorPos + 1);
            return new JavaModule(name, path, Type.APK);
        }

        public static JavaModule newLibraryFromLine(String line) {
            // com.foo.bar -> (jar) /path/to/jar
            String[] tokens = line.trim().split(" ");
            String name = tokens[0];
            String type = tokens[3];
            String path = tokens[4];
            return new JavaModule(name, path, type.equals("(jar)") ? Type.JAR : Type.APK);
        }
    }

    private Stream<JavaModule> getPackages() throws DeviceNotAvailableException {
        return Arrays.stream(device.executeShellCommand("cmd package list packages -f").split("\n"))
                .map(line -> JavaModule.newPackageFromLine(line));
    }

    private Stream<JavaModule> getLibraries() throws DeviceNotAvailableException {
        // cmd package list libraries only shows the name of the libraries, but not their paths.
        // Until it shows the paths as well, let's use the old dumpsys package.
        return Arrays.stream(device.executeShellCommand("dumpsys package libraries").split("\n"))
                .skip(1) // for "Libraries:" header
                .map(line -> JavaModule.newLibraryFromLine(line))
                .filter(module -> module.type == JavaModule.Type.JAR); // only jars
    }

    private String getRealPath(String path) {
        try {
            return device.executeShellCommand(String.format("realpath %s", path));
        } catch (DeviceNotAvailableException e) {
            throw new RuntimeException("Cannot connect to device", e);
        }
    }

    /**
     * Tests whether the downloaded file has code or not, by examining the existence of classes.dex
     * in it
     */
    private boolean hasCode(File file) {
        try {
            ZipFile zipFile = null;
            try {
                zipFile = new ZipFile(file);
                Enumeration<? extends ZipEntry> entries = zipFile.entries();
                while (entries.hasMoreElements()) {
                    ZipEntry e = entries.nextElement();
                    if (e.getName().equals("classes.dex")) {
                        return true;
                    }
                }
            } finally {
                if (zipFile != null) {
                    zipFile.close();
                }
            }
            return false;
        } catch (IOException e) {
            throw new RuntimeException("Error while examining whether code is in " + file, e);
        }
    }

    /**
     * These tests are required only for the Treble-ized devices launching with P or later.
     *
     * @throws DeviceNotAvailableException
     */
    private boolean isTestRequired() throws DeviceNotAvailableException {
        return PropertyUtil.propertyEquals(device, "ro.treble.enabled", "true")
                && PropertyUtil.getFirstApiLevel(device) > 27 /* O_MR1 */;
    }

    /**
     * The main test. If there is any type/method/field reference to unknown type/method/field, then
     * it indicates that vendors are using non-approved APIs.
     */
    @Test
    public void testNonApiReferences() throws Exception {
        if (!isTestRequired()) {
            return;
        }
        Stream<PulledFile> pulledFiles;
        Stream<File> apiFiles;
        ApprovedApis approvedApis;
        DexAnalyzer extractedApis;

        try {
            // pulls packages and libraries which are in vendor partition and have code.
            pulledFiles = Stream.concat(getPackages(), getLibraries())
                    .filter(module -> getRealPath(module.path).startsWith("/vendor"))
                    .map(module -> filePuller.pullFromDevice(module.path, module.name))
                    .filter(file -> hasCode(file.fileInHost));

        } catch (DeviceNotAvailableException e) {
            throw new RuntimeException("Cannot connect to device", e);
        }

        apiFiles = Arrays.stream(new String[] {
            "/current.txt",
            "/system-current.txt",
            "/android-test-base-current.txt",
            "/android-test-runner-current.txt",
            "/android-test-mock-current.txt",
            "/27.txt"
        }).map(name -> new File(name));

        approvedApis = new ApprovedApis(apiFiles);
        extractedApis = new DexAnalyzer(pulledFiles, approvedApis);

        StringBuilder sb = new StringBuilder(10000);
        extractedApis.collectUndefinedTypeReferences().sorted().forEach(
                ref -> sb.append("Undefined type ref: " + ref.getName() + " from: "
                        + ref.printReferencedFrom() + "\n"));
        extractedApis.collectUndefinedMethodReferences().sorted().forEach(
                ref -> sb.append("Undefined method ref: " + ref.getFullSignature() + " from: "
                        + ref.printReferencedFrom() + "\n"));
        extractedApis.collectUndefinedFieldReferences().sorted().forEach(
                ref -> sb.append("Undefined field ref: " + ref.getFullSignature() + " from: "
                        + ref.printReferencedFrom() + "\n"));
        if (sb.length() != 0) {
            fail(sb.toString());
        }
    }

    /**
     * Ensures that vendor apps are not targeting pre-P SDK.
     */
    @Test
    public void testTargetSdk() throws Exception {
        if (!isTestRequired()) {
            return;
        }
        StringBuilder output = new StringBuilder();
        Pattern p = Pattern.compile(".*targetSdk=(.*)\n");
        getPackages()
                .filter(module -> getRealPath(module.path).startsWith("/vendor"))
                .forEach(module -> {
                    try {
                        // This is very dependent on how the information is shown in the dump.
                        // Sould be checked for amendment whenever the format changes in
                        // dumpPackagesLPr in Settings.java
                        // 1. targetSdk version is shown as 'targetSdk=<number>'.
                        // 2: tail -1 is for the case when an apk is updated in the data partition.
                        // In that case, we get dumps for two packages (first for the updated one,
                        // and then for the original one). Among these two, we use targetSdk for
                        // the original one (the last dump), because the updated one can be
                        // uninstalled or wiped out at any time.
                        String result = device.executeShellCommand(
                                "dumpsys package " + module.name + " | grep targetSdk= | tail -1");
                        Matcher m = p.matcher(result);
                        if (m.matches() && m.groupCount() == 1) {
                            String versionString = m.group(1);
                            int versionInt = Integer.parseInt(versionString);
                            if (versionInt <= 27) {
                                output.append("Vendor package " + module.name
                                        + " is targeting old SDK: " + versionString + "\n");
                            }
                        } else {
                            output.append(
                                    "Failed to get targetSDK for vendor package " + module.name
                                            + "\n");
                        }
                    } catch (DeviceNotAvailableException e) {
                        output.append("Failed to get info for vendor package " + module.name
                                + ". Cause: " + e.getMessage() + "\n");
                    }
                });
        if (output.length() != 0) {
            fail(output.toString());
        }
    }
}
