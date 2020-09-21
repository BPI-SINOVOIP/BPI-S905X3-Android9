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

package com.android.tools.build.apkzlib.zip;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import com.google.common.collect.ImmutableList;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.function.BiConsumer;
import java.util.function.Function;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

/**
 * Test setting, removing and updating the extra field of zip entries.
 */
@RunWith(Parameterized.class)
public class ExtraFieldTest {

    @Rule
    public TemporaryFolder mTemporaryFolder = new TemporaryFolder();

    private File mZipFile;

    @Parameterized.Parameter
    public Function<StoredEntry, ExtraField> mExtraFieldGetter;

    @Parameterized.Parameter(1)
    public BiConsumer<StoredEntry, ExtraField> mExtraFieldSetter;

    @Before
    public final void before() throws Exception {
        mZipFile = mTemporaryFolder.newFile();
        mZipFile.delete();
    }

    @Parameterized.Parameters
    public static ImmutableList<Object[]> getParameters() {
        Function<StoredEntry, ExtraField> localGet = StoredEntry::getLocalExtra;
        BiConsumer<StoredEntry, ExtraField> localSet = (se, ef) -> {
            try {
                se.setLocalExtra(ef);
            } catch (IOException e) {
                throw new AssertionError(e);
            }
        };

        Function<StoredEntry, ExtraField> centralGet =
                se -> se.getCentralDirectoryHeader().getExtraField();
        BiConsumer<StoredEntry, ExtraField> centralSet = (se, ef) -> {
            try {
                se.getCentralDirectoryHeader().setExtraField(ef);
            } catch (Exception e) {
                throw new AssertionError(e);
            }
        };

        return ImmutableList.of(
                new Object[]{ localGet, localSet },
                new Object[]{ centralGet, centralSet });
    }

