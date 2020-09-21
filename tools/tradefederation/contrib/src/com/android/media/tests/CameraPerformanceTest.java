/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.media.tests;

import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This test invocation runs android.hardware.camera2.cts.PerformanceTest - Camera2 API use case
 * performance KPIs (Key Performance Indicator), such as camera open time, session creation time,
 * shutter lag etc. The KPI data will be parsed and reported.
 */
@OptionClass(alias = "camera-framework")
public class CameraPerformanceTest extends CameraTestBase {

    private static final String TEST_CAMERA_LAUNCH = "testCameraLaunch";
    private static final String TEST_SINGLE_CAPTURE = "testSingleCapture";
    private static final String TEST_REPROCESSING_LATENCY = "testReprocessingLatency";
    private static final String TEST_REPROCESSING_THROUGHPUT = "testReprocessingThroughput";

    // KPIs to be reported. The key is test methods and the value is KPIs in the method.
    private final ImmutableMultimap<String, String> mReportingKpis =
            new ImmutableMultimap.Builder<String, String>()
                    .put(TEST_CAMERA_LAUNCH, "Camera launch time")
                    .put(TEST_CAMERA_LAUNCH, "Camera start preview time")
                    .put(TEST_SINGLE_CAPTURE, "Camera capture result latency")
                    .put(TEST_REPROCESSING_LATENCY, "YUV reprocessing shot to shot latency")
                    .put(TEST_REPROCESSING_LATENCY, "opaque reprocessing shot to shot latency")
                    .put(TEST_REPROCESSING_THROUGHPUT, "YUV reprocessing capture latency")
                    .put(TEST_REPROCESSING_THROUGHPUT, "opaque reprocessing capture latency")
                    .build();

    // JSON format keymap, key is test method name and the value is stream name in Json file
    private static final ImmutableMap<String, String> METHOD_JSON_KEY_MAP =
            new ImmutableMap.Builder<String, String>()
                    .put(TEST_CAMERA_LAUNCH, "test_camera_launch")
                    .put(TEST_SINGLE_CAPTURE, "test_single_capture")
                    .put(TEST_REPROCESSING_LATENCY, "test_reprocessing_latency")
                    .put(TEST_REPROCESSING_THROUGHPUT, "test_reprocessing_throughput")
                    .build();

    private <E extends Number> double getAverage(List<E> list) {
        double sum = 0;
        int size = list.size();
        for (E num : list) {
            sum += num.doubleValue();
        }
        if (size == 0) {
            return 0.0;
        }
        return (sum / size);
    }

    public CameraPerformanceTest() {
        // Set up the default test info. But this is subject to be overwritten by options passed
        // from commands.
        setTestPackage("android.camera.cts");
        setTestClass("android.hardware.camera2.cts.PerformanceTest");
        setTestRunner("android.support.test.runner.AndroidJUnitRunner");
        setRuKey("camera_framework_performance");
        setTestTimeoutMs(10 * 60 * 1000); // 10 mins
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        runInstrumentationTest(listener, new CollectingListener(listener));
    }

    /**
     * A listener to collect the output from test run and fatal errors
     */
    private class CollectingListener extends DefaultCollectingListener {

        public CollectingListener(ITestInvocationListener listener) {
            super(listener);
        }

        @Override
        public void handleMetricsOnTestEnded(TestDescription test, Map<String, String> testMetrics) {
            // Pass the test name for a key in the aggregated metrics, because
            // it is used to generate the key of the final metrics to post at the end of test run.
            for (Map.Entry<String, String> metric : testMetrics.entrySet()) {
                getAggregatedMetrics().put(test.getTestName(), metric.getValue());
            }
        }

        @Override
        public void handleTestRunEnded(
                ITestInvocationListener listener,
                long elapsedTime,
                Map<String, String> runMetrics) {
            // Report metrics at the end of test run.
            Map<String, String> result = parseResult(getAggregatedMetrics());
            listener.testRunEnded(getTestDurationMs(), TfMetricProtoUtil.upgradeConvert(result));
        }
    }

