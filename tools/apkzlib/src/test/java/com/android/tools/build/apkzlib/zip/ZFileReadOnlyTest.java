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
 * limitations under the License.
 */

package com.android.tools.build.apkzlib.zip;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import javax.annotation.Nonnull;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public class ZFileReadOnlyTest {
    @Rule
    public TemporaryFolder temporaryFolder = new TemporaryFolder();

    @Test
    public void cannotCreateRoFileOnNonExistingFile() throws Exception {
        try {
            new ZFile(new File(temporaryFolder.getRoot(), "foo.zip"), new ZFileOptions(), true);
            fail();
        } catch (IOException e) {
            // Expected.
        }
    }

    @Nonnull
    private File makeTestZip() throws IOException {
        File zip = new File(temporaryFolder.getRoot(), "foo.zip");
        try (ZFile zf = new ZFile(zip)) {
            zf.add("bar", new ByteArrayInputStream(new byte[] { 0, 1, 2, 3, 4, 5 }));
        }

        return zip;
    }

    @Test
    public void cannotUpdateInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.update();
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotAddFilesInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.add("bar2", new ByteArrayInputStream(new byte[] { 6, 7, }));
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotAddRecursivelyInRoMode() throws Exception {
        File folder = temporaryFolder.newFolder();
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.addAllRecursively(folder);
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotReplaceFilesInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.add("bar", new ByteArrayInputStream(new byte[] { 6, 7 }));
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotDeleteFilesInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            StoredEntry bar = zf.get("bar");
            assertNotNull(bar);
            try {
                bar.delete();
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotMergeInRoMode() throws Exception {
        try (ZFile toMerge = new ZFile(new File(temporaryFolder.getRoot(), "a.zip"))) {
            try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
                try {
                    zf.mergeFrom(toMerge, s -> false);
                    fail();
                } catch (IllegalStateException e) {
                    // Expeted.
                }
            }
        }
    }

    @Test
    public void cannotTouchInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.touch();
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotRealignInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.realign();
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotAddExtensionInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.addZFileExtension(new ZFileExtension() {});
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotDirectWriteInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.directWrite(0, new byte[1]);
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotSetEocdCommentInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.setEocdComment(new byte[2]);
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotSetCentralDirectoryOffsetInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.setExtraDirectoryOffset(4);
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void cannotSortZipContentsInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            try {
                zf.sortZipContents();
                fail();
            } catch (IllegalStateException e) {
                // Expeted.
            }
        }
    }

    @Test
    public void canOpenAndReadFilesInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            StoredEntry bar = zf.get("bar");
            assertNotNull(bar);
            assertArrayEquals(new byte[] { 0, 1, 2, 3, 4, 5 }, bar.read());
        }
    }

    @Test
    public void canGetDirectoryAndEocdBytesInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            zf.getCentralDirectoryBytes();
            zf.getEocdBytes();
            zf.getEocdComment();
        }
    }

    @Test
    public void canDirectReadInRoMode() throws Exception {
        try (ZFile zf = new ZFile(makeTestZip(), new ZFileOptions(), true)) {
            zf.directRead(0, new byte[2]);
        }
    }
}
