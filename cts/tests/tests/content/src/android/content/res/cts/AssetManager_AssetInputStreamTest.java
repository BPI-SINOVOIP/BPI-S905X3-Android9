/*
 * Copyright (C) 2008 The Android Open Source Project
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
package android.content.res.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

import android.content.res.AssetManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

@SmallTest
public class AssetManager_AssetInputStreamTest {
    private static final byte[] EXPECTED_BYTES = "OneTwoThreeFourFiveSixSevenEightNineTen".getBytes(
            StandardCharsets.UTF_8);
    private InputStream mAssetInputStream;

    @Before
    public void setUp() throws Exception {
        mAssetInputStream = InstrumentationRegistry.getContext().getAssets().open("text.txt");
    }

    @After
    public void tearDown() throws Exception {
        mAssetInputStream.close();
    }

    @Test
    public void testClose() throws Exception {
        mAssetInputStream.close();
        try {
            mAssetInputStream.read();
            fail("read after close should throw an exception");
        } catch (IllegalStateException e) {
            // expected
        }
    }

    @Test
    public void testGetAssetInt() {
        AssetManager.AssetInputStream assetInputStream =
                (AssetManager.AssetInputStream) mAssetInputStream;
        try {
            // getAssetInt is no longer supported.
            assetInputStream.getAssetInt();
            fail("getAssetInt should throw an exception");
        } catch (UnsupportedOperationException expected) {
        }
    }

    @Test
    public void testMarkReset() throws IOException {
        assertTrue(mAssetInputStream.markSupported());
        for (int i = 0; i < 10; i++) {
            assertEquals(EXPECTED_BYTES[i], mAssetInputStream.read());
        }
        mAssetInputStream.mark(10);
        mAssetInputStream.reset();
        for (int i = 0; i < 10; i++) {
            assertEquals(EXPECTED_BYTES[10 + i], mAssetInputStream.read());
        }
    }

    @Test
    public void testSingleByteRead() throws Exception {
        assertEquals(EXPECTED_BYTES.length, mAssetInputStream.available());
        for (int i = 0; i < EXPECTED_BYTES.length; i++) {
            assertEquals(EXPECTED_BYTES[i], mAssetInputStream.read());
            assertEquals(EXPECTED_BYTES.length - i - 1, mAssetInputStream.available());
        }

        // Check end-of-file condition.
        assertEquals(-1, mAssetInputStream.read());
        assertEquals(0, mAssetInputStream.available());
    }

    @Test
    public void testByteArrayRead() throws Exception {
        byte[] buffer = new byte[10];
        assertEquals(10, mAssetInputStream.read(buffer));
        for (int i = 0; i < 10; i++) {
            assertEquals(EXPECTED_BYTES[i], buffer[i]);
        }

        buffer = new byte[5];
        assertEquals(5, mAssetInputStream.read(buffer));
        for (int i = 0; i < 5; i++) {
            assertEquals(EXPECTED_BYTES[10 + i], buffer[i]);
        }

        // Check end-of-file condition.
        buffer = new byte[EXPECTED_BYTES.length - 15];
        assertEquals(buffer.length, mAssetInputStream.read(buffer));
        assertEquals(-1, mAssetInputStream.read(buffer));
        assertEquals(0, mAssetInputStream.available());
    }

    @Test
    public void testByteArrayReadOffset() throws Exception {
        byte[] buffer = new byte[15];
        assertEquals(10, mAssetInputStream.read(buffer, 0, 10));
        assertEquals(EXPECTED_BYTES.length - 10, mAssetInputStream.available());
        for (int i = 0; i < 10; i++) {
            assertEquals(EXPECTED_BYTES[i], buffer[i]);
        }

        assertEquals(5, mAssetInputStream.read(buffer, 10, 5));
        assertEquals(EXPECTED_BYTES.length - 15, mAssetInputStream.available());
        for (int i = 0; i < 15; i++) {
            assertEquals(EXPECTED_BYTES[i], buffer[i]);
        }

        // Check end-of-file condition.
        buffer = new byte[EXPECTED_BYTES.length];
        assertEquals(EXPECTED_BYTES.length - 15,
                mAssetInputStream.read(buffer, 15, EXPECTED_BYTES.length - 15));
        assertEquals(-1, mAssetInputStream.read(buffer, 0, 1));
        assertEquals(0, mAssetInputStream.available());
    }

    @Test
    public void testSkip() throws Exception {
        assertEquals(8, mAssetInputStream.skip(8));
        assertEquals(EXPECTED_BYTES.length - 8, mAssetInputStream.available());
        assertEquals(EXPECTED_BYTES[8], mAssetInputStream.read());

        // Check that skip respects the available space.
        assertEquals(EXPECTED_BYTES.length - 8 - 1, mAssetInputStream.skip(1000));
        assertEquals(0, mAssetInputStream.available());
    }

    @Test
    public void testArgumentEdgeCases() throws Exception {
        // test read(byte[]): byte[] is null
        try {
            mAssetInputStream.read(null);
            fail("should throw NullPointerException ");
        } catch (NullPointerException e) {
            // expected
        }

        // test read(byte[], int, int): byte[] is null
        try {
            mAssetInputStream.read(null, 0, mAssetInputStream.available());
            fail("should throw NullPointerException ");
        } catch (NullPointerException e) {
            // expected
        }

        // test read(byte[]): byte[] is len 0
        final int previousAvailable = mAssetInputStream.available();
        assertEquals(0, mAssetInputStream.read(new byte[0]));
        assertEquals(previousAvailable, mAssetInputStream.available());

        // test read(byte[]): byte[] is len 0
        assertEquals(0, mAssetInputStream.read(new byte[0], 0, 0));
        assertEquals(previousAvailable, mAssetInputStream.available());

        // test read(byte[], int, int): offset is negative
        try {
            byte[] data = new byte[10];
            mAssetInputStream.read(data, -1, mAssetInputStream.available());
            fail("should throw IndexOutOfBoundsException ");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }

        // test read(byte[], int, int): len + offset greater than data length
        try {
            byte[] data = new byte[10];
            assertEquals(0, mAssetInputStream.read(data, 0, data.length + 2));
            fail("should throw IndexOutOfBoundsException ");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }
    }
}
