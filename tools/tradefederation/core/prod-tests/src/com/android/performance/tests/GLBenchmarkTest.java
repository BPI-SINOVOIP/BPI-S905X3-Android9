/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.performance.tests;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import java.io.File;
import java.io.IOException;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

/**
 * A harness that launches GLBenchmark and reports result. Requires GLBenchmark
 * custom XML. Assumes GLBenchmark app is already installed on device.
 */
public class GLBenchmarkTest implements IDeviceTest, IRemoteTest {

    private static final String RUN_KEY = "glbenchmark";
    private static final long TIMEOUT_MS = 30 * 60 * 1000;
    private static final long POLLING_INTERVAL_MS = 5 * 1000;
    private static final Map<String, String> METRICS_KEY_MAP = createMetricsKeyMap();

    private ITestDevice mDevice;

    @Option(name = "custom-xml-path", description = "local path for GLBenchmark custom.xml",
            importance = Importance.ALWAYS)
    private File mGlbenchmarkCustomXmlLocal = new File("/tmp/glbenchmark_custom.xml");

    @Option(name = "gl-package-name", description = "GLBenchmark package name")
    private String mGlbenchmarkPackageName = "com.glbenchmark.glbenchmark25";

    @Option(name = "gl-version", description = "GLBenchmark version (e.g. 2.5.1_b306a5)")
    private String mGlbenchmarkVersion = "2.5.1_b306a5";

    private String mGlbenchmarkCacheDir =
            "${EXTERNAL_STORAGE}/Android/data/" + mGlbenchmarkPackageName + "/cache/";
    private String mGlbenchmarkCustomXmlPath =
            mGlbenchmarkCacheDir + "custom.xml";
    private String mGlbenchmarkResultXmlPath =
            mGlbenchmarkCacheDir + "last_results_" + mGlbenchmarkVersion + ".xml";
    private String mGlbenchmarkExcelResultXmlPath =
            mGlbenchmarkCacheDir + "results_%s_0.xml";
    private String mGlbenchmarkAllResultXmlPath =
            mGlbenchmarkCacheDir + "results*.xml";

