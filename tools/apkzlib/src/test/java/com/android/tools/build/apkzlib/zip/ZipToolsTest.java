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

package com.android.tools.build.apkzlib.zip;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.google.common.base.Charsets;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Lists;
import com.google.common.collect.Maps;
import com.google.common.io.ByteStreams;
import com.google.common.io.Files;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import org.junit.Assume;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

@RunWith(Parameterized.class)
public class ZipToolsTest {

    @Parameterized.Parameter(0)
    @Nullable
    public String mZipFile;

    @Parameterized.Parameter(1)
    @Nullable
    public List<String> mUnzipCommand;

    @Parameterized.Parameter(2)
    @Nullable
    public String mUnzipLineRegex;

    @Parameterized.Parameter(3)
    public boolean mToolStoresDirectories;

    @Parameterized.Parameter(4)
    public String mName;

    @Rule
    @Nonnull
    public TemporaryFolder mTemporaryFolder = new TemporaryFolder();

    @Parameterized.Parameters(name = "{4} {index}")
    public static Iterable<Object[]> getConfigurations() {
        return Arrays.asList(new Object[][] {
                {
                        "linux-zip.zip",
                        ImmutableList.of("/usr/bin/unzip", "-v"),
                        "^\\s*(?<size>\\d+)\\s+(?:Stored|Defl:N).*\\s(?<name>\\S+)\\S*$",
                        true,
                        "Linux Zip"
                },
                {
                        "windows-7zip.zip",
                        ImmutableList.of("c:\\Program Files\\7-Zip\\7z.exe", "l"),
                        "^(?:\\S+\\s+){3}(?<size>\\d+)\\s+\\d+\\s+(?<name>\\S+)\\s*$",
                        true,
                        "Windows 7-Zip"
                },
                {
                        "windows-cf.zip",
                        ImmutableList.of(
                                "Cannot use compressed folders from cmd line to list zip contents"),
                        "",
                        false,
                        "Windows Compressed Folders"
                }
        });
    }

    private File cloneZipFile() throws Exception {
        File zfile = mTemporaryFolder.newFile("file.zip");
        Files.write(ZipTestUtils.rsrcBytes(mZipFile), zfile);
        return zfile;
    }

    private static void assertFileInZip(@Nonnull ZFile zfile, @Nonnull String name) throws Exception {
        StoredEntry root = zfile.get(name);
        assertNotNull(root);

        InputStream is = root.open();
        byte[] inZipData = ByteStreams.toByteArray(is);
        is.close();

        byte[] inFileData = ZipTestUtils.rsrcBytes(name);
        assertArrayEquals(inFileData, inZipData);
    }

    @Test
    public void zfileReadsZipFile() throws Exception {
        try (ZFile zf = new ZFile(cloneZipFile())) {
            if (mToolStoresDirectories) {
                assertEquals(6, zf.entries().size());
            } else {
                assertEquals(4, zf.entries().size());
            }

            assertFileInZip(zf, "root");
            assertFileInZip(zf, "images/lena.png");
            assertFileInZip(zf, "text-files/rfc2460.txt");
            assertFileInZip(zf, "text-files/wikipedia.html");
        }
    }

    @Test
    public void toolReadsZfFile() throws Exception {
        testReadZFile(false);
    }

    @Test
    public void toolReadsAlignedZfFile() throws Exception {
        testReadZFile(true);
    }

    private void testReadZFile(boolean align) throws Exception {
        String unzipcmd = mUnzipCommand.get(0);
        Assume.assumeTrue(new File(unzipcmd).canExecute());

        ZFileOptions options = new ZFileOptions();
        if (align) {
            options.setAlignmentRule(AlignmentRules.constant(500));
        }

        File zfile = new File (mTemporaryFolder.getRoot(), "zfile.zip");
        try (ZFile zf = new ZFile(zfile, options)) {
            zf.add("root", new ByteArrayInputStream(ZipTestUtils.rsrcBytes("root")));
            zf.add("images/", new ByteArrayInputStream(new byte[0]));
            zf.add(
                    "images/lena.png",
                    new ByteArrayInputStream(ZipTestUtils.rsrcBytes("images/lena.png")));
            zf.add("text-files/", new ByteArrayInputStream(new byte[0]));
            zf.add(
                    "text-files/rfc2460.txt",
                    new ByteArrayInputStream(ZipTestUtils.rsrcBytes("text-files/rfc2460.txt")));
            zf.add(
                    "text-files/wikipedia.html",
                    new ByteArrayInputStream(ZipTestUtils.rsrcBytes("text-files/wikipedia.html")));
        }

        List<String> command = Lists.newArrayList(mUnzipCommand);
        command.add(zfile.getAbsolutePath());
        ProcessBuilder pb = new ProcessBuilder(command);
        Process proc = pb.start();
        InputStream is = proc.getInputStream();
        byte output[] = ByteStreams.toByteArray(is);
        String text = new String(output, Charsets.US_ASCII);
        String lines[] = text.split("\n");
        Map<String, Integer> sizes = Maps.newHashMap();
        for (String l : lines) {
            Matcher m = Pattern.compile(mUnzipLineRegex).matcher(l);
            if (m.matches()) {
                String sizeTxt = m.group("size");
                int size = Integer.parseInt(sizeTxt);
                String name = m.group("name");
                sizes.put(name, size);
            }
        }

        assertEquals(6, sizes.size());

        /*
         * The "images" directory may show up as "images" or "images/".
         */
        String imagesKey = "images/";
        if (!sizes.containsKey(imagesKey)) {
            imagesKey = "images";
        }

        assertTrue(sizes.containsKey(imagesKey));
        assertEquals(0, sizes.get(imagesKey).intValue());

        assertSize(new String[] { "images/", "images" }, 0, sizes);
        assertSize(new String[] { "text-files/", "text-files"}, 0, sizes);
        assertSize(new String[] { "root" }, ZipTestUtils.rsrcBytes("root").length, sizes);
        assertSize(new String[] { "images/lena.png", "images\\lena.png" },
                ZipTestUtils.rsrcBytes("images/lena.png").length, sizes);
        assertSize(new String[] { "text-files/rfc2460.txt", "text-files\\rfc2460.txt" },
                ZipTestUtils.rsrcBytes("text-files/rfc2460.txt").length, sizes);
        assertSize(new String[] { "text-files/wikipedia.html", "text-files\\wikipedia.html" },
                ZipTestUtils.rsrcBytes("text-files/wikipedia.html").length, sizes);
    }

    private static void assertSize(String[] names, long size, Map<String, Integer> sizes) {
        for (String n : names) {
            if (sizes.containsKey(n)) {
                assertEquals((long) sizes.get(n), size);
                return;
            }
        }

        fail();
    }
}
