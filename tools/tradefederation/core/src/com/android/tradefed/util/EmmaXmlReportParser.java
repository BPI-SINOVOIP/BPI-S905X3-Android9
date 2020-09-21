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

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

/**
 * Helper class used to parse the Emma code coverage xml report for the summary.
 */
public class EmmaXmlReportParser {

    private static final Pattern COVERAGE_PATTERN = Pattern.compile("(.*?)%.*");
    // value is formatted like that: "68%  (997/1475)"
    private static final Pattern COVERAGE_DATA_PATTERN = Pattern.compile(
            "(\\d+)(%)(.*\\()(\\d+)(\\.)?(\\d+)?(/)(\\d+)(.*)");
    private static final String COVERAGE_XPATH = "/report/data/all/coverage";

    Map<String, String> mSummaryMetrics = new HashMap<String, String>(4);
    Map<String, String[]> mDataMetrics = new HashMap<String, String[]>(4);

    public EmmaXmlReportParser() {
        mSummaryMetrics.put(EmmaXmlConstants.CLASS_TAG, "0");
        mSummaryMetrics.put(EmmaXmlConstants.METHOD_TAG, "0");
        mSummaryMetrics.put(EmmaXmlConstants.BLOCK_TAG, "0");
        mSummaryMetrics.put(EmmaXmlConstants.LINE_TAG, "0");
    }

    public void parseXmlFile(File XMLfile) {
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        try {
            DocumentBuilder documentBuilder = factory.newDocumentBuilder();
            Document doc = documentBuilder.parse(XMLfile);
            XPathFactory xpFactory = XPathFactory.newInstance();
            XPath xpath = xpFactory.newXPath();
            XPathExpression expr = xpath.compile(COVERAGE_XPATH);
            NodeList nl = (NodeList) expr.evaluate(doc, XPathConstants.NODESET);
            for (int i = 0; i < nl.getLength(); ++i) {
                Node n = nl.item(i);
                String coverageType = getAttributeValue((Element) n, "type");
                String value = getAttributeValue((Element) n, "value");
                if (coverageType.contains(EmmaXmlConstants.CLASS_TAG)) {
                    parseCoveragePercentage(EmmaXmlConstants.CLASS_TAG, value);
                    parseCoverageData(EmmaXmlConstants.CLASS_TAG, value);
                } else if (coverageType.contains(EmmaXmlConstants.METHOD_TAG)) {
                    parseCoveragePercentage(EmmaXmlConstants.METHOD_TAG, value);
                    parseCoverageData(EmmaXmlConstants.METHOD_TAG, value);
                } else if (coverageType.contains(EmmaXmlConstants.BLOCK_TAG)) {
                    parseCoveragePercentage(EmmaXmlConstants.BLOCK_TAG, value);
                    parseCoverageData(EmmaXmlConstants.BLOCK_TAG, value);
                } else if (coverageType.contains(EmmaXmlConstants.LINE_TAG)) {
                    parseCoveragePercentage(EmmaXmlConstants.LINE_TAG, value);
                    parseCoverageData(EmmaXmlConstants.LINE_TAG, value);
                }
            }
        } catch (ParserConfigurationException | IOException | XPathExpressionException |
                org.xml.sax.SAXException e) {
            CLog.e(e);
            throw new RuntimeException(e);
        }
    }

    /**
     * Return the value of an attribute in an element or null if the attribute
     * is not found.
     *
     * @param element {@link Element} whose attribute is looked for
     * @param attrName {@link String} name of attribute to look for
     *
     * @return the attribute value
     */
    static public String getAttributeValue(Element element, String attrName) {
        String attrValue = null;
        Attr attr = element.getAttributeNode(attrName);
        if (attr != null) {
            attrValue = attr.getValue();
        }
        return attrValue;
    }

    /**
     * Parse the percentage coverage from the XML value.
     *
     * @param label {@link String} to tag the value as
     * @param value {@link String} value to parse
     */
    private void parseCoveragePercentage(String label, String value) {
        String percentage = null;
        Matcher m = COVERAGE_PATTERN.matcher(value);
        while (m.find()) {
            percentage = m.group(1);
        }
        if (percentage != null) {
            mSummaryMetrics.put(label, percentage);
            CLog.d("Found: %s: %s", label, percentage);
        }
    }

    /**
     * Parse the data coverage from the XML value. (number covered and total number)
     *
     * @param label {@link String} to tag the value as
     * @param value {@link String} value to parse
     */
    private void parseCoverageData(String label, String value) {
        Matcher matcher = COVERAGE_DATA_PATTERN.matcher(value);
        if (matcher.find()) {
            String[] tmpData = new String[2];
            tmpData[0] = matcher.group(4);
            tmpData[1] = matcher.group(8);
            mDataMetrics.put(label, tmpData);
        } else {
            CLog.w("%s doesn't match a results pattern", value);
        }
    }

    public Map<String, String[]> getDataMetrics() {
        return mDataMetrics;
    }

    public Map<String, String> getSummaryMetrics() {
        return mSummaryMetrics;
    }
}
