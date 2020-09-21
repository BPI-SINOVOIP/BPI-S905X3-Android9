/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.tradefed.util.net;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.util.List;

/**
 * Unit tests for {@link XmlRpcHelper} module
 */
public class XmlRpcHelperTest extends TestCase {
    private static final String RESPONSE_TEMPLATE =
            "<?xml version='1.0'?>\r\n" +
            "<methodResponse>\r\n" +
            "<params>\r\n" +
            "<param>\r\n" +
            "%s" +
            "</param>\r\n" +
            "</params>\r\n" +
            "</methodResponse>\r\n" +
            "\r\n";

    private static final String TRUE_RESPONSE = String.format(RESPONSE_TEMPLATE,
            "<value><boolean>1</boolean></value>\r\n");
    private static final String FALSE_RESPONSE = String.format(RESPONSE_TEMPLATE,
            "<value><boolean>0</boolean></value>\r\n");

    public void testParseTrue() {
        List<String> resp = null;
        resp = XmlRpcHelper.parseResponseTuple(new ByteArrayInputStream(TRUE_RESPONSE.getBytes()));
        assertNotNull(resp);
        assertEquals(2, resp.size());
        assertEquals("boolean", resp.get(0));
        assertEquals("1", resp.get(1));
    }

    public void testParseFalse() {
        List<String> resp = null;
        resp = XmlRpcHelper.parseResponseTuple(new ByteArrayInputStream(FALSE_RESPONSE.getBytes()));
        assertNotNull(resp);
        assertEquals(2, resp.size());
        assertEquals("boolean", resp.get(0));
        assertEquals("0", resp.get(1));
    }

    public void testParse_multiValue() {
        String response = String.format(RESPONSE_TEMPLATE,
                "  <string>1234</string>\r\n" +
                "</param>\r\n" +
                "<param>\r\n" +
                "  <i4>1</i4>\r\n" +
                "</param>\r\n" +
                "<param>\r\n" +
                "  <string>hypotenuse</string>\r\n");

        List<String> resp = null;
        resp = XmlRpcHelper.parseResponseTuple(new ByteArrayInputStream(response.getBytes()));
        assertNotNull(resp);
        assertEquals(6, resp.size());
        assertEquals("string", resp.get(0));
        assertEquals("1234", resp.get(1));
        assertEquals("i4", resp.get(2));
        assertEquals("1", resp.get(3));
        assertEquals("string", resp.get(4));
        assertEquals("hypotenuse", resp.get(5));
    }
}

