/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.tradefed.util.xml;

import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

/**
 * Unit tests for {@link AndroidManifestWriter}.
 */
public class AndroidManifestWriterTest extends TestCase {

    private static final String TEST_SDK_VERSION = "99";

    /**
     * Success case test for {@link AndroidManifestWriter} when minSdkVersion is already set.
     */
    public void testSetMinSdkVersion() throws Exception {
        assertMinSdkChange("AndroidManifest_usessdk.xml", "AndroidManifest_usessdk_result.xml");
    }

    /**
     * Success case test for {@link AndroidManifestWriter} when minSdkVersion is not present in xml.
     */
    public void testSetMinSdkVersion_missing() throws Exception {
        assertMinSdkChange("AndroidManifest_missing.xml", "AndroidManifest_missing_result.xml");
    }

    /**
     * Negative case test for {@link AndroidManifestWriter} when xml is invalid.
     */
    public void testSetMinSdkVersion_invalid() throws IOException {
        File manifest = extractTestXml("AndroidManifest_invalid.xml");
        try {
            assertNull(AndroidManifestWriter.parse(manifest.getAbsolutePath()));
        } finally {
            FileUtil.deleteFile(manifest);
        }
    }

    private void assertMinSdkChange(String inputFileName, String resultFileName)
            throws IOException {
        File manifest = extractTestXml(inputFileName);
        File expectedResultFile = extractTestXml(resultFileName);
        try {
            AndroidManifestWriter writer = AndroidManifestWriter.parse(manifest.getAbsolutePath());
            assertNotNull(writer);
            writer.setMinSdkVersion(TEST_SDK_VERSION);
            assertTrue(String.format("File contents of %s and %s are not equal", inputFileName,
                    resultFileName), FileUtil.compareFileContents(manifest, expectedResultFile));
        } finally {
            FileUtil.deleteFile(manifest);
            FileUtil.deleteFile(expectedResultFile);
        }
    }

    /**
     * Helper method to extract a test data file from current jar, and store it as a file on local
     * disk.
     *
     * @param fileName the base file name
     * @return the {@link File}
     * @throws IOException
     */
    private File extractTestXml(String fileName) throws IOException {
        InputStream testStream = getClass().getResourceAsStream(File.separator + "xml" +
                File.separator + fileName);
        assertNotNull(testStream);
        File tmpFile = FileUtil.createTempFile(fileName, ".xml");
        FileUtil.writeToFile(testStream, tmpFile);
        return tmpFile;
    }
}
