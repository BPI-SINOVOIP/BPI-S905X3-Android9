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

package com.android.cts.apicoverage;

import com.android.cts.apicoverage.CtsReportProto.*;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

/**
 * {@link DefaultHandler} that builds an empty {@link ApiCoverage} object from scanning
 * TestModule.xml.
 */
class CtsReportHandler extends DefaultHandler {
    private static final String MODULE_TAG = "Module";
    private static final String TEST_CASE_TAG = "TestCase";
    private static final String TEST_TAG = "Test";
    private static final String NAME_TAG = "name";
    private static final String RESULT_TAG = "result";
    private static final String ABI_TAG = "abi";
    private static final String DONE_TAG = "done";
    private static final String PASS_TAG = "pass";
    private static final String FAILED_TAG = "failed";
    private static final String RUNTIME_TAG = "runtime";

    private CtsReport.Builder mCtsReportBld;
    private CtsReport.TestPackage.Builder mTestPackageBld;
    private CtsReport.TestPackage.TestSuite.Builder mTestSuiteBld;
    private CtsReport.TestPackage.TestSuite.TestCase.Builder mTestCaseBld;

    CtsReportHandler(String configFileName) {
        mCtsReportBld = CtsReport.newBuilder();
    }

    @Override
    public void startElement(String uri, String localName, String name, Attributes attributes)
            throws SAXException {
        super.startElement(uri, localName, name, attributes);

        try {
            if (MODULE_TAG.equalsIgnoreCase(localName)) {
                mTestPackageBld = CtsReport.TestPackage.newBuilder();
                mTestSuiteBld = CtsReport.TestPackage.TestSuite.newBuilder();
                mTestSuiteBld.setName(attributes.getValue(NAME_TAG));
                mTestPackageBld.setName(attributes.getValue(NAME_TAG));
                // ABI is option fro a Report
                if (null != attributes.getValue(NAME_TAG)) {
                    mTestPackageBld.setAbi(attributes.getValue(ABI_TAG));
                }
            } else if (TEST_CASE_TAG.equalsIgnoreCase(localName)) {
                mTestCaseBld = CtsReport.TestPackage.TestSuite.TestCase.newBuilder();
                mTestCaseBld.setName(attributes.getValue(NAME_TAG));
            } else if (TEST_TAG.equalsIgnoreCase(localName)) {
                CtsReport.TestPackage.TestSuite.TestCase.Test.Builder testBld;
                testBld = CtsReport.TestPackage.TestSuite.TestCase.Test.newBuilder();
                testBld.setName(attributes.getValue(NAME_TAG));
                testBld.setResult(attributes.getValue(RESULT_TAG));
                mTestCaseBld.addTest(testBld);
            }
        } catch (Exception ex) {
            System.err.println(localName + " " + name);
        }
    }

    @Override
    public void endElement(String uri, String localName, String name) throws SAXException {
        super.endElement(uri, localName, name);

        if (MODULE_TAG.equalsIgnoreCase(localName)) {
            mTestPackageBld.addTestSuite(mTestSuiteBld);
            mCtsReportBld.addTestPackage(mTestPackageBld);
            mTestPackageBld = null;
            mTestSuiteBld = null;
        } else if (TEST_CASE_TAG.equalsIgnoreCase(localName)) {
            mTestSuiteBld.addTestCase(mTestCaseBld);
            mTestCaseBld = null;
        }
    }

    public CtsReport getCtsReport() {
        return mCtsReportBld.build();
    }
}