    @Test
    public void readEntryWithNoExtraField() throws Exception {
        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(mZipFile))) {
            zos.putNextEntry(new ZipEntry("foo"));
            zos.write(new byte[] { 1, 2, 3 });
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry foo = zf.get("foo");
            assertNotNull(foo);
            assertEquals(3, foo.getCentralDirectoryHeader().getUncompressedSize());
            assertEquals(0, mExtraFieldGetter.apply(foo).size());
        }
    }

    @Test
    public void readSingleExtraField() throws Exception {
        /*
         * Header ID: 0x0A0B
         * Data Size: 0x0004
         * Data: 0x01 0x02 0x03 0x04
         *
         * In little endian is:
         *
         * 0xCDAB040001020304
         */
        byte[] extraField = new byte[] { 0x0B, 0x0A, 0x04, 0x00, 0x01, 0x02, 0x03, 0x04 };

        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(mZipFile))) {
            ZipEntry ze = new ZipEntry("foo");
            ze.setExtra(extraField);
            zos.putNextEntry(ze);
            zos.write(new byte[] { 1, 2, 3 });
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry foo = zf.get("foo");
            assertNotNull(foo);
            assertEquals(3, foo.getCentralDirectoryHeader().getUncompressedSize());
            assertEquals(8, mExtraFieldGetter.apply(foo).size());
            ImmutableList<ExtraField.Segment> segments = mExtraFieldGetter.apply(foo).getSegments();
            assertEquals(1, segments.size());
            assertEquals(0x0A0B, segments.get(0).getHeaderId());
            byte[] segData = new byte[8];
            segments.get(0).write(ByteBuffer.wrap(segData));
            assertArrayEquals(extraField, segData);
        }
    }

    @Test
    public void readMultipleExtraFields() throws Exception {
        /*
         * Header ID: 0x0A01
         * Data Size: 0x0002
         * Data: 0x01 0x02
         *
         * Header ID: 0x0A02
         * Data Size: 0x0001
         * Data: 0x03
         *
         * Header ID: 0x0A02
         * Data Size: 0x0001
         * Dataa: 0x04
         *
         * In little endian is:
         *
         * 0x010A02000102 020A010003 020A010004
         */
        byte[] extraField =
                new byte[] {
                        0x01, 0x0A, 0x02, 0x00, 0x01, 0x02,
                        0x02, 0x0A, 0x01, 0x00, 0x03,
                        0x02, 0x0A, 0x01, 0x00, 0x04 };

        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(mZipFile))) {
            ZipEntry ze = new ZipEntry("foo");

            ze.setExtra(extraField);
            zos.putNextEntry(ze);
            zos.write(new byte[] { 1, 2, 3 });
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry foo = zf.get("foo");
            assertNotNull(foo);
            assertEquals(3, foo.getCentralDirectoryHeader().getUncompressedSize());
            assertEquals(16, mExtraFieldGetter.apply(foo).size());
            ImmutableList<ExtraField.Segment> segments = mExtraFieldGetter.apply(foo).getSegments();
            assertEquals(3, segments.size());

            assertEquals(0x0A01, segments.get(0).getHeaderId());
            byte[] segData = new byte[6];
            segments.get(0).write(ByteBuffer.wrap(segData));
            assertArrayEquals(new byte[] { 0x01, 0x0A, 0x02, 0x00, 0x01, 0x02 }, segData);

            assertEquals(0x0A02, segments.get(1).getHeaderId());
            segData = new byte[5];
            segments.get(1).write(ByteBuffer.wrap(segData));
            assertArrayEquals(new byte[] { 0x02, 0x0A, 0x01, 0x00, 0x03 }, segData);

            assertEquals(0x0A02, segments.get(2).getHeaderId());
            segData = new byte[5];
            segments.get(2).write(ByteBuffer.wrap(segData));
            assertArrayEquals(new byte[] { 0x02, 0x0A, 0x01, 0x00, 0x04 }, segData);
        }
    }

    @Test
    public void addExtraFieldToExistingEntry() throws Exception {
        try (ZFile zf = new ZFile(mZipFile)) {
            zf.add("before", new ByteArrayInputStream(new byte[] { 0, 1, 2 }));
            zf.add("extra", new ByteArrayInputStream(new byte[] { 3, 4, 5 }));
            zf.add("after", new ByteArrayInputStream(new byte[] { 6, 7, 8 }));
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry ex = zf.get("extra");
            assertNotNull(ex);
            mExtraFieldSetter.accept(ex,
                    new ExtraField(
                            ImmutableList.of(
                                    new ExtraField.RawDataSegment(
                                            0x7654,
                                            new byte[] { 1, 1, 3, 3 }))));
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry before = zf.get("before");
            assertNotNull(before);
            assertArrayEquals(new byte[] { 0, 1, 2 }, before.read());

            StoredEntry extra = zf.get("extra");
            assertNotNull(extra);
            assertArrayEquals(new byte[] { 3, 4, 5 }, extra.read());

            StoredEntry after = zf.get("after");
            assertNotNull(after);
            assertArrayEquals(new byte[] { 6, 7, 8 }, after.read());

            ExtraField ef = mExtraFieldGetter.apply(extra);
            assertEquals(1, ef.getSegments().size());
            ExtraField.Segment s = ef.getSingleSegment(0x7654);
            assertNotNull(s);
            byte[] sData = new byte[8];
            s.write(ByteBuffer.wrap(sData));
            assertArrayEquals(new byte[] { 0x54, 0x76, 0x04, 0x00, 1, 1, 3, 3 }, sData);
        }
    }

    @Test
    public void removeExtraFieldFromExistingEntry() throws Exception {
        try (ZFile zf = new ZFile(mZipFile)) {
            zf.add("before", new ByteArrayInputStream(new byte[] { 0, 1, 2 }));
            zf.add("extra", new ByteArrayInputStream(new byte[] { 3, 4, 5 }));
            zf.add("after", new ByteArrayInputStream(new byte[] { 6, 7, 8 }));
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry ex = zf.get("extra");
            assertNotNull(ex);
            mExtraFieldSetter.accept(ex,
                    new ExtraField(
                            ImmutableList.of(
                                    new ExtraField.RawDataSegment(
                                            0x7654,
                                            new byte[] { 1, 1, 3, 3 }))));
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry ex = zf.get("extra");
            assertNotNull(ex);
            mExtraFieldSetter.accept(ex, new ExtraField());
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry before = zf.get("before");
            assertNotNull(before);
            assertArrayEquals(new byte[] { 0, 1, 2 }, before.read());

            StoredEntry extra = zf.get("extra");
            assertNotNull(extra);
            assertArrayEquals(new byte[] { 3, 4, 5 }, extra.read());

            StoredEntry after = zf.get("after");
            assertNotNull(after);
            assertArrayEquals(new byte[] { 6, 7, 8 }, after.read());

            ExtraField ef = mExtraFieldGetter.apply(extra);
            assertEquals(0, ef.getSegments().size());
        }
    }

    @Test
    public void updateExtraFieldOfExistingEntry() throws Exception {
        try (ZFile zf = new ZFile(mZipFile)) {
            zf.add("before", new ByteArrayInputStream(new byte[] { 0, 1, 2 }));
            zf.add("extra", new ByteArrayInputStream(new byte[] { 3, 4, 5 }));
            zf.add("after", new ByteArrayInputStream(new byte[] { 6, 7, 8 }));
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry ex = zf.get("extra");
            assertNotNull(ex);
            mExtraFieldSetter.accept(ex,
                    new ExtraField(
                            ImmutableList.of(
                                    new ExtraField.RawDataSegment(
                                            0x7654,
                                            new byte[] { 1, 1, 3, 3 }))));
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry ex = zf.get("extra");
            assertNotNull(ex);
            mExtraFieldSetter.accept(ex,
                    new ExtraField(
                            ImmutableList.of(
                                    new ExtraField.RawDataSegment(
                                            0x7654,
                                            new byte[] { 2, 4, 2, 4 }))));
        }

        try (ZFile zf = new ZFile(mZipFile)) {
            StoredEntry before = zf.get("before");
            assertNotNull(before);
            assertArrayEquals(new byte[] { 0, 1, 2 }, before.read());

            StoredEntry extra = zf.get("extra");
            assertNotNull(extra);
            assertArrayEquals(new byte[] { 3, 4, 5 }, extra.read());

            StoredEntry after = zf.get("after");
            assertNotNull(after);
            assertArrayEquals(new byte[] { 6, 7, 8 }, after.read());

            ExtraField ef = mExtraFieldGetter.apply(extra);
            assertEquals(1, ef.getSegments().size());
            ExtraField.Segment s = ef.getSingleSegment(0x7654);
            assertNotNull(s);
            byte[] sData = new byte[8];
            s.write(ByteBuffer.wrap(sData));
            assertArrayEquals(new byte[] { 0x54, 0x76, 0x04, 0x00, 2, 4, 2, 4 }, sData);
        }
    }

    @Test
    public void parseInvalidExtraFieldWithInvalidHeader() throws Exception {
        byte[] raw = new byte[1];
        ExtraField ef = new ExtraField(raw);
        try {
            ef.getSegments();
            fail();
        } catch (IOException e) {
            // Expected.
        }
    }

    @Test
    public void parseInvalidExtraFieldWithInsufficientData() throws Exception {
        // Remember: 0x05, 0x00 = 5 in little endian!
        byte[] raw = new byte[] { /* Header */ 0x01, 0x02, /* Size */ 0x05, 0x00, /* Data */ 0x01 };
        ExtraField ef = new ExtraField(raw);
        try {
            ef.getSegments();
            fail();
        } catch (IOException e) {
            // Expected.
        }
    }
}
