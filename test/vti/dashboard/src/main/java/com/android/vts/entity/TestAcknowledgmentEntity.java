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

package com.android.vts.entity;

import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Text;
import com.google.appengine.api.users.User;
import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import com.google.gson.reflect.TypeToken;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing a test failure acknowledgment. */
public class TestAcknowledgmentEntity implements DashboardEntity {
    protected static final Logger logger =
            Logger.getLogger(TestAcknowledgmentEntity.class.getName());

    public static final String KIND = "TestAcknowledgment";
    public static final String KEY = "key";
    public static final String TEST_KEY = "testKey";
    public static final String TEST_NAME = "testName";
    public static final String USER = "user";
    public static final String CREATED = "created";
    public static final String BRANCHES = "branches";
    public static final String DEVICES = "devices";
    public static final String TEST_CASE_NAMES = "testCaseNames";
    public static final String NOTE = "note";

    private final Key key;
    private final long created;
    public final Key test;
    public final User user;
    public final Set<String> branches;
    public final Set<String> devices;
    public final Set<String> testCaseNames;
    public final String note;

    /**
     * Create a AcknowledgmentEntity object.
     *
     * @param key The key of the AcknowledgmentEntity in the database.
     * @param created The timestamp when the entity was created (in microseconds).
     * @param test The key of the test.
     * @param user The user who created or last modified the entity.
     * @param branches The list of branch names for which the acknowledgment applies (or null if
     *     all).
     * @param devices The list of device build flavors for which the acknowledgment applies (or
     *     null if all).
     * @param testCaseNames The list of test case names known to fail (or null if all).
     * @param note A text blob with details about the failure (or null if all).
     */
    private TestAcknowledgmentEntity(
            Key key,
            long created,
            Key test,
            User user,
            List<String> branches,
            List<String> devices,
            List<String> testCaseNames,
            Text note) {
        this.test = test;
        this.user = user;
        if (branches != null) this.branches = new HashSet(branches);
        else this.branches = new HashSet<>();

        if (devices != null) this.devices = new HashSet(devices);
        else this.devices = new HashSet<>();

        if (testCaseNames != null) this.testCaseNames = new HashSet(testCaseNames);
        else this.testCaseNames = new HashSet<>();

        if (note != null) this.note = note.getValue();
        else this.note = null;

        this.key = key;
        this.created = created;
    }

    /**
     * Create a AcknowledgmentEntity object.
     *
     * @param test The key of the test.
     * @param user The user who created or last modified the entity.
     * @param branches The list of branch names for which the acknowledgment applies (or null if
     *     all).
     * @param devices The list of device build flavors for which the acknowledgment applies (or
     *     null if all).
     * @param testCaseNames The list of test case names known to fail (or null if all).
     * @param note A text blob with details about the failure (or null if all).
     */
    public TestAcknowledgmentEntity(
            Key test,
            User user,
            List<String> branches,
            List<String> devices,
            List<String> testCaseNames,
            Text note) {
        this(null, -1, test, user, branches, devices, testCaseNames, note);
    }

    @Override
    public Entity toEntity() {
        Entity ackEntity;
        if (this.key == null) ackEntity = new Entity(KIND);
        else ackEntity = new Entity(key);

        ackEntity.setProperty(TEST_KEY, this.test);
        ackEntity.setProperty(USER, this.user);

        long created = this.created;
        if (created < 0) created = TimeUnit.MILLISECONDS.toMicros(System.currentTimeMillis());
        ackEntity.setProperty(CREATED, created);

        if (this.branches != null && this.branches.size() > 0)
            ackEntity.setUnindexedProperty(BRANCHES, new ArrayList<>(this.branches));

        if (this.devices != null && this.devices.size() > 0)
            ackEntity.setUnindexedProperty(DEVICES, new ArrayList<>(this.devices));

        if (this.testCaseNames != null && this.testCaseNames.size() > 0)
            ackEntity.setUnindexedProperty(TEST_CASE_NAMES, new ArrayList<>(this.testCaseNames));

        if (this.note != null) ackEntity.setUnindexedProperty(NOTE, new Text(this.note));
        return ackEntity;
    }