    /**
     * Parse Camera Performance KPIs results and then put them all together to post the final
     * report.
     *
     * @return a {@link HashMap} that contains pairs of kpiName and kpiValue
     */
    private Map<String, String> parseResult(Map<String, String> metrics) {

        // if json report exists, return the parse results
        CtsJsonResultParser ctsJsonResultParser = new CtsJsonResultParser();

        if (ctsJsonResultParser.isJsonFileExist()) {
            return ctsJsonResultParser.parse();
        }

        Map<String, String> resultsAll = new HashMap<String, String>();

        CtsResultParserBase parser;
        for (Map.Entry<String, String> metric : metrics.entrySet()) {
            String testMethod = metric.getKey();
            String testResult = metric.getValue();
            CLog.d("test name %s", testMethod);
            CLog.d("test result %s", testResult);
            // Probe which result parser should be used.
            if (shouldUseCtsXmlResultParser(testResult)) {
                parser = new CtsXmlResultParser();
            } else {
                parser = new CtsDelimitedResultParser();
            }

            // Get pairs of { KPI name, KPI value } from stdout that each test outputs.
            // Assuming that a device has both the front and back cameras, parser will return
            // 2 KPIs in HashMap. For an example of testCameraLaunch,
            //   {
            //     ("Camera 0 Camera launch time", "379.2"),
            //     ("Camera 1 Camera launch time", "272.8"),
            //   }
            Map<String, String> testKpis = parser.parse(testResult, testMethod);
            for (String k : testKpis.keySet()) {
                if (resultsAll.containsKey(k)) {
                    throw new RuntimeException(
                            String.format("KPI name (%s) conflicts with the existing names.", k));
                }
            }
            parser.clear();

            // Put each result together to post the final result
            resultsAll.putAll(testKpis);
        }
        return resultsAll;
    }

    public boolean shouldUseCtsXmlResultParser(String result) {
        final String XML_DECLARATION = "<?xml";
        return (result.startsWith(XML_DECLARATION)
                || result.startsWith(XML_DECLARATION.toUpperCase()));
    }

    /** Data class of CTS test results for Camera framework performance test */
    public static class CtsMetric {
        String testMethod;  // "testSingleCapture"
        String source;      // "android.hardware.camera2.cts.PerformanceTest#testSingleCapture:327"
        // or "testSingleCapture" (just test method name)
        String message; // "Camera 0: Camera capture latency"
        String type; // "lower_better"
        String unit;        // "ms"
        String value;       // "691.0" (is an average of 736.0 688.0 679.0 667.0 686.0)
        String schemaKey;   // RU schema key = message (+ testMethodName if needed), derived

        // eg. "android.hardware.camera2.cts.PerformanceTest#testSingleCapture:327"
        public static final Pattern SOURCE_REGEX =
                Pattern.compile("^(?<package>[a-zA-Z\\d\\._$]+)#(?<method>[a-zA-Z\\d_$]+)(:\\d+)?");
        // eg. "Camera 0: Camera capture latency"
        public static final Pattern MESSAGE_REGEX =
                Pattern.compile("^Camera\\s+(?<cameraId>\\d+):\\s+(?<kpiName>.*)");

        CtsMetric(
                String testMethod,
                String source,
                String message,
                String type,
                String unit,
                String value) {
            this.testMethod = testMethod;
            this.source = source;
            this.message = message;
            this.type = type;
            this.unit = unit;
            this.value = value;
            this.schemaKey = getRuSchemaKeyName(message);
        }

        public boolean matches(String testMethod, String kpiName) {
            return (this.testMethod.equals(testMethod) && this.message.endsWith(kpiName));
        }

        public String getRuSchemaKeyName(String message) {
            // Note 1: The key shouldn't contain ":" for side by side report.
            String schemaKey = message.replace(":", "");
            // Note 2: Two tests testReprocessingLatency & testReprocessingThroughput have the
            // same metric names to report results. To make the report key name distinct,
            // the test name is added as prefix for these tests for them.
            final String[] TEST_NAMES_AS_PREFIX = {
                "testReprocessingLatency", "testReprocessingThroughput"
            };
            for (String testName : TEST_NAMES_AS_PREFIX) {
                if (testMethod.endsWith(testName)) {
                    schemaKey = String.format("%s_%s", testName, schemaKey);
                    break;
                }
            }
            return schemaKey;
        }

