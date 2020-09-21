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
package com.android.tradefed.testtype;

import static org.junit.Assert.assertEquals;

import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import org.easymock.Capture;
import org.easymock.EasyMock;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link GoogleBenchmarkResultParser}. */
@RunWith(JUnit4.class)
public class GoogleBenchmarkResultParserTest {

    private static final String TEST_TYPE_DIR = "testtype";
    private static final String GBENCH_OUTPUT_FILE_1 = "gbench_output1.json";
    private static final String GBENCH_OUTPUT_FILE_2 = "gbench_output2.json";
    private static final String GBENCH_OUTPUT_FILE_3 = "gbench_output3.json";
    private static final String GBENCH_OUTPUT_FILE_4 = "gbench_output4.json";
    private static final String GBENCH_OUTPUT_FILE_5 = "gbench_output5.json";

    private static final String TEST_RUN = "test_run";

    /**
     * Helper to read a file from the res/testtype directory and return its contents as a String[]
     *
     * @param filename the name of the file (without the extension) in the res/testtype directory
     * @return a String[] of the
     */
    private CollectingOutputReceiver readInFile(String filename) {
        CollectingOutputReceiver output = null;
        try {
            output = new CollectingOutputReceiver();
            InputStream gtestResultStream1 = getClass().getResourceAsStream(File.separator +
                    TEST_TYPE_DIR + File.separator + filename);
            BufferedReader reader = new BufferedReader(new InputStreamReader(gtestResultStream1));
            String line = null;
            while ((line = reader.readLine()) != null) {
                output.addOutput(line.getBytes(), 0, line.length());
            }
        }
        catch (NullPointerException e) {
            CLog.e("GTest output file does not exist: " + filename);
        }
        catch (IOException e) {
            CLog.e("Unable to read contents of gtest output file: " + filename);
        }
        return output;
    }

