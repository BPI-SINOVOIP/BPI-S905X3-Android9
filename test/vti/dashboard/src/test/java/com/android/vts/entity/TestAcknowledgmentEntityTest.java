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

import static org.junit.Assert.*;

import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Text;
import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.appengine.tools.development.testing.LocalServiceTestHelper;
import com.google.appengine.tools.development.testing.LocalUserServiceTestConfig;
import com.google.gson.JsonObject;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class TestAcknowledgmentEntityTest {
    private final LocalServiceTestHelper helper =
            new LocalServiceTestHelper(new LocalUserServiceTestConfig())
                    .setEnvIsAdmin(true)
                    .setEnvIsLoggedIn(true)
                    .setEnvEmail("testemail@domain.com")
                    .setEnvAuthDomain("test");

    @Before
    public void setUp() {
        helper.setUp();
    }

    @After
    public void tearDown() {
        helper.tearDown();
    }

    /** Test serialization to/from Entity objects. */
    @Test
    public void testEntitySerialization() {
        Key key = KeyFactory.createKey(TestEntity.KIND, "test");
        User user = UserServiceFactory.getUserService().getCurrentUser();
        List<String> branches = new ArrayList<>();
        branches.add("branch1");
        List<String> devices = new ArrayList<>();
        devices.add("device1");
        List<String> testCaseNames = new ArrayList<>();
        testCaseNames.add("testCase1");
        Text note = new Text("note");
        TestAcknowledgmentEntity ack =
                new TestAcknowledgmentEntity(key, user, branches, devices, testCaseNames, note);
        Entity e = ack.toEntity();

        Assert.assertNotNull(e);
        Assert.assertEquals(key, e.getProperty(TestAcknowledgmentEntity.TEST_KEY));
        Assert.assertEquals(user, e.getProperty(TestAcknowledgmentEntity.USER));
        Assert.assertTrue(
                ((List<String>) e.getProperty(TestAcknowledgmentEntity.BRANCHES))
                        .containsAll(branches));
        Assert.assertTrue(
                ((List<String>) e.getProperty(TestAcknowledgmentEntity.DEVICES))
                        .containsAll(devices));
        Assert.assertTrue(
                ((List<String>) e.getProperty(TestAcknowledgmentEntity.TEST_CASE_NAMES))
                        .containsAll(testCaseNames));
        Assert.assertEquals(note, e.getProperty(TestAcknowledgmentEntity.NOTE));

        TestAcknowledgmentEntity deserialized = TestAcknowledgmentEntity.fromEntity(e);
        Assert.assertNotNull(deserialized);
        Assert.assertEquals(key, deserialized.test);
        Assert.assertEquals(user, deserialized.user);
        Assert.assertTrue(deserialized.branches.containsAll(branches));
        Assert.assertTrue(deserialized.devices.containsAll(devices));
        Assert.assertTrue(deserialized.testCaseNames.containsAll(testCaseNames));
        Assert.assertEquals(note.getValue(), deserialized.note);
    }

    /** Test serialization to/from Entity objects when optional parameters are null. */
    @Test
    public void testEntitySerializationWithNulls() {
        Key key = KeyFactory.createKey(TestEntity.KIND, "test");
        User user = UserServiceFactory.getUserService().getCurrentUser();
        TestAcknowledgmentEntity ack =
                new TestAcknowledgmentEntity(key, user, null, null, null, null);
        Entity e = ack.toEntity();

        Assert.assertNotNull(e);
        Assert.assertEquals(key, e.getProperty(TestAcknowledgmentEntity.TEST_KEY));
        Assert.assertEquals(user, e.getProperty(TestAcknowledgmentEntity.USER));
        Assert.assertFalse(e.hasProperty(TestAcknowledgmentEntity.BRANCHES));
        Assert.assertFalse(e.hasProperty(TestAcknowledgmentEntity.DEVICES));
        Assert.assertFalse(e.hasProperty(TestAcknowledgmentEntity.TEST_CASE_NAMES));
        Assert.assertFalse(e.hasProperty(TestAcknowledgmentEntity.NOTE));

        TestAcknowledgmentEntity deserialized = TestAcknowledgmentEntity.fromEntity(e);
        Assert.assertNotNull(deserialized);
        Assert.assertEquals(key, deserialized.test);
        Assert.assertEquals(user, deserialized.user);
        Assert.assertEquals(0, deserialized.branches.size());
        Assert.assertEquals(0, deserialized.devices.size());
        Assert.assertEquals(0, deserialized.testCaseNames.size());
        Assert.assertNull(deserialized.note);
    }

    /** Test serialization to/from Json objects. */
    @Test
    public void testJsonSerialization() {
        Key key = KeyFactory.createKey(TestEntity.KIND, "test");
        User user = UserServiceFactory.getUserService().getCurrentUser();
        List<String> branches = new ArrayList<>();
        branches.add("branch1");
        List<String> devices = new ArrayList<>();
        devices.add("device1");
        List<String> testCaseNames = new ArrayList<>();
        testCaseNames.add("testCase1");
        Text note = new Text("note");
        TestAcknowledgmentEntity ack =
                new TestAcknowledgmentEntity(key, user, branches, devices, testCaseNames, note);
        Entity e = new Entity(KeyFactory.createKey(TestAcknowledgmentEntity.KIND, "fakekey"));
        e.setPropertiesFrom(ack.toEntity());
        JsonObject json = TestAcknowledgmentEntity.fromEntity(e).toJson();

        TestAcknowledgmentEntity deserialized = TestAcknowledgmentEntity.fromJson(user, json);
        Assert.assertNotNull(deserialized);
        Assert.assertEquals(key, deserialized.test);
        Assert.assertEquals(user, deserialized.user);
        Assert.assertTrue(deserialized.branches.containsAll(branches));
        Assert.assertTrue(deserialized.devices.containsAll(devices));
        Assert.assertTrue(deserialized.testCaseNames.containsAll(testCaseNames));
        Assert.assertEquals(note.getValue(), deserialized.note);
    }

    /** Test serialization to/from Json objects when optional properties are null. */
    @Test
    public void testJsonSerializationWithNulls() {
        Key key = KeyFactory.createKey(TestEntity.KIND, "test");
        User user = UserServiceFactory.getUserService().getCurrentUser();
        TestAcknowledgmentEntity ack =
                new TestAcknowledgmentEntity(key, user, null, null, null, null);
        Entity e = new Entity(KeyFactory.createKey(TestAcknowledgmentEntity.KIND, "fakekey"));
        e.setPropertiesFrom(ack.toEntity());
        JsonObject json = TestAcknowledgmentEntity.fromEntity(e).toJson();

        TestAcknowledgmentEntity deserialized = TestAcknowledgmentEntity.fromJson(user, json);
        Assert.assertNotNull(deserialized);
        Assert.assertEquals(key, deserialized.test);
        Assert.assertEquals(user, deserialized.user);
        Assert.assertEquals(0, deserialized.branches.size());
        Assert.assertEquals(0, deserialized.devices.size());
        Assert.assertEquals(0, deserialized.testCaseNames.size());
        Assert.assertEquals("", deserialized.note);
    }
}
