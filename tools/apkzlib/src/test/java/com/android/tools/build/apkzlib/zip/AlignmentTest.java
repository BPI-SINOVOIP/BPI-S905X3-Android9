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
import static junit.framework.TestCase.assertEquals;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import com.google.common.base.Charsets;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.util.Random;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public class AlignmentTest {

    private static final AlignmentRule SUFFIX_ALIGNMENT_RULES =
            AlignmentRules.compose(
                    // Disable 4-aligning of uncompressed *.u files, so we can more easily
                    // calculate offsets for testing.
                    AlignmentRules.constantForSuffix(".u", 1),
                    AlignmentRules.constantForSuffix(".a", 1024));
    @Rule
    public TemporaryFolder mTemporaryFolder = new TemporaryFolder();

    @Test
    public void addAlignedFile() throws Exception {
        File newZFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte testBytes[] = "This is some text.".getBytes(Charsets.US_ASCII);

        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constantForSuffix(".txt", 1024));
        try (ZFile zf = new ZFile(newZFile, options)) {
            zf.add("test.txt", new ByteArrayInputStream(testBytes), false);
        }

        byte found[] = readSegment(newZFile, 1024, testBytes.length);
        assertArrayEquals(testBytes, found);
    }

    @Test
    public void addNonAlignedFile() throws Exception {
        File newZFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte testBytes[] = "This is some text.".getBytes(Charsets.US_ASCII);

        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constantForSuffix(".txt", 1024));
        try (ZFile zf = new ZFile(newZFile, options)) {
            zf.add("test.txt.foo", new ByteArrayInputStream(testBytes), false);
        }

        assertTrue(newZFile.length() < 1024);
    }

    @Test
    public void realignSingleFile() throws Exception {
        File newZFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte testBytes0[] = "Text number 1".getBytes(Charsets.US_ASCII);
        byte testBytes1[] = "Text number 2, which is actually 1".getBytes(Charsets.US_ASCII);

        long offset0;
        try (ZFile zf = new ZFile(newZFile)) {
            zf.add("file1.txt", new ByteArrayInputStream(testBytes1), false);
            zf.add("file0.txt", new ByteArrayInputStream(testBytes0), false);
            zf.close();

            StoredEntry se0 = zf.get("file0.txt");
            assertNotNull(se0);
            offset0 = se0.getCentralDirectoryHeader().getOffset();

            StoredEntry se1 = zf.get("file1.txt");
            assertNotNull(se1);

            assertTrue(newZFile.length() < 1024);
        }

        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constantForSuffix(".txt", 1024));
        try (ZFile zf = new ZFile(newZFile, options)) {
            StoredEntry se1 = zf.get("file1.txt");
            assertNotNull(se1);
            se1.realign();
            zf.close();

            StoredEntry se0 = zf.get("file0.txt");
            assertNotNull(se0);
            assertEquals(offset0, se0.getCentralDirectoryHeader().getOffset());

            se1 = zf.get("file1.txt");
            assertNotNull(se1);
            assertTrue(se1.getCentralDirectoryHeader().getOffset() > 950);
            assertTrue(se1.getCentralDirectoryHeader().getOffset() < 1024);
            assertArrayEquals(testBytes1, readSegment(newZFile, 1024, testBytes1.length));

            assertTrue(newZFile.length() > 1024);
        }
    }

    @Test
    public void realignFile() throws Exception {
        File newZFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte testBytes0[] = "Text number 1".getBytes(Charsets.US_ASCII);
        byte testBytes1[] = "Text number 2, which is actually 1".getBytes(Charsets.US_ASCII);

        try (ZFile zf = new ZFile(newZFile)) {
            zf.add("file0.txt", new ByteArrayInputStream(testBytes0), false);
            zf.add("file1.txt", new ByteArrayInputStream(testBytes1), false);
        }

        assertTrue(newZFile.length() < 1024);

        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constantForSuffix(".txt", 1024));
        try (ZFile zf = new ZFile(newZFile, options)) {
            zf.realign();
            zf.update();

            StoredEntry se0 = zf.get("file0.txt");
            assertNotNull(se0);
            long off0 = 1024;

            StoredEntry se1 = zf.get("file1.txt");
            assertNotNull(se1);
            long off1 = 2048;

            /*
             * ZFile does not guarantee any order.
             */
            if (se1.getCentralDirectoryHeader().getOffset() <
                    se0.getCentralDirectoryHeader().getOffset()) {
                off0 = 2048;
                off1 = 1024;
            }

            assertArrayEquals(testBytes0, readSegment(newZFile, off0, testBytes0.length));
            assertArrayEquals(testBytes1, readSegment(newZFile, off1, testBytes1.length));
        }
    }

    @Test
    public void realignAlignedEntry() throws Exception {
        File newZFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte testBytes[] = "This is some text.".getBytes(Charsets.US_ASCII);

        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constantForSuffix(".txt", 1024));
        try (ZFile zf = new ZFile(newZFile, options)) {
            zf.add("test.txt", new ByteArrayInputStream(testBytes), false);
        }

        assertArrayEquals(testBytes, readSegment(newZFile, 1024, testBytes.length));

        int flen = (int) newZFile.length();

        try (ZFile zf = new ZFile(newZFile)) {
            StoredEntry entry = zf.get("test.txt");
            assertNotNull(entry);
            assertFalse(entry.realign());
        }

        assertEquals(flen, (int) newZFile.length());
        assertArrayEquals(testBytes, readSegment(newZFile, 1024, testBytes.length));
    }

    @Test
    public void alignmentRulesDoNotAffectAddedFiles() throws Exception {
        File newZFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte testBytes0[] = "Text number 1".getBytes(Charsets.US_ASCII);
        byte testBytes1[] = "Text number 2, which is actually 1".getBytes(Charsets.US_ASCII);

        try (ZFile zf = new ZFile(newZFile)) {
            zf.add("file0.txt", new ByteArrayInputStream(testBytes0), false);
        }

        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constantForSuffix(".txt", 1024));
        try (ZFile zf = new ZFile(newZFile, options)) {
            zf.add("file1.txt", new ByteArrayInputStream(testBytes1), false);
            zf.update();

            StoredEntry se0 = zf.get("file0.txt");
            assertNotNull(se0);

            StoredEntry se1 = zf.get("file1.txt");
            assertNotNull(se1);
            assertArrayEquals(testBytes1, readSegment(newZFile, 1024, testBytes1.length));
        }
    }

    @Test
    public void realignStreamedZip() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte[] pattern = new byte[1024];
        new Random().nextBytes(pattern);

        String name = "";
        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(zipFile))) {
            for (int j = 0; j < 10; j++) {
                name = name + "a";
                ZipEntry ze = new ZipEntry(name);
                zos.putNextEntry(ze);
                for (int i = 0; i < 1000; i++) {
                    zos.write(pattern);
                }
            }
        }

        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constant(10));
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.realign();
        }
    }

    @Test
    public void alignFirstEntryUsingExtraField() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte[] recognizable = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(true);
        options.setAlignmentRule(AlignmentRules.constant(1024));
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("foo", new ByteArrayInputStream(recognizable), false);
        }

        /*
         * Contents should be at 1024 bytes.
         */
        assertArrayEquals(recognizable, readSegment(zipFile, 1024, recognizable.length));

        /*
         * But local header should be in the beginning.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry entry = zf.get("foo");
            assertNotNull(entry);
            assertEquals(0, entry.getCentralDirectoryHeader().getOffset());
        }
    }

    @Test
    public void alignFirstEntryUsingOffset() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte[] recognizable = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(false);
        options.setAlignmentRule(AlignmentRules.constant(1024));
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("foo", new ByteArrayInputStream(recognizable), false);
        }

        /*
         * Contents should be at 1024 bytes.
         */
        assertArrayEquals(recognizable, readSegment(zipFile, 1024, recognizable.length));

        /*
         * Local header should start at 991 (1024 - LOCAL_HEADER_SIZE - 3).
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry entry = zf.get("foo");
            assertNotNull(entry);
            assertEquals(991, entry.getCentralDirectoryHeader().getOffset());
        }
    }

    @Test
    public void alignMiddleEntryUsingExtraField() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte[] recognizable = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(true);
        options.setAlignmentRule(SUFFIX_ALIGNMENT_RULES);
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("first.u", new ByteArrayInputStream(new byte[1024]), false);
            zf.add("middle.a", new ByteArrayInputStream(recognizable), false);
            zf.add("last.u", new ByteArrayInputStream(new byte[1024]), false);
        }

        /*
         * Contents should be at 2048 bytes.
         */
        assertArrayEquals(recognizable, readSegment(zipFile, 2048, recognizable.length));

        /*
         * But local header should be right after the first entry.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry middleEntry = zf.get("middle.a");
            assertNotNull(middleEntry);
            assertEquals(
                    ZFileTestConstants.LOCAL_HEADER_SIZE + "first.u".length() + 1024,
                    middleEntry.getCentralDirectoryHeader().getOffset());
        }
    }

    @Test
    public void alignMiddleEntryUsingOffset() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte[] recognizable = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(false);
        options.setAlignmentRule(AlignmentRules.constantForSuffix(".a", 1024));
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("bar1", new ByteArrayInputStream(new byte[1024]), false);
            zf.add("foo.a", new ByteArrayInputStream(recognizable), false);
            zf.add("bar2", new ByteArrayInputStream(new byte[1024]), false);
        }

        /*
         * Contents should be at 2048 bytes.
         */
        assertArrayEquals(recognizable, readSegment(zipFile, 2048, recognizable.length));

        /*
         * Local header should start at 2015 (2048 - LOCAL_HEADER_SIZE - 5).
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry entry = zf.get("foo.a");
            assertNotNull(entry);
            assertEquals(2013, entry.getCentralDirectoryHeader().getOffset());
        }
    }

    @Test
    public void alignUsingOffsetAllowsSmallSpaces() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        int fixedLh = ZFileTestConstants.LOCAL_HEADER_SIZE + 3;

        byte[] recognizable = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(false);
        options.setAlignmentRule(AlignmentRules.constant(fixedLh));
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("f", new ByteArrayInputStream(recognizable), false);
        }

        assertArrayEquals(recognizable, readSegment(zipFile, fixedLh, recognizable.length));
    }

    @Test
    public void alignUsingExtraFieldDoesNotAllowSmallSpaces() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        int fixedLh = ZFileTestConstants.LOCAL_HEADER_SIZE + 3;

        byte[] recognizable = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(true);
        options.setAlignmentRule(AlignmentRules.constant(fixedLh));
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("f", new ByteArrayInputStream(recognizable), false);
        }

        assertArrayEquals(recognizable, readSegment(zipFile, fixedLh * 2, recognizable.length));
    }

    @Test
    public void extraFieldSpaceUsedForAlignmentCanBeReclaimedBeforeUpdate() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte[] recognizable1 = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };
        byte[] recognizable2 = new byte[] { 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4 };

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(true);
        options.setAlignmentRule(SUFFIX_ALIGNMENT_RULES);
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("f.a", new ByteArrayInputStream(recognizable1), false);
            zf.add("f.u", new ByteArrayInputStream(recognizable2), false);
        }

        assertArrayEquals(recognizable1, readSegment(zipFile, 1024, recognizable1.length));
        assertArrayEquals(
                recognizable2,
                readSegment(
                        zipFile,
                        ZFileTestConstants.LOCAL_HEADER_SIZE + "f.u".length(),
                        recognizable2.length));
    }

    @Test
    @Ignore("See ZFile.readData() contents to understand why this is ignored")
    public void extraFieldSpaceUsedForAlignmentCanBeReclaimedAfterUpdate() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "test.zip");

        byte[] recognizable1 = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };
        byte[] recognizable2 = new byte[] { 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4 };

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(true);
        options.setAlignmentRule(AlignmentRules.constantForSuffix(".a", 1024));
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("f.a", new ByteArrayInputStream(recognizable1), false);
        }

        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("f.b", new ByteArrayInputStream(recognizable2), false);
        }

        assertArrayEquals(recognizable1, readSegment(zipFile, 1024, recognizable1.length));
        assertArrayEquals(
                recognizable2,
                readSegment(
                        zipFile,
                        ZFileTestConstants.LOCAL_HEADER_SIZE + "f.b".length(),
                        recognizable2.length));
    }

    @Test
    public void fillEmptySpaceWithExtraFieldAfterDelete() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "large.zip");

        byte[] recognizable1 = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };
        byte[] recognizable2 = new byte[] { 9, 8, 7, 6, 5, 4, 3, 2 };

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(true);
        options.setAlignmentRule(SUFFIX_ALIGNMENT_RULES);
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("first.u", new ByteArrayInputStream(recognizable1), false);
            zf.add("second.u", new ByteArrayInputStream(recognizable2), false);

            zf.update();

            StoredEntry firstEntry = zf.get("first.u");
            assertNotNull(firstEntry);
            firstEntry.delete();
        }

        try (ZFile zf = new ZFile(zipFile)) {
            Set<StoredEntry> entries = zf.entries();
            assertEquals(1, entries.size());

            StoredEntry entry = entries.iterator().next();
            assertEquals("second.u", entry.getCentralDirectoryHeader().getName());
            assertEquals(0, entry.getCentralDirectoryHeader().getOffset());
            assertEquals(
                    ZFileTestConstants.LOCAL_HEADER_SIZE
                            + "first.u".length()
                            + recognizable1.length,
                    entry.getLocalExtra().size());
        }
    }

    @Test
    public void fillInLargeGapsWithExtraField() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "large.zip");

        byte[] recognizable1 = new byte[] { 1, 2, 3, 4, 4, 3, 2, 1 };
        byte[] recognizable2 = new byte[] { 9, 8, 7, 6, 5, 4, 3, 2 };
        byte[] bigEmpty = new byte[10 * 1024];

        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(true);
        options.setAlignmentRule(SUFFIX_ALIGNMENT_RULES);
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("begin.u", new ByteArrayInputStream(recognizable1), false);
            zf.add("middle.u", new ByteArrayInputStream(bigEmpty), false);
            zf.add("end.u", new ByteArrayInputStream(recognizable2), false);

            zf.update();

            StoredEntry middleEntry = zf.get("middle.u");
            assertNotNull(middleEntry);
            middleEntry.delete();
        }

        /*
         * Find the two recognizable files.
         */
        int recognizable1Start = ZFileTestConstants.LOCAL_HEADER_SIZE + "begin.u".length();
        assertArrayEquals(
                recognizable1,
                readSegment(zipFile, recognizable1Start, recognizable1.length));

        int recognizable2Start =
                3 * ZFileTestConstants.LOCAL_HEADER_SIZE
                        + "begin.u".length()
                        + "middle.u".length()
                        + "end.u".length()
                        + recognizable1.length
                        + bigEmpty.length;
        assertArrayEquals(
                recognizable2,
                readSegment(zipFile, recognizable2Start, recognizable2.length));
    }

    @Test
    public void fillHoleWithExactEntry() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        Random random = new Random();

        byte[] fourtyFour = new byte[44];
        random.nextBytes(fourtyFour);
        byte[] recognizable = new byte[] { 1, 5, 5, 1, 5, 1, 1, 5 };
        byte[] twoHundred = new byte[200];
        random.nextBytes(twoHundred);

        /*
         * Start | Header End | Name end | Contents End | Name
         * 0     | 30         | 59       | 103          | "File taking exactly 103 bytes"
         * 103   | 133        | 136      | 144          | "foo"
         * 144   | 174        | 196      | 396          | "File taking more space"
         */
        try (ZFile zf = new ZFile(zipFile)) {
            zf.add("File taking exactly 103 bytes", new ByteArrayInputStream(fourtyFour), false);
            zf.add("foo", new ByteArrayInputStream(recognizable), false);
            zf.add("File taking more space", new ByteArrayInputStream(twoHundred), false);
        }

        assertArrayEquals(fourtyFour, readSegment(zipFile, 59, fourtyFour.length));
        assertArrayEquals(recognizable, readSegment(zipFile, 136, recognizable.length));
        assertArrayEquals(twoHundred, readSegment(zipFile, 196, twoHundred.length));

        /*
         * Remove the middle file.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry fooEntry = zf.get("foo");
            assertNotNull(fooEntry);
            fooEntry.delete();
        }

        /*
         * Add the file again with 4-byte alignment. Because the file fits exactly in the hole, it
         * is placed there.
         */
        byte[] recognizable2 = new byte[] { 2, 6, 6, 2, 6, 2, 2, 6 };

        ZFileOptions zfo = new ZFileOptions();
        zfo.setCoverEmptySpaceUsingExtraField(true);
        zfo.setAlignmentRule(AlignmentRules.constant(4));
        try (ZFile zf = new ZFile(zipFile, zfo)) {
            zf.add("bar", new ByteArrayInputStream(recognizable2), false);
        }

        assertArrayEquals(fourtyFour, readSegment(zipFile, 59, fourtyFour.length));
        assertArrayEquals(recognizable2, readSegment(zipFile, 136, recognizable2.length));
        assertArrayEquals(twoHundred, readSegment(zipFile, 196, twoHundred.length));
    }

    @Test
    public void fillHoleWithSmallEntry() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        Random random = new Random();

        byte[] fourtyFour = new byte[44];
        random.nextBytes(fourtyFour);
        byte[] recognizable = new byte[] { 1, 5, 5, 1, 5, 1, 1, 5, 1, 5, 5, 1, 5, 1, 1, 5 };
        byte[] twoHundred = new byte[200];
        random.nextBytes(twoHundred);

        /*
         * Start | Header End | Name end | Contents End | Name
         * 0     | 30         | 59       | 103          | "File taking exactly 103 bytes"
         * 103   | 133        | 136      | 152          | "foo"
         * 152   | 182        | 204      | 404          | "File taking more space"
         */
        try (ZFile zf = new ZFile(zipFile)) {
            zf.add("File taking exactly 103 bytes", new ByteArrayInputStream(fourtyFour), false);
            zf.add("foo", new ByteArrayInputStream(recognizable), false);
            zf.add("File taking more space", new ByteArrayInputStream(twoHundred), false);
        }

        assertArrayEquals(fourtyFour, readSegment(zipFile, 59, fourtyFour.length));
        assertArrayEquals(recognizable, readSegment(zipFile, 136, recognizable.length));
        assertArrayEquals(twoHundred, readSegment(zipFile, 204, twoHundred.length));

        /*
         * Remove the middle file.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry fooEntry = zf.get("foo");
            assertNotNull(fooEntry);
            fooEntry.delete();
        }

        /*
         * Add a smaller file. It should fit nicely as:
         *
         * Start | Header End | Name end | Contents End | Name
         * 0     | 30         | 59       | 103          | "File taking exactly 103 bytes"
         * 103   | 133        | 136      | 140          | "bar"
         * 140 - 152 (empty)
         * 152   | 182        | 204      | 404          | "File taking more space"
         */
        byte[] recognizable2 = new byte[] { 7, 7, 7, 7 };

        ZFileOptions zfo = new ZFileOptions();
        zfo.setCoverEmptySpaceUsingExtraField(true);
        zfo.setAlignmentRule(AlignmentRules.constant(4));
        try (ZFile zf = new ZFile(zipFile, zfo)) {
            zf.add("bar", new ByteArrayInputStream(recognizable2), false);
        }

        assertArrayEquals(fourtyFour, readSegment(zipFile, 59, fourtyFour.length));
        assertArrayEquals(recognizable2, readSegment(zipFile, 136, recognizable2.length));
        assertArrayEquals(twoHundred, readSegment(zipFile, 204, twoHundred.length));
    }

    @Test
    public void fillHoleWithSmallerEntryNotEnoughFreeSpace() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        Random random = new Random();

        byte[] fourtyFour = new byte[44];
        random.nextBytes(fourtyFour);
        byte[] recognizable = new byte[] { 1, 5, 5, 1, 5, 1, 1, 5, 1, 5, 5, 1, 5, 1, 1, 5 };
        byte[] twoHundred = new byte[200];
        random.nextBytes(twoHundred);

        /*
         * Start | Header End | Name end | Contents End | Name
         * 0     | 30         | 59       | 103          | "File taking exactly 103 bytes"
         * 103   | 133        | 136      | 152          | "foo"
         * 152   | 182        | 204      | 404          | "File taking more space"
         */
        try (ZFile zf = new ZFile(zipFile)) {
            zf.add("File taking exactly 103 bytes", new ByteArrayInputStream(fourtyFour), false);
            zf.add("foo", new ByteArrayInputStream(recognizable), false);
            zf.add("File taking more space", new ByteArrayInputStream(twoHundred), false);
        }

        assertArrayEquals(fourtyFour, readSegment(zipFile, 59, fourtyFour.length));
        assertArrayEquals(recognizable, readSegment(zipFile, 136, recognizable.length));
        assertArrayEquals(twoHundred, readSegment(zipFile, 204, twoHundred.length));

        /*
         * Remove the middle file.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry fooEntry = zf.get("foo");
            assertNotNull(fooEntry);
            fooEntry.delete();
        }

        /*
         * Add a smaller file. But it can't fit because it would leave less than 6 bytes to
         * cover in the next file:
         *
         * Start | Header End | Name end | Contents End | Name
         * 0     | 30         | 59       | 103          | "File taking exactly 103 bytes"
         * 103   | 133        | 136      | 148          | "foo"
         * 148 - 152 (empty)
         * 152   | 182        | 204      | 404          | "File taking more space"
         *
         * So we end up with:
         *
         * Start | Header End | Name end | Contents End | Name
         * 0     | 30         | 59       | 103          | "File taking exactly 103 bytes"
         * 152   | 182        | 204      | 404          | "File taking more space"
         * 404   | 434 -> 441 | 444      | 456          | "bar"
         */
        byte[] recognizable2 = new byte[] { 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9 };

        ZFileOptions zfo = new ZFileOptions();
        zfo.setCoverEmptySpaceUsingExtraField(true);
        zfo.setAlignmentRule(AlignmentRules.constant(4));
        try (ZFile zf = new ZFile(zipFile, zfo)) {
            zf.add("bar", new ByteArrayInputStream(recognizable2), false);
        }

        assertArrayEquals(fourtyFour, readSegment(zipFile, 59, fourtyFour.length));
        assertArrayEquals(recognizable2, readSegment(zipFile, 444, recognizable2.length));
        assertArrayEquals(twoHundred, readSegment(zipFile, 204, twoHundred.length));
    }

    @Test
    public void fillHoleWithSmallerEntryEnoughFreeSpaceButRequiresExtraOffset() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        Random random = new Random();

        byte[] fourtyFour = new byte[44];
        random.nextBytes(fourtyFour);
        byte[] recognizable = new byte[] { 1, 5, 5, 1, 5, 1, 1, 5, 1, 5, 5, 1, 5, 1, 1, 5 };
        byte[] twoHundred = new byte[200];
        random.nextBytes(twoHundred);

        /*
         * Start | Header End | Name end | Contents End | Name
         * 0     | 30         | 59       | 103          | "File taking exactly 103 bytes"
         * 103   | 133        | 136      | 152          | "foo"
         * 152   | 182        | 204      | 404          | "File taking more space"
         */
        try (ZFile zf = new ZFile(zipFile)) {
            zf.add("File taking exactly 103 bytes", new ByteArrayInputStream(fourtyFour), false);
            zf.add("foo", new ByteArrayInputStream(recognizable), false);
            zf.add("File taking more space", new ByteArrayInputStream(twoHundred), false);
        }

        assertArrayEquals(fourtyFour, readSegment(zipFile, 59, fourtyFour.length));
        assertArrayEquals(recognizable, readSegment(zipFile, 136, recognizable.length));
        assertArrayEquals(twoHundred, readSegment(zipFile, 204, twoHundred.length));

        /*
         * Remove the middle file.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            StoredEntry fooEntry = zf.get("foo");
            assertNotNull(fooEntry);
            fooEntry.delete();
        }

        /*
         * Add a smaller file. It will fit, but not aligned at 140 because that would require
         * adding less than 6 bytes in the local header. It has to move to 150.
         *
         * Start | Header End | Name end | Contents End | Name
         * 0     | 30         | 59       | 103          | "File taking exactly 103 bytes"
         * 103   | 133        | 150      | 152          | "foo"
         * 152   | 182        | 204      | 404          | "File taking more space"
         */
        byte[] recognizable2 = new byte[] { 10, 10 };

        ZFileOptions zfo = new ZFileOptions();
        zfo.setCoverEmptySpaceUsingExtraField(true);
        zfo.setAlignmentRule(AlignmentRules.constant(10));
        try (ZFile zf = new ZFile(zipFile, zfo)) {
            zf.add("bar", new ByteArrayInputStream(recognizable2), false);
        }

        assertArrayEquals(fourtyFour, readSegment(zipFile, 59, fourtyFour.length));
        assertArrayEquals(recognizable2, readSegment(zipFile, 150, recognizable2.length));
        assertArrayEquals(twoHundred, readSegment(zipFile, 204, twoHundred.length));
    }

    @Test
    public void alignCoveringEmptySpaceWhenExtraFieldIsInvalid() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(true);
        options.setAlignmentRule(AlignmentRules.constant(100));
        try (ZFile zf = new ZFile(zipFile, options)) {
            zf.add("foo", new ByteArrayInputStream(new byte[] { 1, 2, 3, 4 }));
            StoredEntry foo = zf.get("foo");
            assertNotNull(foo);
            foo.setLocalExtra(new ExtraField(new byte[] { 0, 0 }));
            zf.add("bar", new ByteArrayInputStream(new byte[] { 5, 6, 7, 8 }));
        }
    }

    @Test
    public void fourByteAlignment() throws Exception {
        // When aligning with 4 bytes, there are are only 3 possible cases:
        // - We're 2 bytes short and so need to add +6 bytes (6 bytes for header + no zeroes)
        // - We're 3 bytes short and so need to add +7 bytes (6 bytes for header + 1 zero)
        // - We're 1 byte short and so need to add +9 bytes (6 bytes for header + 3 zeroes)

        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        ZFileOptions options = new ZFileOptions();
        options.setCoverEmptySpaceUsingExtraField(true);
        options.setAlignmentRule(AlignmentRules.constant(4));
        try (ZFile zf = new ZFile(zipFile, options)) {
            // File header starts at 0.
            // File name starts at 30 (LOCAL_HEADER_SIZE).
            // If unaligned we would have data starting at 33, but with aligned we have data
            // starting at 40 (36 isn't enough for the extra data header).
            String fooName = "foo";
            byte[] fooData = new byte[] { 1, 2, 3, 4, 5 };
            zf.add(fooName, new ByteArrayInputStream(fooData), false);
            zf.update();
            StoredEntry foo = zf.get(fooName);
            long fooOffset = ZFileTestConstants.LOCAL_HEADER_SIZE + fooName.length() + 7;
            assertEquals(fooOffset, foo.getLocalHeaderSize());

            // Bar header starts at 45 (foo data starts at 40 and is 5 bytes long).
            // Bar header ends at 75.
            // If unaligned we would have data starting at 78, but with aligned we have data
            // starting at 84 (80 isn't enough for the extra header).
            String barName = "bar";
            byte[] barData = new byte[] { 6 };
            zf.add(barName, new ByteArrayInputStream(barData), false);
            zf.update();

            StoredEntry bar = zf.get(barName);
            long barStart = bar.getCentralDirectoryHeader().getOffset();
            assertEquals(fooOffset + fooData.length, barStart);

            long barStartOffset = ZFileTestConstants.LOCAL_HEADER_SIZE + barName.length() + 6;
            assertEquals(barStartOffset, bar.getLocalHeaderSize());

            // Xpto header starts at 85 (bar data starts at 84 and is 1 byte long).
            // Xpto header ends at 115.
            // If unaligned we would have data starting at 119, but with aligned we have data
            // starting at 128 (120 & 124 are not enough for the extra header).
            String xptoName = "xpto";
            byte[] xptoData = new byte[] { 7, 8, 9, 10 };
            zf.add(xptoName, new ByteArrayInputStream(xptoData), false);
            zf.update();

            StoredEntry xpto = zf.get(xptoName);
            long xptoStart = xpto.getCentralDirectoryHeader().getOffset();
            assertEquals(barStart + barStartOffset + barData.length, xptoStart);

            long xptoStartOffset = ZFileTestConstants.LOCAL_HEADER_SIZE + xptoName.length() + 9;
            assertEquals(xptoStartOffset, xpto.getLocalHeaderSize());

            // Dummy header starts at 133 (xpto data starts at 128 and is 6 bytes long).
            String dummyName = "dummy";
            byte[] dummyData = new byte[] { 11 };
            zf.add(dummyName, new ByteArrayInputStream(dummyData), false);
            zf.update();

            StoredEntry dummy = zf.get(dummyName);
            long dummyStart = dummy.getCentralDirectoryHeader().getOffset();
            assertEquals(xptoStart + xptoStartOffset + xptoData.length, dummyStart);
        }
    }
}
