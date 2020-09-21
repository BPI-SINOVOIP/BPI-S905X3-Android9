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
import com.google.appengine.api.datastore.KeyFactory;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Entity describing test metadata. */
public class TestEntity implements DashboardEntity {
    protected static final Logger logger = Logger.getLogger(TestEntity.class.getName());

    public static final String KIND = "Test";
    public static final String HAS_PROFILING_DATA = "hasProfilingData";

    public final String testName;
    public final Key key;
    public boolean hasProfilingData;

    /**
     * Create a TestEntity object.
     *
     * @param testName The name of the test.
     * @param hasProfilingData True if the test includes profiling data.
     */
    public TestEntity(String testName, boolean hasProfilingData) {
        this.testName = testName;
        this.key = KeyFactory.createKey(KIND, testName);
        this.hasProfilingData = hasProfilingData;
    }

    /**
     * Create a TestEntity object.
     *
     * @param testName The name of the test.
     */
    public TestEntity(String testName) {
        this(testName, false);
    }

    @Override
    public Entity toEntity() {
        Entity testEntity = new Entity(this.key);
        testEntity.setProperty(HAS_PROFILING_DATA, this.hasProfilingData);
        return testEntity;
    }

    /**
     * Set to true if the test has profiling data.
     *
     * @param hasProfilingData The value to store.
     */
    public void setHasProfilingData(boolean hasProfilingData) {
        this.hasProfilingData = hasProfilingData;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null || !TestEntity.class.isAssignableFrom(obj.getClass())) {
            return false;
        }
        TestEntity test2 = (TestEntity) obj;
        return (
            this.testName.equals(test2.testName) &&
                this.hasProfilingData == test2.hasProfilingData);
    }

    /**
     * Convert an Entity object to a TestEntity.
     *
     * @param e The entity to process.
     * @return TestEntity object with the properties from e processed, or null if incompatible.
     */
    @SuppressWarnings("unchecked")
    public static TestEntity fromEntity(Entity e) {
        if (!e.getKind().equals(KIND) || e.getKey().getName() == null) {
            logger.log(Level.WARNING, "Missing test attributes in entity: " + e.toString());
            return null;
        }
        String testName = e.getKey().getName();
        boolean hasProfilingData = false;
        if (e.hasProperty(HAS_PROFILING_DATA)) {
            hasProfilingData = (boolean) e.getProperty(HAS_PROFILING_DATA);
        }
        return new TestEntity(testName, hasProfilingData);
    }
}