        public String getTestMethodNameInSource(String source) {
            Matcher m = SOURCE_REGEX.matcher(source);
            if (!m.matches()) {
                return source;
            }
            return m.group("method");
        }
    }

    /**
     * Base class of CTS test result parser. This is inherited to two derived parsers,
     * {@link CtsDelimitedResultParser} for legacy delimiter separated format and
     * {@link CtsXmlResultParser} for XML typed format introduced since NYC.
     */
    public abstract class CtsResultParserBase {

        protected CtsMetric mSummary;
        protected List<CtsMetric> mDetails = new ArrayList<>();

        /**
         * Parse Camera Performance KPIs result first, then leave the only KPIs that matter.
         *
         * @param result String to be parsed
         * @param testMethod test method name used to leave the only metric that matters
         * @return a {@link HashMap} that contains kpiName and kpiValue
         */
        public abstract Map<String, String> parse(String result, String testMethod);

        protected Map<String, String> filter(List<CtsMetric> metrics, String testMethod) {
            Map<String, String> filtered = new HashMap<String, String>();
            for (CtsMetric metric : metrics) {
                for (String kpiName : mReportingKpis.get(testMethod)) {
                    // Post the data only when it matches with the given methods and KPI names.
                    if (metric.matches(testMethod, kpiName)) {
                        filtered.put(metric.schemaKey, metric.value);
                    }
                }
            }
            return filtered;
        }

        protected void setSummary(CtsMetric summary) {
            mSummary = summary;
        }

        protected void addDetail(CtsMetric detail) {
            mDetails.add(detail);
        }

        protected List<CtsMetric> getDetails() {
            return mDetails;
        }

        void clear() {
            mSummary = null;
            mDetails.clear();
        }
    }

    /**
     * Parses the camera performance test generated by the underlying instrumentation test and
     * returns it to test runner for later reporting.
     *
     * <p>TODO(liuyg): Rename this class to not reference CTS.
     *
     * <p>Format: (summary message)| |(type)|(unit)|(value) ++++
     * (source)|(message)|(type)|(unit)|(value)... +++ ...
     *
     * <p>Example: Camera launch average time for Camera 1| |lower_better|ms|586.6++++
     * android.hardware.camera2.cts.PerformanceTest#testCameraLaunch:171| Camera 0: Camera open
     * time|lower_better|ms|74.0 100.0 70.0 67.0 82.0 +++
     * android.hardware.camera2.cts.PerformanceTest#testCameraLaunch:171| Camera 0: Camera configure
     * stream time|lower_better|ms|9.0 5.0 5.0 8.0 5.0 ...
     *
     * <p>See also com.android.cts.util.ReportLog for the format detail.
     */
    public class CtsDelimitedResultParser extends CtsResultParserBase {
        private static final String LOG_SEPARATOR = "\\+\\+\\+";
        private static final String SUMMARY_SEPARATOR = "\\+\\+\\+\\+";
        private final Pattern mSummaryRegex =
                Pattern.compile(
                        "^(?<message>[^|]+)\\| \\|(?<type>[^|]+)\\|(?<unit>[^|]+)\\|(?<value>[0-9 .]+)");
        private final Pattern mDetailRegex =
                Pattern.compile(
                        "^(?<source>[^|]+)\\|(?<message>[^|]+)\\|(?<type>[^|]+)\\|(?<unit>[^|]+)\\|"
                                + "(?<values>[0-9 .]+)");

        @Override
        public Map<String, String> parse(String result, String testMethod) {
            parseToCtsMetrics(result, testMethod);
            parseToCtsMetrics(result, testMethod);
            return filter(getDetails(), testMethod);
        }

