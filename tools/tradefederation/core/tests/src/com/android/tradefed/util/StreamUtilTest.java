/*
 * Copyright (C) 2012 The Android Open Source Project
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
package com.android.tradefed.util;

import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.util.StreamUtil;

import junit.framework.TestCase;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;

/**
 * Unit tests for the {@link StreamUtil} utility class
 */
public class StreamUtilTest extends TestCase {

    /**
     * Verify that {@link StreamUtil#getByteArrayListFromSource} works as expected.
     */
    public void testGetByteArrayListFromSource() throws Exception {
        final String contents = "this is a string";
        final byte[] contentBytes = contents.getBytes();
        try (final InputStreamSource source = new ByteArrayInputStreamSource(contentBytes)) {
            final InputStream stream = source.createInputStream();
            final ByteArrayList output = StreamUtil.getByteArrayListFromStream(stream);
            final byte[] outputBytes = output.getContents();

            assertEquals(contentBytes.length, outputBytes.length);
            for (int i = 0; i < contentBytes.length; ++i) {
                assertEquals(contentBytes[i], outputBytes[i]);
            }
        }
    }

    /**
     * Verify that {@link StreamUtil#getByteArrayListFromStream} works as expected.
     */
    public void testGetByteArrayListFromStream() throws Exception {
        final String contents = "this is a string";
        final byte[] contentBytes = contents.getBytes();
        final ByteArrayList output = StreamUtil.getByteArrayListFromStream(
                new ByteArrayInputStream(contentBytes));
        final byte[] outputBytes = output.getContents();

        assertEquals(contentBytes.length, outputBytes.length);
        for (int i = 0; i < contentBytes.length; ++i) {
            assertEquals(contentBytes[i], outputBytes[i]);
        }
    }

    /**
     * Verify that {@link StreamUtil#getStringFromSource} works as expected.
     */
    public void testGetStringFromSource() throws Exception {
        final String contents = "this is a string";
        try (InputStreamSource source = new ByteArrayInputStreamSource(contents.getBytes())) {
            final InputStream stream = source.createInputStream();
            final String output = StreamUtil.getStringFromStream(stream);
            assertEquals(contents, output);
        }
    }

    /**
     * Verify that {@link StreamUtil#getBufferedReaderFromStreamSrc} works as expected.
     */
    public void testGetBufferedReaderFromInputStream() throws Exception {
        final String contents = "this is a string";
        BufferedReader output = null;
        try (InputStreamSource source = new ByteArrayInputStreamSource(contents.getBytes())) {
            output = StreamUtil.getBufferedReaderFromStreamSrc(source);
            assertEquals(contents, output.readLine());
        } finally {
            if (null != output) {
                output.close();
            }
        }
    }

    /**
     * Verify that {@link StreamUtil#countLinesFromSource} works as expected.
     */
    public void testCountLinesFromSource() throws Exception {
        final String contents = "foo\nbar\n\foo\n";
        final InputStreamSource source = new ByteArrayInputStreamSource(contents.getBytes());
        assertEquals(3, StreamUtil.countLinesFromSource(source));
    }

    /**
     * Verify that {@link StreamUtil#getStringFromStream} works as expected.
     */
    public void testGetStringFromStream() throws Exception {
        final String contents = "this is a string";
        final String output = StreamUtil.getStringFromStream(
                new ByteArrayInputStream(contents.getBytes()));
        assertEquals(contents, output);
    }

    /**
     * Verify that {@link StreamUtil#calculateMd5(InputStream)} works as expected.
     * @throws IOException
     */
    public void testCalculateMd5() throws IOException {
        final String source = "testtesttesttesttest";
        final String md5 = "f317f682fafe0309c6a423af0b4efa59";
        ByteArrayInputStream inputSource = new ByteArrayInputStream(source.getBytes());
        String actualMd5 = StreamUtil.calculateMd5(inputSource);
        assertEquals(md5, actualMd5);
    }

    public void testCopyStreams() throws Exception {
        String text = getLargeText();
        ByteArrayInputStream bais = new ByteArrayInputStream(text.getBytes());
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        StreamUtil.copyStreams(bais, baos);
        bais.close();
        baos.close();
        assertEquals(text, baos.toString());
    }

    public void testCopyStreamToWriter() throws Exception {
        String text = getLargeText();
        ByteArrayInputStream bais = new ByteArrayInputStream(text.getBytes());
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        Writer writer = new OutputStreamWriter(baos);
        StreamUtil.copyStreamToWriter(bais, writer);
        bais.close();
        writer.close();
        baos.close();
        assertEquals(text, baos.toString());
    }

    /**
     * Returns a large chunk of text that's at least 16K in size
     */
    private String getLargeText() {
        String text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                + "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
                + "aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "
                + "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
                + "occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit "
                + "anim id est laborum."; // 446 bytes
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 40; i ++) {
            sb.append(text);
        }
        return sb.toString();
    }
}
