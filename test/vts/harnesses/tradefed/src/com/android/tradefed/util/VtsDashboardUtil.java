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

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.LinkedList;
import java.util.NoSuchElementException;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.VtsVendorConfigFileUtil;

import com.google.api.client.auth.oauth2.Credential;
import com.google.api.client.googleapis.auth.oauth2.GoogleCredential;
import com.google.api.client.googleapis.javanet.GoogleNetHttpTransport;
import com.google.api.client.json.jackson2.JacksonFactory;
import com.google.api.client.json.JsonFactory;

import com.android.vts.proto.VtsReportMessage.DashboardPostMessage;
import com.android.vts.proto.VtsReportMessage.TestPlanReportMessage;

/**
 * Uploads the VTS test plan execution result to the web DB using a RESTful API and
 * an OAuth2 credential kept in a json file.
 */
public class VtsDashboardUtil {
    private static final String PLUS_ME = "https://www.googleapis.com/auth/plus.me";
    private static final int BASE_TIMEOUT_MSECS = 1000 * 60;
    private static VtsVendorConfigFileUtil mConfigReader;
    IRunUtil mRunUtil = new RunUtil();

    public VtsDashboardUtil(VtsVendorConfigFileUtil configReader) {
        mConfigReader = configReader;
    }

    /*
     * Returns an OAuth2 token string obtained using a service account json keyfile.
     *
     * Uses the service account keyfile located at config variable 'service_key_json_path'
     * to request an OAuth2 token.
     */
    private String GetToken() {
        String keyFilePath;
        try {
            keyFilePath = mConfigReader.GetVendorConfigVariable("service_key_json_path");
        } catch (NoSuchElementException e) {
            return null;
        }

        JsonFactory jsonFactory = JacksonFactory.getDefaultInstance();
        Credential credential = null;
        try {
            List<String> listStrings = new LinkedList<String>();
            listStrings.add(PLUS_ME);
            credential = GoogleCredential.fromStream(new FileInputStream(keyFilePath))
                                 .createScoped(listStrings);
            credential.refreshToken();
            return credential.getAccessToken();
        } catch (FileNotFoundException e) {
            CLog.e(String.format("Service key file %s doesn't exist.", keyFilePath));
        } catch (IOException e) {
            CLog.e(String.format("Can't read the service key file, %s", keyFilePath));
        }
        return null;
    }

    /*
     * Uploads the given message to the web DB.
     *
     * @param message, DashboardPostMessage that keeps the result to upload.
     */
    public void Upload(DashboardPostMessage message) {
        String token = GetToken();
        if (token == null) {
            CLog.d("Token is not available for DashboardPostMessage.");
            return;
        }
        message.setAccessToken(token);
        try {
            String messageFilePath = WriteToTempFile(
                    Base64.getEncoder().encodeToString(message.toByteArray()).getBytes());
            Upload(messageFilePath);
        } catch (IOException e) {
            CLog.e("Couldn't write a proto message to a temp file.");
        } catch (NullPointerException e) {
            CLog.e("Couldn't serialize proto message.");
        }
    }

    /*
     * Uploads the given message file path to the web DB.
     *
     * @param message, DashboardPostMessage file path that keeps the result to upload.
     */
    public void Upload(String messageFilePath) {
        try {
            String commandTemplate =
                    mConfigReader.GetVendorConfigVariable("dashboard_post_command");
            commandTemplate = commandTemplate.replace("{path}", messageFilePath);
            // removes ', while keeping any substrings quoted by "".
            commandTemplate = commandTemplate.replace("'", "");
            CLog.i(String.format("Upload command: %s", commandTemplate));
            List<String> commandList = new ArrayList<String>();
            Matcher matcher = Pattern.compile("([^\"]\\S*|\".+?\")\\s*").matcher(commandTemplate);
            while (matcher.find()) {
                commandList.add(matcher.group(1));
            }
            CommandResult c = mRunUtil.runTimedCmd(BASE_TIMEOUT_MSECS * 3,
                    (String[]) commandList.toArray(new String[commandList.size()]));
            if (c == null || c.getStatus() != CommandStatus.SUCCESS) {
                CLog.e("Uploading the test plan execution result to GAE DB faiied.");
                CLog.e("Stdout: %s", c.getStdout());
                CLog.e("Stderr: %s", c.getStderr());
            }
            FileUtil.deleteFile(new File(messageFilePath));
        } catch (NoSuchElementException e) {
            CLog.e("dashboard_post_command unspecified in vendor config.");
        }
    }

    /*
     * Simple wrapper to write data to a temp file.
     *
     * @param data, actual data to write to a file.
     * @throws IOException
     */
    private String WriteToTempFile(byte[] data) throws IOException {
        File tempFile = File.createTempFile("tempfile", ".tmp");
        String filePath = tempFile.getAbsolutePath();
        FileOutputStream out = new FileOutputStream(filePath);
        out.write(data);
        out.close();
        return filePath;
    }
}
