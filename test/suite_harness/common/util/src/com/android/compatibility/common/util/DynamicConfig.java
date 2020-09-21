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

package com.android.compatibility.common.util;

import static org.junit.Assert.assertTrue;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Load dynamic config for test cases
 */
public class DynamicConfig {

    //XML constant
    public static final String NS = null;
    public static final String CONFIG_TAG = "dynamicConfig";
    public static final String ENTRY_TAG = "entry";
    public static final String VALUE_TAG = "value";
    public static final String KEY_ATTR = "key";

    public static final String REMOTE_CONFIG_REQUIRED_KEY = "remote_config_required";
    public static final String REMOTE_CONFIG_RETRIEVED_KEY = "remote_config_retrieved";
    public static final String CONFIG_FOLDER_ON_DEVICE = "/sdcard/dynamic-config-files/";

    protected Map<String, List<String>> mDynamicConfigMap = new HashMap<String, List<String>>();

    public void initializeConfig(File file) throws XmlPullParserException, IOException {
        mDynamicConfigMap = createConfigMap(file);
    }

    public String getValue(String key) {
        assertRemoteConfigRequirementMet();
        List<String> singleValue = mDynamicConfigMap.get(key);
        if (singleValue == null || singleValue.size() == 0 || singleValue.size() > 1) {
            // key must exist in the map, and map to a list containing exactly one string
            return null;
        }
        return singleValue.get(0);
    }

    public List<String> getValues(String key) {
        assertRemoteConfigRequirementMet();
        return mDynamicConfigMap.get(key);
    }

    public Set<String> keySet() {
        assertRemoteConfigRequirementMet();
        return mDynamicConfigMap.keySet();
    }

    public boolean remoteConfigRequired() {
        if (mDynamicConfigMap.containsKey(REMOTE_CONFIG_REQUIRED_KEY)) {
            String val = mDynamicConfigMap.get(REMOTE_CONFIG_REQUIRED_KEY).get(0);
            return Boolean.parseBoolean(val);
        }
        return false;
    }

    public boolean remoteConfigRetrieved() {
        // assume config will always contain exactly one value, populated by DynamicConfigHandler
        String val = mDynamicConfigMap.get(REMOTE_CONFIG_RETRIEVED_KEY).get(0);
        return Boolean.parseBoolean(val);
    }

    public void assertRemoteConfigRequirementMet() {
        assertTrue("Remote connection to DynamicConfigService required for this test",
                !remoteConfigRequired() || remoteConfigRetrieved());
    }

    public static File getConfigFile(File configFolder, String moduleName)
            throws FileNotFoundException {
        File config = getConfigFileUnchecked(configFolder, moduleName);
        if (!config.exists()) {
            throw new FileNotFoundException(String.format("Cannot find %s.dynamic", moduleName));
        }
        return config;
    }

    public static File getConfigFileUnchecked(File configFolder, String moduleName) {
        return new File(configFolder, String.format("%s.dynamic", moduleName));
    }

    public static Map<String, List<String>> createConfigMap(File file)
            throws XmlPullParserException, IOException {

        Map<String, List<String>> dynamicConfigMap = new HashMap<String, List<String>>();
        XmlPullParser parser = XmlPullParserFactory.newInstance().newPullParser();
        parser.setInput(new InputStreamReader(new FileInputStream(file)));
        parser.nextTag();
        parser.require(XmlPullParser.START_TAG, NS, CONFIG_TAG);

        while (parser.nextTag() == XmlPullParser.START_TAG) {
            parser.require(XmlPullParser.START_TAG, NS, ENTRY_TAG);
            String key = parser.getAttributeValue(NS, KEY_ATTR);
            List<String> valueList = new ArrayList<String>();
            while (parser.nextTag() == XmlPullParser.START_TAG) {
                parser.require(XmlPullParser.START_TAG, NS, VALUE_TAG);
                valueList.add(parser.nextText());
                parser.require(XmlPullParser.END_TAG, NS, VALUE_TAG);
            }
            parser.require(XmlPullParser.END_TAG, NS, ENTRY_TAG);
            if (key != null && !key.isEmpty()) {
                dynamicConfigMap.put(key, valueList);
            }
        }

        parser.require(XmlPullParser.END_TAG, NS, CONFIG_TAG);
        return dynamicConfigMap;
    }
}
