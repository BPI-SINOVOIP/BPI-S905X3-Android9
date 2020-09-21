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

package com.android.server.wifi.hotspot2.anqp.eap;

import static org.junit.Assert.assertEquals;

import android.net.wifi.EAPConstants;
import android.support.test.filters.SmallTest;

import org.junit.Test;

import java.net.ProtocolException;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;

/**
 * Unit tests for {@link com.android.server.wifi.hotspot2.anqp.eap.InnerAuthEAP}.
 */
@SmallTest
public class InnerAuthEAPTest {
    private static final int TEST_EAP_METHOD_ID = EAPConstants.EAP_TTLS;

    /**
     * Helper function for generating the test buffer.
     *
     * @return {@link ByteBuffer}
     */
    private ByteBuffer getTestBuffer() {
        return ByteBuffer.wrap(new byte[] {(byte) TEST_EAP_METHOD_ID});
    }

    /**
     * Verify that BufferUnderflowException will be thrown when parsing from an empty buffer.
     *
     * @throws Exception
     */
    @Test(expected = BufferUnderflowException.class)
    public void parseEmptyBuffer() throws Exception {
        InnerAuthEAP.parse(ByteBuffer.wrap(new byte[0]), InnerAuthEAP.EXPECTED_LENGTH_VALUE);
    }

    /**
     * Verify that ProtocolException will be thrown when the data length is not the expected
     * length.
     *
     * @throws Exception
     */
    @Test(expected = ProtocolException.class)
    public void parseBufferWithInvalidLength() throws Exception {
        InnerAuthEAP.parse(getTestBuffer(), InnerAuthEAP.EXPECTED_LENGTH_VALUE - 1);
    }

    /**
     * Verify that an expected InnerAuthEAP is returned when parsing a buffer contained
     * the expected EAP method ID.
     *
     * @throws Exception
     */
    @Test
    public void parseBuffer() throws Exception {
        InnerAuthEAP expected = new InnerAuthEAP(TEST_EAP_METHOD_ID);
        InnerAuthEAP actual =
                InnerAuthEAP.parse(getTestBuffer(), InnerAuthEAP.EXPECTED_LENGTH_VALUE);
        assertEquals(expected, actual);
    }
}
