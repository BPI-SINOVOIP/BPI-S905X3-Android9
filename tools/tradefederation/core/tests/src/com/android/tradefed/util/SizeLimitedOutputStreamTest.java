/*
 * Copyright (C) 2011 The Android Open Source Project
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

import java.io.IOException;
import java.io.InputStream;

/**
 * Unit tests for {@link SizeLimitedOutputStreamTest}
 */
public class SizeLimitedOutputStreamTest extends TestCase {

    /**
     * Test the file size limiting.
     */
    public void testMaxFileSizeHelper() throws IOException {
        final byte[] data = new byte[29];

        // fill data with values
        for (byte i = 0; i < data.length; i++) {
            data[i] = i;
        }

        // use a max size of 20 - expect first 10 bytes to get dropped
        SizeLimitedOutputStream outStream = new SizeLimitedOutputStream(20, 4, "foo", "bar");
        try {
            outStream.write(data);
            outStream.close();
            InputStream readStream = outStream.getData();
            byte[] readData = new byte[64];
            int readDataPos = 0;
            int read;
            while ((read = readStream.read()) != -1) {
                readData[readDataPos] = (byte)read;
                readDataPos++;
            }
            int bytesRead = readDataPos;
            assertEquals(19, bytesRead);
            assertEquals(10, readData[0]);
            assertEquals(28, readData[18]);
        } finally {
            outStream.delete();
        }
    }
}
