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

package com.android.vts.job;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.TestAcknowledgmentEntity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.appengine.tools.development.testing.LocalServiceTestHelper;
import com.google.appengine.tools.development.testing.LocalUserServiceTestConfig;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class VtsAlertJobServletTest {
    private final LocalServiceTestHelper userHelper =
            new LocalServiceTestHelper(new LocalUserServiceTestConfig())
                    .setEnvIsAdmin(true)
                    .setEnvIsLoggedIn(true)
                    .setEnvEmail("testemail@domain.com")
                    .setEnvAuthDomain("test");

    User user;
    private Key testKey;
    private Set<String> allTestCases;
    private List<DeviceInfoEntity> allDevices;

    @Before
    public void setUp() {
        userHelper.setUp();
        user = UserServiceFactory.getUserService().getCurrentUser();

        testKey = KeyFactory.createKey(TestAcknowledgmentEntity.KIND, "test");

        allTestCases = new HashSet<>();
        allTestCases.add("testCase1");
        allTestCases.add("testCase2");
        allTestCases.add("testCase3");

        allDevices = new ArrayList<>();
        DeviceInfoEntity device1 =
                new DeviceInfoEntity(
                        testKey, "branch1", "product1", "flavor1", "1234", "32", "abi");
        DeviceInfoEntity device2 =
                new DeviceInfoEntity(
                        testKey, "branch2", "product2", "flavor2", "1235", "32", "abi");
        allDevices.add(device1);
        allDevices.add(device2);
    }

    @After
    public void tearDown() {
        userHelper.tearDown();
    }

    /** Test that acknowledge-all works correctly. */
    @Test
    public void testSeparateAcknowledgedAll() {
        Set<String> testCases = new HashSet<>(allTestCases);
        List<TestAcknowledgmentEntity> acks = new ArrayList<>();
        TestAcknowledgmentEntity ack =
                new TestAcknowledgmentEntity(testKey, user, null, null, null, null);
        acks.add(ack);

        Set<String> acknowledged =
                VtsAlertJobServlet.separateAcknowledged(testCases, allDevices, acks);
        assertEquals(allTestCases.size(), acknowledged.size());
        assertTrue(acknowledged.containsAll(allTestCases));
        assertEquals(0, testCases.size());
    }

    /** Test that specific branch/device/test case acknowledgement works correctly. */
    @Test
    public void testSeparateAcknowledgedSpecific() {
        Set<String> testCases = new HashSet<>(allTestCases);
        List<TestAcknowledgmentEntity> acks = new ArrayList<>();
        List<String> branches = new ArrayList<>();
        branches.add("branch1");

        List<String> devices = new ArrayList<>();
        devices.add("flavor2");

        List<String> testCaseNames = new ArrayList<>();
        testCaseNames.add("testCase1");

        TestAcknowledgmentEntity ack =
                new TestAcknowledgmentEntity(
                        testKey, user, branches, devices, testCaseNames, null);
        acks.add(ack);

        Set<String> acknowledged =
                VtsAlertJobServlet.separateAcknowledged(testCases, allDevices, acks);
        assertEquals(0, acknowledged.size());
        assertEquals(allTestCases.size(), testCases.size());
    }

    /** Test that specific branch/device/test case acknowledgement skips device mismatches. */
    @Test
    public void testSeparateAcknowledgedSpecificMismatch() {
        Set<String> testCases = new HashSet<>(allTestCases);
        List<TestAcknowledgmentEntity> acks = new ArrayList<>();
        List<String> branches = new ArrayList<>();
        branches.add("branch1");

        List<String> devices = new ArrayList<>();
        devices.add("flavor1");

        List<String> testCaseNames = new ArrayList<>();
        testCaseNames.add("testCase1");

        TestAcknowledgmentEntity ack =
                new TestAcknowledgmentEntity(
                        testKey, user, branches, devices, testCaseNames, null);
        acks.add(ack);

        Set<String> acknowledged =
                VtsAlertJobServlet.separateAcknowledged(testCases, allDevices, acks);
        assertEquals(1, acknowledged.size());
        assertTrue(acknowledged.contains("testCase1"));
        assertTrue(!testCases.contains("testCase1"));
    }
}
