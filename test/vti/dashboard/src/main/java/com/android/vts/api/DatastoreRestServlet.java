/*
 * Copyright (c) 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.vts.api;

import com.android.vts.proto.VtsReportMessage.DashboardPostMessage;
import com.android.vts.proto.VtsReportMessage.TestPlanReportMessage;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.util.DatastoreHelper;
import com.google.api.client.googleapis.auth.oauth2.GoogleCredential;
import com.google.api.client.http.javanet.NetHttpTransport;
import com.google.api.client.json.jackson.JacksonFactory;
import com.google.api.services.oauth2.Oauth2;
import com.google.api.services.oauth2.model.Tokeninfo;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.commons.codec.binary.Base64;

/** REST endpoint for posting data to the Dashboard. */
public class DatastoreRestServlet extends HttpServlet {
    private static final String SERVICE_CLIENT_ID = System.getProperty("SERVICE_CLIENT_ID");
    private static final String SERVICE_NAME = "VTS Dashboard";
    private static final Logger logger = Logger.getLogger(DatastoreRestServlet.class.getName());

    @Override
    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        // Retrieve the params
        DashboardPostMessage postMessage;
        try {
            String payload = request.getReader().lines().collect(Collectors.joining());
            byte[] value = Base64.decodeBase64(payload);
            postMessage = DashboardPostMessage.parseFrom(value);
        } catch (IOException e) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            logger.log(Level.WARNING, "Invalid proto: " + e.getLocalizedMessage());
            return;
        }

        // Verify service account access token.
        if (postMessage.hasAccessToken()) {
            String accessToken = postMessage.getAccessToken();
            logger.log(Level.INFO, "accessToken => " + accessToken);
            GoogleCredential credential = new GoogleCredential().setAccessToken(accessToken);
            Oauth2 oauth2 =
                    new Oauth2.Builder(new NetHttpTransport(), new JacksonFactory(), credential)
                            .setApplicationName(SERVICE_NAME)
                            .build();
            Tokeninfo tokenInfo = oauth2.tokeninfo().setAccessToken(accessToken).execute();
            if (tokenInfo.getIssuedTo().equals(SERVICE_CLIENT_ID)) {
                for (TestReportMessage testReportMessage : postMessage.getTestReportList()) {
                    DatastoreHelper.insertTestReport(testReportMessage);
                }

                for (TestPlanReportMessage planReportMessage : postMessage.getTestPlanReportList()) {
                    DatastoreHelper.insertTestPlanReport(planReportMessage);
                }

                response.setStatus(HttpServletResponse.SC_OK);
            } else {
                logger.log(Level.WARNING, "service_client_id didn't match!");
                logger.log(Level.INFO, "SERVICE_CLIENT_ID => " + tokenInfo.getIssuedTo());
                response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            }
        } else {
            logger.log(Level.WARNING, "postMessage do not contain any accessToken!");
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
        }
    }
}
