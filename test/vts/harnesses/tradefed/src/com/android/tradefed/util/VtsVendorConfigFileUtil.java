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

package com.android.tradefed.util;

import java.io.InputStream;
import java.io.IOException;
import java.util.Map;
import java.util.NoSuchElementException;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.StreamUtil;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Util to access a VTS config file.
 */
public class VtsVendorConfigFileUtil {
    public static final String KEY_VENDOR_TEST_CONFIG_DEFAULT_TYPE =
            "vts-vendor-config:default-type";
    public static final String KEY_VENDOR_TEST_CONFIG_FILE_PATH = "vts-vendor-config:file-path";
    public static final String VENDOR_TEST_CONFIG_DEFAULT_TYPE = "prod";
    public static final String VENDOR_TEST_CONFIG_FILE_PATH_PROD =
            "/config/vts-tradefed-vendor-config-prod.json";
    public static final String VENDOR_TEST_CONFIG_FILE_PATH_TEST =
            "/config/vts-tradefed-vendor-config-test.json";

    private JSONObject vendorConfigJson = null;

    // The path of a VTS vendor config file (format: json).
    private String mVendorConfigFilePath = null;

    // The default config file type, e.g., `prod` or `test`.
    private String mDefaultType = VENDOR_TEST_CONFIG_DEFAULT_TYPE;

    /**
     * Returns the specified vendor config file path.
     */
    public String GetVendorConfigFilePath() {
        if (mVendorConfigFilePath == null) {
            if (mDefaultType.toLowerCase().equals(VENDOR_TEST_CONFIG_DEFAULT_TYPE)) {
                mVendorConfigFilePath = VENDOR_TEST_CONFIG_FILE_PATH_PROD;
            } else {
                mVendorConfigFilePath = VENDOR_TEST_CONFIG_FILE_PATH_TEST;
            }
        }
        return mVendorConfigFilePath;
    }

    /**
     * Loads a VTS vendor config file.
     *
     * @param configPath, the path of a config file.
     * @throws RuntimeException
     */
    public boolean LoadVendorConfig(String configPath) throws RuntimeException {
        if (configPath == null || configPath.length() == 0) {
            configPath = GetVendorConfigFilePath();
        }
        CLog.i("Loading vendor test config %s", configPath);
        InputStream config = getClass().getResourceAsStream(configPath);
        if (config == null) {
            CLog.e("Vendor test config file %s does not exist", configPath);
            return false;
        }
        try {
            String content = StreamUtil.getStringFromStream(config);
            if (content == null) {
                CLog.e("Loaded vendor test config is empty");
                return false;
            }
            CLog.i("Loaded vendor test config %s", content);
            vendorConfigJson = new JSONObject(content);
        } catch (IOException e) {
            throw new RuntimeException("Failed to read vendor config json file");
        } catch (JSONException e) {
            throw new RuntimeException("Failed to parse vendor config json data");
        }
        return true;
    }

    /**
     * Loads a VTS vendor config file.
     *
     * @param defaultType, The default config file type, e.g., `prod` or `test`.
     * @param vendorConfigFilePath, The path of a VTS vendor config file (format: json).
     * @throws RuntimeException
     */
    public boolean LoadVendorConfig(String defaultType, String vendorConfigFilePath)
            throws RuntimeException {
        mDefaultType = defaultType;
        mVendorConfigFilePath = vendorConfigFilePath;
        return LoadVendorConfig("");
    }

    /**
     * Loads a VTS vendor config file.
     *
     * @param defaultType, The default config file type, e.g., `prod` or `test`.
     * @param vendorConfigFilePath, The path of a VTS vendor config file (format: json).
     * @throws RuntimeException
     */
    public boolean LoadVendorConfig(IBuildInfo buildInfo) throws RuntimeException {
        Map<String, String> attrs = buildInfo.getBuildAttributes();
        if (attrs.containsKey(KEY_VENDOR_TEST_CONFIG_DEFAULT_TYPE)) {
            mDefaultType = attrs.get(KEY_VENDOR_TEST_CONFIG_DEFAULT_TYPE);
        } else {
            CLog.i("No default vendor test configuration provided. Defaulting to prod.");
        }
        mVendorConfigFilePath = attrs.get(KEY_VENDOR_TEST_CONFIG_FILE_PATH);
        return LoadVendorConfig(mDefaultType, mVendorConfigFilePath);
    }

    /**
     * Gets the value of a config variable.
     *
     * @param varName, the name of a variable.
     * @throws NoSuchElementException
     */
    public String GetVendorConfigVariable(String varName) throws NoSuchElementException {
        if (vendorConfigJson == null) {
            CLog.e("Vendor config json file invalid or not yet loaded.");
            throw new NoSuchElementException("config is empty");
        }
        try {
            return vendorConfigJson.getString(varName);
        } catch (JSONException e) {
            CLog.e("Vendor config file does not define %s", varName);
            throw new NoSuchElementException("config parsing error");
        }
    }

    /**
     * Returns the current vendor config json object.
     */
    public JSONObject GetVendorConfigJson() {
        return vendorConfigJson;
    }
}