    /**
     * Convert an Entity object to a TestAcknowledgmentEntity.
     *
     * @param e The entity to process.
     * @return TestEntity object with the properties from e processed, or null if incompatible.
     */
    @SuppressWarnings("unchecked")
    public static TestAcknowledgmentEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND)
                || !e.hasProperty(TEST_KEY)
                || !e.hasProperty(USER)
                || !e.hasProperty(CREATED)) {
            logger.log(
                    Level.WARNING, "Missing attributes in acknowledgment entity: " + e.toString());
            return null;
        }
        try {
            Key test = (Key) e.getProperty(TEST_KEY);
            User user = (User) e.getProperty(USER);
            long created = (long) e.getProperty(CREATED);

            List<String> branches;
            if (e.hasProperty(BRANCHES)) branches = (List<String>) e.getProperty(BRANCHES);
            else branches = null;

            List<String> devices;
            if (e.hasProperty(DEVICES)) devices = (List<String>) e.getProperty(DEVICES);
            else devices = null;

            List<String> testCaseNames;
            if (e.hasProperty(TEST_CASE_NAMES))
                testCaseNames = (List<String>) e.getProperty(TEST_CASE_NAMES);
            else testCaseNames = null;

            Text note = null;
            if (e.hasProperty(NOTE)) note = (Text) e.getProperty(NOTE);
            return new TestAcknowledgmentEntity(
                    e.getKey(), created, test, user, branches, devices, testCaseNames, note);
        } catch (ClassCastException exception) {
            logger.log(Level.WARNING, "Corrupted data in entity: " + e.getKey());
            return null;
        }
    }

    /**
     * Convert a JSON object to a TestAcknowledgmentEntity.
     *
     * @param user The user requesting the conversion.
     * @param json The json object to convert.
     * @return TestAcknowledgmentEntity with the data from the json object.
     */
    public static TestAcknowledgmentEntity fromJson(User user, JsonObject json) {
        try {
            if (!json.has(TEST_NAME)) return null;
            String testName = json.get(TEST_NAME).getAsString();
            Key testKey = KeyFactory.createKey(TestEntity.KIND, testName);
            List<String> branches = null;

            Type listType = new TypeToken<List<String>>() {}.getType();
            if (json.has(BRANCHES)) {
                branches = new Gson().fromJson(json.get(BRANCHES), listType);
            }

            List<String> devices = null;
            if (json.has(DEVICES)) {
                devices = new Gson().fromJson(json.get(DEVICES), listType);
            }

            List<String> testCaseNames = null;
            if (json.has(TEST_CASE_NAMES)) {
                testCaseNames = new Gson().fromJson(json.get(TEST_CASE_NAMES), listType);
            }

            Text note = null;
            if (json.has(NOTE)) {
                note = new Text(json.get(NOTE).getAsString());
            }

            Key key = null;
            if (json.has(KEY)) {
                key = KeyFactory.stringToKey(json.get(KEY).getAsString());
            }
            return new TestAcknowledgmentEntity(
                    key, -1l, testKey, user, branches, devices, testCaseNames, note);
        } catch (ClassCastException | IllegalStateException e) {
            return null;
        }
    }

    /**
     * Convert the entity to a json object.
     *
     * @return The entity serialized in JSON format.
     */
    public JsonObject toJson() {
        JsonObject json = new JsonObject();
        json.add(KEY, new JsonPrimitive(KeyFactory.keyToString(this.key)));
        json.add(TEST_NAME, new JsonPrimitive(this.test.getName()));
        json.add(USER, new JsonPrimitive(this.user.getEmail()));
        json.add(CREATED, new JsonPrimitive(this.created));

        List<JsonElement> branches = new ArrayList<>();
        if (this.branches != null) {
            for (String branch : this.branches) {
                branches.add(new JsonPrimitive(branch));
            }
        }
        json.add(BRANCHES, new Gson().toJsonTree(branches));

        List<JsonElement> devices = new ArrayList<>();
        if (this.devices != null) {
            for (String device : this.devices) {
                devices.add(new JsonPrimitive(device));
            }
        }
        json.add(DEVICES, new Gson().toJsonTree(devices));

        List<JsonElement> testCaseNames = new ArrayList<>();
        if (this.testCaseNames != null) {
            for (String testCaseName : this.testCaseNames) {
                testCaseNames.add(new JsonPrimitive(testCaseName));
            }
        }
        json.add(TEST_CASE_NAMES, new Gson().toJsonTree(testCaseNames));

        String note = "";
        if (this.note != null) note = this.note;
        json.add(NOTE, new JsonPrimitive(note));
        return json;
    }
}
