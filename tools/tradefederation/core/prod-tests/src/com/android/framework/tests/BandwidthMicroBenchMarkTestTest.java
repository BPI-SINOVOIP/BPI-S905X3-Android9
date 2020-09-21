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
package com.android.framework.tests;

import junit.framework.TestCase;

import java.util.Map;

public class BandwidthMicroBenchMarkTestTest extends TestCase {

    /**
     * Test method for {@link BandwidthMicroBenchMarkTest#parseServerResponse(String)} on empty
     * response.
     */
    public void testParseNullResponse() throws Exception {
        assertNull(BandwidthMicroBenchMarkTest.parseServerResponse(""));
        assertNull(BandwidthMicroBenchMarkTest.parseServerResponse(null));
    }

    /**
     * Test method for {@link BandwidthMicroBenchMarkTest#parseServerResponse(String)} on standard
     * response.
     */
    public void testParseCorrectResponse() throws Exception {
        Map<String, String> result = BandwidthMicroBenchMarkTest.parseServerResponse(
                "foo:bar blue:red\nandroid:google\n");
       assertNotNull(result);
       assertEquals(3, result.size());
       assertTrue(result.containsKey("foo"));
       assertEquals("bar", result.get("foo"));
       assertEquals("red", result.get("blue"));
       assertEquals("google", result.get("android"));
    }
}
