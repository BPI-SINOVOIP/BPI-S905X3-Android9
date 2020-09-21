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

import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestStatusEntity;
import com.android.vts.entity.UserFavoriteEntity;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.PropertyProjection;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.appengine.api.datastore.Query.FilterOperator;
import com.google.appengine.api.datastore.Query.FilterPredicate;
import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;

/** Represents the servlet that is invoked on loading the first page of dashboard. */
public class DashboardMainServlet extends BaseServlet {
    private static final String DASHBOARD_MAIN_JSP = "WEB-INF/jsp/dashboard_main.jsp";
    private static final String NO_TESTS_ERROR = "No test results available.";

    @Override
    public PageType getNavParentType() {
        return PageType.TOT;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        return null;
    }

    /** Helper class for displaying test entries on the main dashboard. */
    public class TestDisplay implements Comparable<TestDisplay> {
        private final Key testKey;
        private final int passCount;
        private final int failCount;
        private boolean muteNotifications;
        private boolean isFavorite;

        /**
         * Test display constructor.
         *
         * @param testKey The key of the test.
         * @param passCount The number of tests passing.
         * @param failCount The number of tests failing.
         * @param muteNotifications The flag for user notification in case of test failure.
         * @param isFavorite The flag for showing favorite mark on All Tests Tab page.
         */
        public TestDisplay(Key testKey, int passCount, int failCount, boolean muteNotifications,
            boolean isFavorite) {
            this.testKey = testKey;
            this.passCount = passCount;
            this.failCount = failCount;
            this.muteNotifications = muteNotifications;
            this.isFavorite = isFavorite;
        }

        /**
         * Get the key of the test.
         *
         * @return The key of the test.
         */
        public String getName() {
            return this.testKey.getName();
        }

        /**
         * Get the number of passing test cases.
         *
         * @return The number of passing test cases.
         */
        public int getPassCount() {
            return this.passCount;
        }

        /**
         * Get the number of failing test cases.
         *
         * @return The number of failing test cases.
         */
        public int getFailCount() {
            return this.failCount;
        }

        /**
         * Get the notification mute status.
         *
         * @return True if the subscriber has muted notifications, false otherwise.
         */
        public boolean getMuteNotifications() {
            return this.muteNotifications;
        }

        /** Set the notification mute status. */
        public void setMuteNotifications(boolean muteNotifications) {
            this.muteNotifications = muteNotifications;
        }

        /**
         * Get the favorate status.
         *
         * @return True if an user set favorate for the test, false otherwise.
         */
        public boolean getIsFavorite() {
            return this.isFavorite;
        }

        /** Set the favorite status. */
        public void setIsFavorite(boolean isFavorite) {
            this.isFavorite = isFavorite;
        }

        @Override
        public int compareTo(TestDisplay test) {
            return this.testKey.getName().compareTo(test.getName());
        }
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        RequestDispatcher dispatcher = null;
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        HttpSession session = request.getSession(true);
        PageType referTo = PageType.TREE;
        if (session.getAttribute("treeDefault") != null) {
            boolean treeDefault = (boolean) session.getAttribute("treeDefault");
            if (!treeDefault) {
                referTo = PageType.TABLE;
            }
        }

        List<TestDisplay> displayedTests = new ArrayList<>();
        List<String> allTestNames = new ArrayList<>();
        List<Key> unprocessedTestKeys = new ArrayList<>();

        Map<Key, TestDisplay> testMap = new HashMap<>(); // map from table key to TestDisplay
        Map<String, String> subscriptionMap = new HashMap<>();

        boolean showAll = request.getParameter("showAll") != null;
        String error = null;

        Query query = new Query(TestEntity.KIND).setKeysOnly();
        for (Entity test : datastore.prepare(query).asIterable()) {
            allTestNames.add(test.getKey().getName());
        }

        List<Key> favoriteKeyList = new ArrayList<Key>();
        Filter userFilter =
                new FilterPredicate(
                        UserFavoriteEntity.USER, FilterOperator.EQUAL, currentUser);
        Query filterQuery = new Query(UserFavoriteEntity.KIND).setFilter(userFilter);
        Iterable<Entity> favoriteIter = datastore.prepare(filterQuery).asIterable();
        favoriteIter.forEach(fe -> {
            Key testKey = UserFavoriteEntity.fromEntity(fe).testKey;
            favoriteKeyList.add(testKey);
            subscriptionMap.put(testKey.getName(), KeyFactory.keyToString(fe.getKey()));
        });

        query =
                new Query(TestStatusEntity.KIND)
                        .addProjection(
                                new PropertyProjection(TestStatusEntity.PASS_COUNT, Long.class))
                        .addProjection(
                                new PropertyProjection(TestStatusEntity.FAIL_COUNT, Long.class));
        for (Entity status : datastore.prepare(query).asIterable()) {
            TestStatusEntity statusEntity = TestStatusEntity.fromEntity(status);
            if (statusEntity == null) continue;
            Key testKey = KeyFactory.createKey(TestEntity.KIND, statusEntity.testName);
            boolean isFavorite = favoriteKeyList.contains(testKey);
            TestDisplay display = new TestDisplay(testKey, -1, -1, false, isFavorite);
            if (!unprocessedTestKeys.contains(testKey)) {
                display = new TestDisplay(testKey, statusEntity.passCount, statusEntity.failCount,
                    false, isFavorite);
            }
            testMap.put(testKey, display);
        }

        if (testMap.size() == 0) {
            error = NO_TESTS_ERROR;
        }

        if (showAll) {
            for (Key testKey : testMap.keySet()) {
                displayedTests.add(testMap.get(testKey));
            }
        } else {
            if (testMap.size() > 0) {
                for (Entity favoriteEntity : favoriteIter) {
                    UserFavoriteEntity favorite = UserFavoriteEntity.fromEntity(favoriteEntity);
                    Key testKey = favorite.testKey;
                    if (!testMap.containsKey(testKey)) {
                        continue;
                    }
                    TestDisplay display = testMap.get(testKey);
                    display.setMuteNotifications(favorite.muteNotifications);
                    displayedTests.add(display);
                }
            }
        }
        displayedTests.sort(Comparator.naturalOrder());

        response.setStatus(HttpServletResponse.SC_OK);
        request.setAttribute("allTestsJson", new Gson().toJson(allTestNames));
        request.setAttribute("subscriptionMapJson", new Gson().toJson(subscriptionMap));
        request.setAttribute("testNames", displayedTests);
        request.setAttribute("showAll", showAll);
        request.setAttribute("error", error);
        request.setAttribute("resultsUrl", referTo.defaultUrl);
        dispatcher = request.getRequestDispatcher(DASHBOARD_MAIN_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Excpetion caught : ", e);
        }
    }
}
