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
package com.android.timezone.distro.tools;

import com.android.timezone.distro.DistroVersion;
import com.android.timezone.distro.TimeZoneDistro;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.Reader;
import java.util.Properties;

/**
 * A command-line tool for creating a time zone update distro file.
 *
 * <p>Args:
 * <dl>
 *     <dt>input properties file</dt>
 *     <dd>the file describing the distro</dd>
 *     <dt>output dir</dt>
 *     <dd>the directory to generate files into</dd>
 * </dl>
 *
 * <p>The input properties file must have the entries:
 * <dl>
 *     <dt>rules.version</dt>
 *     <dd>The IANA rules version.</dd>
 *     <dt>revision</dt>
 *     <dd>The distro revision (typically 1).</dd>
 *     <dt>tzdata.file</dt>
 *     <dd>The location of the tzdata file.</dd>
 *     <dt>icu.file</dt>
 *     <dd>The location of the ICU overlay .dat file.</dd>
 *     <dt>tzlookup.file</dt>
 *     <dd>The location of the tzlookup.xml file.</dd>
 * </dl>
 *
 * <p>The output consists of:
 * <ul>
 *     <li>A distro .zip containing the input files. See
 *     {@link com.android.timezone.distro.TimeZoneDistro}</li>
 * </ul>
 */
public class CreateTimeZoneDistro {

    private CreateTimeZoneDistro() {}

    public static void main(String[] args) throws Exception {
        if (args.length != 2) {
            printUsage();
            System.exit(1);
        }
        File f = new File(args[0]);
        if (!f.exists()) {
            System.err.println("Properties file " + f + " not found");
            printUsage();
            System.exit(2);
        }
        Properties inputProperties = loadProperties(f);
        String ianaRulesVersion = getMandatoryProperty(inputProperties, "rules.version");
        int revision = Integer.parseInt(getMandatoryProperty(inputProperties, "revision"));
        DistroVersion distroVersion = new DistroVersion(
                DistroVersion.CURRENT_FORMAT_MAJOR_VERSION,
                DistroVersion.CURRENT_FORMAT_MINOR_VERSION,
                ianaRulesVersion,
                revision);
        TimeZoneDistroBuilder builder = new TimeZoneDistroBuilder()
                .setDistroVersion(distroVersion)
                .setTzDataFile(getMandatoryPropertyFile(inputProperties, "tzdata.file"))
                .setIcuDataFile(getMandatoryPropertyFile(inputProperties, "icu.file"))
                .setTzLookupFile(getMandatoryPropertyFile(inputProperties, "tzlookup.file"));

        byte[] distroBytes = builder.buildBytes();

        // Write the distro file.
        File outputDir = new File(args[1]);
        File outputDistroFile = new File(outputDir, TimeZoneDistro.FILE_NAME);
        try (OutputStream os = new FileOutputStream(outputDistroFile)) {
            os.write(distroBytes);
        }
        System.out.println("Wrote " + outputDistroFile);
    }

    private static File getMandatoryPropertyFile(Properties p, String propertyName) {
        String fileName = getMandatoryProperty(p, propertyName);
        File file = new File(fileName);
        if (!file.exists()) {
            System.out.println(
                    "Missing file: " + file + " for property " + propertyName + " does not exist.");
            printUsage();
            System.exit(4);
        }
        return file;
    }

    private static String getMandatoryProperty(Properties p, String propertyName) {
        String value = p.getProperty(propertyName);
        if (value == null) {
            System.out.println("Missing property: " + propertyName);
            printUsage();
            System.exit(3);
        }
        return value;
    }

    private static Properties loadProperties(File f) throws IOException {
        Properties p = new Properties();
        try (Reader reader = new InputStreamReader(new FileInputStream(f))) {
            p.load(reader);
        }
        return p;
    }

    private static void printUsage() {
        System.out.println("Usage:");
        System.out.println("\t" + CreateTimeZoneDistro.class.getName() +
                " <tzupdate.properties file> <output dir>");
    }
}
