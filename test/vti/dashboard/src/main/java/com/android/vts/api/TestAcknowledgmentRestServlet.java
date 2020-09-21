/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.vts.entity.TestAcknowledgmentEntity;
import com.android.vts.util.DatastoreHelper;
import com.google.appengine.api.datastore.DatastoreFailureException;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.DatastoreTimeoutException;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Transaction;
import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.ConcurrentModificationException;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Servlet for handling requests to fetch test acknowledgments. */
public class TestAcknowledgmentRestServlet extends HttpServlet {
    protected static final Logger logger =
            Logger.getLogger(TestAcknowledgmentRestServlet.class.getName());

    /** Read all test acknowledgments. */
    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

        Query ackQuery = new Query(TestAcknowledgmentEntity.KIND);
        List<JsonObject> testAcks = new ArrayList<>();
        for (Entity ackEntity :
                datastore.prepare(ackQuery).asIterable(DatastoreHelper.getLargeBatchOptions())) {
            TestAcknowledgmentEntity ack = TestAcknowledgmentEntity.fromEntity(ackEntity);
            if (ack == null) continue;
            testAcks.add(ack.toJson());
        }
        response.setContentType("application/json");
        PrintWriter writer = response.getWriter();
        writer.print(new Gson().toJson(testAcks));
        writer.flush();
    }

    /** Create a test acknowledgment. */
    @Override
    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        if (!userService.isUserAdmin()) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        User currentUser = userService.getCurrentUser();
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

        StringBuilder sb = new StringBuilder();
        BufferedReader br = new BufferedReader(request.getReader());
        String str;
        while ((str = br.readLine()) != null) {
            sb.append(str);
        }
        JsonObject json;
        try {
            json = new JsonParser().parse(sb.toString()).getAsJsonObject();
        } catch (IllegalStateException e) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }
        TestAcknowledgmentEntity ack = TestAcknowledgmentEntity.fromJson(currentUser, json);
        Transaction txn = datastore.beginTransaction();
        try {
            Key key = datastore.put(ack.toEntity());
            txn.commit();

            response.setContentType("application/json");
            PrintWriter writer = response.getWriter();
            writer.print(new Gson().toJson(KeyFactory.keyToString(key)));
            writer.flush();
        } catch (ConcurrentModificationException
                | DatastoreFailureException
                | DatastoreTimeoutException e) {
            txn.rollback();
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        } finally {
            if (txn.isActive()) {
                logger.log(Level.WARNING, "Transaction rollback forced acknowledgment post.");
                txn.rollback();
                response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
                return;
            }
        }
        response.setStatus(HttpServletResponse.SC_OK);
    }

    /** Remove a test acknowledgment. */
    @Override
    public void doDelete(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        if (!userService.isUserAdmin()) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        String stringKey = request.getPathInfo();
        if (stringKey == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }
        if (stringKey.startsWith("/")) {
            stringKey = stringKey.substring(1);
        }
        Key key = KeyFactory.stringToKey(stringKey);
        if (!key.getKind().equals(TestAcknowledgmentEntity.KIND)) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }
        datastore.delete(key);
        response.setStatus(HttpServletResponse.SC_OK);
    }
}