        void parseToCtsMetrics(String result, String testMethod) {
            // Split summary and KPIs from stdout passes as parameter.
            String[] output = result.split(SUMMARY_SEPARATOR);
            if (output.length != 2) {
                throw new RuntimeException("Value not in the correct format");
            }
            Matcher summaryMatcher = mSummaryRegex.matcher(output[0].trim());

            // Parse summary.
            // Example: "Camera launch average time for Camera 1| |lower_better|ms|586.6++++"
            if (summaryMatcher.matches()) {
                setSummary(
                        new CtsMetric(
                                testMethod,
                                null,
                                summaryMatcher.group("message"),
                                summaryMatcher.group("type"),
                                summaryMatcher.group("unit"),
                                summaryMatcher.group("value")));
            } else {
                // Fall through since the summary is not posted as results.
                CLog.w("Summary not in the correct format");
            }

            // Parse KPIs.
            // Example: "android.hardware.camera2.cts.PerformanceTest#testCameraLaunch:171|Camera 0:
            // Camera open time|lower_better|ms|74.0 100.0 70.0 67.0 82.0 +++"
            String[] details = output[1].split(LOG_SEPARATOR);
            for (String detail : details) {
                Matcher detailMatcher = mDetailRegex.matcher(detail.trim());
                if (detailMatcher.matches()) {
                    // get average of kpi values
                    List<Double> values = new ArrayList<>();
                    for (String value : detailMatcher.group("values").split("\\s+")) {
                        values.add(Double.parseDouble(value));
                    }
                    String kpiValue = String.format("%.1f", getAverage(values));
                    addDetail(
                            new CtsMetric(
                                    testMethod,
                                    detailMatcher.group("source"),
                                    detailMatcher.group("message"),
                                    detailMatcher.group("type"),
                                    detailMatcher.group("unit"),
                                    kpiValue));
                } else {
                    throw new RuntimeException("KPI not in the correct format");
                }
            }
        }
    }

    /**
     * Parses the CTS test results in a XML format introduced since NYC.
     * Format:
     *   <Summary>
     *       <Metric source="android.hardware.camera2.cts.PerformanceTest#testSingleCapture:327"
     *               message="Camera capture result average latency for all cameras "
     *               score_type="lower_better"
     *               score_unit="ms"
     *           <Value>353.9</Value>
     *       </Metric>
     *   </Summary>
     *   <Detail>
     *       <Metric source="android.hardware.camera2.cts.PerformanceTest#testSingleCapture:303"
     *               message="Camera 0: Camera capture latency"
     *               score_type="lower_better"
     *               score_unit="ms">
     *           <Value>335.0</Value>
     *           <Value>302.0</Value>
     *           <Value>316.0</Value>
     *       </Metric>
     *   </Detail>
     *  }
     * See also com.android.compatibility.common.util.ReportLog for the format detail.
     */
    public class CtsXmlResultParser extends CtsResultParserBase {
        private static final String ENCODING = "UTF-8";
        // XML constants
        private static final String DETAIL_TAG = "Detail";
        private static final String METRIC_TAG = "Metric";
        private static final String MESSAGE_ATTR = "message";
        private static final String SCORETYPE_ATTR = "score_type";
        private static final String SCOREUNIT_ATTR = "score_unit";
        private static final String SOURCE_ATTR = "source";
        private static final String SUMMARY_TAG = "Summary";
        private static final String VALUE_TAG = "Value";
        private String mTestMethod;

        @Override
        public Map<String, String> parse(String result, String testMethod) {
            try {
                mTestMethod = testMethod;
                XmlPullParserFactory factory = XmlPullParserFactory.newInstance();
                XmlPullParser parser = factory.newPullParser();
                parser.setInput(new ByteArrayInputStream(result.getBytes(ENCODING)), ENCODING);
                parser.nextTag();
                parse(parser);
                return filter(getDetails(), testMethod);
            } catch (XmlPullParserException | IOException e) {
                throw new RuntimeException("Failed to parse results in XML.", e);
            }
        }

        /**
         * Parses a {@link CtsMetric} from the given XML parser.
         *
         * @param parser
         * @throws IOException
         * @throws XmlPullParserException
         */
        private void parse(XmlPullParser parser) throws XmlPullParserException, IOException {
            parser.require(XmlPullParser.START_TAG, null, SUMMARY_TAG);
            parser.nextTag();
            setSummary(parseToCtsMetrics(parser));
            parser.nextTag();
            parser.require(XmlPullParser.END_TAG, null, SUMMARY_TAG);
            parser.nextTag();
            if (parser.getName().equals(DETAIL_TAG)) {
                while (parser.nextTag() == XmlPullParser.START_TAG) {
                    addDetail(parseToCtsMetrics(parser));
                }
                parser.require(XmlPullParser.END_TAG, null, DETAIL_TAG);
            }
        }

