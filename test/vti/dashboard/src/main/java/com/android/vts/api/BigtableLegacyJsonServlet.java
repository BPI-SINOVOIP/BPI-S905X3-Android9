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

import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.util.DatastoreHelper;
import com.google.api.client.googleapis.auth.oauth2.GoogleCredential;
import com.google.api.client.http.javanet.NetHttpTransport;
import com.google.api.client.json.jackson.JacksonFactory;
import com.google.api.services.oauth2.Oauth2;
import com.google.api.services.oauth2.model.Tokeninfo;
import com.google.protobuf.InvalidProtocolBufferException;
import java.io.BufferedReader;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.commons.codec.binary.Base64;
import org.json.JSONException;
import org.json.JSONObject;

/** REST endpoint for posting data JSON to the Dashboard. */
@Deprecated
public class BigtableLegacyJsonServlet extends HttpServlet {
    private static final String SERVICE_CLIENT_ID = System.getProperty("SERVICE_CLIENT_ID");
    private static final String SERVICE_NAME = "VTS Dashboard";
    private static final Logger logger =
            Logger.getLogger(BigtableLegacyJsonServlet.class.getName());

    @Override
    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        // Retrieve the params
        String payload = new String();
        JSONObject payloadJson;
        try {
            String line = null;
            BufferedReader reader = request.getReader();
            while ((line = reader.readLine()) != null) {
                payload += line;
            }
            payloadJson = new JSONObject(payload);
        } catch (IOException | JSONException e) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            logger.log(Level.WARNING, "Invalid JSON: " + payload);
            return;
        }

        // Verify service account access token.
        boolean authorized = false;
        if (payloadJson.has("accessToken")) {
            String accessToken = payloadJson.getString("accessToken").trim();
            GoogleCredential credential = new GoogleCredential().setAccessToken(accessToken);
            Oauth2 oauth2 =
                    new Oauth2.Builder(new NetHttpTransport(), new JacksonFactory(), credential)
                            .setApplicationName(SERVICE_NAME)
                            .build();
            Tokeninfo tokenInfo = oauth2.tokeninfo().setAccessToken(accessToken).execute();
            if (tokenInfo.getIssuedTo().equals(SERVICE_CLIENT_ID)) {
                authorized = true;
            }
        }

        if (!authorized) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }

        // Parse the desired action and execute the command
        try {
            if (payloadJson.has("verb")) {
                switch (payloadJson.getString("verb")) {
                    case "createTable":
                        logger.log(Level.INFO, "Deprecated verb: createTable.");
                        break;
                    case "insertRow":
                        insertData(payloadJson);
                        break;
                    default:
                        logger.log(Level.WARNING,
                                "Invalid Datastore REST verb: " + payloadJson.getString("verb"));
                        throw new IOException("Unsupported POST verb.");
                }
            }
        } catch (IOException e) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }
        response.setStatus(HttpServletResponse.SC_OK);
    }

    /**
     * Inserts a data into the Cloud Datastore
     *
     * @param payloadJson The JSON object representing the row to be inserted. Of the form: {
     *     (deprecated) 'tableName' : 'table', (deprecated) 'rowKey' : 'row', (deprecated) 'family'
     * :
     *     'family', (deprecated) 'qualifier' : 'qualifier', 'value' : 'value' }
     * @throws IOException
     */
    private void insertData(JSONObject payloadJson) throws IOException {
        if (!payloadJson.has("value")) {
            logger.log(Level.WARNING, "Missing attributes for datastore api insertRow().");
            return;
        }
        try {
            byte[] value = Base64.decodeBase64(payloadJson.getString("value"));
            TestReportMessage testReportMessage = TestReportMessage.parseFrom(value);
            DatastoreHelper.insertTestReport(testReportMessage);
        } catch (InvalidProtocolBufferException e) {
            logger.log(Level.WARNING, "Invalid report posted to dashboard.");
        }
    }
}
