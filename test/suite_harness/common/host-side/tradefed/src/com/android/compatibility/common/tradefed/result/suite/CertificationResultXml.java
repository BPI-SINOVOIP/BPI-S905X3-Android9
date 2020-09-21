/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.compatibility.common.tradefed.result.suite;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.suite.SuiteResultHolder;
import com.android.tradefed.result.suite.XmlSuiteResultFormatter;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.IOException;

/**
 * Utility class to save a Compatibility run as an XML.
 */
public class CertificationResultXml extends XmlSuiteResultFormatter {

    private static final String LOG_URL_ATTR = "log_url";
    private static final String REPORT_VERSION_ATTR = "report_version";
    private static final String REFERENCE_URL_ATTR = "reference_url";
    private static final String RESULT_FILE_VERSION = "5.0";
    private static final String SUITE_NAME_ATTR = "suite_name";
    private static final String SUITE_PLAN_ATTR = "suite_plan";
    private static final String SUITE_VERSION_ATTR = "suite_version";
    private static final String SUITE_BUILD_ATTR = "suite_build_number";

    private String mSuiteName;
    private String mSuiteVersion;
    private String mSuitePlan;
    private String mSuiteBuild;
    private String mReferenceUrl;
    private String mLogUrl;

    /**
     * Create an XML report specialized for the Compatibility Test cases.
     */
    public CertificationResultXml(String suiteName,
            String suiteVersion,
            String suitePlan,
            String suiteBuild,
            String referenceUrl,
            String logUrl) {
        mSuiteName = suiteName;
        mSuiteVersion = suiteVersion;
        mSuitePlan = suitePlan;
        mSuiteBuild = suiteBuild;
        mReferenceUrl = referenceUrl;
        mLogUrl = logUrl;
    }

    /**
     * Add Compatibility specific attributes.
     */
    @Override
    public void addSuiteAttributes(XmlSerializer serializer)
            throws IllegalArgumentException, IllegalStateException, IOException {
        serializer.attribute(NS, SUITE_NAME_ATTR, mSuiteName);
        serializer.attribute(NS, SUITE_VERSION_ATTR, mSuiteVersion);
        serializer.attribute(NS, SUITE_PLAN_ATTR, mSuitePlan);
        serializer.attribute(NS, SUITE_BUILD_ATTR, mSuiteBuild);
        serializer.attribute(NS, REPORT_VERSION_ATTR, RESULT_FILE_VERSION);

        if (mReferenceUrl != null) {
            serializer.attribute(NS, REFERENCE_URL_ATTR, mReferenceUrl);
        }

        if (mLogUrl != null) {
            serializer.attribute(NS, LOG_URL_ATTR, mLogUrl);
        }
    }

    @Override
    public void parseSuiteAttributes(XmlPullParser parser, IInvocationContext context)
            throws XmlPullParserException {
        mSuiteName = parser.getAttributeValue(NS, SUITE_NAME_ATTR);
        mSuiteVersion = parser.getAttributeValue(NS, SUITE_VERSION_ATTR);
        mSuitePlan = parser.getAttributeValue(NS, SUITE_PLAN_ATTR);
        mSuiteBuild = parser.getAttributeValue(NS, SUITE_BUILD_ATTR);

        mReferenceUrl = parser.getAttributeValue(NS, REFERENCE_URL_ATTR);
        mLogUrl = parser.getAttributeValue(NS, LOG_URL_ATTR);
    }

    /**
     * Add compatibility specific build info attributes.
     */
    @Override
    public void addBuildInfoAttributes(XmlSerializer serializer, SuiteResultHolder holder)
            throws IllegalArgumentException, IllegalStateException, IOException {
        for (IBuildInfo build : holder.context.getBuildInfos()) {
            for (String key : build.getBuildAttributes().keySet()) {
                if (key.startsWith(getAttributesPrefix())) {
                    String newKey = key.split(getAttributesPrefix())[1];
                    serializer.attribute(NS, newKey, build.getBuildAttributes().get(key));
                }
            }
        }
    }

    /**
     * Returns the compatibility prefix for attributes.
     */
    public String getAttributesPrefix() {
        return "cts:";
    }
}
