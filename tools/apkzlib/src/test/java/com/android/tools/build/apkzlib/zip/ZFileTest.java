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

import static com.android.tools.build.apkzlib.utils.ApkZFileTestUtils.readSegment;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tools.build.apkzlib.zip.compress.DeflateExecutionCompressor;
import com.android.tools.build.apkzlib.zip.utils.CloseableByteSource;
import com.android.tools.build.apkzlib.zip.utils.RandomAccessFileUtils;
import com.google.common.base.Charsets;
import com.google.common.base.Strings;
import com.google.common.base.Throwables;
import com.google.common.hash.Hashing;
import com.google.common.io.ByteStreams;
import com.google.common.io.Closer;
import com.google.common.io.Files;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.util.Locale;
import java.util.Random;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.zip.Deflater;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;
import javax.annotation.Nonnull;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public class ZFileTest {
    @Rule
    public TemporaryFolder mTemporaryFolder = new TemporaryFolder();

    @Test
    public void getZipPath() throws Exception {
        File temporaryDir = mTemporaryFolder.getRoot();
        File zpath = new File(temporaryDir, "a");
        try (ZFile zf = new ZFile(zpath)) {
            assertEquals(zpath, zf.getFile());
        }
    }

    @Test
    public void readNonExistingFile() throws Exception {
        File temporaryDir = mTemporaryFolder.getRoot();
        File zf = new File(temporaryDir, "a");
        try (ZFile azf = new ZFile(zf)) {
            azf.touch();
        }

        assertTrue(zf.exists());
    }

    @Test(expected = IOException.class)
    public void readExistingEmptyFile() throws Exception {
        File temporaryDir = mTemporaryFolder.getRoot();
        File zf = new File(temporaryDir, "a");
        Files.write(new byte[0], zf);
        try (ZFile azf = new ZFile(zf)) {
            /*
             * Just open and close.
             */
        }
    }

    @Test
    public void readAlmostEmptyZip() throws Exception {
        File zf = ZipTestUtils.cloneRsrc("empty-zip.zip", mTemporaryFolder);

        try (ZFile azf = new ZFile(zf)) {
            assertEquals(1, azf.entries().size());

            StoredEntry z = azf.get("z/");
            assertNotNull(z);
            assertSame(StoredEntryType.DIRECTORY, z.getType());
        }
    }

    @Test
    public void readZipWithTwoFilesOneDirectory() throws Exception {
        File zf = ZipTestUtils.cloneRsrc("simple-zip.zip", mTemporaryFolder);

        try (ZFile azf = new ZFile(zf)) {
            assertEquals(3, azf.entries().size());

            StoredEntry e0 = azf.get("dir/");
            assertNotNull(e0);
            assertSame(StoredEntryType.DIRECTORY, e0.getType());

            StoredEntry e1 = azf.get("dir/inside");
            assertNotNull(e1);
            assertSame(StoredEntryType.FILE, e1.getType());
            ByteArrayOutputStream e1BytesOut = new ByteArrayOutputStream();
            ByteStreams.copy(e1.open(), e1BytesOut);
            byte e1Bytes[] = e1BytesOut.toByteArray();
            String e1Txt = new String(e1Bytes, Charsets.US_ASCII);
            assertEquals("inside", e1Txt);

            StoredEntry e2 = azf.get("file.txt");
            assertNotNull(e2);
            assertSame(StoredEntryType.FILE, e2.getType());
            ByteArrayOutputStream e2BytesOut = new ByteArrayOutputStream();
            ByteStreams.copy(e2.open(), e2BytesOut);
            byte e2Bytes[] = e2BytesOut.toByteArray();
            String e2Txt = new String(e2Bytes, Charsets.US_ASCII);
            assertEquals("file with more text to allow deflating to be useful", e2Txt);
        }
    }

    @Test
    public void readOnlyZipSupport() throws Exception {
        File testZip = ZipTestUtils.cloneRsrc("empty-zip.zip", mTemporaryFolder);

        assertTrue(testZip.setWritable(false));

        try (ZFile zf = new ZFile(testZip)) {
            assertEquals(1, zf.entries().size());
            assertTrue(zf.getCentralDirectoryOffset() > 0);
            assertTrue(zf.getEocdOffset() > 0);
        }
    }

    @Test
    public void readOnlyV2SignedApkSupport() throws Exception {
        File testZip = ZipTestUtils.cloneRsrc("v2-signed.apk", mTemporaryFolder);

        assertTrue(testZip.setWritable(false));

        try (ZFile zf = new ZFile(testZip)) {
            assertEquals(416, zf.entries().size());
            assertTrue(zf.getCentralDirectoryOffset() > 0);
            assertTrue(zf.getEocdOffset() > 0);
        }
    }

    @Test
    public void compressedFilesReadableByJavaZip() throws Exception {
        File testZip = new File(mTemporaryFolder.getRoot(), "t.zip");

        File wiki = ZipTestUtils
                .cloneRsrc("text-files/wikipedia.html", mTemporaryFolder, "wiki");
        File rfc = ZipTestUtils.cloneRsrc("text-files/rfc2460.txt", mTemporaryFolder, "rfc");
        File lena = ZipTestUtils.cloneRsrc("images/lena.png", mTemporaryFolder, "lena");
        byte[] wikiData = Files.toByteArray(wiki);
        byte[] rfcData = Files.toByteArray(rfc);
        byte[] lenaData = Files.toByteArray(lena);

        try (ZFile zf = new ZFile(testZip)) {
            zf.add("wiki", new ByteArrayInputStream(wikiData));
            zf.add("rfc", new ByteArrayInputStream(rfcData));
            zf.add("lena", new ByteArrayInputStream(lenaData));
        }

        try(ZipFile jz = new ZipFile(testZip)) {
            ZipEntry ze = jz.getEntry("wiki");
            assertNotNull(ze);
            assertEquals(ZipEntry.DEFLATED, ze.getMethod());
            assertTrue(ze.getCompressedSize() < wikiData.length);
            InputStream zeis = jz.getInputStream(ze);
            assertArrayEquals(wikiData, ByteStreams.toByteArray(zeis));
            zeis.close();

            ze = jz.getEntry("rfc");
            assertNotNull(ze);
            assertEquals(ZipEntry.DEFLATED, ze.getMethod());
            assertTrue(ze.getCompressedSize() < rfcData.length);
            zeis = jz.getInputStream(ze);
            assertArrayEquals(rfcData, ByteStreams.toByteArray(zeis));
            zeis.close();

            ze = jz.getEntry("lena");
            assertNotNull(ze);
            assertEquals(ZipEntry.STORED, ze.getMethod());
            assertTrue(ze.getCompressedSize() == lenaData.length);
            zeis = jz.getInputStream(ze);
            assertArrayEquals(lenaData, ByteStreams.toByteArray(zeis));
            zeis.close();
        }
    }

    @Test
    public void removeFileFromZip() throws Exception {
        File zipFile = mTemporaryFolder.newFile("test.zip");

        try(ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            ZipEntry entry = new ZipEntry("foo/");
            entry.setMethod(ZipEntry.STORED);
            entry.setSize(0);
            entry.setCompressedSize(0);
            entry.setCrc(0);
            zos.putNextEntry(entry);
            zos.putNextEntry(new ZipEntry("foo/bar"));
            zos.write(new byte[] { 1, 2, 3, 4 });
            zos.closeEntry();
        }

        try (ZFile zf = new ZFile(zipFile)) {
            assertEquals(2, zf.entries().size());
            for (StoredEntry e : zf.entries()) {
                if (e.getType() == StoredEntryType.FILE) {
                    e.delete();
                }
            }

            zf.update();

            try (ZipInputStream zis = new ZipInputStream(new FileInputStream(zipFile))) {
                ZipEntry e1 = zis.getNextEntry();
                assertNotNull(e1);

                assertEquals("foo/", e1.getName());

                ZipEntry e2 = zis.getNextEntry();
                assertNull(e2);
            }
        }
    }

    @Test
    public void addFileToZip() throws Exception {
        File zipFile = mTemporaryFolder.newFile("test.zip");

        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            ZipEntry fooDir = new ZipEntry("foo/");
            fooDir.setCrc(0);
            fooDir.setCompressedSize(0);
            fooDir.setSize(0);
            fooDir.setMethod(ZipEntry.STORED);
            zos.putNextEntry(fooDir);
            zos.closeEntry();
        }

        ZFile zf = new ZFile(zipFile);
        assertEquals(1, zf.entries().size());

        zf.update();

        try (ZipInputStream zis = new ZipInputStream(new FileInputStream(zipFile))) {
            ZipEntry e1 = zis.getNextEntry();
            assertNotNull(e1);

            assertEquals("foo/", e1.getName());

            ZipEntry e2 = zis.getNextEntry();
            assertNull(e2);
        }
    }

    @Test
    public void createNewZip() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        ZFile zf = new ZFile(zipFile);
        zf.add("foo", new ByteArrayInputStream(new byte[] { 0, 1 }));
        zf.close();

        try (ZipFile jzf = new ZipFile(zipFile)) {
            assertEquals(1, jzf.size());

            ZipEntry fooEntry = jzf.getEntry("foo");
            assertNotNull(fooEntry);
            assertEquals("foo", fooEntry.getName());
            assertEquals(2, fooEntry.getSize());

            InputStream is = jzf.getInputStream(fooEntry);
            assertEquals(0, is.read());
            assertEquals(1, is.read());
            assertEquals(-1, is.read());

            is.close();
        }
    }

    @Test
    public void replaceFileWithSmallerInMiddle() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            zos.putNextEntry(new ZipEntry("file1"));
            zos.write(new byte[] { 1, 2, 3, 4, 5 });
            zos.putNextEntry(new ZipEntry("file2"));
            zos.write(new byte[] { 6, 7, 8 });
            zos.putNextEntry(new ZipEntry("file3"));
            zos.write(new byte[] { 9, 0, 1, 2, 3, 4 });
        }

        int totalSize = (int) zipFile.length();

        try (ZFile zf = new ZFile(zipFile)) {
            assertEquals(3, zf.entries().size());

            StoredEntry file2 = zf.get("file2");
            assertNotNull(file2);
            assertEquals(3, file2.getCentralDirectoryHeader().getUncompressedSize());

            assertArrayEquals(new byte[] { 6, 7, 8 }, file2.read());

            zf.add("file2", new ByteArrayInputStream(new byte[] { 11, 12 }));

            int newTotalSize = (int) zipFile.length();
            assertTrue(newTotalSize + " == " + totalSize, newTotalSize == totalSize);

            file2 = zf.get("file2");
            assertNotNull(file2);
            assertArrayEquals(new byte[] { 11, 12 }, file2.read());
        }

        try (ZFile zf2 = new ZFile(zipFile)) {
            StoredEntry file2 = zf2.get("file2");
            assertNotNull(file2);
            assertArrayEquals(new byte[] { 11, 12 }, file2.read());
        }
    }

    @Test
    public void replaceFileWithSmallerAtEnd() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");
        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            zos.putNextEntry(new ZipEntry("file1"));
            zos.write(new byte[]{1, 2, 3, 4, 5});
            zos.putNextEntry(new ZipEntry("file2"));
            zos.write(new byte[]{6, 7, 8});
            zos.putNextEntry(new ZipEntry("file3"));
            zos.write(new byte[]{9, 0, 1, 2, 3, 4});
        }

        int totalSize = (int) zipFile.length();

        try (ZFile zf = new ZFile(zipFile)) {
            assertEquals(3, zf.entries().size());

            StoredEntry file3 = zf.get("file3");
            assertNotNull(file3);
            assertEquals(6, file3.getCentralDirectoryHeader().getUncompressedSize());

            assertArrayEquals(new byte[]{9, 0, 1, 2, 3, 4}, file3.read());

            zf.add("file3", new ByteArrayInputStream(new byte[]{11, 12}));
            zf.close();

            int newTotalSize = (int) zipFile.length();
            assertTrue(newTotalSize + " < " + totalSize, newTotalSize < totalSize);

            file3 = zf.get("file3");
            assertNotNull(file3);
            assertArrayEquals(new byte[]{11, 12,}, file3.read());
        }

        try (ZFile zf2 = new ZFile(zipFile)) {
            StoredEntry file3 = zf2.get("file3");
            assertNotNull(file3);
            assertArrayEquals(new byte[]{11, 12,}, file3.read());
        }
    }

    @Test
    public void replaceFileWithBiggerAtBegin() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            zos.putNextEntry(new ZipEntry("file1"));
            zos.write(new byte[]{1, 2, 3, 4, 5});
            zos.putNextEntry(new ZipEntry("file2"));
            zos.write(new byte[]{6, 7, 8});
            zos.putNextEntry(new ZipEntry("file3"));
            zos.write(new byte[]{9, 0, 1, 2, 3, 4});
        }

        int totalSize = (int) zipFile.length();
        byte[] newData = new byte[100];

        try (ZFile zf = new ZFile(zipFile)) {
            assertEquals(3, zf.entries().size());

            StoredEntry file1 = zf.get("file1");
            assertNotNull(file1);
            assertEquals(5, file1.getCentralDirectoryHeader().getUncompressedSize());

            assertArrayEquals(new byte[]{1, 2, 3, 4, 5}, file1.read());

            /*
             * Need some data because java zip API uses data descriptors which we don't and makes
             * the entries bigger (meaning just adding a couple of bytes would still fit in the
             * same place).
             */
            Random r = new Random();
            r.nextBytes(newData);

            zf.add("file1", new ByteArrayInputStream(newData));
            zf.close();

            int newTotalSize = (int) zipFile.length();
            assertTrue(newTotalSize + " > " + totalSize, newTotalSize > totalSize);

            file1 = zf.get("file1");
            assertNotNull(file1);
            assertArrayEquals(newData, file1.read());
        }

        try (ZFile zf2 = new ZFile(zipFile)) {
            StoredEntry file1 = zf2.get("file1");
            assertNotNull(file1);
            assertArrayEquals(newData, file1.read());
        }
    }

    @Test
    public void replaceFileWithBiggerAtEnd() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            zos.putNextEntry(new ZipEntry("file1"));
            zos.write(new byte[]{1, 2, 3, 4, 5});
            zos.putNextEntry(new ZipEntry("file2"));
            zos.write(new byte[]{6, 7, 8});
            zos.putNextEntry(new ZipEntry("file3"));
            zos.write(new byte[]{9, 0, 1, 2, 3, 4});
        }

        int totalSize = (int) zipFile.length();
        byte[] newData = new byte[100];

        try (ZFile zf = new ZFile(zipFile)) {
            assertEquals(3, zf.entries().size());

            StoredEntry file3 = zf.get("file3");
            assertNotNull(file3);
            assertEquals(6, file3.getCentralDirectoryHeader().getUncompressedSize());

            assertArrayEquals(new byte[]{9, 0, 1, 2, 3, 4}, file3.read());

            /*
             * Need some data because java zip API uses data descriptors which we don't and makes
             * the entries bigger (meaning just adding a couple of bytes would still fit in the
             * same place).
             */
            Random r = new Random();
            r.nextBytes(newData);

            zf.add("file3", new ByteArrayInputStream(newData));
            zf.close();

            int newTotalSize = (int) zipFile.length();
            assertTrue(newTotalSize + " > " + totalSize, newTotalSize > totalSize);

            file3 = zf.get("file3");
            assertNotNull(file3);
            assertArrayEquals(newData, file3.read());
        }

        try (ZFile zf2 = new ZFile(zipFile)) {
            StoredEntry file3 = zf2.get("file3");
            assertNotNull(file3);
            assertArrayEquals(newData, file3.read());
        }
    }

    @Test
    public void ignoredFilesDuringMerge() throws Exception {
        File zip1 = mTemporaryFolder.newFile("t1.zip");

        try (ZipOutputStream zos1 = new ZipOutputStream(new FileOutputStream(zip1))) {
            zos1.putNextEntry(new ZipEntry("only_in_1"));
            zos1.write(new byte[] { 1, 2 });
            zos1.putNextEntry(new ZipEntry("overridden_by_2"));
            zos1.write(new byte[] { 2, 3 });
            zos1.putNextEntry(new ZipEntry("not_overridden_by_2"));
            zos1.write(new byte[] { 3, 4 });
        }

        File zip2 = mTemporaryFolder.newFile("t2.zip");
        try (ZipOutputStream zos2 = new ZipOutputStream(new FileOutputStream(zip2))) {
            zos2.putNextEntry(new ZipEntry("only_in_2"));
            zos2.write(new byte[] { 4, 5 });
            zos2.putNextEntry(new ZipEntry("overridden_by_2"));
            zos2.write(new byte[] { 5, 6 });
            zos2.putNextEntry(new ZipEntry("not_overridden_by_2"));
            zos2.write(new byte[] { 6, 7 });
            zos2.putNextEntry(new ZipEntry("ignored_in_2"));
            zos2.write(new byte[] { 7, 8 });
        }

        try (
                ZFile zf1 = new ZFile(zip1);
                ZFile zf2 = new ZFile(zip2)) {
            zf1.mergeFrom(zf2, (input) -> input.matches("not.*") || input.matches(".*gnored.*"));

            StoredEntry only_in_1 = zf1.get("only_in_1");
            assertNotNull(only_in_1);
            assertArrayEquals(new byte[]{1, 2}, only_in_1.read());

            StoredEntry overridden_by_2 = zf1.get("overridden_by_2");
            assertNotNull(overridden_by_2);
            assertArrayEquals(new byte[]{5, 6}, overridden_by_2.read());

            StoredEntry not_overridden_by_2 = zf1.get("not_overridden_by_2");
            assertNotNull(not_overridden_by_2);
            assertArrayEquals(new byte[]{3, 4}, not_overridden_by_2.read());

            StoredEntry only_in_2 = zf1.get("only_in_2");
            assertNotNull(only_in_2);
            assertArrayEquals(new byte[]{4, 5}, only_in_2.read());

            StoredEntry ignored_in_2 = zf1.get("ignored_in_2");
            assertNull(ignored_in_2);
        }
    }

    @Test
    public void addingFileDoesNotAddDirectoriesAutomatically() throws Exception {
        File zip = new File(mTemporaryFolder.getRoot(), "z.zip");
        try (ZFile zf = new ZFile(zip)) {
            zf.add("a/b/c", new ByteArrayInputStream(new byte[]{1, 2, 3}));
            zf.update();
            assertEquals(1, zf.entries().size());

            StoredEntry c = zf.get("a/b/c");
            assertNotNull(c);
            assertEquals(3, c.read().length);
        }
    }

    @Test
    public void zipFileWithEocdSignatureInComment() throws Exception {
        File zip = mTemporaryFolder.newFile("f.zip");
        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zip))) {
            zos.putNextEntry(new ZipEntry("a"));
            zos.write(new byte[] { 1, 2, 3 });
            zos.setComment("Random comment with XXXX weird characters. There must be enough "
                    + "characters to survive skipping back the EOCD size.");
        }

        byte zipBytes[] = Files.toByteArray(zip);
        boolean didX4 = false;
        for (int i = 0; i < zipBytes.length - 3; i++) {
            boolean x4 = true;
            for (int j = 0; j < 4; j++) {
                if (zipBytes[i + j] != 'X') {
                    x4 = false;
                    break;
                }
            }

            if (x4) {
                zipBytes[i] = (byte) 0x50;
                zipBytes[i + 1] = (byte) 0x4b;
                zipBytes[i + 2] = (byte) 0x05;
                zipBytes[i + 3] = (byte) 0x06;
                didX4 = true;
                break;
            }
        }

        assertTrue(didX4);

        Files.write(zipBytes, zip);

        try (ZFile zf = new ZFile(zip)) {
            assertEquals(1, zf.entries().size());
            StoredEntry a = zf.get("a");
            assertNotNull(a);
            assertArrayEquals(new byte[]{1, 2, 3}, a.read());
        }
    }

    @Test
    public void addFileRecursively() throws Exception {
        File tdir = mTemporaryFolder.newFolder();
        File tfile = new File(tdir, "blah-blah");
        Files.write("blah", tfile, Charsets.US_ASCII);

        File zip = new File(tdir, "f.zip");
        try (ZFile zf = new ZFile(zip)) {
            zf.addAllRecursively(tfile);

            StoredEntry blahEntry = zf.get("blah-blah");
            assertNotNull(blahEntry);
            String contents = new String(blahEntry.read(), Charsets.US_ASCII);
            assertEquals("blah", contents);
        }
    }

    @Test
    public void addDirectoryRecursively() throws Exception {
        File tdir = mTemporaryFolder.newFolder();

        String boom = Strings.repeat("BOOM!", 100);
        String kaboom = Strings.repeat("KABOOM!", 100);

        Files.write(boom, new File(tdir, "danger"), Charsets.US_ASCII);
        Files.write(kaboom, new File(tdir, "do not touch"), Charsets.US_ASCII);
        File safeDir = new File(tdir, "safe");
        assertTrue(safeDir.mkdir());

        String iLoveChocolate = Strings.repeat("I love chocolate! ", 200);
        String iLoveOrange = Strings.repeat("I love orange! ", 50);
        String loremIpsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean vitae "
                + "turpis quis justo scelerisque vulputate in et magna. Suspendisse eleifend "
                + "ultricies nisi, placerat consequat risus accumsan et. Pellentesque habitant "
                + "morbi tristique senectus et netus et malesuada fames ac turpis egestas. "
                + "Integer vitae leo purus. Nulla facilisi. Duis ligula libero, lacinia a "
                + "malesuada a, viverra tempor sapien. Donec eget consequat sapien, ultrices"
                + "interdum diam. Maecenas ipsum erat, suscipit at iaculis a, mollis nec risus. "
                + "Quisque tristique ac velit sed auctor. Nulla lacus diam, tristique id sem non, "
                + "pellentesque commodo mauris.";

        Files.write(iLoveChocolate, new File(safeDir, "eat.sweet"), Charsets.US_ASCII);
        Files.write(iLoveOrange, new File(safeDir, "eat.fruit"), Charsets.US_ASCII);
        Files.write(loremIpsum, new File(safeDir, "bedtime.reading.txt"), Charsets.US_ASCII);

        File zip = new File(tdir, "f.zip");
        try (ZFile zf = new ZFile(zip)) {
            zf.addAllRecursively(tdir, (f) -> !f.getName().startsWith("eat."));

            assertEquals(6, zf.entries().size());

            StoredEntry boomEntry = zf.get("danger");
            assertNotNull(boomEntry);
            assertEquals(CompressionMethod.DEFLATE,
                    boomEntry.getCentralDirectoryHeader().getCompressionInfoWithWait().getMethod());
            assertEquals(boom, new String(boomEntry.read(), Charsets.US_ASCII));

            StoredEntry kaboomEntry = zf.get("do not touch");
            assertNotNull(kaboomEntry);
            assertEquals(CompressionMethod.DEFLATE,
                    kaboomEntry
                            .getCentralDirectoryHeader()
                            .getCompressionInfoWithWait()
                            .getMethod());
            assertEquals(kaboom, new String(kaboomEntry.read(), Charsets.US_ASCII));

            StoredEntry safeEntry = zf.get("safe/");
            assertNotNull(safeEntry);
            assertEquals(0, safeEntry.read().length);

            StoredEntry choc = zf.get("safe/eat.sweet");
            assertNotNull(choc);
            assertEquals(CompressionMethod.STORE,
                    choc.getCentralDirectoryHeader().getCompressionInfoWithWait().getMethod());
            assertEquals(iLoveChocolate, new String(choc.read(), Charsets.US_ASCII));

            StoredEntry orangeEntry = zf.get("safe/eat.fruit");
            assertNotNull(orangeEntry);
            assertEquals(CompressionMethod.STORE,
                    orangeEntry
                            .getCentralDirectoryHeader()
                            .getCompressionInfoWithWait()
                            .getMethod());
            assertEquals(iLoveOrange, new String(orangeEntry.read(), Charsets.US_ASCII));

            StoredEntry loremEntry = zf.get("safe/bedtime.reading.txt");
            assertNotNull(loremEntry);
            assertEquals(CompressionMethod.DEFLATE,
                    loremEntry
                            .getCentralDirectoryHeader()
                            .getCompressionInfoWithWait()
                            .getMethod());
            assertEquals(loremIpsum, new String(loremEntry.read(), Charsets.US_ASCII));
        }
    }

    @Test
    public void extraDirectoryOffsetEmptyFile() throws Exception {
        File zipNoOffsetFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        File zipWithOffsetFile = new File(mTemporaryFolder.getRoot(), "b.zip");

        int offset = 31;

        long zipNoOffsetSize;
        try (
                ZFile zipNoOffset = new ZFile(zipNoOffsetFile);
                ZFile zipWithOffset = new ZFile(zipWithOffsetFile)) {
            zipWithOffset.setExtraDirectoryOffset(offset);

            zipNoOffset.close();
            zipWithOffset.close();

            zipNoOffsetSize = zipNoOffsetFile.length();
            long zipWithOffsetSize = zipWithOffsetFile.length();

            assertEquals(zipNoOffsetSize + offset, zipWithOffsetSize);

            /*
             * EOCD with no comment has 22 bytes.
             */
            assertEquals(0, zipNoOffset.getCentralDirectoryOffset());
            assertEquals(0, zipNoOffset.getCentralDirectorySize());
            assertEquals(0, zipNoOffset.getEocdOffset());
            assertEquals(ZFileTestConstants.EOCD_SIZE, zipNoOffset.getEocdSize());
            assertEquals(offset, zipWithOffset.getCentralDirectoryOffset());
            assertEquals(0, zipWithOffset.getCentralDirectorySize());
            assertEquals(offset, zipWithOffset.getEocdOffset());
            assertEquals(ZFileTestConstants.EOCD_SIZE, zipWithOffset.getEocdSize());
        }

        /*
         * The EOCDs should not differ up until the end of the Central Directory size and should
         * not differ after the offset
         */
        int p1Start = 0;
        int p1Size = Eocd.F_CD_SIZE.endOffset();
        int p2Start = Eocd.F_CD_OFFSET.endOffset();
        int p2Size = (int) zipNoOffsetSize - p2Start;

        byte[] noOffsetData1 = readSegment(zipNoOffsetFile, p1Start, p1Size);
        byte[] noOffsetData2 = readSegment(zipNoOffsetFile, p2Start, p2Size);
        byte[] withOffsetData1 = readSegment(zipWithOffsetFile, offset, p1Size);
        byte[] withOffsetData2 = readSegment(zipWithOffsetFile, offset + p2Start, p2Size);

        assertArrayEquals(noOffsetData1, withOffsetData1);
        assertArrayEquals(noOffsetData2, withOffsetData2);

        try (ZFile readWithOffset = new ZFile(zipWithOffsetFile)) {
            assertEquals(0, readWithOffset.entries().size());
        }
    }

    @Test
    public void extraDirectoryOffsetNonEmptyFile() throws Exception {
        File zipNoOffsetFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        File zipWithOffsetFile = new File(mTemporaryFolder.getRoot(), "b.zip");

        int cdSize;

        // The byte arrays below are larger when compressed, so we end up storing them uncompressed,
        // which would normally cause them to be 4-aligned. Disable that, to make calculations
        // easier.
        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constant(AlignmentRule.NO_ALIGNMENT));

        try (ZFile zipNoOffset = new ZFile(zipNoOffsetFile, options);
                ZFile zipWithOffset = new ZFile(zipWithOffsetFile, options)) {
            zipWithOffset.setExtraDirectoryOffset(37);

            zipNoOffset.add("x", new ByteArrayInputStream(new byte[]{1, 2}));
            zipWithOffset.add("x", new ByteArrayInputStream(new byte[]{1, 2}));

            zipNoOffset.close();
            zipWithOffset.close();

            long zipNoOffsetSize = zipNoOffsetFile.length();
            long zipWithOffsetSize = zipWithOffsetFile.length();

            assertEquals(zipNoOffsetSize + 37, zipWithOffsetSize);

            /*
             * Local file header has 30 bytes + name.
             * Central directory entry has 46 bytes + name
             * EOCD with no comment has 22 bytes.
             */
            assertEquals(ZFileTestConstants.LOCAL_HEADER_SIZE + 1 + 2,
                    zipNoOffset.getCentralDirectoryOffset());
            cdSize = (int) zipNoOffset.getCentralDirectorySize();
            assertEquals(ZFileTestConstants.CENTRAL_DIRECTORY_ENTRY_SIZE + 1, cdSize);
            assertEquals(ZFileTestConstants.LOCAL_HEADER_SIZE + 1 + 2 + cdSize,
                    zipNoOffset.getEocdOffset());
            assertEquals(ZFileTestConstants.EOCD_SIZE, zipNoOffset.getEocdSize());
            assertEquals(ZFileTestConstants.LOCAL_HEADER_SIZE + 1 + 2 + 37,
                    zipWithOffset.getCentralDirectoryOffset());
            assertEquals(cdSize, zipWithOffset.getCentralDirectorySize());
            assertEquals(ZFileTestConstants.LOCAL_HEADER_SIZE + 1 + 2 + 37 + cdSize,
                    zipWithOffset.getEocdOffset());
            assertEquals(ZFileTestConstants.EOCD_SIZE, zipWithOffset.getEocdSize());
        }

        /*
         * The files should be equal: until the end of the first entry, from the beginning of the
         * central directory until the offset field in the EOCD and after the offset field.
         */
        int p1Start = 0;
        int p1Size = ZFileTestConstants.LOCAL_HEADER_SIZE + 1 + 2;
        int p2Start = ZFileTestConstants.LOCAL_HEADER_SIZE + 1 + 2;
        int p2Size = cdSize + Eocd.F_CD_SIZE.endOffset();
        int p3Start = p2Start + cdSize + Eocd.F_CD_OFFSET.endOffset();
        int p3Size = ZFileTestConstants.EOCD_SIZE - Eocd.F_CD_OFFSET.endOffset();

        byte[] noOffsetData1 = readSegment(zipNoOffsetFile, p1Start, p1Size);
        byte[] noOffsetData2 = readSegment(zipNoOffsetFile, p2Start, p2Size);
        byte[] noOffsetData3 = readSegment(zipNoOffsetFile, p3Start, p3Size);
        byte[] withOffsetData1 = readSegment(zipWithOffsetFile, p1Start, p1Size);
        byte[] withOffsetData2 = readSegment(zipWithOffsetFile, 37 + p2Start, p2Size);
        byte[] withOffsetData3 = readSegment(zipWithOffsetFile, 37 + p3Start, p3Size);

        assertArrayEquals(noOffsetData1, withOffsetData1);
        assertArrayEquals(noOffsetData2, withOffsetData2);
        assertArrayEquals(noOffsetData3, withOffsetData3);

        try (ZFile readWithOffset = new ZFile(zipWithOffsetFile)) {
            assertEquals(1, readWithOffset.entries().size());
        }
    }

    @Test
    public void changeExtraDirectoryOffset() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        try (ZFile zip = new ZFile(zipFile)) {
            zip.add("x", new ByteArrayInputStream(new byte[]{1, 2}));
            zip.close();

            long noOffsetSize = zipFile.length();

            zip.setExtraDirectoryOffset(177);
            zip.close();

            long withOffsetSize = zipFile.length();

            assertEquals(noOffsetSize + 177, withOffsetSize);
        }
    }

    @Test
    public void computeOffsetWhenReadingEmptyFile() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        try (ZFile zip = new ZFile(zipFile)) {
            zip.setExtraDirectoryOffset(18);
        }

        try (ZFile zip = new ZFile(zipFile)) {
            assertEquals(18, zip.getExtraDirectoryOffset());
        }
    }

    @Test
    public void computeOffsetWhenReadingNonEmptyFile() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        try (ZFile zip = new ZFile(zipFile)) {
            zip.setExtraDirectoryOffset(287);
            zip.add("x", new ByteArrayInputStream(new byte[]{1, 2}));
        }

        try (ZFile zip = new ZFile(zipFile)) {
            assertEquals(287, zip.getExtraDirectoryOffset());
        }
    }

    @Test
    public void obtainingCDAndEocdWhenEntriesWrittenOnEmptyZip() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        byte[][] cd = new byte[1][];
        byte[][] eocd = new byte[1][];

        try (ZFile zip = new ZFile(zipFile)) {
            zip.addZFileExtension(new ZFileExtension() {
                @Override
                public void entriesWritten() throws IOException {
                    cd[0] = zip.getCentralDirectoryBytes();
                    eocd[0] = zip.getEocdBytes();
                }
            });
        }

        assertNotNull(cd[0]);
        assertEquals(0, cd[0].length);
        assertNotNull(eocd[0]);
        assertEquals(22, eocd[0].length);
    }

    @Test
    public void obtainingCDAndEocdWhenEntriesWrittenOnNonEmptyZip() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        byte[][] cd = new byte[1][];
        byte[][] eocd = new byte[1][];

        try (ZFile zip = new ZFile(zipFile)) {
            zip.add("foo", new ByteArrayInputStream(new byte[0]));
            zip.addZFileExtension(new ZFileExtension() {
                @Override
                public void entriesWritten() throws IOException {
                    cd[0] = zip.getCentralDirectoryBytes();
                    eocd[0] = zip.getEocdBytes();
                }
            });
        }

        /*
         * Central directory entry has 46 bytes + name
         * EOCD with no comment has 22 bytes.
         */
        assertNotNull(cd[0]);
        assertEquals(46 + 3, cd[0].length);
        assertNotNull(eocd[0]);
        assertEquals(22, eocd[0].length);
    }

    @Test
    public void java7JarSupported() throws Exception {
        File jar = ZipTestUtils.cloneRsrc("j7.jar", mTemporaryFolder);

        try (ZFile j = new ZFile(jar)) {
            assertEquals(8, j.entries().size());
        }
    }

    @Test
    public void java8JarSupported() throws Exception {
        File jar = ZipTestUtils.cloneRsrc("j8.jar", mTemporaryFolder);

        try (ZFile j = new ZFile(jar)) {
            assertEquals(8, j.entries().size());
        }
    }

    @Test
    public void utf8NamesSupportedOnReading() throws Exception {
        File zip = ZipTestUtils.cloneRsrc("zip-with-utf8-filename.zip", mTemporaryFolder);

        try (ZFile f = new ZFile(zip)) {
            assertEquals(1, f.entries().size());

            StoredEntry entry = f.entries().iterator().next();
            String filetMignonKorean = "\uc548\uc2eC \uc694\ub9ac";
            String isGoodJapanese = "\u3068\u3066\u3082\u826f\u3044";

            assertEquals(
                    filetMignonKorean + " " + isGoodJapanese,
                    entry.getCentralDirectoryHeader().getName());
            assertArrayEquals(
                    "Stuff about food is good.\n".getBytes(Charsets.US_ASCII), entry.read());
        }
    }

    @Test
    public void utf8NamesSupportedOnReadingWithoutUtf8Flag() throws Exception {
        File zip = ZipTestUtils.cloneRsrc("zip-with-utf8-filename.zip", mTemporaryFolder);

        // Reset bytes 7 and 122 that have the flag in the local header and central directory.
        byte[] data = Files.toByteArray(zip);
        data[7] = 0;
        data[122] = 0;
        Files.write(data, zip);

        try (ZFile f = new ZFile(zip)) {
            assertEquals(1, f.entries().size());

            StoredEntry entry = f.entries().iterator().next();
            String filetMignonKorean = "\uc548\uc2eC \uc694\ub9ac";
            String isGoodJapanese = "\u3068\u3066\u3082\u826f\u3044";

            assertEquals(
                    filetMignonKorean + " " + isGoodJapanese,
                    entry.getCentralDirectoryHeader().getName());
            assertArrayEquals(
                    "Stuff about food is good.\n".getBytes(Charsets.US_ASCII),
                    entry.read());
        }
    }

    @Test
    public void utf8NamesSupportedOnWriting() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        String lettuceIsHealthyArmenian = "\u0533\u0561\u0566\u0561\u0580\u0020\u0561\u057C"
                + "\u0578\u0572\u057B";

        try (ZFile zip = new ZFile(zipFile)) {
            zip.add(lettuceIsHealthyArmenian, new ByteArrayInputStream(new byte[]{0}));
        }

        try (ZFile zip2 = new ZFile(zipFile)) {
            assertEquals(1, zip2.entries().size());
            StoredEntry entry = zip2.entries().iterator().next();
            assertEquals(lettuceIsHealthyArmenian, entry.getCentralDirectoryHeader().getName());
        }
    }

    @Test
    public void zipMemoryUsageIsZeroAfterClose() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        ZFileOptions options = new ZFileOptions();
        long used;
        try (ZFile zip = new ZFile(zipFile, options)) {

            assertEquals(0, options.getTracker().getBytesUsed());
            assertEquals(0, options.getTracker().getMaxBytesUsed());

            zip.add("Blah", new ByteArrayInputStream(new byte[500]));
            used = options.getTracker().getBytesUsed();
            assertTrue(used > 500);
            assertEquals(used, options.getTracker().getMaxBytesUsed());
        }

        assertEquals(0, options.getTracker().getBytesUsed());
        assertEquals(used, options.getTracker().getMaxBytesUsed());
    }

    @Test
    public void unusedZipAreasAreClearedOnWrite() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constantForSuffix(".txt", 1000));
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("test1.txt", new ByteArrayInputStream(new byte[]{1}), false);
        }

        /*
         * Write dummy data in some unused portion of the file.
         */
        try (RandomAccessFile raf = new RandomAccessFile(zipFile, "rw")) {

            raf.seek(500);
            byte[] dummyData = "Dummy".getBytes(Charsets.US_ASCII);
            raf.write(dummyData);
        }

        try (ZFile zf = new ZFile(zipFile)) {
            zf.touch();
        }

        try (RandomAccessFile raf = new RandomAccessFile(zipFile, "r")) {

            /*
             * test1.txt won't take more than 200 bytes. Additionally, the header for
             */
            byte[] data = new byte[900];
            RandomAccessFileUtils.fullyRead(raf, data);

            byte[] zeroData = new byte[data.length];
            assertArrayEquals(zeroData, data);
        }
    }

    @Test
    public void deferredCompression() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        ExecutorService executor = Executors.newSingleThreadExecutor();

        ZFileOptions options = new ZFileOptions();
        boolean[] done = new boolean[1];
        options.setCompressor(new DeflateExecutionCompressor(executor, options.getTracker(),
                Deflater.BEST_COMPRESSION) {
            @Nonnull
            @Override
            protected CompressionResult immediateCompress(@Nonnull CloseableByteSource source)
                    throws Exception {
                Thread.sleep(500);
                CompressionResult cr = super.immediateCompress(source);
                done[0] = true;
                return cr;
            }
        });

        try (ZFile zip = new ZFile(zipFile, options)) {
            byte sequences = 100;
            int seqCount = 1000;
            byte[] compressableData = new byte[sequences * seqCount];
            for (byte i = 0; i < sequences; i++) {
                for (int j = 0; j < seqCount; j++) {
                    compressableData[i * seqCount + j] = i;
                }
            }

            zip.add("compressedFile", new ByteArrayInputStream(compressableData));
            assertFalse(done[0]);

            /*
             * Even before closing, eventually all the stream will be read.
             */
            long tooLong = System.currentTimeMillis() + 10000;
            while (!done[0] && System.currentTimeMillis() < tooLong) {
                Thread.sleep(10);
            }

            assertTrue(done[0]);
        }

        executor.shutdownNow();
    }

    @Test
    public void zipFileWithEocdMarkerInComment() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "x");

        try (Closer closer = Closer.create()) {
            ZipOutputStream zos = closer.register(
                    new ZipOutputStream(new FileOutputStream(zipFile)));
            zos.setComment("\u0065\u4b50");
            zos.putNextEntry(new ZipEntry("foo"));
            zos.write(new byte[] { 1, 2, 3, 4 });
            zos.close();

            ZFile zf = closer.register(new ZFile(zipFile));
            StoredEntry entry = zf.get("foo");
            assertNotNull(entry);
            assertEquals(4, entry.getCentralDirectoryHeader().getUncompressedSize());
        }
    }

    @Test
    public void zipFileWithEocdMarkerInFileName() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "x");

        String fname = "tricky-\u0050\u004b\u0005\u0006";
        byte[] bytes = new byte[] { 1, 2, 3, 4 };

        try (Closer closer = Closer.create()) {
            ZipOutputStream zos = closer.register(
                    new ZipOutputStream(new FileOutputStream(zipFile)));
            zos.putNextEntry(new ZipEntry(fname));
            zos.write(bytes);
            zos.close();

            ZFile zf = closer.register(new ZFile(zipFile));
            StoredEntry entry = zf.get(fname);
            assertNotNull(entry);
            assertEquals(4, entry.getCentralDirectoryHeader().getUncompressedSize());
        }
    }

    @Test
    public void zipFileWithEocdMarkerInFileContents() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "x");

        byte[] bytes = new byte[] { 0x50, 0x4b, 0x05, 0x06 };

        try (Closer closer = Closer.create()) {
            ZipOutputStream zos = closer.register(
                    new ZipOutputStream(new FileOutputStream(zipFile)));
            ZipEntry zipEntry = new ZipEntry("file");
            zipEntry.setMethod(ZipEntry.STORED);
            zipEntry.setCompressedSize(4);
            zipEntry.setSize(4);
            zipEntry.setCrc(Hashing.crc32().hashBytes(bytes).padToLong());
            zos.putNextEntry(zipEntry);
            zos.write(bytes);
            zos.close();

            ZFile zf = closer.register(new ZFile(zipFile));
            StoredEntry entry = zf.get("file");
            assertNotNull(entry);
            assertEquals(4, entry.getCentralDirectoryHeader().getUncompressedSize());
        }
    }

    @Test
    public void replaceVeryLargeFileWithBiggerInMiddleOfZip() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "x");

        long small1Offset;
        long small2Offset;
        ZFileOptions coverOptions = new ZFileOptions();
        coverOptions.setCoverEmptySpaceUsingExtraField(true);
        try (ZFile zf = new ZFile(zipFile, coverOptions)) {
            zf.add("small1", new ByteArrayInputStream(new byte[] { 0, 1 }));
        }

        try (ZFile zf = new ZFile(zipFile, coverOptions)) {
            zf.add("verybig", new ByteArrayInputStream(new byte[100_000]), false);
        }

        try (ZFile zf = new ZFile(zipFile, coverOptions)) {
            zf.add("small2", new ByteArrayInputStream(new byte[] { 0, 1 }));
        }

        try (ZFile zf = new ZFile(zipFile, coverOptions)) {
            StoredEntry se = zf.get("small1");
            assertNotNull(se);
            small1Offset = se.getCentralDirectoryHeader().getOffset();

            se = zf.get("small2");
            assertNotNull(se);
            small2Offset = se.getCentralDirectoryHeader().getOffset();

            se = zf.get("verybig");
            assertNotNull(se);
            se.delete();

            zf.add("evenbigger", new ByteArrayInputStream(new byte[110_000]), false);
        }

        try (ZFile zf = new ZFile(zipFile, coverOptions)) {
            StoredEntry se = zf.get("small1");
            assertNotNull(se);
            assertEquals(se.getCentralDirectoryHeader().getOffset(), small1Offset);

            se = zf.get("small2");
            assertNotNull(se);
            assertNotEquals(se.getCentralDirectoryHeader().getOffset(), small2Offset);
        }
    }

    @Test
    public void regressionRepackingDoesNotFail() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "x");

        ZFileOptions coverOptions = new ZFileOptions();
        coverOptions.setCoverEmptySpaceUsingExtraField(true);
        try (ZFile zf = new ZFile(zipFile, coverOptions)) {
            zf.add("small_1", new ByteArrayInputStream(new byte[] { 0, 1 }));
            zf.add("very_big", new ByteArrayInputStream(new byte[100_000]), false);
            zf.add("small_2", new ByteArrayInputStream(new byte[] { 0, 1 }));
            zf.add("big", new ByteArrayInputStream(new byte[10_000]), false);
            zf.add("small_3", new ByteArrayInputStream(new byte[] { 0, 1 }));
        }

        /*
         * Regression we're covering is that small_2 cannot be extended to cover up for the space
         * taken by very_big and needs to be repositioned. However, the algorithm to reposition
         * will put it in the best-fitting block, which is the one in "big", failing to actually
         * move it backwards in the file.
         */
        try (ZFile zf = new ZFile(zipFile, coverOptions)) {
            StoredEntry se = zf.get("big");
            assertNotNull(se);
            se.delete();

            se = zf.get("very_big");
            assertNotNull(se);
            se.delete();
        }
    }

    @Test
    public void cannotAddMoreThan0x7fffExtraField() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        ZFileOptions zfo = new ZFileOptions();
        zfo.setCoverEmptySpaceUsingExtraField(true);

        /*
         * Create a zip file with:
         *
         * [small file][large file with exactly 0x8000 bytes][small file 2]
         */
        long smallFile1Offset;
        long smallFile2Offset;
        long largeFileOffset;
        String largeFileName = "Large file";
        try (ZFile zf = new ZFile(zipFile, zfo)) {
            zf.add("Small file", new ByteArrayInputStream(new byte[] { 0, 1 }));

            int largeFileTotalSize = 0x8000;
            int largeFileContentsSize =
                    largeFileTotalSize
                            - ZFileTestConstants.LOCAL_HEADER_SIZE
                            - largeFileName.length();

            zf.add(largeFileName, new ByteArrayInputStream(new byte[largeFileContentsSize]), false);
            zf.add("Small file 2", new ByteArrayInputStream(new byte[] { 0, 1 }));

            zf.update();

            StoredEntry sfEntry = zf.get("Small file");
            assertNotNull(sfEntry);
            smallFile1Offset = sfEntry.getCentralDirectoryHeader().getOffset();
            assertEquals(0, smallFile1Offset);

            StoredEntry lfEntry = zf.get(largeFileName);
            assertNotNull(lfEntry);
            largeFileOffset = lfEntry.getCentralDirectoryHeader().getOffset();

            StoredEntry sf2Entry = zf.get("Small file 2");
            assertNotNull(sf2Entry);
            smallFile2Offset = sf2Entry.getCentralDirectoryHeader().getOffset();

            assertEquals(largeFileTotalSize, smallFile2Offset - largeFileOffset);
        }

        /*
         * Remove the large file from the zip file and check that small file 2 has been moved, but
         * no extra field has been added.
         */
        try (ZFile zf = new ZFile(zipFile, zfo)) {
            StoredEntry lfEntry = zf.get(largeFileName);
            assertNotNull(lfEntry);
            lfEntry.delete();

            zf.update();

            StoredEntry sfEntry = zf.get("Small file");
            assertNotNull(sfEntry);
            smallFile1Offset = sfEntry.getCentralDirectoryHeader().getOffset();
            assertEquals(0, smallFile1Offset);

            StoredEntry sf2Entry = zf.get("Small file 2");
            assertNotNull(sf2Entry);
            long newSmallFile2Offset = sf2Entry.getCentralDirectoryHeader().getOffset();
            assertEquals(largeFileOffset, newSmallFile2Offset);

            assertEquals(0, sf2Entry.getLocalExtra().size());
        }
    }

    @Test
    public void canAddMoreThan0x7fffExtraField() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        ZFileOptions zfo = new ZFileOptions();
        zfo.setCoverEmptySpaceUsingExtraField(true);

        /*
         * Create a zip file with:
         *
         * [small file][large file with exactly 0x7fff bytes][small file 2]
         */
        long smallFile1Offset;
        long smallFile2Offset;
        long largeFileOffset;
        String largeFileName = "Large file";
        int largeFileTotalSize = 0x7fff;
        try (ZFile zf = new ZFile(zipFile, zfo)) {
            zf.add("Small file", new ByteArrayInputStream(new byte[] { 0, 1 }));

            int largeFileContentsSize =
                    largeFileTotalSize
                            - ZFileTestConstants.LOCAL_HEADER_SIZE
                            - largeFileName.length();

            zf.add(largeFileName, new ByteArrayInputStream(new byte[largeFileContentsSize]), false);
            zf.add("Small file 2", new ByteArrayInputStream(new byte[] { 0, 1 }));

            zf.update();

            StoredEntry sfEntry = zf.get("Small file");
            assertNotNull(sfEntry);
            smallFile1Offset = sfEntry.getCentralDirectoryHeader().getOffset();
            assertEquals(0, smallFile1Offset);

            StoredEntry lfEntry = zf.get(largeFileName);
            assertNotNull(lfEntry);
            largeFileOffset = lfEntry.getCentralDirectoryHeader().getOffset();

            StoredEntry sf2Entry = zf.get("Small file 2");
            assertNotNull(sf2Entry);
            smallFile2Offset = sf2Entry.getCentralDirectoryHeader().getOffset();

            assertEquals(largeFileTotalSize, smallFile2Offset - largeFileOffset);
        }

        /*
         * Remove the large file from the zip file and check that small file 2 has been moved back
         * but with 0x7fff extra space added.
         */
        try (ZFile zf = new ZFile(zipFile, zfo)) {
            StoredEntry lfEntry = zf.get(largeFileName);
            assertNotNull(lfEntry);
            lfEntry.delete();

            zf.update();

            StoredEntry sfEntry = zf.get("Small file");
            assertNotNull(sfEntry);
            smallFile1Offset = sfEntry.getCentralDirectoryHeader().getOffset();
            assertEquals(0, smallFile1Offset);

            StoredEntry sf2Entry = zf.get("Small file 2");
            assertNotNull(sf2Entry);
            long newSmallFile2Offset = sf2Entry.getCentralDirectoryHeader().getOffset();

            assertEquals(largeFileOffset, newSmallFile2Offset);
            assertEquals(largeFileTotalSize, sf2Entry.getLocalExtra().size());
        }
    }

    @Test
    public void detectIncorrectCRC32InLocalHeader() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        /*
         * Zip files created by ZFile never have data descriptors so we need to create one using
         * java's zip.
         */
        try (
                FileOutputStream fos = new FileOutputStream(zipFile);
                ZipOutputStream zos = new ZipOutputStream(fos)) {
            ZipEntry ze = new ZipEntry("foo");
            zos.putNextEntry(ze);
            byte[] randomBytes = new byte[512];
            new Random().nextBytes(randomBytes);
            zos.write(randomBytes);
        }

        /*
         * Open the zip file and compute where the local header CRC32 is.
         */
        long crcOffset;
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry se = zf.get("foo");
            assertNotNull(se);
            long cdOffset = zf.getCentralDirectoryOffset();

            /*
             * Twelve bytes from the CD offset, we have the start of the CRC32 of the zip entry.
             */
            crcOffset = cdOffset - 12;
        }

        /*
         * Corrupt the CRC32.
         */
        byte[] crc = readSegment(zipFile, crcOffset, 4);
        crc[0]++;
        try (RandomAccessFile raf = new RandomAccessFile(zipFile, "rw")) {
            raf.seek(crcOffset);
            raf.write(crc);
        }

        /*
         * Now open the zip file and it should write a message in the log.
         */
        ZFileOptions options = new ZFileOptions();
        options.setVerifyLogFactory(VerifyLogs::unlimited);
        try (ZFile zf = new ZFile(zipFile, options)) {
            VerifyLog vl = zf.getVerifyLog();
            assertTrue(vl.getLogs().isEmpty());
            StoredEntry fooEntry = zf.get("foo");
            vl = fooEntry.getVerifyLog();
            assertEquals(1, vl.getLogs().size());
            assertTrue(vl.getLogs().get(0).contains("CRC32"));
        }
    }

    @Test
    public void detectIncorrectVersionToExtractInCentralDirectory() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        /*
         * Create a valid zip file.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            zf.add("foo", new ByteArrayInputStream(new byte[0]));
        }

        /*
         * Change the "version to extract" in the central directory to 0x7777.
         */
        int versionToExtractOffset =
                ZFileTestConstants.LOCAL_HEADER_SIZE
                        + 3
                        + CentralDirectory.F_VERSION_EXTRACT.offset();
        byte[] allZipBytes = Files.toByteArray(zipFile);
        allZipBytes[versionToExtractOffset] = 0x77;
        allZipBytes[versionToExtractOffset + 1] = 0x77;
        Files.write(allZipBytes, zipFile);

        /*
         * Opening the file and it should write a message in the log. The entry has the right
         * version to extract (20), but it issues a warning because it is not equal to the one
         * in the central directory.
         */
        ZFileOptions options = new ZFileOptions();
        options.setVerifyLogFactory(VerifyLogs::unlimited);
        try (ZFile zf = new ZFile(zipFile, options)) {
            VerifyLog vl = zf.getVerifyLog();
            assertEquals(1, vl.getLogs().size());
            assertTrue(vl.getLogs().get(0).toLowerCase(Locale.US).contains("version"));
            assertTrue(vl.getLogs().get(0).toLowerCase(Locale.US).contains("extract"));
            StoredEntry fooEntry = zf.get("foo");
            vl = fooEntry.getVerifyLog();
            assertEquals(1, vl.getLogs().size());
            assertTrue(vl.getLogs().get(0).toLowerCase(Locale.US).contains("version"));
            assertTrue(vl.getLogs().get(0).toLowerCase(Locale.US).contains("extract"));
        }
    }

    @Test
    public void detectIncorrectVersionToExtractInLocalHeader() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        /*
         * Create a valid zip file.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            zf.add("foo", new ByteArrayInputStream(new byte[0]));
        }

        /*
         * Change the "version to extract" in the local header to 0x7777.
         */
        int versionToExtractOffset = StoredEntry.F_VERSION_EXTRACT.offset();
        byte[] allZipBytes = Files.toByteArray(zipFile);
        allZipBytes[versionToExtractOffset] = 0x77;
        allZipBytes[versionToExtractOffset + 1] = 0x77;
        Files.write(allZipBytes, zipFile);

        /*
         * Opening the file should log an error message.
         */
        ZFileOptions options = new ZFileOptions();
        options.setVerifyLogFactory(VerifyLogs::unlimited);
        try (ZFile zf = new ZFile(zipFile, options)) {
            VerifyLog vl = zf.getVerifyLog();
            assertTrue(vl.getLogs().isEmpty());
            StoredEntry fooEntry = zf.get("foo");
            vl = fooEntry.getVerifyLog();
            assertEquals(1, vl.getLogs().size());
            assertTrue(vl.getLogs().get(0).toLowerCase(Locale.US).contains("version"));
            assertTrue(vl.getLogs().get(0).toLowerCase(Locale.US).contains("extract"));
        }
    }

    @Test
    public void sortZipContentsWithDeferredCrc32() throws Exception {
        /*
         * Create a zip file with deferred CRC32 and files in non-alphabetical order.
         * ZipOutputStream always creates deferred CRC32 entries.
         */
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            zos.putNextEntry(new ZipEntry("b"));
            zos.write(new byte[1000]);
            zos.putNextEntry(new ZipEntry("a"));
            zos.write(new byte[1000]);
        }

        /*
         * Now open the zip using a ZFile and sort the contents and check that the deferred CRC32
         * bits were reset.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry a = zf.get("a");
            assertNotNull(a);
            assertNotSame(DataDescriptorType.NO_DATA_DESCRIPTOR, a.getDataDescriptorType());
            StoredEntry b = zf.get("b");
            assertNotNull(b);
            assertNotSame(DataDescriptorType.NO_DATA_DESCRIPTOR, b.getDataDescriptorType());
            assertTrue(
                    a.getCentralDirectoryHeader().getOffset()
                            > b.getCentralDirectoryHeader().getOffset());

            zf.sortZipContents();
            zf.update();

            a = zf.get("a");
            assertNotNull(a);
            assertSame(DataDescriptorType.NO_DATA_DESCRIPTOR, a.getDataDescriptorType());
            b = zf.get("b");
            assertNotNull(b);
            assertSame(DataDescriptorType.NO_DATA_DESCRIPTOR, b.getDataDescriptorType());

            assertTrue(
                    a.getCentralDirectoryHeader().getOffset()
                            < b.getCentralDirectoryHeader().getOffset());
        }

        /*
         * Open the file again and check there are no warnings.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            VerifyLog vl = zf.getVerifyLog();
            assertEquals(0, vl.getLogs().size());

            StoredEntry a = zf.get("a");
            assertNotNull(a);
            vl = a.getVerifyLog();
            assertEquals(0, vl.getLogs().size());

            StoredEntry b = zf.get("b");
            assertNotNull(b);
            vl = b.getVerifyLog();
            assertEquals(0, vl.getLogs().size());
        }
    }

    @Test
    public void alignZipContentsWithDeferredCrc32() throws Exception {
        /*
         * Create an unaligned zip file with deferred CRC32 and files in non-alphabetical order.
         * We need an uncompressed file to make realigning have any effect.
         */
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            zos.putNextEntry(new ZipEntry("x"));
            zos.write(new byte[1000]);
            zos.putNextEntry(new ZipEntry("y"));
            zos.write(new byte[1000]);
            ZipEntry zEntry = new ZipEntry("z");
            zEntry.setSize(1000);
            zEntry.setMethod(ZipEntry.STORED);
            zEntry.setCrc(Hashing.crc32().hashBytes(new byte[1000]).asInt());
            zos.putNextEntry(zEntry);
            zos.write(new byte[1000]);
        }

        /*
         * Now open the zip using a ZFile and realign the contents and check that the deferred CRC32
         * bits were reset.
         */
        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constant(2000));
        try (ZFile zf = new ZFile(zipFile, options)) {
            StoredEntry x = zf.get("x");
            assertNotNull(x);
            assertNotSame(DataDescriptorType.NO_DATA_DESCRIPTOR, x.getDataDescriptorType());
            StoredEntry y = zf.get("y");
            assertNotNull(y);
            assertNotSame(DataDescriptorType.NO_DATA_DESCRIPTOR, y.getDataDescriptorType());
            StoredEntry z = zf.get("z");
            assertNotNull(z);
            assertSame(DataDescriptorType.NO_DATA_DESCRIPTOR, z.getDataDescriptorType());

            zf.realign();
            zf.update();

            x = zf.get("x");
            assertNotNull(x);
            assertSame(DataDescriptorType.NO_DATA_DESCRIPTOR, x.getDataDescriptorType());
            y = zf.get("y");
            assertNotNull(y);
            assertSame(DataDescriptorType.NO_DATA_DESCRIPTOR, y.getDataDescriptorType());
            z = zf.get("z");
            assertNotNull(z);
            assertSame(DataDescriptorType.NO_DATA_DESCRIPTOR, z.getDataDescriptorType());
        }
    }

    @Test
    public void openingZFileDoesNotRemoveDataDescriptors() throws Exception {
        /*
         * Create a zip file with deferred CRC32.
         */
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            zos.putNextEntry(new ZipEntry("a"));
            zos.write(new byte[1000]);
        }

        /*
         * Open using ZFile and check that the deferred CRC32 is there.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry se = zf.get("a");
            assertNotNull(se);
            assertNotEquals(DataDescriptorType.NO_DATA_DESCRIPTOR, se.getDataDescriptorType());
        }

        /*
         * Open using ZFile (again) and check that the deferred CRC32 is there.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry se = zf.get("a");
            assertNotNull(se);
            assertNotEquals(DataDescriptorType.NO_DATA_DESCRIPTOR, se.getDataDescriptorType());
        }
    }

    @Test
    public void zipCommentsAreSaved() throws Exception {
        File zipFileWithComments = new File(mTemporaryFolder.getRoot(), "a.zip");
        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFileWithComments))) {
            zos.setComment("foo");
        }

        /*
         * Open the zip file and check the comment is there.
         */
        try (ZFile zf = new ZFile(zipFileWithComments)) {
            byte[] comment = zf.getEocdComment();
            assertArrayEquals(new byte[] { 'f', 'o', 'o' }, comment);

            /*
             * Modify the comment and write the file.
             */
            zf.setEocdComment(new byte[] { 'b', 'a', 'r', 'r' });
        }

        /*
         * Open the file and see that the comment is there (both with java and zfile).
         */
        try (ZipFile zf2 = new ZipFile(zipFileWithComments)) {
            assertEquals("barr", zf2.getComment());
        }

        try (ZFile zf3 = new ZFile(zipFileWithComments)) {
            assertArrayEquals(new byte[] { 'b', 'a', 'r', 'r' }, zf3.getEocdComment());
        }
    }

    @Test
    public void eocdCommentsWithMoreThan64kNotAllowed() throws Exception {
        File zipFileWithComments = new File(mTemporaryFolder.getRoot(), "a.zip");
        try (ZFile zf = new ZFile(zipFileWithComments)) {
            try {
                zf.setEocdComment(new byte[65536]);
                fail();
            } catch (IllegalArgumentException e) {
                // Expected.
            }

            zf.setEocdComment(new byte[65535]);
        }
    }

    @Test
    public void eocdCommentsWithTheEocdMarkerAreAllowed() throws Exception {
        File zipFileWithComments = new File(mTemporaryFolder.getRoot(), "a.zip");
        byte[] data = new byte[100];
        data[50] = 0x50; // Signature
        data[51] = 0x4b;
        data[52] = 0x05;
        data[53] = 0x06;
        data[54] = 0x00; // Number of disk
        data[55] = 0x00;
        data[56] = 0x00; // Disk CD start
        data[57] = 0x00;
        data[54] = 0x01; // Total records 1
        data[55] = 0x00;
        data[56] = 0x02; // Total records 2, must be = to total records 1
        data[57] = 0x00;

        try (ZFile zf = new ZFile(zipFileWithComments)) {
            zf.setEocdComment(data);
        }

        try (ZFile zf = new ZFile(zipFileWithComments)) {
            assertArrayEquals(data, zf.getEocdComment());
        }
    }

    @Test
    public void eocdCommentsWithTheEocdMarkerThatAreInvalidAreNotAllowed() throws Exception {
        File zipFileWithComments = new File(mTemporaryFolder.getRoot(), "a.zip");
        byte[] data = new byte[100];
        data[50] = 0x50;
        data[51] = 0x4b;
        data[52] = 0x05;
        data[53] = 0x06;
        data[67] = 0x00;

        try (ZFile zf = new ZFile(zipFileWithComments)) {
            try {
                zf.setEocdComment(data);
                fail();
            } catch (IllegalArgumentException e) {
                // Expected.
            }
        }
    }

    @Test
    public void zipCommentsArePreservedWithFileChanges() throws Exception {
        File zipFileWithComments = new File(mTemporaryFolder.getRoot(), "a.zip");
        byte[] comment = new byte[] { 1, 3, 4 };
        try (ZFile zf = new ZFile(zipFileWithComments)) {
            zf.add("foo", new ByteArrayInputStream(new byte[50]));
            zf.setEocdComment(comment);
        }

        try (ZFile zf = new ZFile(zipFileWithComments)) {
            assertArrayEquals(comment, zf.getEocdComment());
            zf.add("bar", new ByteArrayInputStream(new byte[100]));
        }

        try (ZFile zf = new ZFile(zipFileWithComments)) {
            assertArrayEquals(comment, zf.getEocdComment());
        }
    }

    @Test
    public void overlappingZipEntries() throws Exception {
        File myZip = ZipTestUtils.cloneRsrc("overlapping.zip", mTemporaryFolder);
        try (ZFile zf = new ZFile(myZip)) {
            fail();
        } catch (IOException e) {
            assertTrue(Throwables.getStackTraceAsString(e).contains("overlapping/bbb"));
            assertTrue(Throwables.getStackTraceAsString(e).contains("overlapping/ddd"));
            assertFalse(Throwables.getStackTraceAsString(e).contains("Central Directory"));
        }
    }

    @Test
    public void overlappingZipEntryWithCentralDirectory() throws Exception {
        File myZip = ZipTestUtils.cloneRsrc("overlapping2.zip", mTemporaryFolder);
        try (ZFile zf = new ZFile(myZip)) {
            fail();
        } catch (IOException e) {
            assertFalse(Throwables.getStackTraceAsString(e).contains("overlapping/bbb"));
            assertTrue(Throwables.getStackTraceAsString(e).contains("overlapping/ddd"));
            assertTrue(Throwables.getStackTraceAsString(e).contains("Central Directory"));
        }
    }

    @Test
    public void readFileWithOffsetBeyondFileEnd() throws Exception {
        File myZip = ZipTestUtils.cloneRsrc("entry-outside-file.zip", mTemporaryFolder);
        try (ZFile zf = new ZFile(myZip)) {
            fail();
        } catch (IOException e) {
            assertTrue(Throwables.getStackTraceAsString(e).contains("entry-outside-file/foo"));
            assertTrue(Throwables.getStackTraceAsString(e).contains("EOF"));
        }
    }
}
