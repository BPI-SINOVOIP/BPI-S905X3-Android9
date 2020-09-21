/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.loganalysis.parser;

import static org.junit.Assert.fail;

import com.android.loganalysis.item.TraceFormatItem;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Arrays;
import java.util.List;
import java.util.regex.Matcher;

/** Test for {@link TraceFormatParser}. */
@RunWith(JUnit4.class)
public class TraceFormatParserTest {
    private TraceFormatParser mParser;

    // "unwrap" the regex strings so that we can compare with the generated regex
    private static final String MATCH_NUM_UNESCAPED =
            TraceFormatParser.MATCH_NUM.replaceAll("\\\\\\\\", "\\\\");
    private static final String MATCH_HEX_UNESCAPED =
            TraceFormatParser.MATCH_HEX.replaceAll("\\\\\\\\", "\\\\");
    private static final String MATCH_STR_UNESCAPED =
            TraceFormatParser.MATCH_STR.replaceAll("\\\\\\\\", "\\\\");

    @Before
    public void setUp() {
        mParser = new TraceFormatParser();
    }

    @Test
    public void testParseFormatLine() {
        List<String> formatLine =
                Arrays.asList("print fmt: \"foo=%llu, bar=%s\", REC->foo, REC->bar");
        String expectedRegex =
                String.format(
                        "foo=(?<foo>%s), bar=(?<bar>%s)", MATCH_NUM_UNESCAPED, MATCH_STR_UNESCAPED);
        List<String> expectedParameters = Arrays.asList("foo", "bar");
        List<String> expectedNumericParameters = Arrays.asList("foo");
        List<String> expectedHexParameters = Arrays.asList();
        List<String> expectedStringParameters = Arrays.asList("bar");
        String shouldMatch = "foo=123, bar=enabled";

        TraceFormatItem parsedItem = mParser.parse(formatLine);
        Assert.assertEquals(expectedParameters, parsedItem.getParameters());
        Assert.assertEquals(expectedNumericParameters, parsedItem.getNumericParameters());
        Assert.assertEquals(expectedHexParameters, parsedItem.getHexParameters());
        Assert.assertEquals(expectedStringParameters, parsedItem.getStringParameters());
        Assert.assertEquals(expectedRegex, parsedItem.getRegex().toString());
        Matcher m = parsedItem.getRegex().matcher(shouldMatch);
        Assert.assertTrue(m.matches());
        Assert.assertEquals(m.group("foo"), "123");
        Assert.assertEquals(m.group("bar"), "enabled");
    }

    @Test
    public void testNoParameters() {
        List<String> formatLine = Arrays.asList("print fmt: \"foo\"");
        String expectedRegex = "foo";
        List<String> expectedParameters = Arrays.asList();
        String shouldMatch = "foo";

        TraceFormatItem parsedItem = mParser.parse(formatLine);
        Assert.assertEquals(expectedParameters, parsedItem.getParameters());
        Assert.assertEquals(expectedRegex, parsedItem.getRegex().toString());
        Matcher m = parsedItem.getRegex().matcher(shouldMatch);
        Assert.assertTrue(m.matches());
    }

    @Test
    public void testNullInput() {
        try {
            mParser.parse(null);
            fail("Expected an exception thrown by TraceFormatParser");
        } catch (RuntimeException e) {
            // expected
        }
    }

    @Test
    public void testEmptyInput() {
        List<String> formatLine = Arrays.asList("");
        try {
            mParser.parse(formatLine);
            fail("Expected an exception thrown by TraceFormatParser");
        } catch (RuntimeException e) {
            // expected
        }
    }

    @Test
    public void testMultiLineInput() {
        List<String> formatLine = Arrays.asList("foo", "bar");
        try {
            mParser.parse(formatLine);
            fail("Expected an exception thrown by TraceFormatParser");
        } catch (RuntimeException e) {
            // expected
        }
    }

    @Test
    public void testOneLineInvalidInput() {
        List<String> formatLine = Arrays.asList("foo bar");
        try {
            mParser.parse(formatLine);
            fail("Expected an exception thrown by TraceFormatParser");
        } catch (RuntimeException e) {
            // expected
        }
    }

