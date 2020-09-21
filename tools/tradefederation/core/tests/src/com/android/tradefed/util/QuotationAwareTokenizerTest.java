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

package com.android.tradefed.util;

import junit.framework.TestCase;

import java.util.Arrays;

/**
 * Unit tests for {@link QuotationAwareTokenizer}
 */
public class QuotationAwareTokenizerTest extends TestCase {

    private static void verify(String input, String[] expected, String delimiter)
            throws IllegalArgumentException {
        String[] observed = QuotationAwareTokenizer.tokenizeLine(input, delimiter);

        if (expected.length != observed.length) {
            fail(String.format("Expected and observed arrays are different lengths: expected %s " +
                    "but observed %s.", Arrays.toString(expected), Arrays.toString(observed)));
        }

        for (int i = 0; i < expected.length; ++i) {
            assertEquals(
                    String.format(
                            "Array compare failed at element %d: %s vs %s",
                            i, expected[i], observed[i]),
                    expected[i],
                    observed[i]);
        }
    }

    private static void verify(String input, String[] expected) throws IllegalArgumentException {
        verify(input, expected, " ");
    }

    /**
     * Simple parse
     */
    public void testTokenizeLine_simple() throws IllegalArgumentException {
        String input = "  one  two three";
        String[] expected = new String[] {"one", "two", "three"};
        verify(input, expected);
    }

    public void testCombineTokens_simple() throws IllegalArgumentException {
        assertEquals("one two three", QuotationAwareTokenizer.combineTokens("one", "two", "three"));
    }

    /**
     * Whitespace inside of the quoted section should be preserved.
     */
    public void testTokenizeLine_whitespace() throws IllegalArgumentException {
        String input = "--foo \"this is a config\"";
        String[] expected = new String[] {"--foo", "this is a config"};
        verify(input, expected);
    }

    /**
     * Tokenize a line with comma as delimiter
     */
    public void testTokenizeLine_comma() throws IllegalArgumentException {
        String input = "--foo,bar";
        String[] expected = new String[] {"--foo", "bar"};
        verify(input, expected, ",");
    }

    /**
     * Delimiter (using comma) inside of the quoted section should be preserved.
     */
    public void testTokenizeLine_commaAndQuote() throws IllegalArgumentException {
        String input = "--foo,\"a,config\"";
        String[] expected = new String[] {"--foo", "a,config"};
        verify(input, expected, ",");
    }

    /**
     * Tokenizing line with whitespace, using command as delimiter.
     */
    public void testTokenizeLine_commaAndWhitespace() throws IllegalArgumentException {
        String input = "--foo,this is a,config";
        String[] expected = new String[] {"--foo", "this is a", "config"};
        verify(input, expected, ",");
    }

    /**
     * Inverse of {@link #testTokenizeLine_whitespace}.
     */
    public void testCombineTokens_whitespace() throws IllegalArgumentException {
        assertEquals("--foo \"this is a config\"", QuotationAwareTokenizer.combineTokens("--foo",
                "this is a config"));
    }

    /**
     * Verify embedded escaped quotation marks are be ignored.
     */
    public void testTokenizeLine_escapedQuotation() throws IllegalArgumentException {
        String input = "--bar \"escap\\\\ed \\\" quotation\"";
        String[] expected = new String[] {"--bar",
                "escap\\\\ed \\\" quotation"};
        verify(input, expected);
    }

    /**
     * Inverse of {@link #testTokenizeLine_escapedQuotation}.
     */
    public void testCombineTokens_escapedQuotation() throws IllegalArgumentException {
        assertEquals("--bar \"escap\\\\ed \\\" quotation\"", QuotationAwareTokenizer.combineTokens(
                "--bar", "escap\\\\ed \\\" quotation"));
    }

    /**
     * Test the scenario where the input ends inside a quotation.
     */
    public void testTokenizeLine_endOnQuote() {
        String input = "--foo \"this is truncated";

        try {
            QuotationAwareTokenizer.tokenizeLine(input);
            fail("IllegalArgumentException not thrown.");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test the scenario where the input ends in the middle of an escape character.
     */
    public void testTokenizeLine_endWithEscape() {
        String input = "--foo escape\\";

        try {
            QuotationAwareTokenizer.tokenizeLine(input);
            fail("IllegalArgumentException not thrown.");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }
}
