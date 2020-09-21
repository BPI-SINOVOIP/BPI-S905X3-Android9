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

package libcore.java.util.zip;

import java.io.ByteArrayOutputStream;
import java.util.zip.Adler32;
import java.util.zip.Deflater;
import java.util.zip.Inflater;
import libcore.junit.junit3.TestCaseWithRules;
import libcore.junit.util.ResourceLeakageDetector;
import org.junit.Rule;
import org.junit.rules.TestRule;

public class InflaterTest extends TestCaseWithRules {
    @Rule
    public TestRule resourceLeakageDetectorRule = ResourceLeakageDetector.getRule();

    public void testDefaultDictionary() throws Exception {
        assertRoundTrip(null);
    }

    public void testPresetCustomDictionary() throws Exception {
        assertRoundTrip("dictionary".getBytes("UTF-8"));
    }

    private static void assertRoundTrip(byte[] dictionary) throws Exception {
        // Construct a nice long input byte sequence.
        String expected = makeString();
        byte[] expectedBytes = expected.getBytes("UTF-8");

        // Compress the bytes, using the passed-in dictionary (or no dictionary).
        byte[] deflatedBytes = deflate(expectedBytes, dictionary);

        // Get ready to decompress deflatedBytes back to the original bytes ...
        Inflater inflater = new Inflater();
        // We'll only supply the input a little bit at a time, so that zlib has to ask for more.
        final int CHUNK_SIZE = 16;
        int offset = 0;
        inflater.setInput(deflatedBytes, offset, CHUNK_SIZE);
        offset += CHUNK_SIZE;
        // If we used a dictionary to compress, check that we're asked for that same dictionary.
        if (dictionary != null) {
            // 1. there's no data available immediately...
            assertEquals(0, inflater.inflate(new byte[8]));
            // 2. ...because you need a dictionary.
            assertTrue(inflater.needsDictionary());
            // 3. ...and that dictionary has the same Adler32 as the dictionary we used.
            assertEquals(adler32(dictionary), inflater.getAdler());
            inflater.setDictionary(dictionary);
        }
        // Do the actual decompression, now the dictionary's set up appropriately...
        // We use a tiny output buffer to ensure that we call inflate multiple times, and
        // a tiny input buffer to ensure that zlib has to ask for more input.
        ByteArrayOutputStream inflatedBytes = new ByteArrayOutputStream();
        byte[] buf = new byte[8];
        while (inflatedBytes.size() != expectedBytes.length) {
            if (inflater.needsInput()) {
                int nextChunkByteCount = Math.min(CHUNK_SIZE, deflatedBytes.length - offset);
                inflater.setInput(deflatedBytes, offset, nextChunkByteCount);
                offset += nextChunkByteCount;
            } else {
                int inflatedByteCount = inflater.inflate(buf);
                if (inflatedByteCount > 0) {
                    inflatedBytes.write(buf, 0, inflatedByteCount);
                }
            }
        }
        inflater.end();

        assertEquals(expected, new String(inflatedBytes.toByteArray(), "UTF-8"));
    }

    private static String makeString() {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 1024; ++i) {
            // This is arbitrary but convenient in that it gives our string
            // an easily-recognizable beginning and end.
            sb.append(i + 1024);
        }
        return sb.toString();
    }

    /**
     * http://code.google.com/p/android/issues/detail?id=11755
     */
    public void testEmptyFileAndEmptyBuffer() throws Exception {
        byte[] emptyInput = deflate(new byte[0], null);
        Inflater inflater = new Inflater();
        inflater.setInput(emptyInput);
        assertFalse(inflater.finished());
        assertEquals(0, inflater.inflate(new byte[0], 0, 0));
        assertTrue(inflater.finished());
        inflater.end();
    }

    private static byte[] deflate(byte[] input, byte[] dictionary) {
        Deflater deflater = new Deflater();
        ByteArrayOutputStream deflatedBytes = new ByteArrayOutputStream();
        try {
            if (dictionary != null) {
                deflater.setDictionary(dictionary);
            }
            deflater.setInput(input);
            deflater.finish();
            byte[] buf = new byte[8];
            while (!deflater.finished()) {
                int byteCount = deflater.deflate(buf);
                deflatedBytes.write(buf, 0, byteCount);
            }
        } finally {
            deflater.end();
        }
        return deflatedBytes.toByteArray();
    }

    private static int adler32(byte[] bytes) {
        Adler32 adler32 = new Adler32();
        adler32.update(bytes);
        return (int) adler32.getValue();
    }

    public void testInflaterCounts() throws Exception {
        Inflater inflater = new Inflater();
        byte[] decompressed = new byte[32];
        byte[] compressed = deflate(new byte[] { 1, 2, 3 }, null);
        assertEquals(11, compressed.length);

        // Feed in bytes [0, 5) to the first iteration.
        inflater.setInput(compressed, 0, 5);
        inflater.inflate(decompressed, 0, decompressed.length);
        assertEquals(5, inflater.getBytesRead());
        assertEquals(5, inflater.getTotalIn());
        assertEquals(2, inflater.getBytesWritten());
        assertEquals(2, inflater.getTotalOut());

        // Feed in bytes [5, 11) to the second iteration.
        assertEquals(true, inflater.needsInput());
        inflater.setInput(compressed, 5, 6);
        assertEquals(1, inflater.inflate(decompressed, 0, decompressed.length));
        assertEquals(11, inflater.getBytesRead());
        assertEquals(11, inflater.getTotalIn());
        assertEquals(3, inflater.getBytesWritten());
        assertEquals(3, inflater.getTotalOut());

        inflater.reset();
        assertEquals(0, inflater.getBytesRead());
        assertEquals(0, inflater.getTotalIn());
        assertEquals(0, inflater.getBytesWritten());
        assertEquals(0, inflater.getTotalOut());
        inflater.end();
    }
}