    @Test
    public void testQuoteInParams() {
        List<String> formatLine =
                Arrays.asList("print fmt: \"foo %s\", REC->foo ? \"online\" : \"offline\"");
        String expectedRegex = String.format("foo (?<foo>%s)", MATCH_STR_UNESCAPED);
        String shouldMatch = "foo online";

        TraceFormatItem parsedItem = mParser.parse(formatLine);
        Assert.assertEquals(expectedRegex, parsedItem.getRegex().toString());
        Matcher m = parsedItem.getRegex().matcher(shouldMatch);
        Assert.assertTrue(m.matches());
        Assert.assertEquals(m.group("foo"), "online");
    }

    @Test
    public void testCategorizeParameters() {
        List<String> formatLine =
                Arrays.asList(
                        "print fmt: \"num1=%lu, num2=%f, hex=%08x, str=%s\", REC->num1, REC->num2, REC->hex, REC->str");
        List<String> expectedNumericParameters = Arrays.asList("num1", "num2");
        List<String> expectedHexParameters = Arrays.asList("hex");
        List<String> expectedStringParameters = Arrays.asList("str");

        TraceFormatItem parsedItem = mParser.parse(formatLine);
        Assert.assertEquals(expectedNumericParameters, parsedItem.getNumericParameters());
        Assert.assertEquals(expectedHexParameters, parsedItem.getHexParameters());
        Assert.assertEquals(expectedStringParameters, parsedItem.getStringParameters());
    }

    @Test
    public void testCaseConvertParameterName() {
        List<String> formatLine = Arrays.asList("print fmt: \"foo_bar=%llu\", REC->foo_bar");
        List<String> expectedParameters = Arrays.asList("fooBar");
        String shouldMatch = "foo_bar=123";

        TraceFormatItem parsedItem = mParser.parse(formatLine);
        Assert.assertEquals(expectedParameters, parsedItem.getParameters());
        Matcher m = parsedItem.getRegex().matcher(shouldMatch);
        Assert.assertTrue(m.matches());
        Assert.assertEquals(m.group("fooBar"), "123");
    }

    @Test
    public void testMatchInt() {
        List<String> formatLine =
                Arrays.asList("print fmt: \"foo=%d, bar=%lu\", REC->foo, REC->bar");
        String shouldMatch = "foo=-123, bar=456";

        TraceFormatItem parsedItem = mParser.parse(formatLine);
        Matcher m = parsedItem.getRegex().matcher(shouldMatch);
        Assert.assertTrue(m.matches());
        Assert.assertEquals(m.group("foo"), "-123");
        Assert.assertEquals(m.group("bar"), "456");
    }

    @Test
    public void testMatchFloat() {
        List<String> formatLine =
                Arrays.asList("print fmt: \"foo=%f, bar=%.2f\", REC->foo, REC->bar");
        String shouldMatch = "foo=123.4567, bar=456.78";

        TraceFormatItem parsedItem = mParser.parse(formatLine);
        Matcher m = parsedItem.getRegex().matcher(shouldMatch);
        Assert.assertTrue(m.matches());
        Assert.assertEquals(m.group("foo"), "123.4567");
        Assert.assertEquals(m.group("bar"), "456.78");
    }

    @Test
    public void testMatchHex() {
        List<String> formatLine =
                Arrays.asList(
                        "print fmt: \"foo=0x%04x, bar=0x%08X, baz=%x\", REC->foo, REC->bar, REC->baz");
        String shouldMatch = "foo=0x007b, bar=0x000001C8, baz=7b";

        TraceFormatItem parsedItem = mParser.parse(formatLine);
        Matcher m = parsedItem.getRegex().matcher(shouldMatch);
        Assert.assertTrue(m.matches());
        Assert.assertEquals(m.group("foo"), "007b");
        Assert.assertEquals(m.group("bar"), "000001C8");
        Assert.assertEquals(m.group("baz"), "7b");
    }

    @Test
    public void testMatchString() {
        List<String> formatLine =
                Arrays.asList("print fmt: \"foo=%s, bar=%s\", REC->foo, REC->bar");
        String shouldMatch = "foo=oof, bar=123";

        TraceFormatItem parsedItem = mParser.parse(formatLine);
        Matcher m = parsedItem.getRegex().matcher(shouldMatch);
        Assert.assertTrue(m.matches());
        Assert.assertEquals(m.group("foo"), "oof");
        Assert.assertEquals(m.group("bar"), "123");
    }
}
