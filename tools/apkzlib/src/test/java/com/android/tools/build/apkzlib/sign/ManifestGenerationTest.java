/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tools.build.apkzlib.sign;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import com.android.tools.build.apkzlib.utils.ApkZFileTestUtils;
import com.android.tools.build.apkzlib.zip.StoredEntry;
import com.android.tools.build.apkzlib.zip.ZFile;
import com.google.common.base.Charsets;
import com.google.common.io.Closer;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.InputStream;
import java.util.HashSet;
import java.util.Set;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public class ManifestGenerationTest {

    private static final String WIKI_PATH = "/testData/packaging/text-files/wikipedia.html";

    @Rule
    public TemporaryFolder mTemporaryFolder = new TemporaryFolder();

    @Test
    public void elementaryManifestGeneration() throws Exception {
        File zip = new File(mTemporaryFolder.getRoot(), "f.zip");

        try (ZFile zf = new ZFile(zip)) {
            zf.add("abc", new ByteArrayInputStream(new byte[]{1}));
            zf.add("x/", new ByteArrayInputStream(new byte[0]));
            zf.add("x/abc", new ByteArrayInputStream(new byte[]{2}));

            ManifestGenerationExtension extension =
                    new ManifestGenerationExtension("Me, of course", "Myself");
            extension.register(zf);

            zf.update();

            StoredEntry se = zf.get("META-INF/MANIFEST.MF");
            assertNotNull(se);

            String text = new String(se.read(), Charsets.US_ASCII);
            text = text.trim();
            String lines[] = text.split(System.getProperty("line.separator"));
            assertEquals(3, lines.length);

            assertEquals("Manifest-Version: 1.0", lines[0].trim());

            Set<String> linesSet = new HashSet<>();
            for (String l : lines) {
                linesSet.add(l.trim());
            }

            assertTrue(linesSet.contains("Built-By: Me, of course"));
            assertTrue(linesSet.contains("Created-By: Myself"));
        }
    }

    @Test
    public void manifestGenerationOnHalfWrittenFile() throws Exception {
        File zip = new File(mTemporaryFolder.getRoot(), "f.zip");
        try (Closer closer = Closer.create()) {
            ZFile zf = closer.register(new ZFile(zip));

            try (InputStream wiki = getClass().getResourceAsStream(WIKI_PATH)) {
                zf.add("wiki", wiki);
            }

            ManifestGenerationExtension extension =
                    new ManifestGenerationExtension("Me, of course", "Myself");
            extension.register(zf);

            zf.close();

            StoredEntry se = zf.get("META-INF/MANIFEST.MF");
            assertNotNull(se);

            String text = new String(se.read(), Charsets.US_ASCII);
            text = text.trim();
            String lines[] = text.split(System.getProperty("line.separator"));
            assertEquals(3, lines.length);

            assertEquals("Manifest-Version: 1.0", lines[0].trim());

            Set<String> linesSet = new HashSet<>();
            for (String l : lines) {
                linesSet.add(l.trim());
            }

            assertTrue(linesSet.contains("Built-By: Me, of course"));
            assertTrue(linesSet.contains("Created-By: Myself"));
        }
    }

    @Test
    public void manifestGenerationOnExistingFile() throws Exception {
        File zip = new File(mTemporaryFolder.getRoot(), "f.zip");
        try (Closer closer = Closer.create()) {
            ZFile zf = closer.register(new ZFile(zip));

            try (InputStream wiki = getClass().getResourceAsStream(WIKI_PATH)) {
                zf.add("wiki", wiki);
            }

            zf.close();

            ManifestGenerationExtension extension =
                    new ManifestGenerationExtension("Me, of course", "Myself");
            extension.register(zf);

            zf.close();

            StoredEntry se = zf.get("META-INF/MANIFEST.MF");
            assertNotNull(se);

            String text = new String(se.read(), Charsets.US_ASCII);
            text = text.trim();
            String lines[] = text.split(System.getProperty("line.separator"));
            assertEquals(3, lines.length);

            assertEquals("Manifest-Version: 1.0", lines[0].trim());

            Set<String> linesSet = new HashSet<>();
            for (String l : lines) {
                linesSet.add(l.trim());
            }

            assertTrue(linesSet.contains("Built-By: Me, of course"));
            assertTrue(linesSet.contains("Created-By: Myself"));
        }
    }

    @Test
    public void manifestGenerationOnIncrementalNoChanges() throws Exception {
        File zip = new File(mTemporaryFolder.getRoot(), "f.zip");
        try (Closer closer = Closer.create()) {
            ZFile zf = closer.register(new ZFile(zip));

            ManifestGenerationExtension extension =
                    new ManifestGenerationExtension("Me, of course", "Myself");
            extension.register(zf);

            try (InputStream wiki = getClass().getResourceAsStream(WIKI_PATH)) {
                zf.add("wiki", wiki);
            }

            zf.close();

            long timeOfWriting = zip.lastModified();

            ApkZFileTestUtils.waitForFileSystemTick(timeOfWriting);

            zf = closer.register(new ZFile(zip));
            zf.close();

            long secondTimeOfWriting = zip.lastModified();
            assertEquals(timeOfWriting, secondTimeOfWriting);
        }
    }
}