    private static Map<String, String> createMetricsKeyMap() {
        Map<String, String> result = new HashMap<String, String>();
        result.put("Fill rate - C24Z16", "fill-rate");
        result.put("Fill rate - C24Z16 Offscreen", "fill-rate-offscreen");
        result.put("Triangle throughput: Textured - C24Z16", "triangle-c24z16");
        result.put("Triangle throughput: Textured - C24Z16 Offscreen",
                "triangle-c24z16-offscreen");
        result.put("Triangle throughput: Textured - C24Z16 Vertex lit",
                "triangle-c24z16-vertex-lit");
        result.put("Triangle throughput: Textured - C24Z16 Offscreen Vertex lit",
                "triangle-c24z16-offscreen-vertex-lit");
        result.put("Triangle throughput: Textured - C24Z16 Fragment lit",
                "triangle-c24z16-fragment-lit");
        result.put("Triangle throughput: Textured - C24Z16 Offscreen Fragment lit",
                "triangle-c24z16-offscreen-fragment-lit");
        result.put("GLBenchmark 2.5 Egypt HD - C24Z16", "egypt-hd-c24z16");
        result.put("GLBenchmark 2.5 Egypt HD - C24Z16 Offscreen", "egypt-hd-c24z16-offscreen");
        result.put("GLBenchmark 2.5 Egypt HD PVRTC4 - C24Z16", "egypt-hd-pvrtc4-c24z16");
        result.put("GLBenchmark 2.5 Egypt HD PVRTC4 - C24Z16 Offscreen",
                "egypt-hd-pvrtc4-c24z16-offscreen");
        result.put("GLBenchmark 2.5 Egypt HD - C24Z24MS4", "egypt-hd-c24z24ms4");
        result.put("GLBenchmark 2.5 Egypt HD - C24Z16 Fixed timestep",
                "egypt-hd-c24z16-fixed-timestep");
        result.put("GLBenchmark 2.5 Egypt HD - C24Z16 Fixed timestep Offscreen",
                "egypt-hd-c24z16-fixed-timestep-offscreen");
        result.put("GLBenchmark 2.1 Egypt Classic - C16Z16", "egypt-classic-c16z16");
        result.put("GLBenchmark 2.1 Egypt Classic - C16Z16 Offscreen",
                "egypt-classic-c16z16-offscreen");
        return Collections.unmodifiableMap(result);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        TestDescription testId = new TestDescription(getClass().getCanonicalName(), RUN_KEY);
        ITestDevice device = getDevice();

        // delete old result
        device.executeShellCommand(String.format("rm %s", mGlbenchmarkResultXmlPath));
        device.executeShellCommand(String.format("rm %s", mGlbenchmarkAllResultXmlPath));

        // push glbenchmark custom xml to device
        device.pushFile(mGlbenchmarkCustomXmlLocal, mGlbenchmarkCustomXmlPath);

        listener.testRunStarted(RUN_KEY, 0);
        listener.testStarted(testId);

        long testStartTime = System.currentTimeMillis();
        boolean isRunningBenchmark;

        boolean isTimedOut = false;
        boolean isResultGenerated = false;
        Map<String, String> metrics = new HashMap<String, String>();
        String errMsg = null;

        String deviceModel = device.executeShellCommand("getprop ro.product.model");
        String resultExcelXmlPath = String.format(mGlbenchmarkExcelResultXmlPath,
                deviceModel.trim().replaceAll("[ -]", "_").toLowerCase());
        CLog.i("Result excel xml path:" + resultExcelXmlPath);

        // start glbenchmark and wait for test to complete
        isTimedOut = false;
        long benchmarkStartTime = System.currentTimeMillis();

        device.executeShellCommand("am start -a android.intent.action.MAIN "
                + "-n com.glbenchmark.glbenchmark25/com.glbenchmark.activities.MainActivity "
                + "--ez custom true");
        isRunningBenchmark = true;
        while (isRunningBenchmark && !isResultGenerated && !isTimedOut) {
            RunUtil.getDefault().sleep(POLLING_INTERVAL_MS);
            isTimedOut = (System.currentTimeMillis() - benchmarkStartTime >= TIMEOUT_MS);
            isResultGenerated = device.doesFileExist(resultExcelXmlPath);
            isRunningBenchmark = device.executeShellCommand("ps").contains("glbenchmark");
        }

        if (isTimedOut) {
            errMsg = "GLBenchmark timed out.";
        } else {
            // pull result from device
            File benchmarkReport = device.pullFile(mGlbenchmarkResultXmlPath);
            if (benchmarkReport != null) {
                // parse result
                CLog.i("== GLBenchmark result ==");
                Map<String, String> benchmarkResult = parseResultXml(benchmarkReport);
                if (benchmarkResult == null) {
                    errMsg = "Failed to parse GLBenchmark result XML.";
                } else {
                    metrics = benchmarkResult;
                }
                // delete results from device and host
                device.executeShellCommand(String.format("rm %s", mGlbenchmarkResultXmlPath));
                device.executeShellCommand(String.format("rm %s", resultExcelXmlPath));
                benchmarkReport.delete();
            } else {
                errMsg = "GLBenchmark report not found.";
            }
        }
        if (errMsg != null) {
            CLog.e(errMsg);
            listener.testFailed(testId, errMsg);
            listener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(metrics));
            listener.testRunFailed(errMsg);
        } else {
            long durationMs = System.currentTimeMillis() - testStartTime;
            listener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(metrics));
            listener.testRunEnded(durationMs, TfMetricProtoUtil.upgradeConvert(metrics));
        }
    }

    /**
     * Parse GLBenchmark result XML.
     *
     * @param resultXml GLBenchmark result XML {@link File}
     * @return a {@link HashMap} that contains metrics key and result
     */
    private Map<String, String> parseResultXml(File resultXml) {
        Map<String, String> benchmarkResult = new HashMap<String, String>();
        DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
        Document doc = null;
        try {
            DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
            doc = dBuilder.parse(resultXml);
        } catch (ParserConfigurationException e) {
            return null;
        } catch (IOException e) {
            return null;
        } catch (SAXException e) {
            return null;
        } catch (IllegalArgumentException e) {
            return null;
        }
        doc.getDocumentElement().normalize();

        NodeList nodes = doc.getElementsByTagName("test_result");
        for (int i = 0; i < nodes.getLength(); i++) {
            Node node = nodes.item(i);
            if (node.getNodeType() == Node.ELEMENT_NODE) {
                Element testResult = (Element) node;
                String testTitle = getData(testResult, "title");
                String testType = getData(testResult, "type");
                String fps = getData(testResult, "fps");
                String score = getData(testResult, "score");
                String testName = String.format("%s - %s", testTitle, testType);
                if (METRICS_KEY_MAP.containsKey(testName)) {
                    if (testName.contains("Fill") || testName.contains("Triangle")) {
                        // Use Mtexels/sec or MTriangles/sec as unit
                        score = String.valueOf((long)(Double.parseDouble(score) / 1.0E6));
                    }
                    CLog.i(String.format("%s: %s (fps=%s)", testName, score, fps));
                    String testKey = METRICS_KEY_MAP.get(testName);
                    if (score != null && !score.trim().equals("0")) {
                        benchmarkResult.put(testKey, score);
                        if (fps != null && !fps.trim().equals("0.0")) {
                            try {
                                float fpsValue = Float.parseFloat(fps.replace("fps", ""));
                                benchmarkResult.put(testKey + "-fps", String.valueOf(fpsValue));
                            } catch (NumberFormatException e) {
                                CLog.i(String.format("Got %s for fps value. Ignored.", fps));
                            }
                        }
                    }
                }
            }
        }
        return benchmarkResult;
    }

    /**
     * Get value in the first matching tag under the element
     *
     * @param element the parent {@link Element} of the tag
     * @param tag {@link String} of the tag name
     * @return a {@link String} that contains the value in the tag; returns null if not found.
     */
    private String getData(Element element, String tag) {
        NodeList tagNodes = element.getElementsByTagName(tag);
        if (tagNodes.getLength() > 0) {
            Node tagNode = tagNodes.item(0);
            if (tagNode.getNodeType() == Node.ELEMENT_NODE) {
                Node node = tagNode.getChildNodes().item(0);
                if (node != null) {
                    return node.getNodeValue();
                } else {
                    return null;
                }
            }
        }
        return null;
    }
}
