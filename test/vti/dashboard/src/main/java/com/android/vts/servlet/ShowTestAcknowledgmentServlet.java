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

package com.android.vts.servlet;

import com.android.vts.entity.TestAcknowledgmentEntity;
import com.android.vts.entity.TestEntity;
import com.android.vts.util.DatastoreHelper;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

public class ShowTestAcknowledgmentServlet extends BaseServlet {
    private static final String TEST_ACK_JSP = "WEB-INF/jsp/show_test_acknowledgments.jsp";

    @Override
    public PageType getNavParentType() {
        return PageType.TOT;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        return links;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Query ackQuery = new Query(TestAcknowledgmentEntity.KIND);
        List<JsonObject> testAcks = new ArrayList<>();
        for (Entity ackEntity :
                datastore.prepare(ackQuery).asIterable(DatastoreHelper.getLargeBatchOptions())) {
            TestAcknowledgmentEntity ack = TestAcknowledgmentEntity.fromEntity(ackEntity);
            if (ack == null) continue;
            testAcks.add(ack.toJson());
        }

        List<String> allTestNames = new ArrayList<>();
        Query query = new Query(TestEntity.KIND).setKeysOnly();
        for (Entity test : datastore.prepare(query).asIterable()) {
            allTestNames.add(test.getKey().getName());
        }

        request.setAttribute("testAcknowledgments", new Gson().toJson(testAcks));
        request.setAttribute("allTests", new Gson().toJson(allTestNames));
        request.setAttribute("branches", new Gson().toJson(DatastoreHelper.getAllBranches()));
        request.setAttribute("devices", new Gson().toJson(DatastoreHelper.getAllBuildFlavors()));
        request.setAttribute("readOnly", new Gson().toJson(!UserServiceFactory.getUserService().isUserAdmin()));

        RequestDispatcher dispatcher = request.getRequestDispatcher(TEST_ACK_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : ", e);
        }
    }
}
