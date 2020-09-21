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
package com.android.tradefed.result;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.net.HttpHelper;
import com.android.tradefed.util.net.IHttpHelper;

import com.google.common.base.Joiner;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * A result reporter that encode test metrics results and branch, device info into JSON and POST
 * into an HTTP service endpoint
 */
@OptionClass(alias = "json-reporter")
public class JsonHttpTestResultReporter extends CollectingTestListener {

    /** separator for class name and method name when encoding test identifier */
    private final static String SEPARATOR = "#";
    private final static String RESULT_SEPARATOR = "##";

    /** constants used as keys in JSON results to be posted to remote end */
    private final static String KEY_METRICS = "metrics";
    private final static String KEY_BRANCH = "branch";
    private final static String KEY_BUILD_FLAVOR = "build_flavor";
    private final static String KEY_BUILD_ID = "build_id";
    private final static String KEY_RESULTS_NAME = "results_name";

    /** timeout for HTTP connection to posting endpoint */
    private final static int CONNECTION_TIMEOUT_MS = 60 * 1000;

    @Option(name="include-run-name", description="include test run name in reporting unit")
    private boolean mIncludeRunName = false;

    @Option(name = "posting-endpoint", description = "url for the HTTP data posting endpoint",
            importance = Importance.ALWAYS)
    private String mPostingEndpoint;

    @Option(name = "disable", description =
            "flag to skip reporting of all the results")
    private boolean mSkipReporting = false;

    @Option(name = "reporting-unit-key-suffix",
            description = "suffix to append after the regular reporting unit key")
    private String mReportingUnitKeySuffix = null;


    private boolean mHasInvocationFailures = false;
    private IInvocationContext mInvocationContext = null;

    @Override
    public void invocationStarted(IInvocationContext context) {
        super.invocationStarted(context);
        mInvocationContext = context;
    }

    @Override
    public void invocationFailed(Throwable cause) {
        super.invocationFailed(cause);
        mHasInvocationFailures = true;
    }

    @Override
    public void invocationEnded(long elapsedTime) {
        super.invocationEnded(elapsedTime);

        if (mSkipReporting) {
            CLog.d("Skipping reporting because it's disabled.");
        } else if (mHasInvocationFailures) {
            CLog.d("Skipping reporting beacuse there are invocation failures.");
        } else {
            try {
                postResults(convertMetricsToJson(getRunResults()));
            } catch (JSONException e) {
                CLog.e("JSONException while converting test metrics.");
                CLog.e(e);
            }
        }
    }

    /**
     * Post data to the specified HTTP endpoint
     * @param postData data to be posted
     */
    protected void postResults(JSONObject postData) {
        IHttpHelper helper = new HttpHelper();
        OutputStream outputStream = null;
        String data = postData.toString();
        CLog.d("Attempting to post %s: Data: '%s'", mPostingEndpoint, data);
        try {
            HttpURLConnection conn = helper.createJsonConnection(
                    new URL(mPostingEndpoint), "POST");
            conn.setConnectTimeout(CONNECTION_TIMEOUT_MS);
            conn.setReadTimeout(CONNECTION_TIMEOUT_MS);
            outputStream = conn.getOutputStream();
            outputStream.write(data.getBytes());

            String response = StreamUtil.getStringFromStream(conn.getInputStream()).trim();
            int responseCode = conn.getResponseCode();
            if (responseCode < 200 || responseCode >= 300) {
                // log an error but don't do any explicit exceptions if response code is not 2xx
                CLog.e("Posting failure. code: %d, response: %s", responseCode, response);
            } else {
                IBuildInfo buildInfo = mInvocationContext.getBuildInfos().get(0);
                CLog.d("Successfully posted results, build: %s, raw data: %s",
                        buildInfo.getBuildId(), postData);
            }
        } catch (IOException e) {
            CLog.e("IOException occurred while reporting to HTTP endpoint: %s", mPostingEndpoint);
            CLog.e(e);
        } finally {
            StreamUtil.close(outputStream);
        }
    }

    /**
     * A util method that converts test metrics and invocation context to json format
     */
    JSONObject convertMetricsToJson(Collection<TestRunResult> runResults) throws JSONException {
        JSONObject allTestMetrics = new JSONObject();

        StringBuffer resultsName = new StringBuffer();
        // loops over all test runs
        for (TestRunResult runResult : runResults) {

            // Parse run metrics
            if (runResult.getRunMetrics().size() > 0) {
                JSONObject runResultMetrics = new JSONObject(runResult.getRunMetrics());
                String reportingUnit = runResult.getName();
                if (mReportingUnitKeySuffix != null && !mReportingUnitKeySuffix.isEmpty()) {
                    reportingUnit += mReportingUnitKeySuffix;
                }
                allTestMetrics.put(reportingUnit, runResultMetrics);
                resultsName.append(String.format("%s%s", reportingUnit, RESULT_SEPARATOR));
            } else {
                CLog.d("Skipping metrics for %s because results are empty.", runResult.getName());
            }

            // Parse test metrics
            Map<TestDescription, TestResult> testResultMap = runResult.getTestResults();
            for (Entry<TestDescription, TestResult> entry : testResultMap.entrySet()) {
                TestDescription testDescription = entry.getKey();
                TestResult testResult = entry.getValue();
                Joiner joiner = Joiner.on(SEPARATOR).skipNulls();
                String reportingUnit = joiner.join(
                        mIncludeRunName ? runResult.getName() : null,
                                testDescription.getClassName(), testDescription.getTestName());
                if (mReportingUnitKeySuffix != null && !mReportingUnitKeySuffix.isEmpty()) {
                    reportingUnit += mReportingUnitKeySuffix;
                }
                resultsName.append(String.format("%s%s", reportingUnit, RESULT_SEPARATOR));
                if (testResult.getMetrics().size() > 0) {
                    JSONObject testResultMetrics = new JSONObject(testResult.getMetrics());
                    allTestMetrics.put(reportingUnit, testResultMetrics);
                } else {
                    CLog.d("Skipping metrics for %s because results are empty.", testDescription);
                }
            }
        }
        // get build info, and throw an exception if there are multiple (not supporting multi-device
        // result reporting
        List<IBuildInfo> buildInfos = mInvocationContext.getBuildInfos();
        if (buildInfos.size() != 1) {
            throw new IllegalArgumentException(String.format(
                    "Only expected 1 build info, actual: [%d]", buildInfos.size()));
        }
        IBuildInfo buildInfo = buildInfos.get(0);
        JSONObject result = new JSONObject();
        result.put(KEY_RESULTS_NAME, resultsName);
        result.put(KEY_METRICS, allTestMetrics);
        result.put(KEY_BRANCH, buildInfo.getBuildBranch());
        result.put(KEY_BUILD_FLAVOR, buildInfo.getBuildFlavor());
        result.put(KEY_BUILD_ID, buildInfo.getBuildId());
        return result;
    }
}
