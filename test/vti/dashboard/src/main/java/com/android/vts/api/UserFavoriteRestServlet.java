/*
 * Copyright (c) 2017 Google Inc. All Rights Reserved.
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

import com.android.vts.entity.TestEntity;
import com.android.vts.entity.UserFavoriteEntity;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.EntityNotFoundException;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Query.CompositeFilterOperator;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.appengine.api.datastore.Query.FilterOperator;
import com.google.appengine.api.datastore.Query.FilterPredicate;
import com.google.appengine.api.datastore.Transaction;
import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Servlet for handling requests to add or remove subscriptions. */
public class UserFavoriteRestServlet extends HttpServlet {
    protected static final Logger logger =
            Logger.getLogger(UserFavoriteRestServlet.class.getName());

    /**
     * Add a new favorite entity.
     *
     * @param user The user for which to add a favorite.
     * @param test The name of the test.
     * @param muteNotifications True if the subscriber has muted notifications, false otherwise.
     * @param response The servlet response object.
     * @return a json object with the generated key to the new favorite entity.
     * @throws IOException
     */
    private static JsonObject addFavorite(
            User user, String test, boolean muteNotifications, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Key addedTestKey = KeyFactory.createKey(TestEntity.KIND, test);
        // Filter the tests that exist from the set of tests to add
        try {
            datastore.get(addedTestKey);
        } catch (EntityNotFoundException e) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return null;
        }

        Filter userFilter =
                new FilterPredicate(UserFavoriteEntity.USER, FilterOperator.EQUAL, user);
        Filter testFilter =
                new FilterPredicate(
                        UserFavoriteEntity.TEST_KEY, FilterOperator.EQUAL, addedTestKey);
        Query q =
                new Query(UserFavoriteEntity.KIND)
                        .setFilter(CompositeFilterOperator.and(userFilter, testFilter))
                        .setKeysOnly();

        Key favoriteKey = null;

        Transaction txn = datastore.beginTransaction();
        try {
            for (Entity e : datastore.prepare(q).asIterable()) {
                favoriteKey = e.getKey();
                break;
            }
            if (favoriteKey == null) {
                UserFavoriteEntity favorite =
                        new UserFavoriteEntity(user, addedTestKey, muteNotifications);
                Entity entity = favorite.toEntity();
                datastore.put(entity);
                favoriteKey = entity.getKey();
            }
            txn.commit();
        } finally {
            if (txn.isActive()) {
                logger.log(
                        Level.WARNING,
                        "Transaction rollback forced for favorite creation: " + test);
                txn.rollback();
            }
        }
        JsonObject json = new JsonObject();
        json.add("key", new JsonPrimitive(KeyFactory.keyToString(favoriteKey)));
        return json;
    }

    /**
     * @param user The user for which to add a favorite.
     * @param favoriteKey The database key to the favorite entity to update.
     * @param muteNotifications True if the subscriber has muted notifications, false otherwise.
     * @param response The servlet response object.
     * @return a json object with the generated key to the new favorite entity.
     * @throws IOException
     */
    private static JsonObject updateFavorite(
            User user, Key favoriteKey, boolean muteNotifications, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Entity favoriteEntity;
        try {
            favoriteEntity = datastore.get(favoriteKey);
        } catch (EntityNotFoundException e) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return null;
        }
        UserFavoriteEntity favorite = UserFavoriteEntity.fromEntity(favoriteEntity);
        if (favorite.user.getUserId() == user.getUserId()) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return null;
        }
        if (favorite.muteNotifications != muteNotifications) {
            Transaction txn = datastore.beginTransaction();
            try {
                favorite.muteNotifications = muteNotifications;
                datastore.put(favorite.toEntity());
                txn.commit();
            } finally {
                if (txn.isActive()) {
                    logger.log(
                            Level.WARNING,
                            "Transaction rollback forced for favorite update: " + favoriteKey);
                    txn.rollback();
                }
            }
        }
        JsonObject json = new JsonObject();
        json.add("key", new JsonPrimitive(KeyFactory.keyToString(favoriteKey)));
        return json;
    }

    /** Add a test to the user's favorites. */
    @Override
    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();

        boolean muteNotifications = false;
        if (request.getParameter(UserFavoriteEntity.MUTE_NOTIFICATIONS) != null) {
            muteNotifications =
                    Boolean.parseBoolean(
                            request.getParameter(UserFavoriteEntity.MUTE_NOTIFICATIONS));
        }

        String userFavoritesKeyString = request.getParameter("userFavoritesKey");
        String testName = request.getParameter("testName");

        JsonObject returnData = null;
        if (userFavoritesKeyString != null) {
            Key userFavoritesKey = KeyFactory.stringToKey(userFavoritesKeyString);
            returnData = updateFavorite(currentUser, userFavoritesKey, muteNotifications, response);
        } else if (testName != null) {
            returnData = addFavorite(currentUser, testName, muteNotifications, response);
        }

        if (returnData != null) {
            response.setContentType("application/json");
            PrintWriter writer = response.getWriter();
            writer.print(new Gson().toJson(returnData));
            writer.flush();
        }
    }

    /** Remove a test from the user's favorites. */
    @Override
    public void doDelete(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        String stringKey = request.getPathInfo();
        if (stringKey == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }
        if (stringKey.startsWith("/")) {
            stringKey = stringKey.substring(1);
        }
        datastore.delete(KeyFactory.stringToKey(stringKey));
        response.setStatus(HttpServletResponse.SC_OK);
    }
}