    /** Tests the parser for a simple test output for 2 tests. */
    @Test
    public void testParseSimpleFile() throws Exception {
        ITestInvocationListener mMockInvocationListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mMockInvocationListener.testStarted((TestDescription) EasyMock.anyObject());
        EasyMock.expectLastCall().times(3);
        Capture<HashMap<String, Metric>> capture1 = new Capture<>();
        Capture<HashMap<String, Metric>> capture2 = new Capture<>();
        Capture<HashMap<String, Metric>> capture3 = new Capture<>();
        mMockInvocationListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.capture(capture1));
        mMockInvocationListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.capture(capture2));
        mMockInvocationListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.capture(capture3));
        EasyMock.replay(mMockInvocationListener);
        CollectingOutputReceiver contents =  readInFile(GBENCH_OUTPUT_FILE_1);
        GoogleBenchmarkResultParser resultParser =
                new GoogleBenchmarkResultParser(TEST_RUN, mMockInvocationListener);
        Map<String, String> results = resultParser.parse(contents);
        Map<String, String> expectedRes = new HashMap<String, String>();
        expectedRes.put("date", "2016-03-07 10:59:01");
        expectedRes.put("num_cpus", "48");
        expectedRes.put("mhz_per_cpu", "2601");
        expectedRes.put("cpu_scaling_enabled", "true");
        expectedRes.put("library_build_type", "debug");

        expectedRes.put("Pass", "3");
        assertEquals(expectedRes, results);
        EasyMock.verify(mMockInvocationListener);

        // Test 1
        HashMap<String, Metric> resultTest1 = capture1.getValue();
        assertEquals(4, resultTest1.size());
        assertEquals("5", resultTest1.get("cpu_time").getMeasurements().getSingleString());
        assertEquals("5", resultTest1.get("real_time").getMeasurements().getSingleString());
        assertEquals("BM_one", resultTest1.get("name").getMeasurements().getSingleString());
        assertEquals(
                "109451958", resultTest1.get("iterations").getMeasurements().getSingleString());

        // Test 2
        HashMap<String, Metric> resultTest2 = capture2.getValue();
        assertEquals(4, resultTest2.size());
        assertEquals("11", resultTest2.get("cpu_time").getMeasurements().getSingleString());
        assertEquals("1", resultTest2.get("real_time").getMeasurements().getSingleString());
        assertEquals("BM_two", resultTest2.get("name").getMeasurements().getSingleString());
        assertEquals("50906784", resultTest2.get("iterations").getMeasurements().getSingleString());

        // Test 3
        HashMap<String, Metric> resultTest3 = capture3.getValue();
        assertEquals(5, resultTest3.size());
        assertEquals("60", resultTest3.get("cpu_time").getMeasurements().getSingleString());
        assertEquals("60", resultTest3.get("real_time").getMeasurements().getSingleString());
        assertEquals(
                "BM_string_strlen/64", resultTest3.get("name").getMeasurements().getSingleString());
        assertEquals("10499948", resultTest3.get("iterations").getMeasurements().getSingleString());
        assertEquals(
                "1061047935",
                resultTest3.get("bytes_per_second").getMeasurements().getSingleString());
    }

    /** Tests the parser for a two simple test with different keys on the second test. */
    @Test
    public void testParseSimpleFile_twoTests() throws Exception {
        ITestInvocationListener mMockInvocationListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mMockInvocationListener.testStarted((TestDescription) EasyMock.anyObject());
        EasyMock.expectLastCall().times(2);
        Capture<HashMap<String, Metric>> capture = new Capture<>();
        mMockInvocationListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.capture(capture));
        mMockInvocationListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mMockInvocationListener);
        CollectingOutputReceiver contents =  readInFile(GBENCH_OUTPUT_FILE_2);
        GoogleBenchmarkResultParser resultParser =
                new GoogleBenchmarkResultParser(TEST_RUN, mMockInvocationListener);
        resultParser.parse(contents);
        EasyMock.verify(mMockInvocationListener);

        HashMap<String, Metric> results = capture.getValue();
        assertEquals(4, results.size());
        assertEquals("5", results.get("cpu_time").getMeasurements().getSingleString());
        assertEquals("5", results.get("real_time").getMeasurements().getSingleString());
        assertEquals("BM_one", results.get("name").getMeasurements().getSingleString());
        assertEquals("109451958", results.get("iterations").getMeasurements().getSingleString());
    }

    /**
     * Tests the parser with an error in the context, should stop parsing because format is
     * unexpected.
     */
    @Test
    public void testParse_contextError() throws Exception {
        ITestInvocationListener mMockInvocationListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mMockInvocationListener.testRunFailed((String)EasyMock.anyObject());
        EasyMock.replay(mMockInvocationListener);
        CollectingOutputReceiver contents =  readInFile(GBENCH_OUTPUT_FILE_3);
        GoogleBenchmarkResultParser resultParser =
                new GoogleBenchmarkResultParser(TEST_RUN, mMockInvocationListener);
        resultParser.parse(contents);
        EasyMock.verify(mMockInvocationListener);
    }

    /** Tests the parser with an error: context is fine but no benchmark results */
    @Test
    public void testParse_noBenchmarkResults() throws Exception {
        ITestInvocationListener mMockInvocationListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mMockInvocationListener.testRunFailed((String)EasyMock.anyObject());
        EasyMock.replay(mMockInvocationListener);
        CollectingOutputReceiver contents =  readInFile(GBENCH_OUTPUT_FILE_4);
        GoogleBenchmarkResultParser resultParser =
                new GoogleBenchmarkResultParser(TEST_RUN, mMockInvocationListener);
        resultParser.parse(contents);
        EasyMock.verify(mMockInvocationListener);
    }

    /** Test for {@link GoogleBenchmarkResultParser#parseJsonToMap(JSONObject)} */
    @Test
    public void testJsonParse() throws JSONException {
        ITestInvocationListener mMockInvocationListener =
                EasyMock.createMock(ITestInvocationListener.class);
        String jsonTest = "{ \"key1\": \"value1\", \"key2\": 2, \"key3\": [\"one\", \"two\"]}";
        JSONObject test = new JSONObject(jsonTest);
        GoogleBenchmarkResultParser resultParser =
                new GoogleBenchmarkResultParser(TEST_RUN, mMockInvocationListener);
        Map<String, String> results = resultParser.parseJsonToMap(test);
        assertEquals(results.get("key1"), "value1");
        assertEquals(results.get("key2"), "2");
        assertEquals(results.get("key3"), "[\"one\",\"two\"]");
    }

    /**
     * Test when a warning is printed before the JSON output. As long as the JSON is fine, we should
     * still parse the output.
     */
    @Test
    public void testParseSimpleFile_withWarning() throws Exception {
        ITestInvocationListener mMockInvocationListener =
                EasyMock.createMock(ITestInvocationListener.class);
        mMockInvocationListener.testStarted((TestDescription) EasyMock.anyObject());
        Capture<HashMap<String, Metric>> capture = new Capture<>();
        mMockInvocationListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.capture(capture));
        EasyMock.replay(mMockInvocationListener);
        CollectingOutputReceiver contents = readInFile(GBENCH_OUTPUT_FILE_5);
        GoogleBenchmarkResultParser resultParser =
                new GoogleBenchmarkResultParser(TEST_RUN, mMockInvocationListener);
        resultParser.parse(contents);
        EasyMock.verify(mMockInvocationListener);

        HashMap<String, Metric> results = capture.getValue();
        assertEquals(5, results.size());
        assertEquals("19361", results.get("cpu_time").getMeasurements().getSingleString());
        assertEquals("44930", results.get("real_time").getMeasurements().getSingleString());
        assertEquals("BM_addInts", results.get("name").getMeasurements().getSingleString());
        assertEquals("36464", results.get("iterations").getMeasurements().getSingleString());
        assertEquals("ns", results.get("time_unit").getMeasurements().getSingleString());
    }
}
