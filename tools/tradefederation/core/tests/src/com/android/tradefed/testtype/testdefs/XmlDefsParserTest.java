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
package com.android.tradefed.testtype.testdefs;

import com.android.tradefed.util.xml.AbstractXmlParser.ParseException;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.InputStream;

/**
 * Unit tests for {@link XmlDefsParser}.
 */
public class XmlDefsParserTest extends TestCase {

    static final String TEST_PKG = "testpkg";
    static final String TEST_COVERAGE_TARGET = "testcoverage";

    private static final String TEST_DATA =
        "<test name=\"frameworks-core\" " +
        "package=\"" + TEST_PKG + "\" " +
        "continuous=\"true\" />";

    private static final String NON_CONTINUOUS_TEST_DATA =
        "<test name=\"frameworks-core\" " +
        "package=\"" + TEST_PKG + "\" " +
        "/>";

    private static final String FALSE_CONTINUOUS_TEST_DATA =
        "<test name=\"frameworks-core\" " +
        "package=\"" + TEST_PKG + "\" " +
        "continuous=\"false\" />";

    static final String FULL_DATA =
        "<test name=\"frameworks-core\" " +
        "package=\"" + TEST_PKG + "\" " +
        "class=\"class\" " +
        "runner=\"runner\" " +
        "continuous=\"true\" " +
        "coverage_target=\"" + TEST_COVERAGE_TARGET + "\" />";

    /**
     * Simple test for parsing a single test definition.
     */
    public void testParseSingleDef() throws ParseException {
        XmlDefsParser parser = new XmlDefsParser();
        parser.parse(getStringAsStream(TEST_DATA));
        assertEquals(1, parser.getTestDefs().size());
        InstrumentationTestDef def = parser.getTestDefs().iterator().next();
        assertEquals("frameworks-core", def.getName());
        assertEquals(TEST_PKG, def.getPackage());
        assertTrue(def.isContinuous());
        assertNull(def.getClassName());
        assertNull(def.getRunner());
        assertNull(def.getCoverageTarget());
    }

    /**
     * Test for parsing a test definition without continuous attribute.
     */
    public void testParseNonContinuous() throws ParseException {
        XmlDefsParser parser = new XmlDefsParser();
        parser.parse(getStringAsStream(NON_CONTINUOUS_TEST_DATA));
        assertEquals(1, parser.getTestDefs().size());
        InstrumentationTestDef def = parser.getTestDefs().iterator().next();
        assertFalse(def.isContinuous());
    }

    /**
     * Test for parsing a test definition with continuous attribute equal false.
     */
    public void testParseFaleContinuous() throws ParseException {
        XmlDefsParser parser = new XmlDefsParser();
        parser.parse(getStringAsStream(FALSE_CONTINUOUS_TEST_DATA));
        assertEquals(1, parser.getTestDefs().size());
        InstrumentationTestDef def = parser.getTestDefs().iterator().next();
        assertFalse(def.isContinuous());
    }

    /**
     * Simple test for parsing a single test definition with all attributes defined.
     */
    public void testParseFullDef() throws ParseException {
        XmlDefsParser parser = new XmlDefsParser();
        parser.parse(getStringAsStream(FULL_DATA));
        assertEquals(1, parser.getTestDefs().size());
        InstrumentationTestDef def = parser.getTestDefs().iterator().next();
        assertEquals("frameworks-core", def.getName());
        assertEquals(TEST_PKG, def.getPackage());
        assertTrue(def.isContinuous());
        assertEquals("class", def.getClassName());
        assertEquals("runner", def.getRunner());
        assertEquals(TEST_COVERAGE_TARGET, def.getCoverageTarget());
    }

    /**
     * Test parsing non-xml data throws a ParseException
     */
    public void testParseException()  {
        try {
            XmlDefsParser parser = new XmlDefsParser();
            parser.parse(getStringAsStream("ghgh"));
            fail("ParseException not thrown");
        } catch (ParseException e) {
            // expected
        }
    }

    private InputStream getStringAsStream(String input) {
        return new ByteArrayInputStream(input.getBytes());
    }
}
