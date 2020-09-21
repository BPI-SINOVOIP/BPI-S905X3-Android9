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

import com.android.vts.entity.TestEntity;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Query;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Servlet for handling requests to display profiling tests. */
public class ShowProfilingListServlet extends BaseServlet {
    private static final String PROFILING_LIST_JSP = "WEB-INF/jsp/show_profiling_list.jsp";

    @Override
    public PageType getNavParentType() {
        return PageType.PROFILING_LIST;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        return null;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Query.Filter profilingFilter = new Query.FilterPredicate(
                TestEntity.HAS_PROFILING_DATA, Query.FilterOperator.EQUAL, true);
        Query query = new Query(TestEntity.KIND)
                            .setFilter(profilingFilter)
                            .setKeysOnly();
        Set<String> profilingTests = new HashSet<>();
        for (Entity test : datastore.prepare(query).asIterable()) {
            profilingTests.add(test.getKey().getName());
        }

        List<String> tests = new ArrayList<>(profilingTests);
        tests.sort(Comparator.naturalOrder());

        response.setStatus(HttpServletResponse.SC_OK);
        request.setAttribute("testNames", tests);
        RequestDispatcher dispatcher = request.getRequestDispatcher(PROFILING_LIST_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Excpetion caught : ", e);
        }
    }
}
