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

import com.android.cts.apicoverage.TestSuiteProto.*;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

/**
 * {@link DefaultHandler} that builds an empty {@link ApiCoverage} object from scanning
 * TestModule.xml.
 */
class TestModuleConfigHandler extends DefaultHandler {
    private static final String CONFIGURATION_TAG = "configuration";
    private static final String DESCRIPTION_TAG = "description";
    private static final String OPTION_TAG = "option";
    private static final String TARGET_PREPARER_TAG = "target_preparer";
    private static final String TEST_TAG = "test";
    private static final String CLASS_TAG = "class";
    private static final String NAME_TAG = "name";
    private static final String KEY_TAG = "key";
    private static final String VALUE_TAG = "value";
    private static final String MODULE_NAME_TAG = "module-name";
    private static final String GTEST_CLASS_TAG = "com.android.tradefed.testtype.GTest";

    private FileMetadata.Builder mFileMetadata;
    private ConfigMetadata.Builder mConfigMetadata;
    private ConfigMetadata.TestClass.Builder mTestCase;
    private ConfigMetadata.TargetPreparer.Builder mTargetPreparer;
    private String mModuleName = null;

    TestModuleConfigHandler(String configFileName) {
        mFileMetadata = FileMetadata.newBuilder();
        mConfigMetadata = ConfigMetadata.newBuilder();
        mTestCase = null;
        mTargetPreparer = null;
        // Default Module Name is the Config File Name
        mModuleName = configFileName.replaceAll(".config$", "");
    }

    @Override
    public void startElement(String uri, String localName, String name, Attributes attributes)
            throws SAXException {
        super.startElement(uri, localName, name, attributes);

        if (CONFIGURATION_TAG.equalsIgnoreCase(localName)) {
            if (null != attributes.getValue(DESCRIPTION_TAG)) {
                mFileMetadata.setDescription(attributes.getValue(DESCRIPTION_TAG));
            } else {
                mFileMetadata.setDescription("WARNING: no description.");
            }
        } else if (TEST_TAG.equalsIgnoreCase(localName)) {
            mTestCase = ConfigMetadata.TestClass.newBuilder();
            mTestCase.setTestClass(attributes.getValue(CLASS_TAG));
        } else if (TARGET_PREPARER_TAG.equalsIgnoreCase(localName)) {
            mTargetPreparer = ConfigMetadata.TargetPreparer.newBuilder();
            mTargetPreparer.setTestClass(attributes.getValue(CLASS_TAG));
        } else if (OPTION_TAG.equalsIgnoreCase(localName)) {
            Option.Builder option = Option.newBuilder();
            option.setName(attributes.getValue(NAME_TAG));
            option.setValue(attributes.getValue(VALUE_TAG));
            String keyStr = attributes.getValue(KEY_TAG);
            if (null != keyStr) {
                option.setKey(keyStr);
            }
            if (null != mTestCase) {
                mTestCase.addOptions(option);
                if (GTEST_CLASS_TAG.equalsIgnoreCase(option.getName())) {
                    mModuleName = option.getValue();
                }
            } else if (null != mTargetPreparer) {
                mTargetPreparer.addOptions(option);
            }
        }
    }

    @Override
    public void endElement(String uri, String localName, String name) throws SAXException {
        super.endElement(uri, localName, name);
        if (TEST_TAG.equalsIgnoreCase(localName)) {
            mConfigMetadata.addTestClasses(mTestCase);
            mTestCase = null;
        } else if (TARGET_PREPARER_TAG.equalsIgnoreCase(localName)) {
            mConfigMetadata.addTargetPreparers(mTargetPreparer);
            mTargetPreparer = null;
        } else if (CONFIGURATION_TAG.equalsIgnoreCase(localName)) {
            mFileMetadata.setConfigMetadata(mConfigMetadata);
        }
    }

    public String getModuleName() {
        return mModuleName;
    }

    public String getTestClassName() {
        //return the 1st Test Class
        return mFileMetadata.getConfigMetadata().getTestClassesList().get(0).getTestClass();
    }

    public FileMetadata getFileMetadata() {
        return mFileMetadata.build();
    }
}