        CtsMetric parseToCtsMetrics(XmlPullParser parser)
                throws IOException, XmlPullParserException {
            parser.require(XmlPullParser.START_TAG, null, METRIC_TAG);
            String source = parser.getAttributeValue(null, SOURCE_ATTR);
            String message = parser.getAttributeValue(null, MESSAGE_ATTR);
            String type = parser.getAttributeValue(null, SCORETYPE_ATTR);
            String unit = parser.getAttributeValue(null, SCOREUNIT_ATTR);
            List<Double> values = new ArrayList<>();
            while (parser.nextTag() == XmlPullParser.START_TAG) {
                parser.require(XmlPullParser.START_TAG, null, VALUE_TAG);
                values.add(Double.parseDouble(parser.nextText()));
                parser.require(XmlPullParser.END_TAG, null, VALUE_TAG);
            }
            String kpiValue = String.format("%.1f", getAverage(values));
            parser.require(XmlPullParser.END_TAG, null, METRIC_TAG);
            return new CtsMetric(mTestMethod, source, message, type, unit, kpiValue);
        }
    }

    /*
     * Parse the Json report from the Json String
     * "test_single_capture":
     * {"camera_id":"0","camera_capture_latency":[264.0,229.0,229.0,237.0,234.0],
     * "camera_capture_result_latency":[230.0,197.0,196.0,204.0,202.0]},"
     * "test_reprocessing_latency":
     * {"camera_id":"0","format":35,"reprocess_type":"YUV reprocessing",
     * "capture_message":"shot to shot latency","latency":[102.0,101.0,99.0,99.0,100.0,101.0],
     * "camera_reprocessing_shot_to_shot_average_latency":100.33333333333333},
     *
     * TODO: move this to a seperate class
     */
    public class CtsJsonResultParser {

        // report json file set in
        // cts/tools/cts-tradefed/res/config/cts-preconditions.xml
        private static final String JSON_RESULT_FILE =
                "/sdcard/report-log-files/CtsCameraTestCases.reportlog.json";
        private static final String CAMERA_ID_KEY = "camera_id";
        private static final String REPROCESS_TYPE_KEY = "reprocess_type";
        private static final String CAPTURE_MESSAGE_KEY = "capture_message";
        private static final String LATENCY_KEY = "latency";

