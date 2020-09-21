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

package com.android.vts.servlet;

import com.android.vts.entity.CoverageEntity;
import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.gson.Gson;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Servlet for handling requests to show code coverage. */
public class ShowCoverageServlet extends BaseServlet {
    private static final String COVERAGE_JSP = "WEB-INF/jsp/show_coverage.jsp";
    private static final String TREE_JSP = "WEB-INF/jsp/show_tree.jsp";

    @Override
    public PageType getNavParentType() {
        return PageType.TOT;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        String testName = request.getParameter("testName");
        links.add(new Page(PageType.TABLE, testName, "?testName=" + testName));

        String startTime = request.getParameter("startTime");
        links.add(new Page(PageType.COVERAGE, "?testName=" + testName + "&startTime=" + startTime));
        return links;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        RequestDispatcher dispatcher = null;
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        String test = request.getParameter("testName");
        String timeString = request.getParameter("startTime");

        // Process the time key requested
        long time = -1;
        try {
            time = Long.parseLong(timeString);
        } catch (NumberFormatException e) {
            request.setAttribute("testName", test);
            dispatcher = request.getRequestDispatcher(TREE_JSP);
            return;
        }

        // Compute the parent test run key based off of the test and time
        Key testKey = KeyFactory.createKey(TestEntity.KIND, test);
        Key testRunKey = KeyFactory.createKey(testKey, TestRunEntity.KIND, time);

        // Create a query for coverage entities
        Query coverageQuery = new Query(CoverageEntity.KIND).setAncestor(testRunKey);

        List<String> sourceFiles = new ArrayList<>(); // list of source files
        List<List<Long>> coverageVectors = new ArrayList<>(); // list of line coverage vectors
        List<String> projects = new ArrayList<>(); // list of project names
        List<String> commits = new ArrayList<>(); // list of project commit hashes
        List<String> indicators = new ArrayList<>(); // list of HTML indicates to display

        /*
         * Map from section name to a list of indexes into the above lists where each coverage
         * report's data is located.
         */
        Map<String, List<Integer>> sectionMap = new HashMap<>();
        for (Entity e : datastore.prepare(coverageQuery).asIterable()) {
            CoverageEntity coverageEntity = CoverageEntity.fromEntity(e);
            if (coverageEntity == null) {
                logger.log(Level.WARNING, "Invalid coverage entity: " + e.getKey());
                continue;
            }
            if (!sectionMap.containsKey(coverageEntity.group)) {
                sectionMap.put(coverageEntity.group, new ArrayList<Integer>());
            }
            sectionMap.get(coverageEntity.group).add(coverageVectors.size());
            coverageVectors.add(coverageEntity.lineCoverage);
            sourceFiles.add(coverageEntity.filePath);
            projects.add(coverageEntity.projectName);
            commits.add(coverageEntity.projectVersion);
            String indicator = "";
            long total = coverageEntity.totalLineCount;
            long covered = coverageEntity.coveredLineCount;
            if (total > 0) {
                double pct = Math.round(covered * 10000d / total) / 100d;
                String color = pct >= 70 ? "green" : "red";
                indicator = "<div class=\"right indicator " + color + "\">" + pct + "%</div>"
                        + "<span class=\"right total-count\">" + covered + "/" + total + "</span>";
            }
            indicators.add(indicator);
        }

        request.setAttribute("testName", request.getParameter("testName"));
        request.setAttribute("gerritURI", new Gson().toJson(GERRIT_URI));
        request.setAttribute("gerritScope", new Gson().toJson(GERRIT_SCOPE));
        request.setAttribute("clientId", new Gson().toJson(CLIENT_ID));
        request.setAttribute("coverageVectors", new Gson().toJson(coverageVectors));
        request.setAttribute("sourceFiles", new Gson().toJson(sourceFiles));
        request.setAttribute("projects", new Gson().toJson(projects));
        request.setAttribute("commits", new Gson().toJson(commits));
        request.setAttribute("indicators", new Gson().toJson(indicators));
        request.setAttribute("sectionMap", new Gson().toJson(sectionMap));
        request.setAttribute("startTime", request.getParameter("startTime"));
        dispatcher = request.getRequestDispatcher(COVERAGE_JSP);

        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : ", e);
        }
    }
}
