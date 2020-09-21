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

package com.android.vts.entity;

import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.users.User;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing subscriptions between a user and a test. */
public class UserFavoriteEntity implements DashboardEntity {
    protected static final Logger logger = Logger.getLogger(UserFavoriteEntity.class.getName());

    public static final String KIND = "UserFavorite";

    // Property keys
    public static final String USER = "user";
    public static final String TEST_KEY = "testKey";
    public static final String MUTE_NOTIFICATIONS = "muteNotifications";

    private final Key key;
    public final User user;
    public final Key testKey;
    public boolean muteNotifications;

    /**
     * Create a user favorite relationship.
     *
     * @param key The key of the entity in the database.
     * @param user The User object for the subscribing user.
     * @param testKey The key of the TestEntity object describing the test.
     * @param muteNotifications True if the subscriber has muted notifications, false otherwise.
     */
    private UserFavoriteEntity(Key key, User user, Key testKey, boolean muteNotifications) {
        this.key = key;
        this.user = user;
        this.testKey = testKey;
        this.muteNotifications = muteNotifications;
    }

    /**
     * Create a user favorite relationship.
     *
     * @param user The User object for the subscribing user.
     * @param testKey The key of the TestEntity object describing the test.
     * @param muteNotifications True if the subscriber has muted notifications, false otherwise.
     */
    public UserFavoriteEntity(User user, Key testKey, boolean muteNotifications) {
        this(null, user, testKey, muteNotifications);
    }

    @Override
    public Entity toEntity() {
        Entity favoriteEntity;
        if (this.key != null) {
            favoriteEntity = new Entity(key);
        } else {
            favoriteEntity = new Entity(KIND);
        }
        favoriteEntity.setProperty(USER, this.user);
        favoriteEntity.setProperty(TEST_KEY, this.testKey);
        favoriteEntity.setProperty(MUTE_NOTIFICATIONS, this.muteNotifications);
        return favoriteEntity;
    }

    /**
     * Convert an Entity object to a UserFavoriteEntity.
     *
     * @param e The entity to process.
     * @return UserFavoriteEntity object with the properties from e, or null if incompatible.
     */
    public static UserFavoriteEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND) || !e.hasProperty(USER) || !e.hasProperty(TEST_KEY)) {
            logger.log(
                    Level.WARNING, "Missing user favorite attributes in entity: " + e.toString());
            return null;
        }
        try {
            User user = (User) e.getProperty(USER);
            Key testKey = (Key) e.getProperty(TEST_KEY);
            boolean muteNotifications = false;
            if (e.hasProperty(MUTE_NOTIFICATIONS)) {
                muteNotifications = (boolean) e.getProperty(MUTE_NOTIFICATIONS);
            }
            return new UserFavoriteEntity(e.getKey(), user, testKey, muteNotifications);
        } catch (ClassCastException exception) {
            // Invalid cast
            logger.log(Level.WARNING, "Error parsing user favorite entity.", exception);
        }
        return null;
    }
}