        public Map<String, String> parse() {

            Map<String, String> metrics = new HashMap<>();

            String jsonString = getFormatedJsonReportFromFile();
            if (null == jsonString) {
                throw new RuntimeException("Get null json report string.");
            }

            Map<String, List<Double>> metricsData = new HashMap<>();

            try {
                JSONObject jsonObject = new JSONObject(jsonString);

                for (String testMethod : METHOD_JSON_KEY_MAP.keySet()) {

                    JSONArray jsonArray =
                            (JSONArray) jsonObject.get(METHOD_JSON_KEY_MAP.get(testMethod));

                    switch (testMethod) {
                        case TEST_REPROCESSING_THROUGHPUT:
                        case TEST_REPROCESSING_LATENCY:
                            for (int i = 0; i < jsonArray.length(); i++) {
                                JSONObject element = jsonArray.getJSONObject(i);

                                // create a kpiKey from camera id,
                                // reprocess type and capture message
                                String cameraId = element.getString(CAMERA_ID_KEY);
                                String reprocessType = element.getString(REPROCESS_TYPE_KEY);
                                String captureMessage = element.getString(CAPTURE_MESSAGE_KEY);
                                String kpiKey =
                                        String.format(
                                                "%s_Camera %s %s %s",
                                                testMethod,
                                                cameraId,
                                                reprocessType,
                                                captureMessage);

                                // read the data array from json object
                                JSONArray jsonDataArray = element.getJSONArray(LATENCY_KEY);
                                if (!metricsData.containsKey(kpiKey)) {
                                    List<Double> list = new ArrayList<>();
                                    metricsData.put(kpiKey, list);
                                }
                                for (int j = 0; j < jsonDataArray.length(); j++) {
                                    metricsData.get(kpiKey).add(jsonDataArray.getDouble(j));
                                }
                            }
                            break;
                        case TEST_SINGLE_CAPTURE:
                        case TEST_CAMERA_LAUNCH:
                            for (int i = 0; i < jsonArray.length(); i++) {
                                JSONObject element = jsonArray.getJSONObject(i);

                                String cameraid = element.getString(CAMERA_ID_KEY);
                                for (String kpiName : mReportingKpis.get(testMethod)) {

                                    // the json key is all lower case
                                    String jsonKey = kpiName.toLowerCase().replace(" ", "_");
                                    String kpiKey =
                                            String.format("Camera %s %s", cameraid, kpiName);
                                    if (!metricsData.containsKey(kpiKey)) {
                                        List<Double> list = new ArrayList<>();
                                        metricsData.put(kpiKey, list);
                                    }
                                    JSONArray jsonDataArray = element.getJSONArray(jsonKey);
                                    for (int j = 0; j < jsonDataArray.length(); j++) {
                                        metricsData.get(kpiKey).add(jsonDataArray.getDouble(j));
                                    }
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }
            } catch (JSONException e) {
                CLog.w("JSONException: %s in string %s", e.getMessage(), jsonString);
            }

            // take the average of all data for reporting
            for (String kpiKey : metricsData.keySet()) {
                String kpiValue = String.format("%.1f", getAverage(metricsData.get(kpiKey)));
                metrics.put(kpiKey, kpiValue);
            }
            return metrics;
        }

        public boolean isJsonFileExist() {
            try {
                return getDevice().doesFileExist(JSON_RESULT_FILE);
            } catch (DeviceNotAvailableException e) {
                throw new RuntimeException("Failed to check json report file on device.", e);
            }
        }

        /*
         * read json report file on the device
         */
        private String getFormatedJsonReportFromFile() {
            String jsonString = null;
            try {
                // pull the json report file from device
                File outputFile = FileUtil.createTempFile("json", ".txt");
                getDevice().pullFile(JSON_RESULT_FILE, outputFile);
                jsonString = reformatJsonString(FileUtil.readStringFromFile(outputFile));
            } catch (IOException e) {
                CLog.w("Couldn't parse the output json log file: ", e);
            } catch (DeviceNotAvailableException e) {
                CLog.w("Could not pull file: %s, error: %s", JSON_RESULT_FILE, e);
            }
            return jsonString;
        }

        // Reformat the json file to remove duplicate keys
        private String reformatJsonString(String jsonString) {

            final String TEST_METRICS_PATTERN = "\\\"([a-z0-9_]*)\\\":(\\{[^{}]*\\})";
            StringBuilder newJsonBuilder = new StringBuilder();
            // Create map of stream names and json objects.
            HashMap<String, List<String>> jsonMap = new HashMap<>();
            Pattern p = Pattern.compile(TEST_METRICS_PATTERN);
            Matcher m = p.matcher(jsonString);
            while (m.find()) {
                String key = m.group(1);
                String value = m.group(2);
                if (!jsonMap.containsKey(key)) {
                    jsonMap.put(key, new ArrayList<String>());
                }
                jsonMap.get(key).add(value);
            }
            // Rewrite json string as arrays.
            newJsonBuilder.append("{");
            boolean firstLine = true;
            for (String key : jsonMap.keySet()) {
                if (!firstLine) {
                    newJsonBuilder.append(",");
                } else {
                    firstLine = false;
                }
                newJsonBuilder.append("\"").append(key).append("\":[");
                boolean firstValue = true;
                for (String stream : jsonMap.get(key)) {
                    if (!firstValue) {
                        newJsonBuilder.append(",");
                    } else {
                        firstValue = false;
                    }
                    newJsonBuilder.append(stream);
                }
                newJsonBuilder.append("]");
            }
            newJsonBuilder.append("}");
            return newJsonBuilder.toString();
        }
    }
}
