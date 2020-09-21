/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache
 * License, Version 2.0 (the "License");
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

package com.android.tradefed.util;

import junit.framework.TestCase;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 * Unit tests for {@link FixedByteArrayOutputStream}.
 */
public class FixedByteArrayOutputStreamTest extends TestCase {

    private static final byte BUF_SIZE = 30;
    private static final String TEXT = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
            + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
            + "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
            + "aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "
            + "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
            + "occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit "
            + "anim id est laborum."; // 446 bytes
    private FixedByteArrayOutputStream mOutStream;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mOutStream = new FixedByteArrayOutputStream(BUF_SIZE);
    }

    /**
     * Util method to write a string into the {@link FixedByteArrayOutputStream} under test and
     *
     * @param text
     * @return content as String
     */
    private String writeTextIntoStreamAndReturn(String text) throws IOException {
        mOutStream.write(text.getBytes());
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        StreamUtil.copyStreams(mOutStream.getData(), baos);
        baos.close();
        return baos.toString();
    }
    /**
     * Test the stream works when data written is less than buffer size.
     */
    public void testLessThanBuffer() throws IOException {
        String text = TEXT.substring(0, BUF_SIZE - 5);
        assertEquals(text, writeTextIntoStreamAndReturn(text));
    }

    /**
     * Test the stream works when data written is exactly equal to buffer size.
     */
    public void testEqualsBuffer() throws IOException {
        String text = TEXT.substring(0, BUF_SIZE);
        assertEquals(text, writeTextIntoStreamAndReturn(text));
    }

    /**
     * Test the stream works when data written is 1 greater than buffer size.
     */
    public void testBufferPlusOne() throws IOException {
        String text = TEXT.substring(0, BUF_SIZE + 1);
        String expected = text.substring(1);
        assertEquals(expected, writeTextIntoStreamAndReturn(text));
    }

    /**
     * Test the stream works when data written is much greater than buffer size.
     */
    public void testBufferPlusPlus() throws IOException {
        String expected = TEXT.substring(TEXT.length() - BUF_SIZE);
        assertEquals(expected, writeTextIntoStreamAndReturn(TEXT));
    }

    /**
     * Testing the buffer wrap around scenario
     * @throws IOException
     */
    public void testWriteWithWrap() throws IOException {
        String prefix = "foobar";
        // larger than 24b because need to overflow the buffer, less than 30b because need to avoid
        // the shortcut in code when incoming is larger than buffer
        String followup = "stringlenbetween24band30b";
        String full = prefix + followup;
        String expected = full.substring(full.length() - BUF_SIZE);
        mOutStream.write(prefix.getBytes());
        assertEquals(expected, writeTextIntoStreamAndReturn(followup));
    }

    /**
     * Test writing using byte array with an offset
     * @throws IOException
     */
    public void testLessThanBufferWithOffset() throws IOException {
        String text = TEXT.substring(0, BUF_SIZE);
        int offset = 5;
        String expected = text.substring(offset);
        mOutStream.write(text.getBytes(), offset, text.length() - offset);
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        StreamUtil.copyStreams(mOutStream.getData(), baos);
        baos.close();
        assertEquals(expected, baos.toString());
    }

    /**
     * Test writing using byte array with an offset
     * @throws IOException
     */
    public void testWriteWithOffsetAndWrap() throws IOException {
        String prefix = "foobar";
        // similar to testWriteWithWrap, but add a tail to account for offset
        String followup = "stringlenbetween24band30b____";
        int offset = 4;
        String full = prefix + followup.substring(offset);
        String expected = full.substring(full.length() - BUF_SIZE);
        mOutStream.write(prefix.getBytes());
        mOutStream.write(followup.getBytes(), offset, followup.length() - offset);
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        StreamUtil.copyStreams(mOutStream.getData(), baos);
        baos.close();
        assertEquals(expected, baos.toString());
    }
}
