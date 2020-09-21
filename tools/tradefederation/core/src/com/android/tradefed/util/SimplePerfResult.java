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

import com.android.tradefed.log.LogUtil.CLog;

import java.util.HashMap;
import java.util.Map;

/**
 * Object to hold all simpleperf test results
 * @see <a href="https://android.googlesource.com/platform/system/extras/+/master/simpleperf/">
 * Introduction of simpleperf</a>
 */
public class SimplePerfResult {

    private String commandRawOutput;
    private String simplePerfRawOutput;
    private Map<String, String> benchmarkMetrics;
    private Map<String, String> benchmarkComments;
    private String totalTestTime;

    /**
     * Constructor
     */
    public SimplePerfResult() {
        commandRawOutput = "";
        simplePerfRawOutput = "";
        totalTestTime = "";
        benchmarkMetrics = new HashMap<>();
        benchmarkComments = new HashMap<>();
    }

    /**
     * Get command raw output string
     * <p/>
     * If no output, an empty string will be returned
     *
     * @return {@link String} contains output for user-specified command
     */
    public String getCommandRawOutput() {
        return commandRawOutput;
    }

    protected void setCommandRawOutput(String s) {
        if (s != null) {
            commandRawOutput = s.trim();
        } else {
            CLog.e("Null command raw output passed in");
        }
    }

    /**
     * Get simpleperf raw output string
     * <p/>
     * If no output, an empty string will be returned
     *
     * @return {@link String} contains output on simpleperf results information
     */
    public String getSimplePerfRawOutput() {
        return simplePerfRawOutput;
    }

    protected void setSimplePerfRawOutput(String s) {
        if (s != null) {
            simplePerfRawOutput = s.trim();
        } else {
            CLog.e("Null simpleperf raw output passed in");
        }
    }

    protected void addBenchmarkMetrics(String key, String val) {
        benchmarkMetrics.put(key, val);
    }

    /**
     * Get benchmark metrics
     *
     * @return {@link Map} key: benchmark name, value: metrics
     */
    public Map<String, String> getBenchmarkMetrics() {
        return benchmarkMetrics;
    }

    protected void addBenchmarkComment(String key, String val) {
        benchmarkComments.put(key, val);
    }

    /**
     * Get benchmark comments
     *
     * @return {@link Map} key: benchmark name, value: comment
     */
    public Map<String, String> getBenchmarkComments() {
        return benchmarkComments;
    }

    /**
     * Get total test time
     *
     * @return {@link String} indicates total test time
     */
    public String getTotalTestTime() {
        return totalTestTime;
    }

    protected void setTotalTestTime(String time) {
        if (time != null) {
            totalTestTime = time;
        }
    }
}
