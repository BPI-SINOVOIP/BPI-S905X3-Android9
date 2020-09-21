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

import java.util.List;
import java.util.Map;

/**
 * Unit test class for {@link SimplePerfStatResultParser}
 */
public class SimplePerfStatResultParserTest extends TestCase {

    public void testParseSingleLineWithEmptyString() {
        String emptyString = "";
        List<String> result = SimplePerfStatResultParser.parseSingleLine(emptyString);
        Assert.assertTrue(result.size() == 0);
    }

    public void testParseSingleLineWithNullString() {
        String nullString = null;
        List<String> result = SimplePerfStatResultParser.parseSingleLine(nullString);
        Assert.assertTrue(result.size() == 0);
    }

    public void testParseSingleLineWithHeaderString() {
        String headerString = "Performance counter statistics:SomeRandomData";
        List<String> result = SimplePerfStatResultParser.parseSingleLine(headerString);
        Assert.assertTrue(result.size() == 0);
    }

    public void testParseSingleLineWithReturnString() {
        String returnString = "\n";
        List<String> result = SimplePerfStatResultParser.parseSingleLine(returnString);
        Assert.assertTrue(result.size() == 0);
    }

    public void testParseSingleLineWithRandomString() {
        String randomString = "  lkweknqwq(#@?>S@$Q  \r\n";
        List<String> result = SimplePerfStatResultParser.parseSingleLine(randomString);
        Assert.assertTrue(result.size() == 0);
    }

    public void testParseSingleLineWithValidMetricString() {
        String validMetricString = "  37,037,350  cpu-cycles   # 0.126492 GHz   (100%)";
        List<String> result = SimplePerfStatResultParser.parseSingleLine(validMetricString);
        Assert.assertTrue(result.size() == 3);
        Assert.assertTrue(result.get(0).equals("37,037,350"));
        Assert.assertTrue(result.get(1).equals("cpu-cycles"));
        Assert.assertTrue(result.get(2).equals("0.126492 GHz   (100%)"));
    }

    public void testParseSingleLineWithValidTotalTimeString() {
        String validTotalTimeString = "Total test time: 0.292803 seconds.";
        List<String> result = SimplePerfStatResultParser.parseSingleLine(validTotalTimeString);
        Assert.assertTrue(result.size() == 1);
        Assert.assertTrue(result.get(0).equals("0.292803"));
    }

    public void testParseSingleLineWithMultiplePoundString() {
        String multiplePoundString = "  37,037,350  cpu-cycles   # 0.126492 # GHz   (100%)";
        List<String> result = SimplePerfStatResultParser.parseSingleLine(multiplePoundString);
        Assert.assertTrue(result.size() == 0);
    }

    public void testParseSingleLineWithMultipleSpaceString() {
        String multiplePoundString = "  37,037,350 3735 cpu-cycles   # 0.126492 GHz   (100%)";
        List<String> result = SimplePerfStatResultParser.parseSingleLine(multiplePoundString);
        Assert.assertTrue(result.size() == 0);
    }

    public void testParseRawOutputWithNullString() {
        String nullString = null;
        SimplePerfResult result = SimplePerfStatResultParser.parseRawOutput(nullString);
        Assert.assertNull(result);
    }

    public void testParseRawOutputWithEmptyString() {
        String emptyString = "\n\n\n\n\n\n";
        SimplePerfResult result = SimplePerfStatResultParser.parseRawOutput(emptyString);
        Assert.assertNull(result);
    }

    public void testParseRawOutputWithValidString() {
        String validString =
                "USER      PID   PPID  VSIZE  RSS   WCHAN            PC  NAME\n"
                + "root      1     0     7740   1380  SyS_epoll_ 0008d55c S /init\n"
                + "root      156   2     0      0     worker_thr 00000000 S kworker/3:1H\n"
                + "root      7262  2     0      0     worker_thr 00000000 S kworker/u8:3\n"
                + "Performance counter statistics:\n"
                + "\n"
                + "  128,716,783  cpu-cycles   # 0.103231 GHz   (100%)\n"
                + "      654,321  dummy-benchmark # test\n"
                + "92.337914(ms)  task-clock            # 175.304352% cpu usage             (50%)\n"
                + "\n"
                + "Total test time: 1.246881 seconds.\n";
        SimplePerfResult result = SimplePerfStatResultParser.parseRawOutput(validString);
        Assert.assertEquals("1.246881", result.getTotalTestTime());
        Map<String, String> benchmarkMetricsMap = result.getBenchmarkMetrics();
        Map<String, String> benchmarkCommentsMap = result.getBenchmarkComments();
        Assert.assertEquals("128,716,783", benchmarkMetricsMap.get("cpu-cycles"));
        Assert.assertEquals("0.103231 GHz   (100%)", benchmarkCommentsMap.get("cpu-cycles"));
        Assert.assertEquals("654,321", benchmarkMetricsMap.get("dummy-benchmark"));
        Assert.assertEquals("test", benchmarkCommentsMap.get("dummy-benchmark"));
    }
}
