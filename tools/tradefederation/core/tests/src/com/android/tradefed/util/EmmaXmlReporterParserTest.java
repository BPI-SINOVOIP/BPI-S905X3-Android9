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
package com.android.tradefed.util;

import junit.framework.TestCase;

import org.junit.Assert;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.Map;

/**
 * Unit tests for {@link EmmaXmlReportParser}.
 */
public class EmmaXmlReporterParserTest extends TestCase {

    static final String FULL_DATA = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
            "<report>" +
            "<stats>" +
            "<packages value=\"33\"/>" +
            "<classes value=\"518\"/>" +
            "<methods value=\"3654\"/>" +
            "<srcfiles value=\"339\"/>" +
            "<srclines value=\"26587\"/>" +
            "</stats>" +
            "<data>" +
            "<all name=\"all classes\">" +
            "<coverage type=\"class, %\" value=\"10%  (50/518)\"/>" +
            "<coverage type=\"method, %\" value=\"7%   (258/3654)\"/>" +
            "<coverage type=\"block, %\" value=\"2%   (4995/202371)\"/>" +
            "<coverage type=\"line, %\" value=\"4%   (1078.6/26587)\"/>" +
            "</all>" +
            "</data>" +
            "</report>";

    private File mXMLTestFile;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mXMLTestFile = FileUtil.createTempFile("test", ".xml");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        // Clean up temp file.
        FileUtil.deleteFile(mXMLTestFile);
    }

    /*
     * Test that normal parsing of a well-formed emma xml report works as
     * expected.
     */
    public void testNormalParsing() throws IOException {
        FileUtil.writeToFile(FULL_DATA, mXMLTestFile);
        EmmaXmlReportParser parser = new EmmaXmlReportParser();
        parser.parseXmlFile(mXMLTestFile);
        Map<String, String> expected = parser.getSummaryMetrics();
        assertMapHasValue(expected, EmmaXmlConstants.CLASS_TAG, "10");
        assertMapHasValue(expected, EmmaXmlConstants.METHOD_TAG, "7");
        assertMapHasValue(expected, EmmaXmlConstants.BLOCK_TAG, "2");
        assertMapHasValue(expected, EmmaXmlConstants.LINE_TAG, "4");
    }

    /**
     * Test {@link EmmaXmlReportParser#parseXmlFile(File)}
     */
    public void testNormalParsingData() throws IOException {
        FileUtil.writeToFile(FULL_DATA, mXMLTestFile);
        EmmaXmlReportParser parser = new EmmaXmlReportParser();
        parser.parseXmlFile(mXMLTestFile);
        Map<String, String[]> expected = parser.getDataMetrics();
        assertMapHasValueArray(expected, EmmaXmlConstants.CLASS_TAG,
                new String[]{"50", "518"});
        assertMapHasValueArray(expected, EmmaXmlConstants.METHOD_TAG,
                new String[]{"258", "3654"});
        assertMapHasValueArray(expected, EmmaXmlConstants.BLOCK_TAG,
                new String[]{"4995", "202371"});
        assertMapHasValueArray(expected, EmmaXmlConstants.LINE_TAG,
                new String[]{"1078", "26587"});
    }

    /**
     * Helper method to assert that a given key is inside the map and the value
     * for that key matches
     *
     * @param map to check
     * @param expectedKey {@link String} key to check
     * @param expectedValue {@link String} value to check
     */
    private void assertMapHasValue(Map<String, String> map, String expectedKey,
            String expectedValue) {
        String error = String.format("Failed to get key '%s' from parsed map", expectedKey);
        Assert.assertTrue(error, map.containsKey(expectedKey));
        String actualValue = map.get(expectedKey);
        error = String.format("Parsed value for '%s' does not match. expected: '%s'; actual: '%s'",
                expectedKey, expectedValue, map.get(expectedKey));
        Assert.assertTrue(error, expectedValue.equals(actualValue));
    }

    /**
     * Helper method to assert that a given key is inside the map and the value
     * for that key matches
     *
     * @param map to check
     * @param expectedKey {@link String} key to check
     * @param expectedValue Array of {@link String} value to check
     */
    private void assertMapHasValueArray(Map<String, String[]> map, String expectedKey,
            String[] expectedValue) {
        String error = String.format("Failed to get key '%s' from parsed map", expectedKey);
        Assert.assertTrue(error, map.containsKey(expectedKey));
        String[] actualValue = map.get(expectedKey);
        error = String.format("Parsed value for '%s' does not match. expected: '%s'; actual: '%s'",
                expectedKey, expectedValue, map.get(expectedKey));
        Assert.assertTrue(error, Arrays.equals(expectedValue, actualValue));
    }
}
