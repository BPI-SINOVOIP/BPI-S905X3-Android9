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

import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * Parses the results of Google Benchmark that run from shell,
 * and return a map with all the results.
 */
public class GoogleBenchmarkResultParser {

    private String mTestClassName;
    private final ITestInvocationListener mTestListener;

    public GoogleBenchmarkResultParser(String testClassName, ITestInvocationListener listener) {
        mTestClassName = testClassName;
        mTestListener = listener;
    }

    /**
     * Parse an individual output line.
     * name,iterations,real_time,cpu_time,bytes_per_second,items_per_second,label
     *
     * @param output  contains the test output
     * @return a map containing the number of tests that ran.
     */
    public Map<String, String> parse(CollectingOutputReceiver output) {
        String outputLogs = output.getOutput();
        Map<String, String> results = new HashMap<String, String>();
        JSONObject res = null;
        outputLogs = sanitizeOutput(outputLogs);
        try {
            res = new JSONObject(outputLogs);
            // Parse context first
            JSONObject context = res.getJSONObject("context");
            results = parseJsonToMap(context);
        } catch (JSONException e) {
            CLog.e("Failed to Parse context:");
            CLog.e(e);
            CLog.d("output was:\n%s\n", outputLogs);
            mTestListener.testRunFailed(String.format("Failed to Parse context: %s", e));
            return results;
        }
        try {
            // Benchmark results next
            JSONArray benchmarks = res.getJSONArray("benchmarks");
            for (int i = 0; i < benchmarks.length(); i++) {
                Map<String, String> testResults = new HashMap<String, String>();
                JSONObject testRes = (JSONObject) benchmarks.get(i);
                String name = testRes.getString("name");
                TestDescription testId = new TestDescription(mTestClassName, name);
                mTestListener.testStarted(testId);
                try {
                    testResults = parseJsonToMap(testRes);
                } catch (JSONException e) {
                    CLog.e(e);
                    mTestListener.testFailed(testId,String.format("Test failed to generate "
                                + "proper results: %s", e.getMessage()));
                }
                mTestListener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(testResults));
            }
            results.put("Pass", Integer.toString(benchmarks.length()));
        } catch (JSONException e) {
            CLog.e(e);
            results.put("ERROR", e.getMessage());
            mTestListener.testRunFailed(String.format("Failed to parse benchmarks results: %s", e));
        }
        return results;
    }

    /**
     * Helper that go over all json keys and put them in a map with their matching value.
     */
    protected Map<String, String> parseJsonToMap(JSONObject j) throws JSONException {
        Map<String, String> testResults = new HashMap<String, String>();
        Iterator<?> i = j.keys();
        while(i.hasNext()) {
            String key = (String) i.next();
            testResults.put(key, j.get(key).toString());
        }
        return testResults;
    }

    /**
     * In some cases a warning is printed before the JSON output. We remove it to avoid parsing
     * failures.
     */
    private String sanitizeOutput(String output) {
        // If it already looks like a proper JSON.
        // TODO: Maybe parse first and if it fails sanitize. Could avoid some failures?
        if (output.startsWith("{")) {
            return output;
        }
        int indexStart = output.indexOf('{');
        if (indexStart == -1) {
            // Nothing we can do here, the parsing will most likely fail afterward.
            CLog.w("Output does not look like a proper JSON.");
            return output;
        }
        String newOuput = output.substring(indexStart);
        CLog.d("We removed the following from the output: '%s'", output.subSequence(0, indexStart));
        return newOuput;
    }
}
